#include <SPI.h>
#include <LoRa.h>

// Arduino Uno + modulo externo SX1276 915 MHz.
// Pinagem escolhida para nao ocupar o D10 usado pelo potenciostato.
constexpr long LORA_FREQUENCY_HZ = 915E6;
constexpr uint8_t LORA_NSS_PIN = 7;
constexpr uint8_t LORA_RESET_PIN = 8;
constexpr uint8_t LORA_DIO0_PIN = 2;
constexpr uint32_t LORA_SPI_HZ = 1000000UL;

uint16_t sequenceNumber = 0;

void configureRadio() {
  LoRa.setPins(LORA_NSS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);

  // Frequencia reduzida por causa do conversor de nivel logico.
  LoRa.setSPIFrequency(LORA_SPI_HZ);

  if (!LoRa.begin(LORA_FREQUENCY_HZ)) {
    Serial.println(F("ERRO: SX1276 nao respondeu."));
    Serial.println(F("Confira 3,3 V, GND, SPI, NSS, RESET e a antena."));
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

bool waitForAck(uint16_t expectedSequence, unsigned long timeoutMs) {
  const unsigned long startedAt = millis();

  while (millis() - startedAt < timeoutMs) {
    const int packetSize = LoRa.parsePacket();
    if (packetSize <= 0) {
      continue;
    }

    String message;
    while (LoRa.available()) {
      message += static_cast<char>(LoRa.read());
    }

    const String expected = String(F("ACK,")) + expectedSequence;
    if (message == expected) {
      Serial.print(F("ACK recebido; RSSI="));
      Serial.print(LoRa.packetRssi());
      Serial.print(F(" dBm; SNR="));
      Serial.print(LoRa.packetSnr(), 1);
      Serial.println(F(" dB"));
      return true;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("Teste LoRa - Arduino Uno transmissor"));
  Serial.println(F("Frequencia: 915 MHz"));
  configureRadio();
  Serial.println(F("Radio inicializado."));
}

void loop() {
  sequenceNumber++;
  const String message = String(F("PING,")) + sequenceNumber;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(message);
  const int result = LoRa.endPacket();
  LoRa.receive();

  Serial.print(F("Enviado: "));
  Serial.print(message);

  if (result == 1 && waitForAck(sequenceNumber, 1500)) {
    Serial.println(F("Resultado: enlace OK."));
  } else {
    Serial.println();
    Serial.println(F("Resultado: sem resposta."));
  }

  delay(2500);
}
