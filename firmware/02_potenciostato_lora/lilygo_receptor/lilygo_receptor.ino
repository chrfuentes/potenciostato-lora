#include <SPI.h>
#include <LoRa.h>

#include "Protocol.h"

// Receptor para LILYGO T3 V1.6.1 / LoRa32 V2.1.6.
// O display OLED nao e usado nesta primeira versao para reduzir dependencias.

constexpr long LORA_FREQUENCY_HZ = 915E6;
constexpr uint8_t LORA_SCK_PIN = 5;
constexpr uint8_t LORA_MISO_PIN = 19;
constexpr uint8_t LORA_MOSI_PIN = 27;
constexpr uint8_t LORA_NSS_PIN = 18;
constexpr uint8_t LORA_RESET_PIN = 23;
constexpr uint8_t LORA_DIO0_PIN = 26;
constexpr uint8_t STATUS_LED_PIN = 25;

constexpr uint8_t PWM_FIRST = 41;
constexpr uint8_t PWM_LAST = 184;
constexpr uint16_t SAMPLES_PER_DIRECTION = PWM_LAST - PWM_FIRST + 1;

uint16_t localSequence = 1;
uint16_t commandSession = 1;
uint16_t activeSession = 0;
uint16_t lastAcceptedSequence = 0;
uint16_t scanRateMillivoltsPerSecond = 0;
uint16_t intervalMilliseconds = 0;
uint16_t receivedSamples = 0;
uint16_t receivedDataCrc = 0xFFFF;

