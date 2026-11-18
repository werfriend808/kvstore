#ifndef KVSTORE_SRC_RECORD_H_
#define KVSTORE_SRC_RECORD_H_

// On-disk record encoding (internal; not part of the public API).
//
// Layout (all integers little-endian, see PROGRESS.md decisions 1-5):
//
//   offset  size  field
//   0       4     crc32       (M1: always 0; computed in M2)
//   4       8     timestamp
//   12      4     key_size
//   16      4     value_size
//   20      1     flags       (bit0 = tombstone)
//   21      k     key
//   21+k    v     value

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "kvstore/status.h"

namespace kvstore {

inline constexpr std::size_t kRecordHeaderSize = 21;
inline constexpr uint8_t kFlagTombstone = 0x01;  // flags bit0

struct RecordHeader {
  uint32_t crc;  // M1: always 0
  uint64_t timestamp;
  uint32_t key_size;
  uint32_t value_size;
  uint8_t flags;
};

// ---------------------------------------------------------------------------
// Fixed-width little-endian integers.
// ---------------------------------------------------------------------------

// Writes exactly 4 bytes to dst, least significant byte first.
void EncodeFixed32(char* dst, uint32_t v);

// Writes exactly 8 bytes to dst, least significant byte first.
void EncodeFixed64(char* dst, uint64_t v);

// Reads 4 bytes written by EncodeFixed32.
uint32_t DecodeFixed32(const char* src);

// Reads 8 bytes written by EncodeFixed64.
uint64_t DecodeFixed64(const char* src);

// ---------------------------------------------------------------------------
// Whole records.
// ---------------------------------------------------------------------------

// Appends header (crc = 0) + key + value to the end of *dst.
// Existing contents of *dst are kept.
// Precondition: key/value sizes fit in uint32_t (Put enforces the limits).
void EncodeRecord(uint64_t timestamp, uint8_t flags, std::string_view key,
                  std::string_view value, std::string* dst);

// Parses the first kRecordHeaderSize bytes of buf into *header.
// Bytes after the header are ignored.
//   Corruption  if buf.size() < kRecordHeaderSize
Status DecodeHeader(std::string_view buf, RecordHeader* header);

// Parses one complete record. On success, *key and *value point into buf
// (no copy) and stay valid only as long as buf's storage does.
//   Corruption  if buf.size() != kRecordHeaderSize + key_size + value_size
Status DecodeRecord(std::string_view buf, RecordHeader* header,
                    std::string_view* key, std::string_view* value);

}  // namespace kvstore

#endif  // KVSTORE_SRC_RECORD_H_
