#include "kvstore/kvstore.h"

namespace kvstore {

KVStore::~KVStore() {
  // TODO(you): close if open. A destructor can't return a Status, so ignore it.
}

Status KVStore::Open(const std::string& dir) {
  (void)dir;
  // TODO(you): create dir if needed, open the data file, replay the log.
  return Status::NotImplemented("Open");
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
