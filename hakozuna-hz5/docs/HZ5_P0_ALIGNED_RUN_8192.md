# HZ5 Implementation Lifecycle Log

This file keeps the private HZ5 implementation work log. The filename is
historical: it began as the P0 aligned-run work order, but now records the HZ5
lifecycle through P14. BenchLab-facing lane policy and measurement triage live
in `allocator-bench-lab/docs/hakozuna-hz5-lane-registry.md` and
`allocator-bench-lab/docs/hakozuna-hz5-sidecar-design.md`.

## Original P0 Work Order

Date: 2026-05-20

## Goal

Create the HZ5 sidecar and add a route-shadow preflight for the first HZ5 seed:
`4K/8K/64K` allocations with `align=8192`.

P0 must not change allocation behavior. Its job is to prove that HZ5's initial
route table sees exactly the rows we intend to move out of HZ3 large path later.

BenchLab aliases:

```text
hakozuna-hz5-p0-route-shadow
hz5-p0-route-shadow
```

## Architecture Decision

HZ5 starts as a sibling directory:

```text
hakozuna/       HZ3, current S276 speed default and true-large fallback
hakozuna-mt/    HZ4, page/run reference implementation
hakozuna-hz5/   HZ5, page/run-first sidecar prototype
```

Do not bury HZ5 in `hakozuna/src/hz3_*`. HZ3's current default is valuable and
should remain stable while HZ5 explores a first-class page/run classifier.

## P0 Route Table

Initial HZ5 candidate route:

```text
size == 4096  && align == 8192 -> HZ5 aligned-run candidate
size == 8192  && align == 8192 -> HZ5 aligned-run candidate
size == 65536 && align == 8192 -> HZ5 aligned-run candidate
otherwise                       -> HZ3 S276 fallback
```

No allocation should actually route to HZ5 in P0.

## Counters

Suggested P0 counters:

```text
hz5_route_candidate_4k_a8192
hz5_route_candidate_8k_a8192
hz5_route_candidate_64k_a8192
hz5_route_taken
hz5_route_fallback_hz3
hz5_true_large_fallback
hz5_route_reject_a4096_guard
```

## Acceptance

P0 GO:

- Build succeeds on Windows.
- No behavior change versus `hz3-speed-default`.
- `hz3-large-a8192` increments the three candidate counters.
- `a4096` guard rows do not route to HZ5.
- `hz5_route_taken == 0` until P1/P2 explicitly enables behavior.

P0 NO-GO:

- Any allocator behavior changes.
- Any `a4096` guard is classified as HZ5 candidate.
- Candidate counters cannot be read cleanly from BenchLab diagnostics.

## Smoke Result

Run roots:

```text
results/synthetic-sweep/20260520_032933_079  hz3-large-a8192 repeat-1
results/synthetic-sweep/20260520_032846_746  hz3-medium-a4096-guards repeat-1
```

Observed target counters:

```text
pc-r90-4k-a8192-t4:
  candidate_4k_a8192=220000
  route_taken=0
  route_fallback_hz3=220000

pc-r90-8k-a8192-t4:
  candidate_8k_a8192=172000
  route_taken=0
  route_fallback_hz3=172000

pc-r90-64k-a8192-t4:
  candidate_64k_a8192=52000
  route_taken=0
  route_fallback_hz3=52000
```

The `a4096` guard profile stayed on the medium path and emitted no HZ5 route
line, so no guard row was classified as a HZ5 candidate.

## Next Patch

If P0 is clean, P1 adds local segment/page metadata and an aligned run allocator
inside `hakozuna-hz5/`, still using HZ3 S276 fallback for all non-target rows.

P1 update:

- local segment/page metadata is implemented under `hakozuna-hz5/`
- local aligned-run allocator smoke passed
- no BenchLab/HZ3 behavior route is enabled yet

P2 update:

- owner token and page-oriented remote-free core are implemented
- worker-thread remote free + TLS flush + owner drain smoke passed
- still no BenchLab/HZ3 behavior route is enabled

P3 update:

- dedicated BenchLab HZ5 adapter is implemented
- exact `4K/8K/64K align=8192` can route to HZ5 P2
- non-target rows fall back to HZ3 S276
- initial same-run smoke is mixed: 4K wins, 8K/64K lose, so the next step is
  owner-local run reuse rather than route expansion

P4/P5/P6 update:

- P4 adds owner-local run cache and remote-drain-to-cache reuse.
- P5 adds diagnostic counters for tcache hits, drain, remote buffering, and
  segment scan steps.
- P5 exposed that a single HZ5 segment let 4K consume the HZ5 arena, after
  which 8K/64K repeatedly scanned and fell back to HZ3.
- P6 adds a bounded multi-segment list and scans/drains all HZ5 segments.
- Focused repeat-3:
  `results/synthetic-sweep/20260520_041008_428`

Median result versus HZ3 speed default:

```text
4K/a8192:  HZ3 2.49M, HZ5-P6 8.99M
8K/a8192:  HZ3 2.50M, HZ5-P6 5.10M
64K/a8192: HZ3 3.93M, HZ5-P6 6.94M
```

