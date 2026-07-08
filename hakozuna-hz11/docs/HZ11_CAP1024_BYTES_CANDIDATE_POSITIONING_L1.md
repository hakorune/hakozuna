# HZ11Cap1024BytesCandidatePositioning-L1

Status: **MIXED.** `cachecap1024-bytes` fixes sh6bench (1.21x tcmalloc on the synthetic
macro gate) but **materially regresses the remote/mixed microbench vs fine128**, so it
becomes a **sh6bench/macro-churn specialist** opt-in lane — NOT the general recommended
candidate. **fine128 remains the recommended opt-in macro candidate.** No default change.

## What changed from fine128

```text
cap1024-bytes = fine128 + -DHZ11_CACHE_CAP=1024u + -DHZ11_CACHE_BYTE_ACCOUNTING=1
  (default HZ11_MAX_CACHED_BYTES = 2 MiB).
The bigger per-class cache (1024 vs 32) absorbs sh6bench churn locally (xfer_insert
~0); the byte cap bounds retention so RSS does not explode. Enabling byte accounting
on the SOA path (and the cached_bytes slow-path fix) came from
HZ11ThreadCacheCapacityByteCap-L1.
```

## Macro evidence (RUNS=5, this box)

| workload | fine128 wall | cap1024-bytes wall | fine128 max RSS KiB | cap1024-bytes max RSS KiB |
|---|---:|---:|---:|---:|
| sh6bench | 3.526 | **0.427** (1.21x tcmalloc) | 323200 | 317040 |
| xmalloc_test | 2.027 | 2.029 | 18432 | 27648 |
| cache_scratch | 1.175 | 1.179 | 3456 | 3968 |
| larson | 4.142 | 4.138 | 273280 | 274176 |
| mstress | 0.219 | 0.215 | 231228 | 236544 |
| python_alloc | 0.031 | 0.031 | 6528 | 6784 |

```text
Macro verdict: GO. sh6bench near-parity reproduces (0.427s, xfer_insert 868M->175K).
The other 5 rows are flat (no wall regression vs fine128). xmalloc_test RSS is bounded
(27648; clearly below tcmalloc and below the plain-CAP256 record of 52864). Correctness
rows OK:5. max RSS inside the guard on every row.
```

## Remote/mixed evidence (RUNS=10, this box) — the GO/MIXED discriminator

cap1024-bytes vs fine128 (both normalized to tcmalloc):

| row | fine128 ops/tcmalloc | cap1024-bytes ops/tcmalloc | fine128 RSS/tcmalloc | cap1024-bytes RSS/tcmalloc |
|---|---:|---:|---:|---:|
| main_local0 | 0.997 | 0.787 | 0.255 | 0.467 |
| main_r50 | 1.791 | 1.582 | 0.640 | 0.872 |
| main_r90 | 2.373 | 1.630 | 0.455 | 1.023 |
| small_remote90 | 1.075 | 1.038 | 0.270 | 0.788 |
| medium_r50 | 4.332 | 1.503 | 0.497 | 0.477 |
| medium_r90 | 5.942 | 1.558 | 0.439 | 0.422 |

```text
Remote/mixed verdict: REGRESSION. cap1024-bytes is slower than fine128 on EVERY row,
catastrophically on the medium rows (medium_r50 4.33x -> 1.50x, medium_r90 5.94x ->
1.56x tcmalloc). RSS is also worse on the main/small rows (main_r90 0.45x -> 1.02x,
small_remote90 0.27x -> 0.79x). The big-cache + byte-cap shape that fixes sh6bench
HURTS the cross-thread-free-heavy remote/mixed pattern where fine128's smaller cache
+ transfer efficiency wins. (xfer_insert for cap1024-bytes is ~0 on these rows: it
absorbs frees locally but at lower throughput.)
```

## Decision

```text
MIXED (per the box's decision tree: sh6bench fixed but remote/mixed regresses).
- cap1024-bytes = sh6bench / macro-churn SPECIALIST opt-in candidate
  (sh6bench 1.21x tcmalloc on the synthetic macro gate; flat on the other macro rows).
- fine128 REMAINS the recommended opt-in macro candidate (generalist: near-parity on
  5/6 macro rows AND strong remote/mixed).
- span-transfer remains the dedicated remote/mixed lane.
- default unchanged. claim boundary unchanged.
The big-cache + byte-cap tuning is a workload-specific trade, not a general upgrade.
```

## Lane taxonomy (now explicit and 3-way)

```text
fine128        (CAP32, no-bytes): RECOMMENDED opt-in macro candidate (generalist).
                                 near-parity-or-better wall on 5/6 macro rows
                                 (0.990x-1.151x); strong remote/mixed. NOT default.
cap1024-bytes  (CAP1024 + 2 MiB byte cap): sh6bench/macro-churn SPECIALIST opt-in.
                                 sh6bench 1.21x tcmalloc; regresses remote/mixed.
                                 Pick this for sh6bench-like multi-thread alloc churn.
span-transfer  (CAP default):    dedicated remote/mixed microbench lane.
global fineclass:                sh6bench RSS research only.
locked percpu / cachecap256/512: NO-GO / tuning diagnostics, not candidates.
default allocator path:          unchanged.
```

## Rollback path

```text
cap1024-bytes is opt-in only. The .so filename is the LD_PRELOAD selector.
- To use the generalist:  LD_PRELOAD=.../libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
- To use the specialist:  LD_PRELOAD=.../libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap1024_bytes.so
Reverting cap1024-bytes = preload fine128 instead. No default-path change; no code
change; the candidate .so and its aliases stay in-tree.
```

## Claim boundary

```text
Allowed:
  - cap1024-bytes is an opt-in sh6bench/macro-churn specialist: sh6bench 1.21x tcmalloc
    on the synthetic macro gate, flat on the other macro rows, RSS bounded.
  - fine128 remains the recommended general opt-in macro candidate.
  - the big-cache + byte-cap shape trades remote/mixed for sh6bench (a workload choice).

Not allowed:
  - cap1024-bytes is the default, or the general/recommended candidate (it regresses
    remote/mixed; fine128 stays recommended).
  - HZ11 generally beats tcmalloc, or sh6bench is "solved" in real apps (synthetic
    macro only; real-app + platform evidence still required).
  - cap1024-bytes is good for remote/mixed workloads (use fine128/span-transfer there).
```

## Next (warrant)

```text
- A real-application box (server/compile/DB) would decide whether cap1024-bytes's
  sh6bench-style win translates to real workloads, and whether fine128's generalist
  balance is safer for a default-adjacent recommendation. Both still need real-app +
  platform evidence before any public claim.
- Optional: a CAP between 256 and 1024 (e.g. 512) with byte cap might trade less
  remote/mixed for slightly less sh6bench win -- a possible middle lane, untested.
```
