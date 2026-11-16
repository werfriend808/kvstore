# PROGRESS

> At the start of each session, read this file first. At the end, update it.
> Coach rule: I write the core logic (serialization, index, recovery, compaction, locks, thread pool, sockets) myself.

## Milestones

### M1 — Skeleton + single-file Bitcask
- [x] Project scaffold (CMake, GoogleTest via FetchContent, KV_SANITIZE option)
- [x] GitHub Actions CI (Debug, ASan+UBSan)
- [x] Record format + keydir entry design (decided by me, reviewed)
- [x] Error handling strategy (Status vs exceptions) → Status
- [x] `KVStore` interface (Open/Put/Get/Delete/Close) with stubbed implementation
- [x] Failing tests: round-trip, overwrite, missing key, delete, empty value, binary key/value, large value, reopen persistence (26 failing, as expected)
- [ ] Implement Put/Get/Delete
- [ ] Implement log replay on Open (rebuild keydir)
- [ ] All M1 tests green under Debug + ASan/UBSan

### M2 — Durability & crash recovery
- [ ] CRC32 per record, verified on read and replay
- [ ] Torn/partial trailing write handling (truncate at last valid record)
- [ ] fsync policy (configurable) + tests that simulate corruption

### M3 — Multiple files & compaction
- [ ] Rotate active file at size threshold; immutable older files
- [ ] Merge/compaction: drop stale values and tombstones
- [ ] Hint files for fast startup

### M4 — Concurrency
- [ ] Thread-safe KVStore (reader/writer locking)
- [ ] Single-writer / multi-reader stress tests under TSan
- [ ] Background compaction without blocking reads

### M5 — Network server
- [ ] Simple text/binary protocol over TCP
- [ ] Thread pool handling client connections
- [ ] Client CLI + integration tests

### M6 — Polish
- [ ] Benchmarks (throughput/latency vs. value size, before/after compaction)
- [ ] Design doc in README (format diagram, tradeoffs)
- [ ] Code cleanup, docs, final review

## Design decisions

### Record format (on disk)
```
┌──────────────────────── header (21 bytes) ─────────────────────┬──── body ────┐
│  crc32 │ timestamp │ key_size │ value_size │ flags             │ key │ value │
│ uint32 │  uint64   │  uint32  │  uint32    │ uint8             │     │       │
└────────────────────────────────────────────────────────────────┴──────────────┘
```
1. **key_size / value_size = uint32_t.** Allows large values; symmetric and simple. (uint16 for keys would save 2 B/record — rejected for simplicity.) Code will enforce smaller max key/value sizes (memory safety on Get; see #6).
2. **All integers little-endian, fixed.** Same bytes on any machine. LE matches nearly all CPUs, so encoding compiles to a plain copy. Fields are serialized one by one, never by writing a struct (padding + endianness).
3. **CRC32 slot at the front, reserved now (written as 0 in M1, computed in M2).** Fixes the format up front so M2 doesn't rewrite read/write code and tests. Placed first so it covers one contiguous range (everything after it). CRC *detects* corruption; it doesn't repair it.
4. **Tombstone = flags byte, bit0.** `value_size == 0` rejected: collides with empty values. Sentinel `value_size == UINT32_MAX` works but is implicit; flags is explicit and leaves 7 bits for future use.
5. **timestamp = uint64_t.** For debugging and future features (TTL); also fixes the format up front. **Recency is decided by log position (file_id, offset), never by timestamp**, because wall clocks can go backwards.

### Keydir entry (in memory)
```cpp
struct KeydirEntry {
    uint32_t file_id;        // constant in M1; needed for M3 multi-file
    uint64_t record_offset;  // start of the whole record; files can exceed 4 GB
    uint32_t record_size;    // header + key + value
};
```
6. **Point at the whole record, not just the value.** Get still does exactly one read, and M2 can verify CRC on Get. (Value-only offset rejected: can't check CRC.)
   - `record_offset` is 64-bit: a 32-bit offset silently wraps past 4 GB → reads the wrong record.
   - `record_size` is 32-bit **only because Put enforces max key/value sizes**; saves memory per key.
   - Values live only on disk; keydir holds key → location (keys must fit in RAM).

### API
7. **Errors via `Status` return, no exceptions.** Failure is visible in every signature; `[[nodiscard]]` + `-Werror` makes ignoring a Status a compile error. Same approach as LevelDB/RocksDB. "Key missing" is `Status::NotFound()`, distinct from real errors (`IOError`, `Corruption`).
   - `Get(key, std::string* value)` instead of returning `optional` — optional can't distinguish "missing" from "I/O failed".
   - Limits: `kMaxKeySize` = 64 KiB, `kMaxValueSize` = 64 MiB (`InvalidArgument` if exceeded). Empty keys rejected; empty values allowed.
   - `Open` creates the directory if missing. `Delete` of a missing key is OK and writes nothing. `Close` without `Open` is OK. Any op while not open → `InvalidArgument`.

## Session log

### Session 1 — 2026-11-15
- Scaffolded repo: CMake (C++17, `-Wall -Wextra -Wpedantic -Werror` on own targets only), GoogleTest v1.15.2 via FetchContent, `KV_SANITIZE` option (validated; rejects thread+address).
- CI: Debug and ASan+UBSan matrix on ubuntu-latest.
- Smoke test passes locally in both configs.
- Next: Bitcask concepts → decide record format & error handling → interface → failing tests.