Decision:

- HZ5 page/run-first is now a confirmed structural win on the target rows.
- Keep the lane diagnostic/prototype until destructor flushing, isolated safety,
  route accounting, and RSS/segment release policy are implemented.

P7 update:

- Windows FLS destructor now flushes each thread's remote buffer.
- Owner liveness prevents remote pushes to already-dead owners.
- Owner destructor releases pending remote lists and TLS cached runs.
- Final diagnostic cleanup releases abandoned pending remote lists before
  printing the P7 safety line.
- Safety smoke:
  `results/synthetic-sweep/20260520_042152_215`
- Result:
  `[HZ5_P7_SAFETY] segments=7 run_starts=0 remote_pending=0`

Decision:

- P7 resolves the first visible orphan/pending-remote safety gap.
- It remains diagnostic because P5 counters and final cleanup are enabled.
- Next patch should create a counter-free P8 candidate lane or a build flag to
  disable HZ5 diagnostic counters for clean speed comparison.

P8 update:

- Added `HZ5_DIAGNOSTIC_STATS=0` counter-free build mode.
- Added BenchLab candidate manifest:
  `manifests/hakozuna-hz5-p8-aligned8192.toml`
- Added aliases:
  `hakozuna-hz5-p8-aligned8192`, `hz5-p8-aligned8192`
- Built:
  `private/windows-allocators/hakozuna/adapters/benchlab_hz5_p8_aligned8192.dll`

Counter-free repeat-3:

```text
combined: results/synthetic-sweep/20260520_043957_235
isolated: results/synthetic-sweep/20260520_044010_669
guard:    results/synthetic-sweep/20260520_044037_908
```

Isolated median result versus HZ3 speed default:

```text
4K/a8192:  HZ3 2.55M, HZ5-P8 7.38M
8K/a8192:  HZ3 2.37M, HZ5-P8 5.37M
64K/a8192: HZ3 2.49M, HZ5-P8 5.23M
```

Decision:

- P8 is a real candidate/watch lane.
- It is not promotion-ready because variance is still high and RSS/segment
  release policy is not production-shaped yet.

P8 external repeat-10:

```text
results/synthetic-sweep/20260520_050316_725
```

Result:

```text
4K/a8192:  HZ3 2.45M, P8 3.31M, HZ4 5.62M
8K/a8192:  HZ3 2.81M, P8 8.12M, HZ4 8.52M
64K/a8192: HZ3 3.30M, P8 3.08M, HZ4 3.08M
```

Decision:

- P8 broad route is not clean.
- `8K/a8192` is strong and near HZ4.
- `64K/a8192` should fall back to HZ3 for now.

P9 update:

- Added `BENCHLAB_HZ5_ROUTE_64K_A8192=0` route guard.
- Added candidate manifest:
  `manifests/hakozuna-hz5-p9-a8192-4k8k.toml`
- Added alias:
  `hz5-p9-a8192-4k8k`
- Built:
  `private/windows-allocators/hakozuna/adapters/benchlab_hz5_p9_a8192_4k8k.dll`

P9 isolated repeat-5:

```text
results/synthetic-sweep/20260520_050735_601
```

Result:

```text
4K/a8192:  HZ3 3.39M, P9 8.12M, HZ4 5.39M
8K/a8192:  HZ3 2.28M, P9 8.42M, HZ4 8.53M
64K/a8192: HZ3 2.95M, P9 2.79M, HZ4 3.03M
```

Decision:

- P9 is the current best HZ5 candidate/watch shape.
- Keep 64K on HZ3 fallback until a separate 64K run-cache/RSS design improves
  it reliably.

P9 guard repeat:

```text
results/synthetic-sweep/20260520_051017_235
```

Result:

```text
4K/a4096:      HZ3 5.79M, P9 5.14M
4K/a4096 r99:  HZ3 3.92M, P9 3.92M
8K/a4096:      HZ3 5.11M, P9 4.66M
64K/a4096:     HZ3 3.45M, P9 3.24M
```

Decision:

- P9 is not promotion-ready.
- The guard rows do not route to HZ5, so next work should isolate adapter
  fallback/free-check overhead before blaming the HZ5 run core.

Fallback-control update:

```text
results/synthetic-sweep/20260520_051218_297
```

The same HZ5 adapter with all HZ5 routes disabled produced mixed guard results:

```text
4K/a4096:      HZ3 4.79M, fallback-control 4.62M
4K/a4096 r99:  HZ3 3.84M, fallback-control 3.89M
8K/a4096:      HZ3 3.65M, fallback-control 4.24M
64K/a4096:     HZ3 3.61M, fallback-control 3.18M
```

Interpretation:

- The guard signal is not specific to HZ5 routing.
- P9 remains candidate/watch, not promotion-ready.

Lazy final-cleanup registration update:

- `win/hakozuna_hz5_adapter.c` now registers HZ5 final cleanup only after a
  successful HZ5 allocation.
- This removes fallback-only `atexit` registration overhead from `a4096` guard
  rows.

