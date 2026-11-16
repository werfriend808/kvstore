#ifndef KVSTORE_TESTS_TEST_UTIL_H_
#define KVSTORE_TESTS_TEST_UTIL_H_

#include <gtest/gtest.h>
#include <stdlib.h>

#include <filesystem>
#include <string>
#include <system_error>

#include "kvstore/status.h"

namespace kvstore::testing {

// Each test gets a fresh, unique directory, removed afterwards.
class TempDirTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::string tmpl =
        (std::filesystem::temp_directory_path() / "kvstore-test-XXXXXX")
            .string();
    ASSERT_NE(mkdtemp(tmpl.data()), nullptr) << "mkdtemp failed";
    dir_ = tmpl;
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);  // best effort
  }

  const std::filesystem::path& dir() const { return dir_; }

 private:
  std::filesystem::path dir_;
};

}  // namespace kvstore::testing

// Print the full Status (code + message) on failure.
#define ASSERT_OK(expr)                              \
  do {                                               \
    const ::kvstore::Status _s = (expr);             \
    ASSERT_TRUE(_s.ok()) << _s.ToString();           \
  } while (0)

#define EXPECT_OK(expr)                              \
  do {                                               \
    const ::kvstore::Status _s = (expr);             \
    EXPECT_TRUE(_s.ok()) << _s.ToString();           \
  } while (0)

#endif  // KVSTORE_TESTS_TEST_UTIL_H_
