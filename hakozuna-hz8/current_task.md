# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

Stable documentation starts at `docs/README.md`. Windows benchmark lane
status is centralized in `docs/HZ8_WINDOWS_LANE_STATUS_L1.md`; do not infer
promotion status from build target names alone.

## Restart Surface: HZ8 Reclaim Integration

## Next Development Order

HZ8 is the active public integration line. Keep HZ10-HZ12 frozen as research
suppliers unless a new measured HZ8 weakness requires reopening one contract.

```text
Step 1: MediumLocalFastTier current-default recheck
  reuse the existing opt-in implementation and attribution
  compare hz8-v2 with hz8-v2-mediumlocalfast on Windows
  keep the candidate research-only and production counters off

Step 2: decide the medium/local box
  close again if local gain is absent or remote/small regresses over 5%
  retain as an opt-in only if the gain is profile-scoped
  promote only after repeatable Windows and Linux Pareto improvement

Step 3: HZ8-native reclaim shadow
  import only the HZ12 bounded reclaim contract
  do not merge the HZ12 allocator core or ownership hot path
  behavior work requires a measurable reclaimable-byte target first
```

Do not open another allocator generation for these steps. Small-span inventory
is now handled by default Mag16; the next candidate must address a different,
measured retention boundary.

The current public matrix fixes the next measured speed gap. HZ8 reaches about
24-26% of tcmalloc on main/medium local rows while retaining much lower post
RSS. Existing attribution already excludes first-touch, remote protocol, and
64K geometry as the primary medium-local limiter. An older active-run local
fast tier reached near-perfect reuse but was HOLD because it enlarged the hot
path and regressed the then-current broad gate. Re-run that exact behavior on
the current Mag16 default before designing another medium cache.

The Windows recheck is complete and remains NO-GO. A long AB repeat-5 median
with production counters disabled measured candidate/baseline ratios of 0.976
on balanced, 0.971 on larger_sizes, 0.970 on fixed 8K, and 0.986 on fixed 16K.
The short matrix signal that initially looked positive did not survive the
longer run. Keep `hz8-v2-mediumlocalfast` as reproducibility evidence only and
do not add another active-run branch, mask, or per-run fast state. The next
medium/local investigation must reduce common-entry fixed cost or change the
substrate outside the HZ8 medium hot path.

Active next box: `HZ8MediumFixed8KCostAudit-L0`. Audit the release-equivalent
fixed 8K alloc/free path before another behavior change. Separate removable
decode/call/layout work from required fail-closed validation and state writes.
If removable work is at least 30% of the path, open one common-entry trim box;
otherwise compare the existing HZ10 intrusive-page/O(1)-pagemap substrate via
an HZ8-native shadow contract. See `docs/HZ8_MEDIUM_FIXED8K_COST_AUDIT_L0.md`.

L0 measured HZ8 at 65.447M / 258.09 process cycles per logical fixed-8K
operation versus tcmalloc at 213.284M / 76.60 cycles. The audited HZ8 medium
alloc/free objects contain no locked RMW, while the static alloc/free bodies
remain large. This rules out atomic contention as the primary explanation but
does not yet prove which cold blocks execute. Active next step: isolate and
classify the active-run alloc and same-owner free basic blocks. Do not implement
another cache or remove a safety check before that split is complete.

The HZ10 substrate sibling is now connected only inside the audit tool. On the
same Windows fixed-8K repeat-3 it measured 184.485M / 94.74 cycles, versus HZ8
66.105M / 254.50 cycles and tcmalloc 216.546M / 75.10 cycles. This is a 2.79x
HZ8 improvement and about 85% of tcmalloc throughput. The substrate gate is
GO; incremental HZ8 medium-entry trimming is no longer the primary track.

Next: write the HZ8/HZ10 contract delta before behavior integration. Preserve
MISS/VALID/INVALID, fail-closed stale/duplicate handling, owner generation,
bounded remote pending, owner-exit recovery, and low post-RSS. Do not copy the
HZ10 public entry wholesale or expose the shadow in the normal matrix.

