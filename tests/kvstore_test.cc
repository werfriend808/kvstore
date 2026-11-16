#include "kvstore/kvstore.h"

#include <gtest/gtest.h>

#include <string>

#include "test_util.h"

namespace kvstore {
namespace {

// Fixture: a store opened in a fresh temp directory.
class KVStoreTest : public testing::TempDirTest {
 protected:
  void SetUp() override {
    TempDirTest::SetUp();
    ASSERT_OK(db_.Open(dir().string()));
  }

  // Close and open again in the same directory: forces log replay.
  void Reopen() {
    ASSERT_OK(db_.Close());
    ASSERT_OK(db_.Open(dir().string()));
  }

  // Asserts that `key` maps to `expected`.
  void ExpectValue(std::string_view key, const std::string& expected) {
    std::string got;
    Status s = db_.Get(key, &got);
    ASSERT_TRUE(s.ok()) << "Get(" << key << "): " << s.ToString();
    EXPECT_EQ(got, expected);
  }

  void ExpectNotFound(std::string_view key) {
    std::string got;
    Status s = db_.Get(key, &got);
    EXPECT_TRUE(s.IsNotFound()) << "Get(" << key << "): " << s.ToString();
  }

  KVStore db_;
};

// ---------------------------------------------------------------------------
// Basic operations
// ---------------------------------------------------------------------------

TEST_F(KVStoreTest, PutThenGetRoundTrips) {
  ASSERT_OK(db_.Put("apple", "red"));
  ExpectValue("apple", "red");
}

TEST_F(KVStoreTest, OverwriteReturnsLatestValue) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Put("apple", "green"));
  ExpectValue("apple", "green");
}

TEST_F(KVStoreTest, KeysAreIndependent) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Put("banana", "yellow"));
  ExpectValue("apple", "red");
  ExpectValue("banana", "yellow");
}

TEST_F(KVStoreTest, GetMissingKeyIsNotFound) { ExpectNotFound("nope"); }

TEST_F(KVStoreTest, DeleteThenGetIsNotFound) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Delete("apple"));
  ExpectNotFound("apple");
}

TEST_F(KVStoreTest, DeleteMissingKeyIsOk) {
  EXPECT_OK(db_.Delete("never-existed"));
}

TEST_F(KVStoreTest, PutAfterDeleteRestoresKey) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Delete("apple"));
  ASSERT_OK(db_.Put("apple", "green"));
  ExpectValue("apple", "green");
}

// ---------------------------------------------------------------------------
// Edge-case values
// ---------------------------------------------------------------------------

// An empty value is a real value, not a deletion (decision 4).
TEST_F(KVStoreTest, EmptyValueIsStoredNotDeleted) {
  ASSERT_OK(db_.Put("empty", ""));
  ExpectValue("empty", "");
}

TEST_F(KVStoreTest, BinaryKeyAndValueWithNulBytes) {
  // Construct with explicit lengths so '\0' isn't treated as a terminator.
  const std::string key("k\0e\0y", 5);
  std::string value;
  for (int i = 0; i < 256; ++i) value.push_back(static_cast<char>(i));

  ASSERT_OK(db_.Put(key, value));
  ExpectValue(key, value);

  // A key that is a prefix up to the first '\0' must be a different key.
  ExpectNotFound(std::string("k", 1));
}

TEST_F(KVStoreTest, LargeValue) {
  std::string big(4 * 1024 * 1024, '\0');  // 4 MiB
  for (std::size_t i = 0; i < big.size(); ++i) {
    big[i] = static_cast<char>(i * 31 + 7);  // non-uniform pattern
  }
  ASSERT_OK(db_.Put("big", big));
  ExpectValue("big", big);
}

// ---------------------------------------------------------------------------
// Input validation
// ---------------------------------------------------------------------------

TEST_F(KVStoreTest, EmptyKeyIsRejected) {
  EXPECT_TRUE(db_.Put("", "v").IsInvalidArgument());
}

TEST_F(KVStoreTest, KeyAtLimitIsAcceptedAndOverLimitRejected) {
  const std::string at_limit(kMaxKeySize, 'k');
  ASSERT_OK(db_.Put(at_limit, "v"));
  ExpectValue(at_limit, "v");

  const std::string too_big(kMaxKeySize + 1, 'k');
  EXPECT_TRUE(db_.Put(too_big, "v").IsInvalidArgument());
}

