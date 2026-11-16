# kvstore

A persistent key-value store in C++17, modeled on the [Bitcask](https://riak.com/assets/bitcask-intro.pdf) design: every write is appended to a log-structured data file on disk, and an in-memory hash index (the *keydir*) maps each key to the location of its latest value, so reads need at most one disk seek. The project is built incrementally: crash recovery with CRC-checked records, log compaction, thread-safe concurrent access, and a TCP server. Each milestone ships with GoogleTest coverage and runs under ASan/UBSan in CI.

## Build

```sh
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
# With sanitizers: -DKV_SANITIZE=address,undefined  (or thread)
```