Post-fix guard repeat:

```text
results/synthetic-sweep/20260520_051932_372
```

Result:

```text
4K/a4096:      HZ3 4.98M, P9 4.83M
4K/a4096 r99:  HZ3 3.76M, P9 3.76M
8K/a4096:      HZ3 4.54M, P9 4.58M
64K/a4096:     HZ3 3.43M, P9 3.85M
```

Decision:

- P9 guard is no longer a clear blocker.
- Keep P9 as candidate/watch until repeat-10 and safety soak are complete.

P9 focused repeat-10:

```text
results/synthetic-sweep/20260520_052256_828
```

Result:

```text
4K/a8192:  HZ3 2.60M, P9 5.94M, HZ4 5.71M, tcmalloc 5.64M
8K/a8192:  HZ3 2.50M, P9 5.87M, HZ4 8.13M, tcmalloc 6.37M
64K/a8192: HZ3 2.55M, P9 2.56M, HZ4 2.96M, mimalloc 3.63M
```

Decision:

- P9 is a confirmed structural win for `4K/8K align=8192`.
- Keep `64K/a8192` on HZ3 fallback.
- P9 remains candidate/watch, because fallback-only rows still need guard
  discipline before any broad/default promotion.

P9 mixed-soak repeat-10:

```text
results/synthetic-sweep/20260520_052625_975
```

Result:

```text
4K/a8192:        HZ3 2.53M, P9 7.86M
8K/a8192:        HZ3 2.52M, P9 7.83M
64K/a8192:       HZ3 2.80M, P9 2.53M
256K/a4096:      HZ3 2.74M, P9 2.41M
64K RSS plateau: HZ3 658 ops/s, P9 657 ops/s
256K plateau:    HZ3 305 ops/s, P9 315 ops/s
```

Interpretation:

- The target 4K/8K rows remain very strong under the mixed-soak profile.
- RSS plateau rows are flat, so P9 is not currently creating an RSS cliff in
  these checks.
- The fallback-only 64K/256K speed rows are watch items. They do not route to
  HZ5, so any repeat-stable loss is likely adapter/free-check overhead or
  measurement carryover rather than the aligned-run core.

P10 lock-free segment ownership lookup:

- `hz5_p1_seg_from_ptr()` no longer takes the global segment lock.
- Segments are published through atomics and never freed during process life, so
  ownership checks can scan the published table safely.
- This is intended to reduce fallback-only `free` overhead after a workload has
  used at least one HZ5 allocation.

P10 smoke:

```text
HZ5-P1/P2/P4/P7 aligned-run smoke passed
```

P10 guard repeat-5:

```text
results/synthetic-sweep/20260520_054013_000
```

Result:

```text
4K/a4096:      HZ3 5.22M, P9/P10 5.12M, fallback-control 4.71M
4K/a4096 r99:  HZ3 3.86M, P9/P10 3.94M, fallback-control 3.76M
8K/a4096:      HZ3 4.22M, P9/P10 5.54M, fallback-control 4.74M
64K/a4096:     HZ3 3.90M, P9/P10 3.23M, fallback-control 3.82M
```

P10 target repeat-5:

```text
results/synthetic-sweep/20260520_054127_744
```

Result:

```text
4K/a8192:  HZ3 2.89M, P9/P10 10.80M, HZ4 5.67M, tcmalloc 6.88M
8K/a8192:  HZ3 2.49M, P9/P10 10.93M, HZ4 8.09M, tcmalloc 5.26M
64K/a8192: HZ3 4.06M, P9/P10 2.70M, HZ4 3.88M, tcmalloc 2.16M
```

Decision:

- The 4K/8K HZ5 seed is now very strong.
- The 64K rows remain separate work. Do not use P9/P10 as a 64K promotion lane.
- Next work should avoid HZ5 checks on fallback-only 64K rows or introduce a
  distinct 64K HZ5 design rather than broadening the current P9 route.

P11 segment range fast reject:

- Added atomic `min_base` / `max_end` tracking for published HZ5 segments.
- `hz5_p1_seg_from_ptr()` now rejects pointers outside the HZ5 segment span
  before scanning the segment table.
- This keeps P9 routing unchanged while reducing fallback-only ownership-check
  overhead.

P11 64K guard repeat-5:

```text
results/synthetic-sweep/20260520_054554_593
```

Result:

```text
64K/a4096: HZ3 3.65M, P9/P11 3.94M, fallback-control 3.99M
```

P11 target repeat-5:

```text
results/synthetic-sweep/20260520_054554_756
```

Result:

```text
4K/a8192:  HZ3 3.22M, P9/P11 10.93M, HZ4 5.19M
8K/a8192:  HZ3 3.86M, P9/P11 10.39M, HZ4 8.46M
64K/a8192: HZ3 3.22M, P9/P11 2.99M, HZ4 3.20M
```

Decision:

- P11 is the best current HZ5 seed state.
- `4K/8K align=8192` are repeatedly strong.
- `64K/a4096` guard recovered in the focused check.
- `64K/a8192` remains fallback-only and should not be routed to HZ5 until a
  dedicated 64K design exists.

