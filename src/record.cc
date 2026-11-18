#include "record.h"

namespace kvstore {

void EncodeFixed32(char* dst, uint32_t v) {
  (void)dst;
  (void)v;
  // TODO(you)
}

void EncodeFixed64(char* dst, uint64_t v) {
  (void)dst;
  (void)v;
  // TODO(you)
}

uint32_t DecodeFixed32(const char* src) {
  (void)src;
  // TODO(you)
  return 0;
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
