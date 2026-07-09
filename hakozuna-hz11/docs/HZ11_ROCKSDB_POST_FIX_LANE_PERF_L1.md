# HZ11RocksdbPostFixLanePerf-L1

Status: **GO for fine128 on real multi-thread DB; cap-specialist NO-GO.** fine128 is
near-parity with tcmalloc on rocksdb (wall 1.009×, readrandom 1.01×, RSS 0.956×).
cap768/cap1024 are ≈ fine128 or slightly slower with higher RSS — their sh6bench win
does NOT translate to rocksdb. The cap lanes are confirmed **sh6bench-synthetic
specialists only**. This closes the "does cap help real multi-thread" question.

## Context

`HZ11RocksdbReadrandomCrashRootCause-L1` fixed the `malloc_usable_size` segfault (all
lanes rebuilt with the fix). This box measures lane perf on rocksdb (fillrandom+readrandom,
8 threads, num=200000) to test whether cap768/cap1024's sh6bench churn win generalizes
to a real multi-threaded DB.

## Evidence (RUNS=3; all rc=0, no crash regression)

### Runner summary (medians)

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc | xfer_insert | cached_bytes |
|---|---:|---:|---:|---:|---:|---:|
| tcmalloc | 3.725 | 1.000 | 124672 | 1.000 | 0 | 0 |
| jemalloc | 3.810 | 1.023 | 209344 | 1.679 | 0 | 0 |
| **fine128** | **3.758** | **1.009** | **119164** | **0.956** | 4625064 | 0 |
| cap768-bytes | 3.811 | 1.023 | 119464 | 0.958 | 25361 | 5443760 |
| cap1024-bytes | 3.791 | 1.018 | 131712 | 1.056 | 25954 | 5827456 |

### rocksdb fillrandom/readrandom (medians, from `.out`)

| Allocator | fillrandom µs/op | readrandom µs/op | found |
|---|---:|---:|---|
| tcmalloc | 11.79 | 5.79 | ~199922/200000 |
| jemalloc | 12.12 | 5.96 | ~199938 |
| **fine128** | **12.24** | **5.85** | ~199940 |
| cap768-bytes | 12.23 | 6.00 | ~199943 |
| cap1024-bytes | 12.14 | 5.89 | ~199928 |

```text
fine128 vs tcmalloc: wall 1.009x, fillrandom 1.038x, readrandom 1.010x, RSS 0.956x.
  -> near-PARITY on a real multi-thread DB (8 threads, read-heavy). Strong GO.
cap768/cap1024 vs fine128: wall 1.014x / 1.009x (slightly slower); readrandom 1.026x /
  1.007x (same/slower); RSS +0.3% / +10.5%. xfer_insert DID drop (4.6M -> 25K) -- the
  cap's transfer-absorption mechanism works -- but rocksdb's wall bottleneck is NOT
  allocator transfer traffic (it's I/O, memtable, block-cache). So the cap lever bites
  on sh6bench (pure allocator churn) but not on rocksdb (real DB with I/O-bound reads).
```

## Decision (per the brief)

```text
GO for fine128 real multi-thread:
  - rc=0 (no crash regression after the malloc_usable_size fix).
  - readrandom/fillrandom ~tcmalloc (1.01x / 1.04x).
  - RSS slightly below tcmalloc (0.956x).
  - Consistent with espresso (near-parity + 3x RSS win) and sqlite3 (mixed but no crash).

cap-specialist NO-GO:
  - cap768/cap1024 ≈ fine128 or slightly slower on rocksdb, with higher RSS (cap1024 +11%).
  - The sh6bench win (1.2x vs 9.8x) does NOT translate to a real multi-thread DB.
  - cap lanes stay sh6bench-synthetic specialists.

No correctness regression: all lanes rc=0, all found ~199930/200000.
```

### Why cap doesn't translate

```text
sh6bench is PURE allocator churn: every op is a malloc/free of a small object, and the
dominant cost is thread-cache overflow -> transfer-cache mutex traffic. Big CAP cuts that
traffic (xfer_insert drops 99%) and wall follows. rocksdb is a REAL DB: the allocator is
one component among I/O, memtable flush, block-cache lookup, and compression. The cap DID
cut allocator transfer traffic (xfer_insert 4.6M -> 25K), but that traffic is NOT rocksdb's
wall bottleneck, so wall doesn't improve. The cap lever targets a cost that's dominant in
sh6bench but not in rocksdb.
```

## Claim boundary

```text
Allowed:
  - on rocksdb db_bench (fillrandom+readrandom, 8 threads, num=200000, no compression,
    no WAL, this config, one machine), fine128 is near-parity with tcmalloc on wall
    (1.009x) with slightly lower RSS (0.956x).
  - cap768/cap1024 do NOT improve rocksdb vs fine128; they are sh6bench specialists.

Not allowed:
  - HZ11 generally beats tcmalloc, or is production-safe (one config, one machine).
  - cap1024 helps real multi-thread apps (rocksdb says no).
  - rocksdb全般で勝つ (this is one db_bench config, not a general rocksdb claim).
```

## Files / raw data

```text
no code change; no new runner; no build (all lanes already had the malloc_usable_size fix).
measurement: run_hz11_real_app_gate.sh (rocksdb_perf, RUNS=3, tcmalloc + jemalloc +
             fine128 + cap768-bytes + cap1024-bytes).
raw summary: ephemeral tmp outdir; tables above reproduce the data.
```
