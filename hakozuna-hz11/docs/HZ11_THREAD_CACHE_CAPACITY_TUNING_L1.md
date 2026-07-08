# HZ11ThreadCacheCapacityTuning-L1

Status: **GO for the cache-cap root cause; MIXED for promotion.** CAP is confirmed as
a real sh6bench lever (a 1-flag change nearly halves sh6bench wall), but plain CAP
promotion is blocked by an xmalloc_test RSS side effect. fine128 (CAP 32) remains the
recommended opt-in macro candidate; the clean win needs a capacity + byte-cap follow-up.

## Hypothesis

`HZ11_CACHE_CAP=32` is too small for sh6bench: free-side overflow forces constant
mutex-guarded transfer-cache round-trips (`xfer_insert` ~500M / 5 runs), while the
reference tcmalloc (gperftools 2.9.1, per-thread ThreadCache, **no rseq/per-CPU**) has
a large enough cache to absorb the churn (`xfer_insert=0`). The missing mechanism is
cache capacity, not per-CPU locality. This box varies CAP to test that.

## Boundary

```text
one tuning box; opt-in siblings only
no transfer-cache algorithm rewrite; no lock-free structure; no rseq; no default promotion
vary HZ11_CACHE_CAP only (HZ11_MAX_CACHED_BYTES is inert under fine128's
  HZ11_CACHE_BYTE_ACCOUNTING=0, so CAP is the sole retention knob)
candidates: fine128 (=CAP32 baseline) + cachecap64/128/256
```

CAP is a fixed hot-path array dim (`class_items[CLASS_COUNT][CAP]`), so the
thread-cache struct grows with CAP (CAP=256 -> ~34 KiB/thread). Measure cache-locality
/ RSS side effects, not just wall.

## Evidence - focused sh6bench sweep

Source: `run_hz11_sh6bench_path_cost_attribution.sh` with `ALLOC_LIST`, RUNS=3,
sh6bench (4 threads, 1..1000 B, 100k calls/thread).

| Allocator | median wall sec | wall/tcmalloc | xfer_insert | xfer_spill | span_create | max RSS KiB |
|---|---:|---:|---:|---:|---:|---:|
| fine128 (CAP32) | 3.506 | 9.79x | 521,336,051 | 26,502,972 | 15493 | 323584 |
| cachecap64 | 2.905 | 8.12x | 489,593,613 | 27,302,444 | 15534 | 324096 |
| cachecap128 | 2.441 | 6.82x | 430,051,251 | 29,603,017 | 15636 | 323328 |
| cachecap256 | **1.911** | **5.34x** | **312,712,788** | 33,193,014 | 15652 | 324736 |
| tcmalloc | 0.358 | 1.00x | 0 | 0 | 0 | 256512 |

```text
GO signal, clean and monotonic:
  wall:        3.506 -> 2.905 -> 2.441 -> 1.911   (-45% at CAP256)
  wall/tcmalloc: 9.79x -> 8.12x -> 6.82x -> 5.34x
  xfer_insert: 521M -> 490M -> 430M -> 313M       (-40% at CAP256, tracks wall)
  max RSS:     flat ~323-325 KiB (no explosion); span_create flat.
The CAP -> wall -> xfer_insert relationship confirms cache-overflow trip count is a
real sh6bench lever. The trend is still descending at CAP256 (cap512/1024 untested).
```

## Evidence - macro gate regression check (CAP256 vs fine128 vs tcmalloc)

Source: `run_hz11_macro_speed_lane_gate.sh`, RUNS=5.

| workload | fine128 wall | CAP256 wall | fine128 max RSS KiB | CAP256 max RSS KiB |
|---|---:|---:|---:|---:|
| sh6bench | 3.533 | **1.925** | 323584 | 326528 |
| cache_scratch | 1.181 | 1.188 | 3328 | 3456 |
| larson | 4.140 | 4.148 | 273152 | 273664 |
| mstress | 0.215 | 0.217 | 235156 | 238444 |
| python_alloc | 0.031 | 0.031 | 6656 | 6528 |
| xmalloc_test | 2.022 | 2.023 | **18816** | **52864** |

```text
sh6bench: 3.533s -> 1.925s (the win reproduces).
The other 5 rows: wall flat (no regression on the 5/6 near-parity rows).
RSS flat everywhere EXCEPT xmalloc_test: 18816 -> 52864 KiB (2.8x baseline).
  xmalloc_test is still 0.27x tcmalloc RSS (still a win vs tcmalloc), but 2.8x worse
  than the fine128 baseline: the larger cache retains more cached objects (cached-bytes),
  which is unbounded under fine128's HZ11_CACHE_BYTE_ACCOUNTING=0.
```

## Decision (per the box's decision tree)

```text
GO for the cache-cap root cause:
  - CAP is a confirmed sh6bench lever (monotonic CAP -> wall -> xfer_insert; -45% wall
    at CAP256; 5/6 other rows flat).
  - This retroactively confirms the per-CPU/rseq track was a red herring: the reference
    tcmalloc is per-thread (no rseq) and wins with a bigger cache.

MIXED for promotion:
  - Plain CAP256 regresses xmalloc_test RSS 2.8x (cached-bytes retention). This is the
    decision-tree's "wall improves but RSS side-effect" branch.
  -> The clean win needs capacity + byte-cap policy, not plain CAP promotion.
```

### Next lever (warrant for a follow-up box)

```text
- Re-enable HZ11_CACHE_BYTE_ACCOUNTING (or a per-class byte cap) alongside a larger CAP,
  so the sh6bench wall win is kept WITHOUT the xmalloc_test cached-bytes RSS cost. This
  is the direct continuation: it was deferred here to isolate CAP first.
- Extend the CAP sweep (cap512 / cap1024) to find where the wall curve plateaus; the
  trend is still descending at cap256.
- Pick the smallest CAP + byte cap that captures most of the sh6bench win with no RSS
  regression on any macro row, then re-run the macro gate before any candidate change.
```

## Claim Boundary

```text
Allowed:
  - HZ11_CACHE_CAP is a confirmed sh6bench wall lever (cap256: 3.5s -> 1.9s, -45%).
  - The cache-overflow trip-count hypothesis is confirmed; per-CPU/rseq was not the lever.

Not allowed:
  - sh6bench is solved (cap256 is still 5.34x tcmalloc).
  - cap256 (or any CAP lane) is adopted or promoted (xmalloc_test RSS regression).
  - HZ11 generally beats tcmalloc (carried from HZ11PaperPositioning-L1).
```

## Status of the lanes

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap64/128/256.so are built
and registered (focused runner via ALLOC_LIST; cachecap256 also in the macro gate) but
are NOT candidates. They remain in-tree as reproducible tuning diagnostics.
fine128 (CAP32) remains the recommended opt-in macro candidate. Default unchanged.
```