The contract delta is now fixed in
`docs/HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`. Active implementation
order is P0 classification shadow, then P1 detached page-state shadow, then an
opt-in fixed-8K behavior sibling. P2 is not authorized until P0/P1 report zero
route/generation/state disagreements.

P0 classification shadow is implemented and passes the Windows fixed 8K/16K
valid rows with 203,840/203,840 hits, zero miss, zero run mismatch, and zero
exact-invalid. The interior/duplicate smoke records two exact addresses and
one interior invalid while keeping run mismatch zero. Default builds preprocess
all hooks away. P0 is GO; implement P1 detached live/free state comparison next.

P1 detached state shadow is now GO. Fixed 8K local reports 203,840 state
matches and zero mismatch. The interior/duplicate smoke reports two state
matches, one exact-invalid interior pointer, and zero mismatch. Fixed 8K MT
remote90 at T=8 reports 800,800 state matches, zero state/run mismatch,
effective remote 90.01%, and zero fallback/failure. P2 may now begin only as an
opt-in fixed-8K behavior sibling; default HZ8 remains unchanged.

P2 local evidence reaches 147.94M fixed-8K ops/s versus HZ8 66.66M and
tcmalloc 210.29M. Interior/valid/duplicate adapter smoke passes. Direct HZ10
remote lifecycle is a blocker: T=8 remote90 passes short 1K/2K controls but
access-violates at 5K and above. Keep `hz8-medium-page8k-local` research-only,
remove it from MT runners, and do not promote behavior. Next design box is an
HZ8-owned bounded remote adapter for the page substrate, not another capacity
knob and not direct HZ10 remote reuse.

P2-R1 is now fixed in
`docs/HZ8_MEDIUM_PAGE8K_REMOTE_ADAPTER_L1.md`. Reuse only the HZ10-derived
intrusive-page idea. HZ8 owns identity, slot state, pending publication,
qstate, inbox drain, and lifecycle. R1 is exact-8K protocol proof with no page
destroy/reuse or owner exit; R2 adds close-publish fencing, hard exit drain,
adoption, generation reuse, retirement, and RSS gates. R1 stays diagnostic and
must not enter the normal matrix or the default hot path.

The first dedicated Windows MT smoke is green: eight remote claims collapse to
one page notification, owner drain returns all eight slots exactly once,
interior and duplicate frees reject, and final pending/qstate is `0/IDLE`.
This proves the R1 state machine skeleton only. Next is a sustained concurrent
publish/drain stress before connecting the module to an allocator behavior row.

Sustained MPSC stress is now GO: four producers, 10,000 rounds, and 80,000
remote claims produced 80,000 owner drains with zero reject, duplicate,
interior, or lost-notification result. Inbox depth stayed bounded at one page
and final state was `pending=0/qstate=IDLE`. Next is the opt-in HZ8 owner bridge;
do not add owner-exit/reuse until that behavior row preserves this equality.

The opt-in behavior bridge now completes exact-8K T=8 remote90 beyond the old
HZ10 crash boundary. At 500K diagnostic load it reports 1,780,137 claims and
the same drain count, zero reject/lost notification/allocation failure, and
23,377 owner-page-cap transitions to the existing HZ8 medium path. Final
equality uses an untimed post-join hard-drain control because R2 owner close and
adoption are not implemented. Atomic-free speed R5 is unstable (about
8.5M..20.3M, median about 9.4M), so performance/default promotion is HOLD.
Next architecture work is R2 lifecycle plus an owner-local available-page
index, not a page-cap ladder.

The owner-local active/available index and intrusive MPSC page inbox are now
implemented. Four-producer protocol stress passes 100 consecutive runs. They
only move remote90 speed from about 9.4M to 10.1M median; diagnostic equality
remains exact, but page-cap transitions rise to 408,552. Local0 is still a
strong signal at 260.50M versus HZ8 143.80M, though below tcmalloc 433.74M.
Freeze R1 performance tuning. The next valid step is R2 owner close/adoption;
do not tune cap, queue, or available-index policy further.

