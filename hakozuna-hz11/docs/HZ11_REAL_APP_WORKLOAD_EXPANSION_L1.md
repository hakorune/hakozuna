# HZ11RealAppWorkloadExpansion-L1

Status: **GO for fine128 real-app breadth.** fine128 is competitive with tcmalloc on
3 new real-app workload families: **redis build** (7% faster, 20% less RSS), **redis
benchmark** (3% slower, 34% less RSS), and **git repack** (3% slower, 22% more RSS).
cap lanes ≈ fine128 on all (sh6bench specialists confirmed for real apps too). No
correctness regression (redis rc=0; malloc_usable_size fix holds).

## Context

Expands the real-app evidence beyond espresso/sqlite3/rocksdb to 3 new families: a Git
repo workload (clone+repack), a real C compile workload (redis-6.2.7 source build),
and a real server workload (redis-benchmark). All via the existing `run_hz11_real_app_gate.sh`
(process-tree RSS, HZ11 counters, `--preclean-cmd`).

## Evidence (5 lanes; all rc=0)

### git clone + repack (RUNS=3)

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 1.347 | 1.000 | 248124 | 1.000 |
| jemalloc | 1.417 | 1.052 | 312520 | 1.260 |
| fine128 | 1.390 | 1.032 | 301484 | 1.215 |
| cap768-bytes | 1.399 | 1.039 | 292628 | 1.179 |
| cap1024-bytes | 1.411 | 1.048 | 294916 | 1.189 |

```text
near-parity on wall (1.03-1.05x). RSS is HIGHER than tcmalloc (+19-22%): git repack's
large-buffer packfile workload doesn't suit HZ11's arena (arena overhead adds RSS).
```

### redis-6.2.7 source build (RUNS=2; MALLOC=libc)

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 7.477 | 1.000 | 481344 | 1.000 |
| jemalloc | 7.056 | 0.944 | 474614 | 0.986 |
| **fine128** | **6.953** | **0.930** | **388078** | **0.806** |
| cap768-bytes | 6.941 | 0.928 | 427234 | 0.888 |
| cap1024-bytes | 6.915 | 0.925 | 423932 | 0.881 |

```text
fine128 is FASTER than tcmalloc (0.930x = 7% faster!) AND uses ~20% less RSS (0.806x).
A genuine real-compile-workload win. gcc/cc1's small-object churn fits HZ11's thread cache
(xfer_insert=0: no overflow). jemalloc also slightly faster (0.944x).
```

### redis-benchmark (RUNS=3; server lpush+lrange)

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 0.780 | 1.000 | 38528 | 1.000 |
| jemalloc | 0.782 | 1.003 | 27264 | 0.708 |
| fine128 | 0.806 | 1.033 | **25472** | **0.661** |
| cap768-bytes | 0.808 | 1.036 | 25728 | 0.668 |
| cap1024-bytes | 0.807 | 1.035 | 25728 | 0.668 |

```text
fine128 ~3% slower than tcmalloc on wall (1.033x) but uses ~34% LESS RSS (0.661x).
Redis runs cleanly (rc=0; the malloc_usable_size fix covers server-style alloc patterns).
```

### Redis RSS caveat

```text
Redis RSS is process-tree RSS (server + benchmark client combined), not server-only RSS.
Use it as a conservative total-process footprint. Server-only RSS requires a future
Redis-specific runner.
```

## Decision

```text
GO for fine128 real-app breadth. Across 6 real-app workloads now (espresso, sqlite3,
rocksdb, git repack, redis build, redis benchmark):
- fine128 is at or near tcmalloc on wall on 5/6 (redis build is FASTER; git repack
  and redis benchmark are ~3% slower; espresso near-parity; rocksdb near-parity).
- RSS wins on 4/6 (espresso ~3x less, redis build ~20% less, redis benchmark ~34%
  less, sqlite3 ~17% less). RSS loses on git repack (+22%) and rocksdb (+5%).
- cap768/cap1024 ≈ fine128 on all 3 new workloads — no real-app meaning. Confirmed
  sh6bench specialists.
- No correctness regression (all rc=0; redis server-style works).
```

## Claim boundary

```text
Allowed:
  - on redis source build (real C compile), fine128 is faster than tcmalloc (0.93x)
    with ~20% less RSS.
  - on redis-benchmark (real server, this config), fine128 is ~3% slower with ~34% less
    process-tree RSS (server+client combined, not server-only).
  - on git repack, fine128 is near-parity on wall but uses more RSS (arena overhead).
  - fine128 is the recommended general opt-in HZ11 lane with broadest real-app evidence.

Not allowed:
  - HZ11 generally beats tcmalloc (6 workloads, 1 machine; mixed results).
  - cap lanes help real apps (they don't; ≈ fine128 on all real-app workloads).
  - Redis RSS is server-only (it's process-tree total).
  - HZ11 is production-safe (research line; 1 machine; synthetic + real-app only).
```

## Files / raw data

```text
no code change; no new runner; no build (existing lanes with malloc_usable_size fix).
measurement: run_hz11_real_app_gate.sh (3 new workloads via --preclean-cmd + --workload-cmd).
raw summaries: ephemeral tmp outdirs; tables above reproduce the data.
```
