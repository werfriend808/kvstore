#include "kvstore/status.h"

namespace kvstore {

std::string Status::ToString() const {
  const char* name = "Unknown";
  switch (code_) {
    case Code::kOk:
      return "OK";
    case Code::kNotFound:
      name = "NotFound";
      break;
    case Code::kInvalidArgument:
      name = "InvalidArgument";
      break;
    case Code::kIOError:
      name = "IOError";
      break;
    case Code::kCorruption:
      name = "Corruption";
      break;
    case Code::kNotImplemented:
      name = "NotImplemented";
      break;
  }
  std::string out(name);
  if (!message_.empty()) {
    out += ": ";
    out += message_;
  }
  return out;
}

}  // namespace kvstore