R2 owner close/adoption is now implemented as a research box. Page-local
publish fences protect owner reassignment; thread shutdown closes publication,
waits refs, drains, and moves pages to a permanent orphan. Allocation miss can
adopt one drained orphan page. Lifecycle smoke passes close/adopt and full
8-slot reuse. Windows T=8 remote90 500K reports exact claim/drain equality
(1,499,831), zero allocation failure/lost notification, and no drain-all limit.
Correctness is GO; throughput remains about 8.7M diagnostic, so promotion stays
HOLD. Next evidence should stress concurrent close/publish and repeated owner
generation turnover before any RSS/decommit work.

Concurrent lifecycle stress is now GO across 1,000 owner generations. It
records 40 real close/publish retries while preserving 1,000 close/adopt and
8,000 claim/drain equality with zero reject or lost notification. The R2
ownership transition contract is therefore established. Keep performance HOLD;
the next separate box is page retirement/decommit and bounded orphan residency.

Bounded orphan residency is now implemented. At most 16 empty orphan pages
(1MiB) remain resident; additional fully quiescent pages decommit in 64KiB
units and recommit at the same address after adoption. A 32-page smoke produces
16 decommits, 16 recommits, zero failure, and zero final orphan-resident pages.
The policy is split into `h8_medium_page8k_residency.inc`; core remains below
800 lines. Next evaluation is RSS turnover plus remote safety, not another cap
ladder.

Cross-platform audit fixed two lifecycle gaps: page backing is now explicitly
64KiB aligned on Linux as well as Windows, and thread shutdown removes its dead
owner record after orphan handoff. Strict Linux GCC C11 build/residency smoke
passes. A 1,000-generation turnover smoke ends with exactly one owner-list
entry (the permanent orphan), proving dead owner records do not accumulate.

External review blockers F1-F3 are fixed: seq_cst close/ref validation,
type-stable token-validated owner records, and quiescent-only
`drain_all_control`. Windows full suite and lifecycle repeat-100 pass.

R3 is now connected to the real HZ8 medium entry as
`hz8-r3-page8k-integrated`; its counters remain in a separate diagnostic
binary. Corrected Windows AB/BA repeat-5 medians are +81.77% fixed-8K local,
+11.80% balanced, +3.72% wide_ws, and +11.94% larger_sizes. Repeat-3 median
peak RSS is no higher than HZ8 v2 on all four rows. The rebuilt remote90
control has zero allocation failure, reject, and lost notification, and all
six protocol/lifecycle/residency smokes pass.

R3 final posture: Windows selected opt-in GO; Linux correctness-neutral opt-in
GO; public default unchanged. Windows Redis-like gates are -0.04% (small) and
+1.24% (medium); Linux fixed-8K is -0.21% with lower peak RSS. Remote90 remains
correctness evidence. The Windows speed gain is platform-specific.

MagazineTailReclaim L0 is CLOSED. Broad rows expose only 1.0-1.2MiB maximum
hypothetical tail bytes, far below the 8MiB behavior admission threshold.
has zero failure and 473 real publish retries; Linux strict C11 build passes.
R2 is closed as correctness evidence. Default/performance promotion remains
HOLD; only an R3 integrated medium-entry shadow may reopen the track.

Current Windows attribution after the public matrix:

```text
balanced:
  mag pop 21,105 / hit 8,625 / reject 435
  span commit 12,552
  full-preserve 424,298

wide_ws:
  mag pop 8,595 / hit 1,924 / reject 61
  span commit 6,735
  full-preserve 476,423
```

The residual commit count tracks empty Mag pop attempts, not owner-list scan
steps (still zero). `hz8-v2-mag32` was tested as the research capacity boundary.
Do not change the Mag16 default unless balanced/wide peak RSS improves without
remote or local throughput regression.

Windows Mag32 clears that local gate: balanced/wide/larger R5 improve by about
38%/43%/13%, while peak-RSS spot checks fall from 790.57/399.18/193.41 MiB to
450.70/343.69/159.01 MiB. Fixed MT remote R5 is near-neutral, but effective
remote ratios differ. Linux correctness and larger-local gates pass, but
16..256 regresses 9.4% and deterministic remote90 is -2.1%. Status:
`hz8-v2-mag32` is an explicit larger/local opt-in; Mag16 remains default. See
`docs/HZ8_REUSABLE_SPAN_MAG32_L1.md`.