void configureRadio() {
  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);
  LoRa.setPins(LORA_NSS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  LoRa.setSPIFrequency(4000000UL);

  if (!LoRa.begin(LORA_FREQUENCY_HZ)) {
    Serial.println(F("ERRO: radio LoRa integrado nao respondeu."));
    while (true) {
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
      delay(250);
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

void sendAck(const PotProtocol::Header &receivedHeader, uint8_t status = 0) {
  PotProtocol::AckPacket ack = {};
  ack.header =
      PotProtocol::makeHeader(PotProtocol::ACK, receivedHeader.session,
                              localSequence++);
  ack.acknowledgedSequence = receivedHeader.sequence;
  ack.acknowledgedType = receivedHeader.type;
  ack.status = status;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t *>(&ack), sizeof(ack));
  LoRa.endPacket();
  LoRa.receive();
}

void sendStartCommand() {
  PotProtocol::CommandStartPacket command = {};
  command.header = PotProtocol::makeHeader(
      PotProtocol::COMMAND_START, commandSession++, localSequence++);

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t *>(&command), sizeof(command));
  LoRa.endPacket();
  LoRa.receive();

  Serial.print(F("COMMAND_SENT,START,"));
  Serial.println(command.header.sequence);
}

uint8_t pwmForSample(uint16_t sampleIndex) {
  if (sampleIndex < SAMPLES_PER_DIRECTION) {
    return PWM_LAST - sampleIndex;
  }
  return PWM_FIRST + (sampleIndex - SAMPLES_PER_DIRECTION);
}

const __FlashStringHelper *directionForSample(uint16_t sampleIndex) {
  return sampleIndex < SAMPLES_PER_DIRECTION ? F("forward") : F("reverse");
}

void handleMetadata(const uint8_t *buffer, int packetSize, int rssi, float snr) {
  if (packetSize != static_cast<int>(sizeof(PotProtocol::MetadataPacket))) {
    return;
  }

  PotProtocol::MetadataPacket packet = {};
  memcpy(&packet, buffer, sizeof(packet));
  sendAck(packet.header);

  const bool duplicate =
      packet.header.session == activeSession &&
      packet.header.sequence <= lastAcceptedSequence;
  if (duplicate) {
    return;
  }

  activeSession = packet.header.session;
  lastAcceptedSequence = packet.header.sequence;
  scanRateMillivoltsPerSecond = packet.scanRateMillivoltsPerSecond;
  intervalMilliseconds = packet.intervalMilliseconds;
  receivedSamples = 0;
  receivedDataCrc = 0xFFFF;

  Serial.print(F("META,"));
  Serial.print(packet.header.session);
  Serial.print(',');
  Serial.print(packet.cycle);
  Serial.print(',');
  Serial.print(packet.sampleCount);
  Serial.print(',');
  Serial.print(scanRateMillivoltsPerSecond);
  Serial.print(',');
  Serial.print(intervalMilliseconds);
  Serial.print(',');
  Serial.print(rssi);
  Serial.print(',');
  Serial.println(snr, 1);
}

void handleData(const uint8_t *buffer, int packetSize, int rssi, float snr) {
  const size_t minimumSize =
      sizeof(PotProtocol::Header) + sizeof(uint8_t) + sizeof(uint16_t) +
      sizeof(uint8_t);
  if (packetSize < static_cast<int>(minimumSize)) {
    return;
  }

  PotProtocol::DataPacket packet = {};
  memcpy(&packet, buffer,
         packetSize < static_cast<int>(sizeof(packet)) ? packetSize
                                                       : sizeof(packet));
  if (packet.sampleCount > PotProtocol::MAX_SAMPLES_PER_PACKET) {
    return;
  }

  const size_t expectedSize =
      minimumSize + packet.sampleCount * sizeof(packet.adc[0]);
  if (packetSize != static_cast<int>(expectedSize)) {
    return;
  }

  sendAck(packet.header);

  const bool duplicate =
      packet.header.session == activeSession &&
      packet.header.sequence <= lastAcceptedSequence;
  if (duplicate) {
    return;
  }

  activeSession = packet.header.session;
  lastAcceptedSequence = packet.header.sequence;
  digitalWrite(STATUS_LED_PIN, HIGH);

  for (uint8_t index = 0; index < packet.sampleCount; index++) {
    const uint16_t sampleIndex = packet.firstSample + index;
    Serial.print(F("DATA,"));
    Serial.print(packet.header.session);
    Serial.print(',');
    Serial.print(packet.cycle);
    Serial.print(',');
    Serial.print(sampleIndex);
    Serial.print(',');
    Serial.print(directionForSample(sampleIndex));
    Serial.print(',');
    Serial.print(pwmForSample(sampleIndex));
    Serial.print(',');
    Serial.print(packet.adc[index]);
    Serial.print(',');
    Serial.print(scanRateMillivoltsPerSecond);
    Serial.print(',');
    Serial.print(intervalMilliseconds);
    Serial.print(',');
    Serial.print(rssi);
    Serial.print(',');
    Serial.println(snr, 1);
    receivedSamples++;
    receivedDataCrc =
        PotProtocol::crc16Sample(receivedDataCrc, packet.adc[index]);
  }

  digitalWrite(STATUS_LED_PIN, LOW);
}

void handleEnd(const uint8_t *buffer, int packetSize, int rssi, float snr) {
  if (packetSize != static_cast<int>(sizeof(PotProtocol::EndPacket))) {
    return;
  }

  PotProtocol::EndPacket packet = {};
  memcpy(&packet, buffer, sizeof(packet));
  sendAck(packet.header);

  const bool duplicate =
      packet.header.session == activeSession &&
      packet.header.sequence <= lastAcceptedSequence;
  if (duplicate) {
    return;
  }

  activeSession = packet.header.session;
  lastAcceptedSequence = packet.header.sequence;

  Serial.print(F("END,"));
  Serial.print(packet.header.session);
  Serial.print(',');
  Serial.print(packet.cycle);
  Serial.print(',');
  Serial.print(packet.sampleCount);
  Serial.print(',');
  Serial.print(receivedSamples);
  Serial.print(',');
  Serial.print(packet.dataCrc, HEX);
  Serial.print(',');
  Serial.print(receivedDataCrc, HEX);
  Serial.print(',');
  Serial.print(packet.dataCrc == receivedDataCrc ? F("CRC_OK")
                                                 : F("CRC_MISMATCH"));
  Serial.print(',');
  Serial.print(rssi);
  Serial.print(',');
  Serial.println(snr, 1);
}

void handleAck(const uint8_t *buffer, int packetSize) {
  if (packetSize != static_cast<int>(sizeof(PotProtocol::AckPacket))) {
    return;
  }

  PotProtocol::AckPacket ack = {};
  memcpy(&ack, buffer, sizeof(ack));
  Serial.print(F("COMMAND_ACK,"));
  Serial.print(ack.acknowledgedSequence);
  Serial.print(',');
  Serial.println(ack.status);
}

void receivePacket() {
  const int packetSize = LoRa.parsePacket();
  if (packetSize <= 0 || packetSize > 255) {
    return;
  }

  uint8_t buffer[255] = {};
  int bytesRead = 0;
  while (LoRa.available() && bytesRead < packetSize) {
    buffer[bytesRead++] = static_cast<uint8_t>(LoRa.read());
  }
  if (bytesRead < static_cast<int>(sizeof(PotProtocol::Header))) {
    return;
  }

  PotProtocol::Header header = {};
  memcpy(&header, buffer, sizeof(header));
  if (!PotProtocol::isValidHeader(header)) {
    Serial.println(F("WARN,pacote_invalido"));
    return;
  }

  const int rssi = LoRa.packetRssi();
  const float snr = LoRa.packetSnr();

  switch (header.type) {
    case PotProtocol::ACK:
      handleAck(buffer, packetSize);
      break;
    case PotProtocol::METADATA:
      handleMetadata(buffer, packetSize, rssi, snr);
      break;
    case PotProtocol::DATA:
      handleData(buffer, packetSize, rssi, snr);
      break;
    case PotProtocol::END_OF_CYCLE:
      handleEnd(buffer, packetSize, rssi, snr);
      break;
    default:
      Serial.print(F("WARN,tipo_desconhecido,"));
      Serial.println(header.type);
      break;
  }
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("Potenciostato LoRa - LILYGO receptora"));
  Serial.println(F("Digite S para solicitar uma varredura remota."));
  Serial.println(F("CSV: DATA,sessao,ciclo,indice,direcao,pwm,adc,taxa,intervalo,rssi,snr"));

  configureRadio();
}

void loop() {
  if (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    if (command == 's' || command == 'S') {
      sendStartCommand();
    }
  }

  receivePacket();
}
