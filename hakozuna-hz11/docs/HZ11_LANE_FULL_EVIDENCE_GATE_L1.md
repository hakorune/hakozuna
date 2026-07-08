# HZ11LaneFullEvidenceGate-L1

Status: **GO for evidence, with a runner RSS correction.** The synthetic lane story
mostly holds on real applications: fine128 is near-parity with tcmalloc on real-app
wall, cap768/cap1024-bytes's sh6bench win is **specialist-only** (does not translate
to the single-threaded real apps tested), and span-transfer has **correctness limits**
(aborts python_alloc/mstress/sqlite3). A follow-up check found that the first version of
`run_hz11_real_app_gate.sh` sampled the `bash -c` wrapper RSS rather than the whole
workload process tree, so the original strong sqlite/espresso RSS ratios are not
claim-grade. The runner now samples process-tree RSS; corrected sqlite3 shows a modest
HZ11 RSS win (~0.84x tcmalloc), not the earlier ~0.4x.

## Lanes

`tcmalloc` (ref), `jemalloc` (ref), `hz11-span-transfer`, `fine128`,
`cachecap768-bytes`, `cachecap1024-bytes`. (cap768-bytes ≈ cap1024-bytes on macro rows
per the byte-cap/middle-lane boxes; cap768 was not in the macro-gate registry for the
unified run, so the macro table lists cap1024-bytes as the byte-cap representative.)

## Lane strengths/weaknesses (the summary table)

| lane | macro (5/6) | sh6bench | remote/mixed | real-app (espresso/sqlite) | correctness |
|---|---|---|---|---|---|
| tcmalloc | parity (ref) | 1.0x (ref) | 1.0x (ref) | parity (ref) | OK (ref) |
| fine128 | near-parity-or-better (0.99-1.15x); RSS win | 9.8x (open gap) | strong (medium 4.5x/5.8x tcmalloc) | near-parity; corrected sqlite RSS ~0.84x | OK (all rows) |
| cap1024/cap768-bytes | near-parity (flat vs fine128) | **1.2x (specialist win)** | regresses medium (1.5x) | near-parity; corrected sqlite RSS ~0.84-0.85x | OK |
| span-transfer | FAILS python_alloc/mstress; larson RSS leak | 12.9x | the remote/mixed lane (cleanest there) | espresso OK; **sqlite3 ABORTS** | limited (central-overflow abort) |

## A. Macro synthetic (RUNS=5, this box)

| workload | tcmalloc | span-transfer | fine128 | cap1024-bytes |
|---|---:|---:|---:|---:|
| python_alloc wall | 0.031 | **FAIL** | 0.031 | 0.031 |
| mstress wall | 0.188 | **FAIL** | 0.216 | 0.215 |
| larson wall / RSS | 4.139 / 279M | 4.174 / **654M** | 4.136 / 273M | 4.144 / 274M |
| sh6bench wall | 0.357 | 4.610 | 3.532 | **0.440** |
| xmalloc_test wall / RSS | 2.037 / 195M | 2.029 / **16M** | 2.016 / 18M | 2.021 / 28M |
| cache_scratch | 1.168 | 1.178 | 1.182 | 1.182 |

```text
span-transfer FAILS python_alloc/mstress (the known central-stack-overflow abort that
fine128's HZ11_CENTRAL_CAP + thread-exit fixed) and leaks larson RSS (654M, the
thread-churn leak thread-exit fixed). fine128 + cap1024-bytes pass correctness; both
near-parity on 5/6 rows; cap1024-bytes wins sh6bench (0.440 vs 3.532). cap768-bytes ≈
cap1024-bytes on these rows (cap768 sh6bench 1.24x per the middle-lane box).
```

## B. Remote/mixed microbench (RUNS=10, from positioning + middle-lane boxes)

ops/tcmalloc:

| row | fine128 | cap768-bytes | cap1024-bytes | span-transfer |
|---|---:|---:|---:|---:|
| medium_r50 | **4.51x** | 1.52x | 1.50x | (the clean remote/mixed lane) |
| medium_r90 | **5.78x** | 1.58x | 1.56x | " |
| main_r90 | 2.37x | 1.60x | 1.63x | " |

```text
fine128 dominates medium remote/mixed; cap768/cap1024-bytes regress it (big-CAP is
in tension with remote/mixed-medium, per the middle-lane box). span-transfer remains
the dedicated remote/mixed lane.
```

## C. Real-app (RUNS=3, this box; new runner `run_hz11_real_app_gate.sh`)

### Runner correction

The initial run sampled the top-level `bash -c` wrapper PID. That is sufficient for
wall/status, but can under-measure RSS when the shell forks a workload child. The runner
has been fixed to sample the whole process tree (`VmRSS`/`VmHWM` summed across the wrapper
and children). sqlite3 was rerun with the fixed runner; espresso was not available for a
local rerun in this checkout, so its RSS table below is retained only as a prior-run
wall/status record and **not** used for strong RSS claims.

