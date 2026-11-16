#ifndef KVSTORE_STATUS_H_
#define KVSTORE_STATUS_H_

#include <string>
#include <string_view>
#include <utility>

namespace kvstore {

// Result of an operation. Modeled on leveldb::Status / absl::Status.
//
// Every fallible function returns a Status. It is [[nodiscard]], so ignoring
// one is a compile error under -Werror.
//
//   Status s = db.Get("k", &value);
//   if (s.IsNotFound()) { ... }        // a normal outcome, not a failure
//   else if (!s.ok()) { return s; }    // a real error: propagate it
class [[nodiscard]] Status {
 public:
  enum class Code {
    kOk = 0,
    kNotFound,         // key does not exist (expected outcome)
    kInvalidArgument,  // caller error: bad input, wrong state (e.g. not open)
    kIOError,          // OS-level failure: open/read/write/fsync failed
    kCorruption,       // on-disk data failed validation (bad CRC, truncated)
    kNotImplemented,   // stub placeholder
  };

  Status() = default;  // OK

  static Status Ok() { return Status(); }
  static Status NotFound(std::string_view msg = {}) {
    return Status(Code::kNotFound, msg);
  }
  static Status InvalidArgument(std::string_view msg) {
    return Status(Code::kInvalidArgument, msg);
  }
  static Status IOError(std::string_view msg) {
    return Status(Code::kIOError, msg);
  }
  static Status Corruption(std::string_view msg) {
    return Status(Code::kCorruption, msg);
  }
  static Status NotImplemented(std::string_view msg = {}) {
    return Status(Code::kNotImplemented, msg);
  }

  bool ok() const { return code_ == Code::kOk; }
  bool IsNotFound() const { return code_ == Code::kNotFound; }
  bool IsInvalidArgument() const { return code_ == Code::kInvalidArgument; }
  bool IsIOError() const { return code_ == Code::kIOError; }
  bool IsCorruption() const { return code_ == Code::kCorruption; }
  bool IsNotImplemented() const { return code_ == Code::kNotImplemented; }

  Code code() const { return code_; }
  const std::string& message() const { return message_; }

  // e.g. "IOError: open(/tmp/x/data): No such file or directory"
  std::string ToString() const;

 private:
  Status(Code code, std::string_view msg) : code_(code), message_(msg) {}

  Code code_ = Code::kOk;
  std::string message_;
};

}  // namespace kvstore

#endif  // KVSTORE_STATUS_H_
