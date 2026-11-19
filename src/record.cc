#include "record.h"

namespace kvstore {

void EncodeFixed32(char* dst, uint32_t v) {
  dst[0] = static_cast<char>(v & 0xFF);
  dst[1] = static_cast<char>((v >> 8) & 0xFF);
  dst[2] = static_cast<char>((v >> 16) & 0xFF);
  dst[3] = static_cast<char>((v >> 24) & 0xFF);
}

void EncodeFixed64(char* dst, uint64_t v) {
  (void)dst;
  (void)v;
  // TODO(you)
}

uint32_t DecodeFixed32(const char* src) {
  uint32_t b0 = static_cast<uint8_t>(src[0]);
  uint32_t b1 = static_cast<uint8_t>(src[1]);
  uint32_t b2 = static_cast<uint8_t>(src[2]);
  uint32_t b3 = static_cast<uint8_t>(src[3]);

  return b0 | b1 << 8 | b2 << 16 | b3 << 24;
}

uint64_t DecodeFixed64(const char* src) {
  (void)src;
  // TODO(you)
  return 0;
}

void EncodeRecord(uint64_t timestamp, uint8_t flags, std::string_view key,
                  std::string_view value, std::string* dst) {
  (void)timestamp;
  (void)flags;
  (void)key;
  (void)value;
  (void)dst;
  // TODO(you)
}

Status DecodeHeader(std::string_view buf, RecordHeader* header) {
  (void)buf;
  (void)header;
  // TODO(you)
  return Status::NotImplemented("DecodeHeader");
}

Status DecodeRecord(std::string_view buf, RecordHeader* header,
                    std::string_view* key, std::string_view* value) {
  (void)buf;
  (void)header;
  (void)key;
  (void)value;
  // TODO(you)
  return Status::NotImplemented("DecodeRecord");
}

}  // namespace kvstore