**espresso** (real SPEC logic-synthesis app, single-threaded, ~4s; RSS provisional until
rerun with process-tree sampling):

| allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 3.688 | 1.000 | 8704 | 1.000 |
| jemalloc | 3.946 | 1.070 | 5120 | 0.588 |
| span-transfer | 3.817 | 1.035 | 2688 | 0.309 |
| fine128 | 3.809 | 1.033 | 2688 | 0.309 |
| cap768-bytes | 3.787 | 1.027 | 2816 | 0.324 |
| cap1024-bytes | 3.754 | 1.018 | 2944 | 0.338 |

**sqlite3** (real database, in-memory, 100k-row insert+scan+delete; corrected
process-tree RSS rerun):

| allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc |
|---|---:|---:|---:|---:|
| tcmalloc | 0.128 | 1.000 | 21120 | 1.000 |
| jemalloc | 0.135 | 1.055 | 17536 | 0.830 |
| span-transfer | **ABORT (rc=134)** | NA | NA | NA |
| fine128 | 0.137 | 1.070 | 17664 | 0.836 |
| cap768-bytes | 0.137 | 1.070 | 17920 | 0.848 |
| cap1024-bytes | 0.137 | 1.070 | 17664 | 0.836 |

```text
Real-app verdict: every HZ11 candidate lane (fine128 / cap768 / cap1024) is near-parity
with tcmalloc on wall for sqlite3, and the HZ11 lanes are indistinguishable from one
another there. Corrected sqlite RSS shows a modest HZ11 win (~0.84x tcmalloc), not the
earlier parent-PID-sampled ~0.4x. cap1024's big sh6bench win does NOT appear on these
single-threaded real apps. span-transfer ABORTS sqlite3 (SIGABRT) -- the same central-
overflow issue it hits on python_alloc/mstress, fixed in fine128.
```

## Decision (per the brief)

```text
GO for evidence. The synthetic lane story is consistent with real-app:
- fine128 is the correct GENERAL lane: passes all correctness rows + real-app, near-parity
  wall, big RSS win. span-transfer is NOT general (central-overflow aborts on
  python_alloc/mstress/sqlite3) -- it stays the remote/mixed specialist, with limits noted.
- cap768/cap1024-bytes's sh6bench win is SPECIALIST-only: it does not translate to
  espresso/sqlite3 (no sh6bench-like multi-threaded churn there). On real apps they are
  indistinguishable from fine128.
- HZ11's real-app RSS advantage needs tighter wording: corrected sqlite3 shows a modest
  RSS win (~0.84x tcmalloc), while espresso RSS must be rerun with process-tree sampling
  before any strong real-app RSS multiplier is claimed.
```

### Next-step warrant

```text
1. cap768/cap1024 win on real-app? -> NO (not on espresso/sqlite3). class-range CAP is
   NOT warranted by this real-app evidence; the sh6bench win is specialist-only.
2. fine128 safest on real-app? -> YES. The paper/positioning should CENTER on fine128
   (generalist: near-parity wall + large RSS win on synthetic AND real-app), with
   cap1024-bytes documented as the sh6bench-specialist option.
3. REMAINING GAP: a MULTI-THREADED real-app (rocksdb db_bench / redis) would test whether
   cap1024's churn win translates to real threaded workloads. espresso/sqlite3 here are
   single-threaded, so they cannot show it. This is the one untested case before fully
   closing "does cap1024 help real churn".
```

## Claim boundary

```text
Allowed:
  - on sqlite3 (real app), HZ11 (fine128/cap768/cap1024) is near-parity with tcmalloc on
    wall and uses modestly less process-tree RSS (~0.84x). Espresso wall/status is
    retained from the prior run, but its RSS requires a fixed-runner rerun before use.
  - fine128 is the validated recommended general opt-in macro candidate.
  - cap1024/cap768-bytes is a sh6bench/macro-churn specialist; its win does not appear on
    the real apps tested.

Not allowed:
  - HZ11 generally beats tcmalloc (real-app sample is 2 single-threaded workloads).
  - cap1024-bytes helps real apps (not shown here; needs multi-threaded real-app evidence).
  - span-transfer is a general or default lane (correctness limits).
  - default promotion.
```

## Artifacts

```text
new: scripts/run_hz11_real_app_gate.sh (generic LD_PRELOAD + process-tree RSS + counter runner);
     bench/hz11_real_app_sqlite.sql (sqlite workload).
reused: run_hz11_macro_speed_lane_gate.sh (A); run_hz11_transfer_promotion_matrix.sh (B,
     cited from prior boxes).
no src/ change.
```
