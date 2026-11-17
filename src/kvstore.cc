#include "kvstore/kvstore.h"

#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace kvstore {

KVStore::~KVStore() {
  // TODO(you): close if open. A destructor can't return a Status, so ignore it.
}

Status KVStore::Open(const std::string& dir) {
  if (fd_ >= 0) {
    return Status::InvalidArgument("already open");
  }

  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    return Status::IOError(ec.message());
  }

  std::filesystem::path path = std::filesystem::path(dir) / "000001.data";

  fd_ = ::open(path.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);

  if (fd_ < 0) {
    return Status::IOError(std::strerror(errno));
  }

  return Status::Ok();
}

Status KVStore::Put(std::string_view key, std::string_view value) {
  (void)key;
  (void)value;
  // TODO(you): validate, encode the record, append, update the keydir.
  return Status::NotImplemented("Put");
}

Status KVStore::Get(std::string_view key, std::string* value) {
  (void)key;
  (void)value;
  // TODO(you): look up the keydir, read the record once, extract the value.
  return Status::NotImplemented("Get");
}

Status KVStore::Delete(std::string_view key) {
  (void)key;
  // TODO(you): append a tombstone, remove from the keydir.
  return Status::NotImplemented("Delete");
}

Status KVStore::Close() {
  // TODO(you): flush and close the data file, clear state.
  return Status::NotImplemented("Close");
}

}  // namespace kvstore
