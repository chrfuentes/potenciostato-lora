#include <SPI.h>
#include <LoRa.h>

#include "Protocol.h"

// Adaptacao do codigo de varredura apresentado no Apendice A de:
// SOMBRA, V. C. Modelagem, simulacao e implementacao de um potenciostato
// portatil de baixo custo para aplicacao ambiental. UFABC, 2025.
//
// Alteracoes principais:
// - correcao do limite do vetor (pos < count);
// - aquisicao em memoria sem transmissao durante a varredura;
// - envio binario em blocos depois de cada ciclo;
// - ACK, repeticao limitada, numero de sessao e CRC da aplicacao.

constexpr long LORA_FREQUENCY_HZ = 915E6;
constexpr uint8_t LORA_NSS_PIN = 7;
constexpr uint8_t LORA_RESET_PIN = 8;
constexpr uint8_t LORA_DIO0_PIN = 2;
constexpr uint32_t LORA_SPI_HZ = 1000000UL;

constexpr uint8_t PWM_PIN = 10;
constexpr uint8_t ADC_PIN = A0;
constexpr uint8_t PWM_FIRST = 41;
constexpr uint8_t PWM_LAST = 184;
constexpr uint8_t PWM_NEUTRAL = 127;

constexpr uint16_t SAMPLES_PER_DIRECTION = PWM_LAST - PWM_FIRST + 1;
constexpr uint16_t SAMPLES_PER_CYCLE = SAMPLES_PER_DIRECTION * 2;
constexpr uint8_t CYCLES_PER_EXPERIMENT = 2;
constexpr uint16_t SCAN_RATE_MV_PER_SECOND = 100;
constexpr uint16_t STEP_INTERVAL_MS =
    1000000UL / (static_cast<uint32_t>(SCAN_RATE_MV_PER_SECOND) * 128UL);

constexpr uint8_t MAX_RETRIES = 3;
constexpr unsigned long ACK_TIMEOUT_MS = 1200;

uint16_t adcSamples[SAMPLES_PER_CYCLE];
uint16_t currentSession = 0;
uint16_t nextSequence = 1;
bool experimentRunning = false;

void configureRadio() {
  LoRa.setPins(LORA_NSS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  LoRa.setSPIFrequency(LORA_SPI_HZ);

  if (!LoRa.begin(LORA_FREQUENCY_HZ)) {
    Serial.println(F("ERRO: SX1276 nao respondeu."));
    while (true) {
      delay(1000);
    }
  }

  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x34);
  LoRa.setTxPower(10);
  LoRa.enableCrc();
  LoRa.receive();
}

bool readPacket(void *destination, size_t expectedSize) {
  const int packetSize = LoRa.parsePacket();
  if (packetSize != static_cast<int>(expectedSize)) {
    if (packetSize > 0) {
      while (LoRa.available()) {
        LoRa.read();
      }
    }
    return false;
  }

  uint8_t *bytes = static_cast<uint8_t *>(destination);
  for (size_t index = 0; index < expectedSize && LoRa.available(); index++) {
    bytes[index] = static_cast<uint8_t>(LoRa.read());
  }
  return true;
}

void sendAck(uint16_t session, uint16_t acknowledgedSequence,
             uint8_t acknowledgedType, uint8_t status) {
  PotProtocol::AckPacket ack = {};
  ack.header = PotProtocol::makeHeader(PotProtocol::ACK, session, nextSequence++);
  ack.acknowledgedSequence = acknowledgedSequence;
  ack.acknowledgedType = acknowledgedType;
  ack.status = status;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t *>(&ack), sizeof(ack));
  LoRa.endPacket();
  LoRa.receive();
}

bool waitForAck(uint16_t sequence, uint8_t type) {
  const unsigned long startedAt = millis();

  while (millis() - startedAt < ACK_TIMEOUT_MS) {
    PotProtocol::AckPacket ack = {};
    if (!readPacket(&ack, sizeof(ack))) {
      continue;
    }

    if (PotProtocol::isValidHeader(ack.header) &&
        ack.header.type == PotProtocol::ACK &&
        ack.acknowledgedSequence == sequence &&
        ack.acknowledgedType == type &&
        ack.status == 0) {
      return true;
    }
  }

  return false;
}

bool sendWithAck(const void *packet, size_t packetSize, uint16_t sequence,
                 uint8_t type) {
  for (uint8_t attempt = 1; attempt <= MAX_RETRIES; attempt++) {
    LoRa.idle();
    LoRa.beginPacket();
    LoRa.write(reinterpret_cast<const uint8_t *>(packet), packetSize);
    const int sent = LoRa.endPacket();
    LoRa.receive();

    if (sent == 1 && waitForAck(sequence, type)) {
      return true;
    }

    Serial.print(F("Sem ACK; tipo="));
    Serial.print(type);
    Serial.print(F("; sequencia="));
    Serial.print(sequence);
    Serial.print(F("; tentativa="));
    Serial.println(attempt);
  }

  return false;
}

void acquireCycle() {
  uint16_t sampleIndex = 0;

  analogWrite(PWM_PIN, PWM_NEUTRAL);
  delay(5000);

  for (int pwm = PWM_LAST; pwm >= PWM_FIRST; pwm--) {
    analogWrite(PWM_PIN, pwm);
    delay(STEP_INTERVAL_MS);
    adcSamples[sampleIndex++] = analogRead(ADC_PIN);
  }

  for (int pwm = PWM_FIRST; pwm <= PWM_LAST; pwm++) {
    analogWrite(PWM_PIN, pwm);
    delay(STEP_INTERVAL_MS);
    adcSamples[sampleIndex++] = analogRead(ADC_PIN);
  }

  analogWrite(PWM_PIN, PWM_NEUTRAL);
}

