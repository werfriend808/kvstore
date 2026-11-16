# PROGRESS

> At the start of each session, read this file first. At the end, update it.
> Coach rule: I write the core logic (serialization, index, recovery, compaction, locks, thread pool, sockets) myself.

## Milestones

### M1 — Skeleton + single-file Bitcask
- [x] Project scaffold (CMake, GoogleTest via FetchContent, KV_SANITIZE option)
- [x] GitHub Actions CI (Debug, ASan+UBSan)
- [ ] Record format + keydir entry design (decided by me, reviewed)
- [ ] Error handling strategy (Status vs exceptions)
- [ ] `KVStore` interface (Open/Put/Get/Delete/Close) with stubbed implementation
- [ ] Failing tests: round-trip, overwrite, missing key, delete, empty value, binary key/value, large value, reopen persistence
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
_(record decision + reason here)_

## Session log

### Session 1 — 2026-11-15
- Scaffolded repo: CMake (C++17, `-Wall -Wextra -Wpedantic -Werror` on own targets only), GoogleTest v1.15.2 via FetchContent, `KV_SANITIZE` option (validated; rejects thread+address).
- CI: Debug and ASan+UBSan matrix on ubuntu-latest.
- Smoke test passes locally in both configs.
- Next: Bitcask concepts → decide record format & error handling → interface → failing tests.
