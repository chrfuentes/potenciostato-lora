#include <SPI.h>
#include <LoRa.h>

// LILYGO T3 V1.6.1 / LoRa32 V2.1.6 com SX1276 de 868/915 MHz.
constexpr long LORA_FREQUENCY_HZ = 915E6;
constexpr uint8_t LORA_SCK_PIN = 5;
constexpr uint8_t LORA_MISO_PIN = 19;
constexpr uint8_t LORA_MOSI_PIN = 27;
constexpr uint8_t LORA_NSS_PIN = 18;
constexpr uint8_t LORA_RESET_PIN = 23;
constexpr uint8_t LORA_DIO0_PIN = 26;
constexpr uint8_t STATUS_LED_PIN = 25;

void configureRadio() {
  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);
  LoRa.setPins(LORA_NSS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  LoRa.setSPIFrequency(4000000UL);

  if (!LoRa.begin(LORA_FREQUENCY_HZ)) {
    Serial.println(F("ERRO: radio LoRa integrado nao respondeu."));
    Serial.println(F("Confirme a placa T3 V1.6.1 e conecte a antena."));
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

void sendAck(const String &receivedMessage) {
  const int separator = receivedMessage.indexOf(',');
  if (separator < 0) {
    return;
  }

  const String sequence = receivedMessage.substring(separator + 1);
  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(F("ACK,"));
  LoRa.print(sequence);
  LoRa.endPacket();
  LoRa.receive();
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("Teste LoRa - LILYGO T3 V1.6.1 receptora"));
  Serial.println(F("Frequencia: 915 MHz"));
  configureRadio();
  Serial.println(F("Aguardando PING do Arduino Uno..."));
}

void loop() {
  const int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  String message;
  while (LoRa.available()) {
    message += static_cast<char>(LoRa.read());
  }

  const int rssi = LoRa.packetRssi();
  const float snr = LoRa.packetSnr();

  Serial.print(F("Recebido: "));
  Serial.print(message);
  Serial.print(F("; RSSI="));
  Serial.print(rssi);
  Serial.print(F(" dBm; SNR="));
  Serial.print(snr, 1);
  Serial.println(F(" dB"));

  if (message.startsWith(F("PING,"))) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    sendAck(message);
    delay(50);
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}