Per-class balanced attribution is concentrated in class 7: 12,187 of 12,457
empty pops (about 98%), with classes 4/5/6 contributing 8/29/233. Replacement
policy work is closed; the reusable spans are fungible. Class-7-only and
detached-sidecar follow-ups are NO-GO. Linux capacity tuning is closed; do not
test Mag64. Reopen promotion only with a cross-platform application matrix.

Keep HZ8 v2 / KeepRefill frozen as the public default. HZ8 is the integration
line; HZ10, HZ11, and HZ12 remain research suppliers rather than allocator
cores to merge.

`HZ8ReclaimAdapterShadow-L0` is implemented at the existing owner-exit span
walk. It adds no list scan and no malloc/free hook. Windows evidence found
80,538 complete spans / 5.278 GiB in generic 16-4096 churn and 26,574 complete
spans / 1.742 GiB in the remote90 default shape. Live, pending, and state
blockers were zero for those complete sets.

L0 is ACCEPTED as a retirement upper-bound witness. Live-owner
`HZ8ReclaimAdapterBehavior-L1` is CLOSED / NO-GO: cursor and head-window
variants reduced the 5 GiB peak by less than 1% in the repeatable runs while
regressing throughput by roughly 10-33%. The complete spans become visible too
late for commit-time bounded scanning. Do not add a free-hot-path candidate
queue to rescue this track.

`HZ8SpeedAdapterAttribution-L0` is active in a diagnostic-only build. Windows
small rows show an exact active-miss/span-commit coupling and zero owner-list
scan steps when pending is empty. In 16-4096 churn: active miss 80,538, slow
collect 80,538, span commit 80,538, find steps zero. The next narrow box is an
O(1) owner-local reusable-span hint, not an HZ11 core or transfer-cache import.

`HZ8ReusableSpanMagazine-L1` Mag16 is now part of the HZ8 v2 default. It preserves
up to 16 displaced owner-local active hints per class and validates each hint
before reuse. Windows R5 improved 16..256 by 3.15x, 16..2048 by 2.56x, and
16..4096 by 4.11x. The 16..4096 peak fell about 58%, and remote90 median stayed
near-neutral while committed small-span bytes fell about 43%. Windows smokes
pass. Status: PROMOTED after Linux and Windows gates.
Design and task order:
`docs/HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md`.

Linux commit `652fa283` stops active-hint replacement when Mag16 is full.
Linux focused remote90 is now near-neutral (-1.22%) with equal peak RSS, while
focused local remains +7.66%. Windows follow-up R5 also remains positive:
local 16..256 / 16..2048 / 16..4096 improve 2.45x / 1.33x / 2.88x in the
long-lived A/B, and the fixed MT remote row improves 124.891M to 131.737M while
median peak RSS falls from 32.20 MiB to 18.71 MiB. Effective remote ratios
differ, so do not promote a remote-speed claim. Correctness/local candidacy is
GO on both OSes. A broader Windows R5 gate also remained positive: balanced,
wide_ws, and larger_sizes improved by 2.84x, 1.68x, and 1.59x respectively.
Mag16 is now default; `hz8-v2-nomag` is the explicit research control.

Runner policy:

```text
normal comparison:
  hz8-v2

research-only (requires -IncludeHz8Research):
  hz8-v2-nomag
  hz8-v2-mag32
  hz8-v2-mediumlocalfast
  hz8-v3-adaptive-shadow
  hz8-reclaim-shadow
  hz8-speed-attribution
```

The prior adaptive-transfer track is closed. Its L0 shadow was valid evidence,
but L1 was NO-GO because HZ8 already bulk-splices pending bitmap words. Do not
reopen transfer tuning without a new measured weakness.

## Frozen Baselines

```text
small-v0:
  docs/HZ8_SMALL_V0_RC1.md

medium-v1 protocol/geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

medium-v1.1 frozen baseline:
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md

HZ8-v2 current default:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
```