TEST_F(KVStoreTest, ValueOverLimitIsRejected) {
  const std::string too_big(kMaxValueSize + 1, 'v');
  EXPECT_TRUE(db_.Put("k", too_big).IsInvalidArgument());
  ExpectNotFound("k");  // a rejected Put leaves no trace
}

TEST_F(KVStoreTest, GetWithNullOutputIsRejected) {
  ASSERT_OK(db_.Put("k", "v"));
  EXPECT_TRUE(db_.Get("k", nullptr).IsInvalidArgument());
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST(KVStoreLifecycleTest, OperationsBeforeOpenAreRejected) {
  KVStore db;
  std::string v;
  EXPECT_TRUE(db.Put("k", "v").IsInvalidArgument());
  EXPECT_TRUE(db.Get("k", &v).IsInvalidArgument());
  EXPECT_TRUE(db.Delete("k").IsInvalidArgument());
}

TEST(KVStoreLifecycleTest, CloseWithoutOpenIsOk) {
  KVStore db;
  EXPECT_OK(db.Close());
}

TEST_F(KVStoreTest, DoubleOpenIsRejected) {
  EXPECT_TRUE(db_.Open(dir().string()).IsInvalidArgument());
}

TEST_F(KVStoreTest, OperationsAfterCloseAreRejected) {
  ASSERT_OK(db_.Close());
  EXPECT_TRUE(db_.Put("k", "v").IsInvalidArgument());
}

using KVStoreDirTest = testing::TempDirTest;

TEST_F(KVStoreDirTest, OpenCreatesMissingDirectory) {
  const auto missing = dir() / "nested" / "db";
  KVStore db;
  ASSERT_OK(db.Open(missing.string()));
  ASSERT_OK(db.Put("k", "v"));
  ASSERT_OK(db.Close());
  EXPECT_TRUE(std::filesystem::is_directory(missing));
}

// ---------------------------------------------------------------------------
// Persistence (log replay on Open)
// ---------------------------------------------------------------------------

TEST_F(KVStoreTest, DataSurvivesReopen) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Put("banana", "yellow"));
  Reopen();
  ExpectValue("apple", "red");
  ExpectValue("banana", "yellow");
}

// Replay must apply records in log order: the last write wins.
TEST_F(KVStoreTest, OverwriteSurvivesReopen) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Put("apple", "green"));
  Reopen();
  ExpectValue("apple", "green");
}

// Replay must honor tombstones.
TEST_F(KVStoreTest, DeleteSurvivesReopen) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Delete("apple"));
  Reopen();
  ExpectNotFound("apple");
}

TEST_F(KVStoreTest, PutAfterDeleteSurvivesReopen) {
  ASSERT_OK(db_.Put("apple", "red"));
  ASSERT_OK(db_.Delete("apple"));
  ASSERT_OK(db_.Put("apple", "green"));
  Reopen();
  ExpectValue("apple", "green");
}

TEST_F(KVStoreTest, EdgeCaseValuesSurviveReopen) {
  const std::string bin_key("a\0b", 3);
  const std::string bin_val("\0\xff\0", 3);
  const std::string big(1024 * 1024, 'x');
  ASSERT_OK(db_.Put("empty", ""));
  ASSERT_OK(db_.Put(bin_key, bin_val));
  ASSERT_OK(db_.Put("big", big));
  Reopen();
  ExpectValue("empty", "");
  ExpectValue(bin_key, bin_val);
  ExpectValue("big", big);
}

// New writes after reopen must append after existing data, not overwrite it.
TEST_F(KVStoreTest, WritesAfterReopenAppend) {
  ASSERT_OK(db_.Put("a", "1"));
  Reopen();
  ASSERT_OK(db_.Put("b", "2"));
  Reopen();
  ExpectValue("a", "1");
  ExpectValue("b", "2");
}

TEST_F(KVStoreTest, ManyKeysSurviveReopen) {
  constexpr int kN = 1000;
  for (int i = 0; i < kN; ++i) {
    ASSERT_OK(db_.Put("key" + std::to_string(i), "val" + std::to_string(i)));
  }
  for (int i = 0; i < kN; i += 2) {
    ASSERT_OK(db_.Delete("key" + std::to_string(i)));
  }
  Reopen();
  for (int i = 0; i < kN; ++i) {
    const std::string key = "key" + std::to_string(i);
    if (i % 2 == 0) {
      ExpectNotFound(key);
    } else {
      ExpectValue(key, "val" + std::to_string(i));
    }
  }
}

}  // namespace
}  // namespace kvstore
