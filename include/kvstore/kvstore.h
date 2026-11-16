#ifndef KVSTORE_KVSTORE_H_
#define KVSTORE_KVSTORE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "kvstore/status.h"

namespace kvstore {

// Size limits enforced by Put (see PROGRESS.md, decision 6).
// They keep record_size within uint32_t and bound memory used by one Get.
inline constexpr std::size_t kMaxKeySize = 64 * 1024;           // 64 KiB
inline constexpr std::size_t kMaxValueSize = 64 * 1024 * 1024;  // 64 MiB

// A persistent key-value store using the Bitcask design:
// an append-only data file on disk plus an in-memory keydir
// (key -> location of the latest record).
//
// Keys and values are arbitrary byte strings: they may contain '\0' and any
// other byte. Values may be empty. Keys may not be empty.
//
// Not thread-safe (until M4). Not copyable.
class KVStore {
 public:
  KVStore() = default;
  ~KVStore();  // closes the store if still open (errors are ignored)

  KVStore(const KVStore&) = delete;
  KVStore& operator=(const KVStore&) = delete;

  // Opens the store in directory `dir`, creating the directory and data file
  // if they don't exist, and rebuilds the keydir by replaying the log.
  //   InvalidArgument  if already open
  //   IOError          if the directory or file can't be created/read
  Status Open(const std::string& dir);

  // Stores `value` under `key`, replacing any previous value.
  //   InvalidArgument  if not open, key is empty, or key/value exceed limits
  //   IOError          if the write fails
  Status Put(std::string_view key, std::string_view value);

  // On success, sets *value to the latest value for `key`.
  //   NotFound         if the key doesn't exist or was deleted
  //   InvalidArgument  if not open or value == nullptr
  //   IOError          if the read fails
  // *value is unspecified on any non-OK result.
  Status Get(std::string_view key, std::string* value);

  // Removes `key` by appending a tombstone.
  // Deleting a key that doesn't exist is OK (idempotent), and writes nothing.
  //   InvalidArgument  if not open
  //   IOError          if the write fails
  Status Delete(std::string_view key);

  // Flushes and closes the data file. Closing a store that isn't open is OK.
  //   IOError          if flushing/closing fails
  Status Close();

 private:
  struct KeydirEntry {
    uint32_t file_id;
    uint64_t record_offset;
    uint32_t record_size;
  };

  std::unordered_map<std::string, KeydirEntry> keydir_;
  int fd_ = -1;
};

}  // namespace kvstore

#endif  // KVSTORE_KVSTORE_H_