P12.1/P12.2 run16 preflight:

- Added `Hz5RunClassPolicy`.
- Kept 4K/8K P11 owner cache caps unchanged.
- Classified exact 64K/a8192 as `HZ5_RUNBAND_LARGE16` with initial
  `owner_cache_cap=0`.
- Added `hz5-p12-route-shadow` diagnostic lane.
- 64K/a8192 remains HZ3 fallback in this lane.

P12.1/P12.2 repeat-3:

```text
results/synthetic-sweep/20260520_063131_446
```

Result:

```text
4K/a8192:  P11 10.01M, P12-shadow 10.45M
8K/a8192:  P11 10.98M, P12-shadow 10.11M
64K/a8192: P11  2.69M, P12-shadow  2.82M
```

Route-shadow counters:

```text
64K/a8192:
  candidate_64k_a8192=52000
  would_route_64k_a8192=52000
  fallback_64k_a8192=52000

64K/a4096:
  guard_64k_a4096=52000
  candidate_64k_a8192=0

65537/a16 and 65537/a4096:
  reject_65537=52000
  candidate_64k_a8192=0
```

Decision:

- P12.1/P12.2 are GO.
- Exact 64K/a8192 route scope is clean.
- Next step is P12.3 cap0 behavior lane for exact 64K/a8192 only.

P12.3/P12.4 run16 behavior:

- Added `hz5-p12-run16-cap0`.
  - exact 64K/a8192 routes to HZ5 run16.
  - `owner_cache_cap=0`.
  - diagnostic raw-cost lane.
- Added `hz5-p12-run16-cap1`.
  - exact 64K/a8192 routes to HZ5 run16.
  - `owner_cache_cap=1`.
  - minimal reuse lane.
- P11 4K/8K route remains unchanged in both lanes.

Target repeat-5:

```text
results/synthetic-sweep/20260520_063609_046
```

Result:

```text
4K/a8192:  HZ3 2.50M, P11 10.84M, P12-cap0 10.96M, P12-cap1 10.87M, HZ4 5.67M
8K/a8192:  HZ3 3.48M, P11  9.90M, P12-cap0 10.27M, P12-cap1 11.04M, HZ4 8.50M
64K/a8192: HZ3 2.37M, P11  2.41M, P12-cap0  2.48M, P12-cap1  3.46M, HZ4 2.69M
```

Guard repeat-5:

```text
results/synthetic-sweep/20260520_063938_898
```

Result:

```text
4K/a4096:      HZ3 4.65M, P11 5.33M, P12-cap1 5.06M
4K/a4096 r99:  HZ3 3.93M, P11 4.08M, P12-cap1 3.93M
8K/a4096:      HZ3 5.24M, P11 4.44M, P12-cap1 3.99M
64K/a4096:     HZ3 3.75M, P11 2.93M, P12-cap1 3.16M
```

Over-64K fallback guard repeat-3:

```text
results/synthetic-sweep/20260520_063938_959
```

Result:

```text
65537/a16:    HZ3 3.24M, P11 3.16M, P12-cap1 6.72M
65537/a4096:  HZ3 3.29M, P11 5.58M, P12-cap1 4.20M
256K/a4096:   HZ3 4.94M, P11 5.21M, P12-cap1 5.20M
```

Decision:

- P12-cap1 is a strong keep candidate for exact 64K/a8192.
- P12-cap0 is diagnostic only.
- P11's 4K/8K seed route is preserved.
- P12-cap1 is not promotion-ready: fallback-only a4096 guards remain noisy and
  uneven.
- Next step is P13 RSS/segment-release hardening plus longer focused safety for
  P12-cap1.

P12-cap1 mixed safety/RSS:

```text
mixed-soak: results/synthetic-sweep/20260520_064330_910
carryover:  results/synthetic-sweep/20260520_064330_932
```

Mixed-soak repeat-5 result:

```text
4K/a8192:        HZ3 2.66M, P11 10.81M, P12-cap1  9.74M
8K/a8192:        HZ3 2.53M, P11 11.30M, P12-cap1  9.01M
64K/a8192:       HZ3 2.53M, P11  2.83M, P12-cap1  3.11M
256K/a4096:      HZ3 2.40M, P11  3.22M, P12-cap1  2.42M
rss64 idle150:   HZ3 621.65, P11 617.21, P12-cap1 670.04
rss256 idle250:  HZ3 297.20, P11 316.00, P12-cap1 320.95
```

Carryover repeat-3 result:

```text
4K/a8192:               HZ3 3.40M, P11 8.31M, P12-cap1  8.28M
8K/a8192:               HZ3 2.83M, P11 6.94M, P12-cap1 10.83M
64K/a8192:              HZ3 2.44M, P11 2.70M, P12-cap1  3.30M
8K/a4096 after a8192:   HZ3 4.44M, P11 4.69M, P12-cap1  4.32M
64K/a4096 after a8192:  HZ3 2.86M, P11 3.64M, P12-cap1  3.13M
```

Updated decision:

- P12-cap1 remains a strong HZ5 run16 seed.
- No mixed/carryover crash or one-shot safety counter failure was observed.
- Keep P12 exact and diagnostic.
- P13 should add RSS/segment accounting and release hardening before route
  widening or default discussion.

P13 segment accounting / run16 hint:

- Added diagnostic segment snapshot lines:
  - `[HZ5_P13_SEGMENTS.before_cleanup]`
  - `[HZ5_P13_SEGMENTS.after_cleanup]`
- Added per-segment `run16_a8192_hint_page` so 16-page aligned run allocation
  starts near the last useful region and wraps on miss.
- Fixed `tcache_destructor_release` page bucket accounting.

Repeat-3:

```text
results/synthetic-sweep/20260520_065306_764
```

Result:

```text
4K/a8192:  P12-cap1 9.39M, P13 diagnostic 6.28M
8K/a8192:  P12-cap1 9.79M, P13 diagnostic 5.79M
64K/a8192: P12-cap1 3.57M, P13 diagnostic 3.11M
```

64K/a8192 diagnostic signal:

```text
segment_alloc_scan_step median: 308,613
segments before cleanup median: 2
empty_segments before cleanup median: 2
remote_pending: 0
live_pages after cleanup: 0
```

Decision:

- P13 accounting is working.
- The run16 hint is a keep because it reduces 64K page scan work sharply.
- Empty segment release should be the next RSS hardening step, but only after a
  safe segment-table retire design.

P13 64K-only follow-up:

```text
results/synthetic-sweep/20260520_070231_091
results/synthetic-sweep/20260520_070528_872
results/synthetic-sweep/20260520_070554_083
```

- A dedicated `hz5-64k-a8192` workload profile isolates the exact P12 target.
- Isolated repeat-10 keeps P12 cap1 ahead on 64K/a8192:
  - HZ3 speed-default: `2.38M`
  - HZ5-P11/P9 fallback: `2.73M`
  - HZ5-P12 cap1: `3.42M`
  - HZ4: `2.52M`
- The earlier broad-sweep `1.91M` P12 result is treated as volatility/watch, not
  proof that the run16 path is broken.
- P13 diagnostic counters still show `remote_pending=0` and final
  `live_pages=0`.
- `hz5_remote_release_owner()` and `hz5_remote_release_all_pending()` now capture
  `run_pages` before freeing the run so pages16 destructor/final-release counters
  are bucketed correctly.

P13 shutdown-only empty segment release:

```text
results/synthetic-sweep/20260520_070816_774
```

- `after_cleanup`: `segments=2`, `empty_segments=2`, `live_pages=0`,
  `remote_pending=0`
- `shutdown_release`: `released_segments=2`
- `after_release`: `segments=0`, `live_pages=0`, `remote_pending=0`
- This validates shutdown cleanup only. Runtime segment release remains blocked
  on a safe concurrent segment-table retire design.

## P14 Runtime Segment Retire Design

Goal:

```text
Runtime empty-segment retirement without touching P11/P12 speed lanes.
```

Current hazard:

```text
hz5_p1_seg_from_ptr()
  scans atomic segment slots without taking the segment lock.
```

Because of that, runtime code must not remove a segment and immediately
`_aligned_free` it. A racing classifier could have already loaded the segment
pointer.

Candidate order:

1. P14b retired quarantine:
   - mark empty segment retired;
   - keep memory and segment-table slot valid;
   - allocation skips retired segments;
   - snapshots report retired segments/bytes.
2. P14c diagnostic lock-protected release:
   - only for diagnostic lane;
   - proves real `_aligned_free` but intentionally pays lookup locking cost.
3. P14d epoch/hazard or tombstone release:
   - production-grade runtime release candidate;
   - defer until P14b shows the retire state is useful.

Recommendation:

```text
Implement P14b first. Do not runtime-free segment memory yet.
```

P14b does not return RSS, but it proves the state transition and keeps
classification safe. P11/P12 counter-free lanes remain unchanged.

P14b first diagnostic result:

```text
results/synthetic-sweep/20260520_071306_240
```

- `after_cleanup`: `segments=3`, `empty_segments=3`, `retired_segments=0`,
  `live_pages=0`, `remote_pending=0`
- `P14_RETIRED_QUARANTINE`: `retired_segments=3`
- `after_retire`: `segments=3`, `retired_segments=3`, `live_pages=0`,
  `remote_pending=0`
- `shutdown_release`: `released_segments=3`
- `after_release`: `segments=0`, `retired_segments=0`, `live_pages=0`,
  `remote_pending=0`

Implementation note:

- `hz5_p1_segment_get()` now scans for the first non-retired segment instead of
  assuming slot 0 is reusable.
- This matters for later runtime-quarantine diagnostics where slot 0 may be
  retired while other segments remain active.

P14.1 retire pressure counters:

```text
results/synthetic-sweep/20260520_073550_680
```

On `hz5-64k-a8192`:

