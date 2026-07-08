# HZ11ThreadCacheCapacityMiddleLane-L1

Status: **NO-GO for a middle general candidate. The lane split is confirmed.** No
uniform `HZ11_CACHE_CAP` (512-1024) + byte cap keeps remote/mixed ≈ fine128: the
medium-row regression is **inherent to any big-CAP shape**. fine128 stays the
recommended general opt-in macro candidate; cap1024-bytes (or cap768-bytes) stays the
sh6bench specialist; span-transfer stays the remote/mixed lane. The only remaining shot
at a single general candidate is **class-range CAP** (a code change; follow-up box).

## Context

`HZ11Cap1024BytesCandidatePositioning-L1` split the track into two extremes — fine128
(generalist, sh6bench 9.8x) and cap1024-bytes (sh6bench specialist, 1.21x but regresses
remote/mixed). This box asks whether a **middle CAP** (512/768) or a **tighter byte cap**
(1 MiB) lands a candidate that improves sh6bench without breaking remote/mixed — i.e. a
true general candidate. Pure flag-only variants; no code/default change.

## Evidence - focused sh6bench (RUNS=3): the win saturates at CAP768

| candidate | wall sec | wall/tcmalloc | xfer_insert |
|---|---:|---:|---:|
| fine128 (CAP32) | 3.495 | 10.16x | 521M |
| cap512-bytes | 1.131 | 3.29x | 128M |
| cap768-bytes | 0.425 | 1.235x | 105K |
| cap1024-bytes1m (1 MiB cap) | 0.436 | 1.267x | 105K |
| cap1024-bytes (2 MiB cap) | 0.440 | 1.279x | 105K |
| tcmalloc | 0.344 | 1.00x | 0 |

```text
Sharp knee between CAP512 (3.29x) and CAP768 (1.24x): CAP768 already reaches the same
near-parity as CAP1024. The 1 MiB byte cap does NOT cut the sh6bench win (working set
fits). So on sh6bench, cap768-bytes is a slightly leaner equivalent of cap1024-bytes.
```

## Evidence - remote/mixed matrix (RUNS=10): no middle ground

ops/tcmalloc (higher is better). The medium rows are the discriminator:

| row | fine128 | cap512-bytes | cap768-bytes | cap1024-bytes1m |
|---|---:|---:|---:|---:|
| main_local0 | 0.864 | 0.885 | 0.862 | 0.773 |
| main_r50 | 1.560 | 1.525 | 1.479 | 0.459 |
| main_r90 | 2.114 | 1.633 | 1.599 | 0.458 |
| small_remote90 | 1.132 | 1.100 | 1.095 | 1.096 |
| **medium_r50** | **4.511** | **1.524** | **1.520** | **0.500** |
| **medium_r90** | **5.780** | **1.576** | **1.579** | **0.559** |

```text
Even cap512-bytes -- the smallest big-CAP, which only reaches sh6bench 3.29x (NOT
near-parity) -- collapses fine128's medium-row dominance (4.5x/5.8x -> ~1.5x tcmalloc).
cap768-bytes (sh6bench near-parity) regresses medium by the SAME amount. cap1024-bytes1m
(1 MiB cap) trades throughput for RSS (medium RSS 0.36x/0.32x tcmalloc, the lowest) but
throughput is worse (0.50x/0.56x).
=> The medium-row regression is INHERENT to any big-CAP shape, not a tunable side effect.
   fine128's small cache + transfer-cache efficiency is what wins medium remote/mixed, and
   any CAP big enough to help sh6bench (>=512) destroys that. Big-CAP and remote/mixed-
   medium are in fundamental tension.
```

## Decision (per the brief)

```text
NO-GO for a middle general candidate (remote/mixed regression remains for every
candidate). Lane split confirmed and final:
  - fine128        = recommended general opt-in macro candidate (generalist).
  - cap1024-bytes  = sh6bench/macro-churn specialist (cap768-bytes is an equivalent,
                     slightly leaner alternative: same sh6bench near-parity, smaller CAP).
  - span-transfer  = dedicated remote/mixed lane.
  - default unchanged. claim boundary unchanged.
```

## Lane taxonomy (unchanged from positioning; cap768-bytes added as equivalent specialist)

```text
fine128        (CAP32, no-bytes): RECOMMENDED general opt-in macro candidate.
cap1024-bytes  (CAP1024 + 2 MiB): sh6bench/macro-churn SPECIALIST. cap768-bytes is an
                                  equivalent (CAP768 + 2 MiB): same sh6bench 1.24x, smaller
                                  cache. Pick either for sh6bench-like churn.
span-transfer  : dedicated remote/mixed lane.
cap512-bytes / cap1024-bytes1m / cachecap256*: tuning/diagnostic siblings, not candidates.
default path: unchanged.
```

## Claim boundary (unchanged)

```text
Allowed:
  - no uniform-CAP middle candidate is a general candidate (confirmed: big-CAP regresses
    remote/mixed medium rows inherently).
  - fine128 remains the recommended general opt-in macro candidate; cap1024/cap768-bytes
    remain sh6bench specialists.

Not allowed:
  - any big-CAP lane is a general or remote/mixed candidate.
  - HZ11 generally beats tcmalloc; sh6bench "solved" in real apps (synthetic only).
  - default promotion.
```

## Next lever (warrant for a follow-up box)

```text
Class-range CAP: a big CAP ONLY for small classes (where sh6bench churns), keeping the
small CAP for medium classes (where fine128's transfer efficiency wins remote/mixed).
This is the one design NOT yet tested that could plausibly give "sh6bench win without
remote/mixed regression" -- but it requires a code change (per-class-range CAP, e.g.
HZ11_CACHE_CAP_SMALL for classes < N), so it is a separate box. Until then, the lane
split stands.
```
