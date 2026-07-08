# HZ11ThreadCacheCapacityByteCap-L1

Status: **GO for the byte-cap approach.** `cachecap1024-bytes` (CAP=1024 +
`HZ11_CACHE_BYTE_ACCOUNTING=1`, default `HZ11_MAX_CACHED_BYTES`=2 MiB) achieves
sh6bench **near-parity (1.20x tcmalloc)** with xmalloc_test RSS **bounded** (27648
KiB vs plain CAP256's 52864) and no regression on the other 5 macro rows. Not yet
promoted (needs positioning rigor). This essentially closes the long-open sh6bench
10x pathology on the synthetic macro gate.

## Context

`HZ11ThreadCacheCapacityTuning-L1` confirmed CAP is a real sh6bench lever (CAP256:
3.5->1.9s) but plain big CAP leaks xmalloc_test RSS (18816->52864, unbounded under
`HZ11_CACHE_BYTE_ACCOUNTING=0`). This box tests **big CAP + byte cap** to keep the
sh6bench win while bounding retention.

## Critical code finding + correctness fix (important)

fine128 is `HZ11_CACHE_SOA=1`. On the SOA path byte accounting was **not enforced**
and `SOA + BYTE_ACCOUNTING` was a hard `#error`. Worse, the SOA **slow** paths did
not maintain `cached_bytes`:

```text
- hz11_thread_cache.h: added gated byte check to SOA push (overflow-safe subtractive
  form: cnt<CAP && slot<=MAX && cached_bytes<=MAX-slot) and cached_bytes dec on SOA pop.
- hz11_thread_cache.h: relaxed the SOA+BYTE_ACCOUNTING #error; #ifndef'd MAX_CACHED_BYTES.
- BUG (caught before any claim): flush_class (both SOA sub-branches) and overflow_slow's
  SOA re-insert did NOT update cached_bytes. With byte accounting ON, cached_bytes drifted
  UP on every overflow -> the byte cap bound PERMANENTLY -> every free-push overflowed ->
  cap256-bytes / cap512-bytes TIMED OUT (>30s) in the first sweep (cap1024-bytes looked
  fast only because it rarely overflowed, i.e. on a broken counter).
- FIX: added the gated cached_bytes decrement to both SOA flush sub-branches and the
  gated increment to the SOA overflow_slow re-insert (mirror the AoS paths).
  After the fix all lanes complete; cap256/512-bytes are sane.
```

All existing lanes are byte-identical (they build `BYTE_ACCOUNTING=0` -> hit the
unchanged `#else` branches; macro gate confirms fine128 sh6bench 3.537s / xmalloc
18560 KiB unchanged).

## Evidence - focused sh6bench sweep (RUNS=3)

| Allocator | wall sec | wall/tcmalloc | xfer_insert | max RSS KiB |
|---|---:|---:|---:|---:|
| fine128 (CAP32, no-bytes) | 3.510 | 9.83x | 521,358,110 | 322560 |
| cap256 no-bytes | 1.899 | 5.32x | 312,929,096 | 323840 |
| cap256 bytes | 1.910 | 5.35x | 312,138,609 | 322432 |
| cap512 bytes | 1.136 | 3.18x | 128,036,602 | 325120 |
| **cap1024 bytes** | **0.441** | **1.235x** | **105,796** | 321024 |
| tcmalloc | 0.357 | 1.00x | 0 | 263552 |

```text
- cap256-bytes ~= cap256 no-bytes: the 2 MiB byte cap does NOT bind at CAP256
  (256*17*~300B ~= 1.3 MiB < 2 MiB), so it behaves like no-bytes. (Confirms the
  predicted "byte cap binds on big-object workloads, not sh6bench-at-CAP256".)
- cap1024-bytes: the cache holds the sh6bench live set within the 2 MiB byte cap
  (xfer_insert ~0) -> near tcmalloc parity, RSS flat (no explosion).
```

## Evidence - macro gate, cap1024-bytes vs fine128 vs tcmalloc (RUNS=5)

| workload | fine128 wall | cap1024-bytes wall | fine128 RSS KiB | cap1024-bytes RSS KiB |
|---|---:|---:|---:|---:|
| sh6bench | 3.537 | **0.432** | 322816 | 320732 |
| xmalloc_test | 2.027 | 2.019 | 18560 | **27648** |
| cache_scratch | 1.183 | 1.184 | 3328 | 3968 |
| larson | 4.142 | 4.147 | 273280 | 274432 |
| mstress | 0.215 | 0.218 | 234808 | 236124 |
| python_alloc | 0.031 | 0.031 | 6528 | 6912 |

```text
sh6bench: 3.537s -> 0.432s (1.20x tcmalloc). xfer_insert 868M -> 173K. RSS flat.
xmalloc_test RSS: 18560 -> 27648 KiB. BOUNDED by the byte cap: clearly below plain
  CAP256 no-bytes (52864) and still 0.14x tcmalloc (a 7x RSS win). Not back to
  fine128's 18560 (CAP1024 struct + retained bytes), but the leak is contained.
Other 5 rows: wall flat (no regression). RSS bumps are small and bounded by the cap.
All correctness rows OK:5. max RSS inside the guard on every row.
```

## Decision (per the box's GO criteria)

```text
GO. All five criteria hold:
  1. sh6bench wall >> fine128 CAP32            (0.432 vs 3.537)
  2. xmalloc_test RSS clearly < plain CAP256   (27648 vs 52864)
  3. no wall regression on 5/6 near-parity rows (all flat)
  4. max RSS inside the guard                   (sh6bench 1.21x, xmalloc 0.14x tcmalloc)
  5. default unchanged
(The macro gate's auto-"MIXED" is a generic span-soa-baseline comparison artifact,
not the box's criteria.)
```

The cap+byte approach is the clean continuation of `HZ11ThreadCacheCapacityTuning-L1`:
the CAP win is kept and the xmalloc_test RSS side effect is bounded by the byte cap.

## Claim Boundary

```text
Allowed:
  - CAP + byte cap is a confirmed clean sh6bench approach; cap1024-bytes (CAP=1024,
    2 MiB byte cap) reaches sh6bench 1.20x tcmalloc on the synthetic macro gate with
    bounded RSS and no other-row regression.
  - the cached_bytes SOA slow-path bug was found and fixed (correctness).

Not allowed:
  - sh6bench is "solved" in general (1.20x is synthetic-macro; real-app + platform
    evidence still required).
  - cap1024-bytes is adopted, default, or the recommended opt-in macro candidate
    (promotion needs a separate positioning box: real-app workloads, platform/COA
    coverage, rollback, claim wording).
  - HZ11 generally beats tcmalloc (carried from HZ11PaperPositioning-L1).
```

## Next (warrant)

```text
- Optional: tune HZ11_MAX_CACHED_BYTES (e.g. 1 MiB / 4 MiB) to tighten xmalloc_test
  RSS further or confirm 2 MiB is the knee; extend CAP past 1024 if still descending.
- Promotion: a separate positioning box to decide whether cap1024-bytes replaces
  fine128 as the recommended opt-in macro candidate, gated on real-app workloads and
  platform coverage (the line's standing discipline).
- The SOA byte-accounting machinery is now available for any future lane that wants a
  bounded-cache speed shape.
```

## Status of the lanes

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap{256,512,1024}_bytes.so
are built and registered (focused runner via ALLOC_LIST; cap1024-bytes in the macro
gate). cap1024-bytes is a strong opt-in candidate; NOT default. fine128 remains the
documented recommended opt-in macro candidate until a promotion box says otherwise.
```