```text
after_cleanup segments=3 empty_segments=3 live_pages=0 remote_pending=0
after_retire retired_segments=3 retired_bytes=6291456
p14_empty_transition=25
p14_empty_transition_bytes=52428800
p14_retire_candidate_segment=3
p14_retire_candidate_bytes=6291456
p14_retire_ok=3
p14_retire_ok_bytes=6291456
```

Interpretation:

- The 64K workload creates empty-segment churn, but only about `6MiB` remains as
  final retire pressure in this run.
- Keep P14b. Do not add production lookup locks, epochs, or hazards yet.
- P14c locked runtime release should wait for a workload that shows larger
  retained retired bytes or a clear RSS target.

P14 pressure profile:

```text
results/synthetic-sweep/20260520_073829_128
results/synthetic-sweep/20260520_074105_378
```

The BenchLab profile `hz5-p14-segment-pressure` creates stronger empty segment
pressure with existing workload kinds.

P14b results:

```text
p14-rss-64k-a8192-burst512:
  retired_segments=17
  retired_bytes=35.65MiB
  p14_empty_transition_bytes=192.94MiB
  remote_buffer_pending=0
  tcache_refs=0

p14-phase-64k-a8192-release:
  retired_segments=46
  retired_bytes=96.47MiB
  p14_empty_transition_bytes=503.32MiB
  remote_buffer_pending=0
  tcache_refs=0

p14-mixed-a8192-8k64k:
  retired_segments=44
  retired_bytes=92.27MiB
  p14_empty_transition_bytes=96.47MiB
  remote_buffer_pending=0
  tcache_refs=0
```

P14 predicate hardening:

- Added segment `tcache_refs`.
- Added segment `remote_buffer_pending_hint`.
- P14 retire rejects non-zero cache/buffer refs.
- Snapshots now report both fields.

Decision:

- P14c is justified as a diagnostic RSS-proof experiment for pressure workloads.
- P14c is still not production runtime release.
- A correct locked release lane must protect the whole classify/use/free scope,
  not just return a raw `Hz5Seg*` from a locked table lookup.

P14.2 locked-lookup footing:

```text
results/synthetic-sweep/20260520_082119_813
```

Implemented:

- `HZ5_P14_LOCKED_LOOKUP_RELEASE=1`
- `hz5_p1_seg_from_ptr()` takes the segment lock while scanning table slots.
- Added lookup counters:
  - `p14_locked_lookup_call`
  - `p14_locked_lookup_hit`
  - `p14_locked_lookup_miss_range`
  - `p14_locked_lookup_miss_table`
  - `p14_locked_lookup_bad_magic`
- Added BenchLab manifest `hakozuna-hz5-p14-locked-lookup.toml`.

Repeat-1 pressure result:

```text
p14_locked_lookup_call=1086484
p14_locked_lookup_hit=1086484
miss_range=0
miss_table=0
bad_magic=0
retired_segments=46
retired_bytes=96.47MiB
remote_pending=0
remote_buffer_pending=0
tcache_refs=0
```

Interpretation:

- P14.2 confirms the locked table lookup footing can run the pressure profile.
- It visibly slows throughput, so it is diagnostic-only.
- Runtime segment free still needs a scoped lifetime or descriptor/tombstone
  design; this patch deliberately does not free segment memory at runtime.

## P11 speed-core separation

Current-source rebuilds of the P11/P9 lane regressed after P13/P14 segment
retire diagnostics had been added. The old high-speed P11 artifact was still
fast, so the issue was not the page/run idea itself; it was the speed lane
carrying later diagnostic layout and bookkeeping.

Added `HZ5_P11_SPEED_CORE=1` as an explicit clean speed-core switch:

- restores the initial P11 segment layout shape by compiling out
  `run16_a8192_hint_page`, `tcache_refs`, and `remote_buffer_pending_hint`;
- disables retired/quarantine/shutdown-release behavior for speed lanes;
- uses the original simple segment scan/allocation path instead of the run16
  hint path;
- leaves P13/P14 diagnostics available when the switch is off.

BenchLab added `-P11SpeedCore` to the HZ5 behavior-lane build script.

Evidence:

```text
results/synthetic-sweep/20260520_195050_075
results/synthetic-sweep/20260520_195105_673
results/synthetic-sweep/20260520_195122_014
```

Key medians:

```text
P9 rebuilt with -P11SpeedCore:
  pc-r90-4k-a8192-t4: 14.63M
  pc-r90-8k-a8192-t4: 13.32M

P20 lazy fallback rebuilt with -P11SpeedCore:
  pc-r90-4k-a8192-t4: 14.37M
  pc-r90-8k-a8192-t4: 13.57M
  exact-route fallback load_count: 0
  steady VA: ~4.20-4.23 GiB
  steady RSS: ~28-31 MiB

P20 64K/a8192 fallback-trigger:
  load_count: 1
  median throughput: 5.12M vs HZ3 6.22M
```

Decision:

- P11/P20 exact `4K/8K align=8192` speed lanes should use
  `HZ5_P11_SPEED_CORE=1`.
- P13/P14 remain diagnostic-only and must not contaminate speed profiles.
- P20 + P11 speed core is the current route-strict candidate/watch lane because
  it combines restored P11 speed with low VA/RSS lazy fallback behavior.