HZ8-v2 / KeepRefill is the current public Linux default. Keep v1.1 as the
comparison baseline. Do not reopen owner queue, pending/qstate protocol,
medium residency, or medium class geometry for incremental tuning.

## Public Position

```text
public allocator:
  HZ8

public claim:
  balanced low-RSS allocator with practical throughput

do not claim:
  universal tcmalloc replacement
  Windows/Linux parity
  cross128 throughput leadership
```

## Current Default Shape

```text
small:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / qstate remote protocol

medium:
  4097..65536
  8K / 16K / 24K / 32K / 48K / 64K
  q64-v12-48k2 geometry
  KeepRefill active-live retention
  lazy128 retained-empty policy

large/direct:
  >65536 currently falls through to system allocation in the default preload
  H8_LARGE_DIRECT_OWNED_L1 exists as a strong default-candidate probe
```

## Latest Public Evidence

```text
paper matrix:
  docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md

MT lane values:
  docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md

current HZ8 MT rows:
  main_r0          107.633M
  main_r50          29.633M
  main_r90          20.610M
  guard_r0         224.750M
  cross128_r90      37.342k
```

Read `cross128_r90` as a known weakness row, not as a HZ8 public strength.

## Active Weakness

```text
row:
  cross128_r90
  T=16, size=16..131072, remote_pct=90, interleaved=1

observed default:
  R10 full row: 37.342k ops/s
  R3 short row: ~70k ops/s

observed probe:
  default + H8_LARGE_DIRECT_OWNED_L1
  cross128_r90 R3: ~2.811M ops/s
  cross128_r0 R3:  ~6.797M ops/s

observed gate after hot-path closure:
  cross128_r90 R10 ratio: 45.048x
  cross128_r0 R10 ratio:  22.512x
  focused medium_r50 R20 ratio: 0.997 median / 1.015 p25
```

Read:

```text
cross128 weakness is the large/direct boundary, not medium remote protocol.
Default HZ8 does not include H8_LARGE_DIRECT_OWNED_L1.
System-large frees can pay medium lookup/global scan in mixed 16..131072 rows.
```

## Current Box

```text
LargeDirectShardedHotCacheShadow-L1

scope:
  keep HZ8 default as KeepRefill balanced default
  keep LargeDirect as opt-in/profile evidence
  do not promote sysmalloc-backed LargeDirect to default
  model a sharded hot-only direct-large cache

target:
  estimate whether hot-only reuse can avoid the global-lock HotCold bottleneck
  measure shard distribution and required resident bytes
  decide if a behavior box is worth implementing

default:
  HZ8-v2 KeepRefill balanced default remains unchanged

opt-in:
  LargeDirectOwned remains profile / paper evidence
  HotColdCache-L1 remains diagnostic evidence only

next lane, if reopened:
  LargeDirectShardedHotCache-L1
  hot-cache sharding first
  cold tier optional and less eager

shadow model:
  8 shards from current owner slot
  8KiB direct-large buckets from 72KiB through 128KiB
  hot-only cache
  total hot cap: 64MiB
  per-shard hot cap: 16MiB

non-goals:
  no default promotion
  no benchmark-row auto detection
  no medium/small behavior changes
  no further tuning of the current global-lock HotColdCache-L1 shape
```

Initial implementation:

```text
target:
  bench-release-largedirectshardedhotshadow
  preload-largedirectshardedhotshadow

macro:
  H8_LARGE_DIRECT_SHARDED_HOT_SHADOW_L1

behavior:
  shadow only
  no allocation policy change
```

Short smoke observation:

```text
command:
  h8_bench_release_largedirectshardedhotshadow
    --runs 1 --threads 16 --iters 20000
    --min-size 16 --max-size 131072
    --remote-pct 90 --interleaved 1

direct_large_sharded_hot:
  hit:             103,658
  store:           104,018
  raw_alloc:       56,195
  reject:          55,835
  hot_peak:        67,283,968
  max_shard_bytes: 16,773,568

read:
  model is wired and producing shard/cap evidence
  current 64MiB total / 16MiB per-shard cap rejects many frees
  next measurement should compare hit/reject/shard skew before behavior work
```

Cap sweep:

