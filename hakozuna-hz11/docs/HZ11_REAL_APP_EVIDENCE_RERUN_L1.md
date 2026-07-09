# HZ11RealAppEvidenceRerun-L1

Status: **Claim-grade real-app evidence (corrected process-tree RSS runner, RUNS=10).
The result BOUNDS the real-app claim hard.** On the apps that run, espresso is a genuine
HZ11 win (near-parity wall + ~3x less RSS) but sqlite3 is mixed (7-12% slower + modest
RSS win). Critically, **every HZ11 lane SEGFAULTS on rocksdb's multi-threaded
readrandom** while tcmalloc/jemalloc are clean — a fundamental HZ11 correctness gap that
blocks any real-app / multi-thread deployment claim until fixed. This is the most
important finding of the real-app track.

## Context

`HZ11LaneFullEvidenceGate-L1`'s real-app runner under-sampled RSS (only the `bash -c`
wrapper PID), so its strong ~0.4x RSS ratios were not claim-grade. The runner is now
fixed to sample the **whole process tree**. This box reruns the real-app evidence
claim-grade (RUNS=10) with the corrected runner and adds a **multi-threaded real DB**
(rocksdb) — the workload class that single-threaded espresso/sqlite3 could not probe.

## Runner correction (carry-forward)

`run_hz11_real_app_gate.sh` now sums `VmRSS`/`VmHWM` across the process tree
(`collect_tree_pids`/`read_tree_kb`), plus a new `--preclean-cmd` (untimed reset before
each measured run). The earlier parent-PID ratios are superseded by the process-tree
numbers below.

## Evidence (claim-grade, RUNS=10 unless noted; 6 lanes)

### espresso (real SPEC app, single-threaded) — RUNS=10

| allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 3.831 | 1.000 | 8384 | 1.000 |
| jemalloc | 4.117 | 1.075 | 5120 | 0.611 |
| span-transfer | 3.957 | 1.033 | 2688 | 0.321 |
| fine128 | 3.936 | 1.027 | 2688 | 0.321 |
| cap768-bytes | 3.916 | 1.022 | 2816 | 0.336 |
| cap1024-bytes | 3.933 | 1.027 | 2816 | 0.336 |

```text
espresso verdict: HZ11 is NEAR-PARITY with tcmalloc on wall (1.022-1.033x) AND uses
~3x LESS RSS (0.32-0.34x). Genuine real-app win. Lane choice barely matters here.
```

### sqlite3 (real database, in-memory) — RUNS=10

| allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 0.126 | 1.000 | 21120 | 1.000 |
| jemalloc | 0.134 | 1.063 | 17408 | 0.824 |
| span-transfer | **FAIL (abort)** | NA | NA | NA |
| fine128 | 0.135 | 1.071 | 17536 | 0.830 |
| cap768-bytes | 0.141 | 1.119 | 17984 | 0.852 |
| cap1024-bytes | 0.139 | 1.099 | 18048 | 0.855 |

```text
sqlite3 verdict: MIXED. HZ11 is 7-12% SLOWER than tcmalloc on wall (1.07-1.12x) with
only a MODEST RSS win (~0.83x). Not the earlier (buggy) near-parity + 0.4x RSS.
span-transfer FAILS (the central-overflow abort, fixed in fine128). jemalloc is the
closest to tcmalloc here (1.063x, 0.824x RSS).
```

### rocksdb db_bench (real multi-threaded DB, fillrandom+readrandom, 8 threads) — RUNS=2

| allocator | status | wall sec | wall/tcmalloc |
|---|---|---:|---:|
| tcmalloc | OK | 3.705 | 1.000 |
| jemalloc | OK | 3.817 | 1.030 |
| span-transfer | **FAIL (segfault)** | NA | NA |
| fine128 | **FAIL (segfault)** | NA | NA |
| cap768-bytes | **FAIL (segfault)** | NA | NA |
| cap1024-bytes | **FAIL (segfault)** | NA | NA |

```text
rocksdb verdict: EVERY HZ11 lane SEGFAULTS (signal 11) on rocksdb's multi-threaded
readrandom (fillrandom completes ~2.1s, then readrandom crashes). tcmalloc and jemalloc
complete cleanly (tcmalloc fillrandom 1.95s / readrandom 1.12s). This isolates the crash
to HZ11 -- a FUNDAMENTAL correctness bug on a multi-threaded real workload, not a lane
quirk. (Root cause unattributed here; likely mmap/large-alloc or thread-free classify on
a non-arena pointer -- a separate debug box.)
```

## Decision

```text
GO for evidence (the rerun produced decisive, claim-bounding findings), but the findings
RESTRICT the real-app claim strongly:
- espresso (single-threaded compute): HZ11 near-parity + ~3x RSS win. GENUINE.
- sqlite3 (single-threaded DB): HZ11 7-12% slower + modest RSS win. MIXED, not a win.
- rocksdb (multi-threaded DB): ALL HZ11 lanes CRASH. CORRECTNESS GAP.

So "HZ11 is real-app-competitive" is workload-dependent and BOUNDED: true on a
single-threaded compute app, mixed on a single-threaded DB, BROKEN on a multi-threaded
DB. The cap1024-on-real-multi-thread question is BLOCKED by the crash (cannot measure
perf when it segfaults).
```

## Next-step warrant (reprioritized)

```text
1. P0 -- FIX the rocksdb readrandom segfault. This is a fundamental HZ11 correctness bug
   on multi-threaded real reads (all lanes). No real-app / deployment / multi-thread
   claim is defensible until it is root-caused and fixed. This supersedes class-range CAP
   and paper positioning.
2. After the fix: re-run rocksdb to (a) confirm correctness and (b) finally answer
   whether cap1024's sh6bench churn win translates to a real multi-threaded DB.
3. Paper/positioning may proceed ONLY with the bounded claim (espresso competitive;
   sqlite3 mixed; multi-thread real-app correctness was broken and is being fixed). Do
   NOT claim general real-app competitiveness.
```

## Claim boundary (tightened by this box)

```text
Allowed:
  - on espresso (single-threaded real app), HZ11 (fine128/cap768/cap1024) is near-parity
    with tcmalloc on wall and uses ~3x less RSS (RUNS=10, process-tree RSS).
  - on sqlite3, HZ11 is slightly slower (~1.07-1.12x) with a modest RSS win (~0.83x).

Not allowed:
  - HZ11 is real-app-safe or multi-thread-real-app-competitive (rocksdb segfault, all lanes).
  - cap1024/cap768 help real multi-thread apps (untested -- rocksdb crashes).
  - HZ11 generally beats tcmalloc.
  - span-transfer is a general lane (sqlite3 abort).
```

## Artifacts

```text
runner: run_hz11_real_app_gate.sh (process-tree RSS + --preclean-cmd; fixed in the prior
        box, --preclean-cmd added here).
workloads: espresso (SPEC), sqlite3 in-memory (bench/hz11_real_app_sqlite.sql), rocksdb
           db_bench (fillrandom+readrandom, 8 threads, --compression_type=None).
no src/ change.
compile workload: attempted (make -C hakozuna-hz6 has no Makefile; make -C hakozuna-hz11
                  is self-undermining -- `make clean` deletes the preloaded .so -- and
                  too fast at ~1.5s). Deferred; needs a non-allocator-repo build target.
```