- `64K/a8192` remains a separate native-route problem; P20 fallback is safe but
  not a 64K speed solution.

## P43 lowpage64 segment-slot modularization

P43b.1 descriptor-list dry run proved the safety direction:

- no real `MEM_DECOMMIT` yet;
- exact `64K/a8192` stays HZ5-owned with lazy HZ3 fallback load count `0`;
- data-page intrusive ownership is being phased out before real slot decommit.

Cleanup before the next P43b step:

- Split the P43 segment-slot source out of `lowpage/hz5_lowpage64.c`.
- Added `lowpage/hz5_lowpage64_p43_segment.h`.
- Added `lowpage/hz5_lowpage64_p43_segment.c`.
- Moved P43 segment reservation, slot commit/decommit, may-own lookup, and P43
  counters into the new module.
- Kept `hz5_lowpage64.c` as the lowpage policy/cache body that calls the raw
  source module.
- Updated the BenchLab HZ5 behavior-lane build script to compile the new P43
  module.

Smoke after split:

```text
results/synthetic-sweep/20260521_083405_558

pc-r90-64k-a8192-t4:
  1.95M-2.13M ops/s
  steady RSS: 31.32-31.74 MiB
  steady VA: 4.22-4.24 GiB

pc-r99-64k-a8192-t4:
  1.26M-1.37M ops/s
  steady RSS: 32.40-32.70 MiB
  steady VA: 4.24-4.25 GiB

rss-plateau-64k-a8192-idle150:
  steady RSS: 50.39-51.70 MiB
  steady VA: 4.26 GiB
```

Decision:

- The split is behavior-preserving enough for P43b.1 development.
- P43b minimal slot-decommit remains RSS evidence / crash no-go.
- P43b.1 descriptor-owned dry run remains the current safe baseline.
- Next implementation should continue with full SlotRef / descriptor-owned list
  cleanup before enabling real slot `MEM_DECOMMIT` again.

## P43b descriptor SlotRef and ACTIVE-only gate

P43b follow-up implemented the next safety footing:

- P43 descriptor-owned committed SlotRef stack:
  - freed committed slots are linked through descriptor metadata;
  - list traversal no longer uses data-page intrusive `next`;
  - links can cross segment boundaries via `{segment, slot}` references.
- P43 lowpage lookup now returns:
  - `MISS`
  - `OWNED_ACTIVE`
  - `OWNED_NONACTIVE`
- HZ5 policy now checks the lowpage lookup before wrapper decode:
  - `OWNED_ACTIVE` can proceed to wrapper/header decode;
  - `OWNED_NONACTIVE` is treated as stale/double/free inactive and is not sent
    to HZ3 fallback;
  - `MISS` can still use fallback only if fallback is already loaded.

Smoke after SlotRef + ACTIVE gate:

```text
results/synthetic-sweep/20260521_120921_997
results/synthetic-sweep/20260521_121225_344

fallback load_count:
  0 on exact HZ5 route

plateau steady VA:
  4.24-4.26 GiB class

plateau steady RSS:
  mostly 50-53 MiB class in repeat-3 smoke
```

Interpretation:

- Safety direction is cleaner: P43 can now distinguish active vs non-active
  lowpage slots before any data-page header read.
- Speed did not recover; the descriptor-owned direct-release path is still
  lock-heavy and remains below the old P43a/P25 speed lanes.
- This is acceptable as P43b.2 safety groundwork, not a promotion candidate.

Next:

- Add PAGE_NOACCESS diagnostic lane before real `MEM_DECOMMIT`.
- Real slot `MEM_DECOMMIT` should only be re-enabled after PAGE_NOACCESS repeat
  is clean.
- Speed recovery should be addressed later with segment-local rewarm/batching,
  not before the decommit safety proof.

## P43b PAGE_NOACCESS and slot-decommit retry

P43b.2 added a diagnostic PAGE_NOACCESS lane to verify that inactive slot data
pages are no longer used as allocator metadata:

- build switch: `-P43PageNoAccess`
- manifest: `manifests/hakozuna-hz5-p43b2-page-noaccess.toml`
- aliases: `hz5-p43b2-page-noaccess`, `hz5-page-noaccess`
- diagnostic-only; the build script rejects it with `-SpeedLane`

The implementation also tightened the transient state around cold slots:

- freed slots become descriptor-non-active before the VM operation;
- cold slots are excluded from allocation while PAGE_NOACCESS or MEM_DECOMMIT
  is in progress;
- active lookup is restored only after the page protection/commit operation
  succeeds.

PAGE_NOACCESS repeat-3:

```text
results/synthetic-sweep/20260521_161937_302

pc-r90-64k-a8192-t4:
  0.20M-0.22M ops/s
  steady RSS: 31.57-32.04 MiB
  steady VA: 4.22-4.24 GiB

pc-r99-64k-a8192-t4:
  0.20M-0.22M ops/s
  steady RSS: 31.71-32.84 MiB
  steady VA: 4.24 GiB

rss-plateau-64k-a8192-idle150:
  steady RSS: 42.86-43.06 MiB
  steady VA: 4.26 GiB

fallback load_count:
  0
```