```text
record:
  bench_results/20260701T061808Z_sharded_hot_cap_sweep_l1/

row:
  cross128_r90
  R3, T=16, iters=50k, size=16..131072, interleaved=1

64MiB total / 16MiB shard:
  hit_rate:       45.2%
  raw_alloc:      219,033
  reject:         218,350
  hot_peak:       64.0MiB
  max_shard:      16.0MiB

128MiB total / 32MiB shard:
  hit_rate:       66.3%
  raw_alloc:      134,802
  reject:         133,801
  hot_peak:       128.0MiB
  max_shard:      32.0MiB

128MiB total / 64MiB shard:
  hit_rate:       53.8%
  raw_alloc:      184,764
  reject:         183,254
  hot_peak:       128.0MiB
  max_shard:      64.0MiB

192MiB total / 32MiB shard:
  hit_rate:       69.2%
  raw_alloc:      123,174
  reject:         121,874
  hot_peak:       173.6MiB
  max_shard:      32.0MiB
```

Read:

```text
best default-shaped cap:
  128MiB total / 32MiB shard

why:
  materially better hit rate than 64/16
  lower raw_alloc
  128/64 is worse, so per-shard cap still matters
  192/32 improves only modestly while increasing retained-hot contract

behavior decision:
  do not implement yet
  next evidence, if needed, should compare 128/32 behavior against
  sysmalloc-backed LargeDirect and default HZ8 in a paired R10
```

Behavior probe:

```text
target:
  bench-release-largedirectshardedhotcache
  preload-largedirectshardedhotcache

macro:
  H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1

cap:
  128MiB total / 32MiB shard

record:
  bench_results/20260701T063132Z_sharded_hot_cache_l1_light/

cross128_r90 light R3:
  LargeDirect sysmalloc-backed:
    median:   1.688M
    post RSS: 10.39MiB
    peak RSS: 26.79MiB

  ShardedHotCache 128/32:
    median:   1.426M
    post RSS: 27.59MiB
    peak RSS: 55.50MiB
    hit_rate: 100%
    reject/store: 3.7%
    hot_peak: 128MiB
    max_shard: 32MiB
```

Read:

```text
mechanism:
  works
  cache hit rate is excellent

problem:
  even with hits, mmap-backed sharded cache is slower than sysmalloc-backed
  LargeDirect in this light row
  RSS contract is also heavier

disposition:
  keep as opt-in evidence target
  HOLD for default
```

Current read:

```text
throughput:
  LargeDirect default-candidate is strong

regression:
  medium regression from the first probe was direct lookup hot-path tax
  hot-path closure fixes local medium and focused medium_r50 is neutral

remaining decision:
  cross128_r90 RSS rises while throughput improves about 45x
  default promotion is HOLD while HZ8 is positioned as low-RSS balanced
```

RSS policy probes:

```text
record:
  bench_results/20260701T025517Z_large_direct_purgecache_probe/
  bench_results/20260701T025734Z_large_direct_recyclecache_probe/

LargeDirect sysmalloc-backed default candidate:
  cross128_r90 median: 3.286M
  post RSS:            125.87MiB
  peak RSS:            184.02MiB

mmap/munmap payload:
  cross128_r90 median: 150.0k
  post RSS:            10.23MiB
  peak RSS:            39.86MiB
  read:                RSS good, throughput unacceptable

mmap + purge cache:
  cross128_r90 median: 506.2k
  post RSS:            14.84MiB
  peak RSS:            40.32MiB
  read:                RSS good, purge/refault cost too high

mmap + recycle cache:
  cross128_r90 median: 1.459M
  post RSS:            22.16MiB
  peak RSS:            335.23MiB
  read:                throughput better than purge, still far below sysmalloc
                       and peak RSS regresses
```

Decision:

```text
LargeDirectOwned:
  keep as opt-in/profile evidence

LargeDirect default promotion:
  HOLD

RSS policy variants:
  mmap: HOLD
  purge cache: HOLD
  recycle cache: HOLD

paper wording:
  LargeDirect probe closes cross128 throughput but exposes a throughput/RSS
  tradeoff; bounded cache attempts did not yet produce a default-quality
  combined point.
```