bool transmitCycle(uint8_t cycle) {
  PotProtocol::MetadataPacket metadata = {};
  metadata.header =
      PotProtocol::makeHeader(PotProtocol::METADATA, currentSession, nextSequence++);
  metadata.cycle = cycle;
  metadata.sampleCount = SAMPLES_PER_CYCLE;
  metadata.scanRateMillivoltsPerSecond = SCAN_RATE_MV_PER_SECOND;
  metadata.intervalMilliseconds = STEP_INTERVAL_MS;

  if (!sendWithAck(&metadata, sizeof(metadata), metadata.header.sequence,
                   metadata.header.type)) {
    return false;
  }

  for (uint16_t firstSample = 0; firstSample < SAMPLES_PER_CYCLE;
       firstSample += PotProtocol::MAX_SAMPLES_PER_PACKET) {
    PotProtocol::DataPacket data = {};
    data.header =
        PotProtocol::makeHeader(PotProtocol::DATA, currentSession, nextSequence++);
    data.cycle = cycle;
    data.firstSample = firstSample;

    const uint16_t remaining = SAMPLES_PER_CYCLE - firstSample;
    data.sampleCount =
        remaining < PotProtocol::MAX_SAMPLES_PER_PACKET
            ? remaining
            : PotProtocol::MAX_SAMPLES_PER_PACKET;

    for (uint8_t index = 0; index < data.sampleCount; index++) {
      data.adc[index] = adcSamples[firstSample + index];
    }

    const size_t packetSize =
        sizeof(PotProtocol::Header) + sizeof(data.cycle) +
        sizeof(data.firstSample) + sizeof(data.sampleCount) +
        data.sampleCount * sizeof(data.adc[0]);

    if (!sendWithAck(&data, packetSize, data.header.sequence, data.header.type)) {
      return false;
    }
  }

  PotProtocol::EndPacket endPacket = {};
  endPacket.header = PotProtocol::makeHeader(PotProtocol::END_OF_CYCLE,
                                              currentSession, nextSequence++);
  endPacket.cycle = cycle;
  endPacket.sampleCount = SAMPLES_PER_CYCLE;
  endPacket.dataCrc =
      PotProtocol::crc16Samples(adcSamples, SAMPLES_PER_CYCLE);

  return sendWithAck(&endPacket, sizeof(endPacket), endPacket.header.sequence,
                     endPacket.header.type);
}

void runExperiment() {
  if (experimentRunning) {
    return;
  }

  experimentRunning = true;
  currentSession++;
  if (currentSession == 0) {
    currentSession = 1;
  }
  nextSequence = 1;

  Serial.print(F("Experimento iniciado; sessao="));
  Serial.println(currentSession);

  for (uint8_t cycle = 0; cycle < CYCLES_PER_EXPERIMENT; cycle++) {
    Serial.print(F("Adquirindo ciclo "));
    Serial.println(cycle);
    acquireCycle();

    Serial.print(F("Transmitindo ciclo "));
    Serial.println(cycle);
    if (!transmitCycle(cycle)) {
      Serial.println(F("ERRO: transmissao abortada por falta de ACK."));
      experimentRunning = false;
      LoRa.receive();
      return;
    }
  }

  Serial.println(F("Experimento concluido."));
  experimentRunning = false;
  LoRa.receive();
}

void checkRemoteCommand() {
  const int packetSize = LoRa.parsePacket();
  if (packetSize != static_cast<int>(sizeof(PotProtocol::CommandStartPacket))) {
    if (packetSize > 0) {
      while (LoRa.available()) {
        LoRa.read();
      }
    }
    return;
  }

  PotProtocol::CommandStartPacket command = {};
  uint8_t *bytes = reinterpret_cast<uint8_t *>(&command);
  for (size_t index = 0; index < sizeof(command) && LoRa.available(); index++) {
    bytes[index] = static_cast<uint8_t>(LoRa.read());
  }

  if (!PotProtocol::isValidHeader(command.header) ||
      command.header.type != PotProtocol::COMMAND_START) {
    return;
  }

  sendAck(command.header.session, command.header.sequence,
          command.header.type, experimentRunning ? 1 : 0);

  if (!experimentRunning) {
    delay(100);
    runExperiment();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);

  // Mantem a alteracao do Timer 1 utilizada por Vitoria Sombra para o PWM.
  TCCR1B = (TCCR1B & B11111000) | B00000010;
  analogWrite(PWM_PIN, PWM_NEUTRAL);

  delay(1000);
  Serial.println();
  Serial.println(F("Potenciostato LoRa - Arduino Uno"));
  Serial.print(F("Amostras por ciclo: "));
  Serial.println(SAMPLES_PER_CYCLE);
  Serial.print(F("Intervalo nominal: "));
  Serial.print(STEP_INTERVAL_MS);
  Serial.println(F(" ms"));
  Serial.println(F("Digite S para iniciar localmente ou use a LILYGO."));

  configureRadio();
}

void loop() {
  if (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    if (command == 's' || command == 'S') {
      runExperiment();
    }
  }

  if (!experimentRunning) {
    checkRemoteCommand();
  }
}