Decision:

- PAGE_NOACCESS is clean in repeat-3.
- The lane is intentionally slow because every reuse goes through
  `VirtualProtect`.
- Treat it as a safety proof that inactive slot data-page reads are no longer
  present in these profiles, not as a performance lane.

The same transient-state fix was then applied to the real slot MEM_DECOMMIT
observation lane.

Real slot-decommit observation repeat-3:

```text
results/synthetic-sweep/20260521_162014_981

pc-r90-64k-a8192-t4:
  1.95M-2.11M ops/s
  steady RSS: 31.91-32.35 MiB
  steady VA: 4.25 GiB

pc-r99-64k-a8192-t4:
  1.19M-1.43M ops/s
  steady RSS: 33.74-34.77 MiB
  steady VA: 4.26 GiB

rss-plateau-64k-a8192-idle150:
  steady RSS: 51.56-53.67 MiB
  steady VA: 4.27-4.29 GiB

P43 counters:
  p43_slot_decommits: 936-976
  p43_slot_decommit_failures: 0

fallback load_count:
  0
```

Decision:

- The old intermittent crash did not reproduce in this repeat-3 after the
  transient cold-slot fix.
- The RSS/VA signal remains useful.
- Throughput is still far below the P25/P33 speed lanes, so this is not a
  promotion candidate.
- Next work should recover speed with descriptor-owned segment-local reuse,
  not by reintroducing data-page intrusive metadata or direct free-to-decommit
  shortcuts.

## P43c segment-local rewarm4

P43c added a narrow rewarm knob for the P43 segment-slot raw source:

- build switch: `-P43RewarmBatch <N>`
- macro: `BENCHLAB_HZ5_P43_REWARM_BATCH`
- first lane: `P43RewarmBatch=4`
- manifest: `manifests/hakozuna-hz5-p43c-segment-rewarm4.toml`

The mechanism is deliberately local:

- after a cold slot is recommitted for the current allocation, P43c tries to
  recommit up to `N-1` additional cold slots from the same 2 MiB segment;
- the extra slots are left committed and free in descriptor state;
- no global broad refill is performed.

P43c rewarm4 repeat-3:

```text
results/synthetic-sweep/20260521_162654_916

pc-r90-64k-a8192-t4:
  2.05M-2.20M ops/s
  steady RSS: 31.66-32.44 MiB
  steady VA: 4.23-4.25 GiB

pc-r99-64k-a8192-t4:
  1.25M-1.36M ops/s
  steady RSS: 33.72-34.35 MiB
  steady VA: 4.26-4.27 GiB

rss-plateau-64k-a8192-idle150:
  steady RSS: 51.78-52.44 MiB
  steady VA: 4.28-4.30 GiB

fallback load_count:
  0
```

Decision:

- P43c rewarm4 preserves the P43b RSS/VA shape.
- It does not materially recover r99 speed.
- Keep as diagnostic evidence, not promotion.
- The remaining speed issue is likely above the VM call itself: P43 slot
  decommit still interacts with the older lowpage global/stash lifecycle rather
  than a fully descriptor-owned hot/global reuse policy.

## P43d committed-retain64

P43d tested whether P43b/P43c speed loss is mostly caused by excessive
`MEM_DECOMMIT` frequency.

New knob:

- build switch: `-P43CommittedRetainCap <N>`
- macro: `BENCHLAB_HZ5_P43_COMMITTED_RETAIN_CAP`
- first lane: `P43CommittedRetainCap=64`
- manifest: `manifests/hakozuna-hz5-p43d-committed-retain64.toml`

Mechanism:

- when a P43 slot is freed under slot-decommit mode, keep it committed in the
  descriptor-owned SlotRef cache while retained count is below the cap;
- only overflow falls through to slot `MEM_DECOMMIT`;
- TLS SlotRef cache is not enabled yet.

P43d committed-retain64 repeat-3:

```text
results/synthetic-sweep/20260521_170134_180

pc-r90-64k-a8192-t4:
  1.85M-2.15M ops/s
  steady RSS: 31.53-32.46 MiB
  steady VA: 4.23-4.25 GiB

pc-r99-64k-a8192-t4:
  1.36M-1.59M ops/s
  steady RSS: 33.16-35.44 MiB
  steady VA: 4.26 GiB

rss-plateau-64k-a8192-idle150:
  steady RSS: 58.55-60.21 MiB
  steady VA: 4.26-4.29 GiB

p43_slot_decommits:
  69-196

fallback load_count:
  0
```

Decision:

- The cap works: slot decommits dropped by roughly an order of magnitude versus
  P43b/P43c.
- Speed did not recover enough.
- Plateau RSS regressed toward P43a/P33 territory.
- P43d cap64 is no-go / evidence.
- Further fixed-cap sweep is low ROI unless paired with a fuller
  descriptor-owned hot/global lifecycle that removes the global lock/scan and
  old raw-pointer cache topology from the hot path.