Next research gate:

```text
LargeDirectHotColdCacheShadow-L2:
  implemented as build-target shadow

record:
  bench_results/20260701T042501Z_large_direct_hotcold_shadow_l2/

cross128_r90 shadow:
  alloc:       2,000,234
  shadow_hit:  1,999,289
  raw_alloc:   945
  exact_hit:   1,997,990
  near_hit:    1,299
  oversize:    20,182,078 bytes
  hot_peak:    66,758,848 bytes
  cold_peak:   30,696,320 bytes
  demote:      270
  release:     0

cross128_r0 shadow:
  alloc:       2,000,735
  shadow_hit:  2,000,675
  raw_alloc:   60
  hot_peak:    6,172,416 bytes
  cold_peak:   0

read:
  hot/cold cache shape has strong hit-rate evidence
  current measurement is shadow-only; throughput includes shadow lock overhead
  next behavior candidate should be opt-in, not default

LargeDirectHotColdCache-L1:
  implemented as opt-in behavior target
  disposition: HOLD

record:
  bench_results/20260701T043525Z_large_direct_hotcold_cache_l1/

cross128_r90 behavior:
  largedirectdefault median: 3.294M
  hotcoldcache median:       1.354M
  ratio:                     0.411
  post RSS:                  94.02MiB -> 27.88MiB
  peak RSS:                  142.46MiB -> 202.42MiB
  cache hit:                 1,952,706 / 2,000,234
  raw alloc:                 47,528
  demote/release:            47,667 / 46,481

cross128_r0 behavior:
  largedirectdefault median: 4.517M
  hotcoldcache median:       2.372M
  ratio:                     0.525
  RSS:                       near parity

read:
  behavior confirms high reuse, but global lock + demote/purge/release churn
  costs too much
  post RSS improves in r90, but peak RSS and throughput fail default gates
  HotColdCache-L1 is HOLD, not a promotion candidate

counter follow-up:
  bench_results/20260701T044331Z_large_direct_hotcold_counter_l2/

  cross128_r90:
    throughput median: 708.7k
    lock_acquire:      5,874,298
    lock_wait_ns:      55.508s
    lock_hold_ns:      2.436s
    miss_hot_empty:    73,823
    miss_hot_small:    21,961
    miss_cold_empty:   94,643
    miss_cold_small:   66
    demote:            95,256
    release:           93,649
    purge_ns:          0.642s
    release_ns:        0.662s
    raw_alloc_ns:      0.845s

  read:
    added timing counters are diagnostic and make this build slower
    main blocker is global cache lock contention
    secondary blocker is hot/cold churn: many demote+release events
    cold reuse is tiny, so cold tier mostly adds purge/release cost

next design:
  do not tune this global-lock cache directly
  if continuing, redesign around sharded hot cache first
  make cold tier optional or much less eager

LargeDirect default promotion:
  HOLD until a measured Pareto point exists
```

## Hold

```text
LargeDirectHotColdCache-L1:
  HOLD
  evidence target only
  global-lock shape is closed

LargeDirect default promotion:
  HOLD
  no measured low-RSS/high-throughput Pareto point yet

HZ9:
  keep as lab architecture lane
  do not start until HZ8 large/direct probe is resolved

medium remote micro-tuning:
  frozen unless new attribution shows a 5%+ bucket

ChunkArena / SizePolicy:
  separate future lanes
```

## Read First

```text
release / paper:
  README.md
  README.ja.md
  docs/HZ8_PUBLIC_RELEASE_PREP.md

medium default:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md

large/direct:
  docs/HZ8_LARGE_DIRECT_DEFAULT_PROBE_L1.md
  docs/HZ8_LARGE_DIRECT_OWNED_L1.md
  docs/HZ8_LARGEISH_ROUTE_MISS_BOUNDARY.md

benchmark gates:
  docs/HZ8_BENCH_GATE.md
```

## Rules

```text
Keep this file short.
Put long chronology in bench_results/ or docs/archive/.
Do not exceed 800 lines.
Do not promote profile-only lanes without paired throughput, RSS, and safety
guards.
```
