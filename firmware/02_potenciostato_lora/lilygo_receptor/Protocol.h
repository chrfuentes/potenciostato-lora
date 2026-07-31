#pragma once

#include <Arduino.h>

namespace PotProtocol {

constexpr uint16_t MAGIC = 0x504C;  // "PL": Potenciostato LoRa.
constexpr uint8_t VERSION = 1;
constexpr uint8_t MAX_SAMPLES_PER_PACKET = 16;

enum MessageType : uint8_t {
  COMMAND_START = 1,
  ACK = 2,
  METADATA = 3,
  DATA = 4,
  END_OF_CYCLE = 5,
  ERROR_MESSAGE = 6
};

#pragma pack(push, 1)

struct Header {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint16_t session;
  uint16_t sequence;
};

struct CommandStartPacket {
  Header header;
};

struct AckPacket {
  Header header;
  uint16_t acknowledgedSequence;
  uint8_t acknowledgedType;
  uint8_t status;
};

struct MetadataPacket {
  Header header;
  uint8_t cycle;
  uint16_t sampleCount;
  uint16_t scanRateMillivoltsPerSecond;
  uint16_t intervalMilliseconds;
};

struct DataPacket {
  Header header;
  uint8_t cycle;
  uint16_t firstSample;
  uint8_t sampleCount;
  uint16_t adc[MAX_SAMPLES_PER_PACKET];
};

struct EndPacket {
  Header header;
  uint8_t cycle;
  uint16_t sampleCount;
  uint16_t dataCrc;
};

#pragma pack(pop)

static_assert(sizeof(Header) == 8, "Header layout changed");
static_assert(sizeof(CommandStartPacket) == 8, "Command layout changed");
static_assert(sizeof(AckPacket) == 12, "ACK layout changed");
static_assert(sizeof(MetadataPacket) == 15, "Metadata layout changed");
static_assert(sizeof(DataPacket) == 44, "Data layout changed");
static_assert(sizeof(EndPacket) == 13, "End layout changed");

inline Header makeHeader(uint8_t type, uint16_t session, uint16_t sequence) {
  Header header = {MAGIC, VERSION, type, session, sequence};
  return header;
}

inline bool isValidHeader(const Header &header) {
  return header.magic == MAGIC && header.version == VERSION;
}

inline uint16_t crc16Update(uint16_t crc, uint8_t value) {
  crc ^= static_cast<uint16_t>(value) << 8;
  for (uint8_t bit = 0; bit < 8; bit++) {
    crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                         : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

inline uint16_t crc16Sample(uint16_t crc, uint16_t sample) {
  crc = crc16Update(crc, lowByte(sample));
  return crc16Update(crc, highByte(sample));
}

}  // namespace PotProtocol
