#include "record.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "test_util.h"

namespace kvstore {
namespace {

// Byte at index i of s, as an unsigned value (avoids char sign issues).
uint8_t ByteAt(const std::string& s, std::size_t i) {
  return static_cast<uint8_t>(s[i]);
}

// ---------------------------------------------------------------------------
// Fixed-width little-endian integers
// ---------------------------------------------------------------------------

TEST(FixedIntTest, EncodeFixed32IsLittleEndian) {
  char buf[4] = {};
  EncodeFixed32(buf, 0x04030201u);
  EXPECT_EQ(static_cast<uint8_t>(buf[0]), 0x01);  // least significant first
  EXPECT_EQ(static_cast<uint8_t>(buf[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(buf[2]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(buf[3]), 0x04);
}

TEST(FixedIntTest, EncodeFixed64IsLittleEndian) {
  char buf[8] = {};
  EncodeFixed64(buf, 0x0807060504030201ull);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(static_cast<uint8_t>(buf[i]), i + 1) << "byte " << i;
  }
}

// Bytes >= 0x80 catch the classic bug: `char` is signed on x86, so a byte
// like 0xFF becomes -1 and sign-extends into the upper bits when shifted.
TEST(FixedIntTest, Fixed32RoundTripsEdgeValues) {
  const uint32_t values[] = {0u, 1u, 0x7Fu, 0x80u, 0xFFu, 0x8000u,
                             0x12345678u, 0x80808080u, 0xFFFFFFFFu};
  for (uint32_t v : values) {
    char buf[4];
    EncodeFixed32(buf, v);
    EXPECT_EQ(DecodeFixed32(buf), v) << std::hex << "0x" << v;
  }
}

TEST(FixedIntTest, Fixed64RoundTripsEdgeValues) {
  const uint64_t values[] = {0ull,
                             1ull,
                             0xFFull,
                             0xFFFFFFFFull,
                             0x100000000ull,  // just past 32 bits
                             0x0123456789ABCDEFull,
                             0x8080808080808080ull,
                             std::numeric_limits<uint64_t>::max()};
  for (uint64_t v : values) {
    char buf[8];
    EncodeFixed64(buf, v);
    EXPECT_EQ(DecodeFixed64(buf), v) << std::hex << "0x" << v;
  }
}

// ---------------------------------------------------------------------------
// EncodeRecord: exact byte layout
// ---------------------------------------------------------------------------

TEST(EncodeRecordTest, SizeIsHeaderPlusKeyPlusValue) {
  std::string out;
  EncodeRecord(0, 0, "abc", "hello", &out);
  EXPECT_EQ(out.size(), kRecordHeaderSize + 3 + 5);
}

TEST(EncodeRecordTest, HeaderFieldsAtDocumentedOffsets) {
  std::string out;
  EncodeRecord(/*timestamp=*/0x1122334455667788ull, /*flags=*/0, "key",
               "value!", &out);
  ASSERT_EQ(out.size(), kRecordHeaderSize + 3 + 6);

  EXPECT_EQ(DecodeFixed32(out.data() + 0), 0u) << "crc must be 0 in M1";
  EXPECT_EQ(DecodeFixed64(out.data() + 4), 0x1122334455667788ull);
  EXPECT_EQ(DecodeFixed32(out.data() + 12), 3u) << "key_size";
  EXPECT_EQ(DecodeFixed32(out.data() + 16), 6u) << "value_size";
  EXPECT_EQ(ByteAt(out, 20), 0) << "flags";
  EXPECT_EQ(out.substr(21, 3), "key");
  EXPECT_EQ(out.substr(24), "value!");
}

// Checks raw bytes too, so the test doesn't only trust DecodeFixed*.
TEST(EncodeRecordTest, HeaderBytesAreLittleEndian) {
  std::string out;
  EncodeRecord(/*timestamp=*/0x01, 0, "k", "", &out);
  ASSERT_EQ(out.size(), kRecordHeaderSize + 1);
  EXPECT_EQ(ByteAt(out, 4), 0x01) << "timestamp LSB first";
  EXPECT_EQ(ByteAt(out, 11), 0x00) << "timestamp MSB last";
  EXPECT_EQ(ByteAt(out, 12), 0x01) << "key_size LSB first";
  EXPECT_EQ(ByteAt(out, 15), 0x00);
}

TEST(EncodeRecordTest, AppendsWithoutClearingDst) {
  std::string out = "prefix";
  EncodeRecord(0, 0, "k", "v", &out);
  ASSERT_EQ(out.size(), 6 + kRecordHeaderSize + 2);
  EXPECT_EQ(out.substr(0, 6), "prefix");
  EXPECT_EQ(DecodeFixed32(out.data() + 6 + 12), 1u) << "key_size";
}

TEST(EncodeRecordTest, EmptyValue) {
  std::string out;
  EncodeRecord(0, 0, "k", "", &out);
  ASSERT_EQ(out.size(), kRecordHeaderSize + 1);
  EXPECT_EQ(DecodeFixed32(out.data() + 16), 0u) << "value_size";
  EXPECT_EQ(ByteAt(out, 20), 0) << "empty value is NOT a tombstone";
}

TEST(EncodeRecordTest, TombstoneFlagIsBit0) {
  std::string out;
  EncodeRecord(0, kFlagTombstone, "k", "", &out);
  ASSERT_EQ(out.size(), kRecordHeaderSize + 1);
  EXPECT_EQ(ByteAt(out, 20), 0x01);
}

// ---------------------------------------------------------------------------
// DecodeHeader
// ---------------------------------------------------------------------------

TEST(DecodeHeaderTest, ParsesAllFields) {
  std::string out;
  EncodeRecord(42, kFlagTombstone, "abc", "de", &out);

  RecordHeader h{};
  ASSERT_OK(DecodeHeader(out, &h));
  EXPECT_EQ(h.crc, 0u);
  EXPECT_EQ(h.timestamp, 42u);
  EXPECT_EQ(h.key_size, 3u);
  EXPECT_EQ(h.value_size, 2u);
  EXPECT_EQ(h.flags, kFlagTombstone);
}

TEST(DecodeHeaderTest, IgnoresBytesAfterHeader) {
  std::string out;
  EncodeRecord(7, 0, "key", "value", &out);
  // Only the header bytes are passed — the key/value are not needed.
  RecordHeader h{};
  ASSERT_OK(DecodeHeader(std::string_view(out).substr(0, kRecordHeaderSize),
                         &h));
  EXPECT_EQ(h.key_size, 3u);
  EXPECT_EQ(h.value_size, 5u);
}

TEST(DecodeHeaderTest, TooShortIsCorruption) {
  const std::string short_buf(kRecordHeaderSize - 1, '\0');
  RecordHeader h{};
  EXPECT_TRUE(DecodeHeader(short_buf, &h).IsCorruption());
  EXPECT_TRUE(DecodeHeader("", &h).IsCorruption());
}

// ---------------------------------------------------------------------------
// DecodeRecord (used by Get and replay)
// ---------------------------------------------------------------------------

TEST(DecodeRecordTest, RoundTrip) {
  std::string out;
  EncodeRecord(99, 0, "apple", "red", &out);

  RecordHeader h{};
  std::string_view key, value;
  ASSERT_OK(DecodeRecord(out, &h, &key, &value));
  EXPECT_EQ(h.timestamp, 99u);
  EXPECT_EQ(h.flags, 0);
  EXPECT_EQ(key, "apple");
  EXPECT_EQ(value, "red");
}

TEST(DecodeRecordTest, RoundTripBinaryWithNulBytes) {
  const std::string key("k\0e\0y", 5);
  std::string value;
  for (int i = 0; i < 256; ++i) value.push_back(static_cast<char>(i));

  std::string out;
  EncodeRecord(0, 0, key, value, &out);

  RecordHeader h{};
  std::string_view k, v;
  ASSERT_OK(DecodeRecord(out, &h, &k, &v));
  EXPECT_EQ(k, key);
  EXPECT_EQ(v, value);
}

TEST(DecodeRecordTest, RoundTripEmptyValueAndTombstone) {
  std::string out;
  EncodeRecord(0, kFlagTombstone, "gone", "", &out);

  RecordHeader h{};
  std::string_view k, v;
  ASSERT_OK(DecodeRecord(out, &h, &k, &v));
  EXPECT_EQ(h.flags & kFlagTombstone, kFlagTombstone);
  EXPECT_EQ(k, "gone");
  EXPECT_TRUE(v.empty());
}

// key/value must be views into the input buffer, not copies.
TEST(DecodeRecordTest, KeyAndValuePointIntoBuffer) {
  std::string out;
  EncodeRecord(0, 0, "key", "value", &out);

  RecordHeader h{};
  std::string_view k, v;
  ASSERT_OK(DecodeRecord(out, &h, &k, &v));
  EXPECT_EQ(k.data(), out.data() + kRecordHeaderSize);
  EXPECT_EQ(v.data(), out.data() + kRecordHeaderSize + 3);
}

TEST(DecodeRecordTest, TruncatedRecordIsCorruption) {
  std::string out;
  EncodeRecord(0, 0, "key", "value", &out);
  out.pop_back();  // one byte short

  RecordHeader h{};
  std::string_view k, v;
  EXPECT_TRUE(DecodeRecord(out, &h, &k, &v).IsCorruption());
}

TEST(DecodeRecordTest, TrailingBytesAreCorruption) {
  std::string out;
  EncodeRecord(0, 0, "key", "value", &out);
  out.push_back('x');  // one byte too many

  RecordHeader h{};
  std::string_view k, v;
  EXPECT_TRUE(DecodeRecord(out, &h, &k, &v).IsCorruption());
}

TEST(DecodeRecordTest, ShorterThanHeaderIsCorruption) {
  RecordHeader h{};
  std::string_view k, v;
  EXPECT_TRUE(DecodeRecord("abc", &h, &k, &v).IsCorruption());
}

// A header claiming huge sizes must not cause an out-of-bounds read.
// key_size + value_size = 0xFFFFFFFF + 1 wraps to 0 in uint32_t, so a
// 32-bit sum would "match" this 21-byte buffer and then read 4 GiB past it.
TEST(DecodeRecordTest, HugeSizesInHeaderAreCorruption) {
  std::string buf(kRecordHeaderSize, '\0');
  EncodeFixed32(buf.data() + 12, 0xFFFFFFFFu);  // key_size
  EncodeFixed32(buf.data() + 16, 0x00000001u);  // value_size

  RecordHeader h{};
  std::string_view k, v;
  EXPECT_TRUE(DecodeRecord(buf, &h, &k, &v).IsCorruption());
}

}  // namespace
}  // namespace kvstore
