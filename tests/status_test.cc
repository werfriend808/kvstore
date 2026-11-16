#include "kvstore/status.h"

#include <gtest/gtest.h>

namespace kvstore {
namespace {

TEST(StatusTest, DefaultIsOk) {
  Status s;
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.ToString(), "OK");
}

TEST(StatusTest, FactoriesSetCodeAndMessage) {
  Status s = Status::IOError("disk full");
  EXPECT_FALSE(s.ok());
  EXPECT_TRUE(s.IsIOError());
  EXPECT_FALSE(s.IsNotFound());
  EXPECT_EQ(s.message(), "disk full");
  EXPECT_EQ(s.ToString(), "IOError: disk full");
}

TEST(StatusTest, NotFoundWithoutMessage) {
  Status s = Status::NotFound();
  EXPECT_TRUE(s.IsNotFound());
  EXPECT_EQ(s.ToString(), "NotFound");
}

}  // namespace
}  // namespace kvstore
