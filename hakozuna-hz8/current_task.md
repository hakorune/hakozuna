# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

Stable documentation starts at `docs/README.md`. Windows benchmark lane
status is centralized in `docs/HZ8_WINDOWS_LANE_STATUS_L1.md`; do not infer
promotion status from build target names alone.

## Restart Surface: HZ8 Unified Medium Domain L0

## Next Development Order

HZ8 is the active public integration line. TargetDispatch is Windows opt-in GO
and Ubuntu correctness/research GO. Native Ubuntu repeats show stable Page8K
benefit but fail the cross-platform default gate: fixed8K reaches +24% to
+27%, while larger_sizes remains -6% to -8%.

```text
Step 1: freeze the cause
  Ubuntu larger_sizes has 49 Page8K-owned frees
  the same row has 309,487 Page8K classifier misses
  generic medium then performs its own directory lookup
  repeated Page8K MISS before medium authority is the active pressure

Step 2: implement UnifiedMediumDomain-L0
  sparse 64KiB quantum directory
  tagged NONE / MEDIUM_RUN / PAGE8K entries
  dual-publish beside current medium and Page8K registries
  compare one shadow lookup with the current Page8K-first authority
  diagnostic-only counters; no behavior or speed-lane atomics

  status 2026-07-12:
    implementation and dedicated Windows/Linux lanes are present
    Windows fixed8K and 16K..64K focused probes have zero mismatch/conflict
    WSL GCC fixed8K has 20,000/20,000 kind matches

Step 3: L0 acceptance
  kind mismatch = 0
  registration conflict = 0
  larger_sizes medium hits explain the prior Page8K MISS population
  Windows/Linux API and safety smoke pass

  current:
    Windows build/focused run PASS
    WSL GCC -Werror build/focused run PASS
    dedicated WSL smoke/safety stress PASS
    owner exit 8 / handoff 68 / remote free 8,192 remain safe
    Clang blocked by two pre-existing medium unused-variable warnings

Step 4: only then consider L1
  opt-in free dispatch through the unified directory
  backend remains VALID/INVALID authority
  old registries remain diagnostic oracles
  public HZ8 v2 default remains unchanged
```

Small-span inventory is handled by default Mag16. SmallAvailableIndex class
expansion is closed because the Windows gain did not transfer cleanly to Linux.
The page8K substrate is a separate Windows-focused speed/control lane, not a
replacement for the public HZ8 balanced contract.

The current public matrix fixes the remaining measured speed gap. HZ8 reaches
about 24-26% of tcmalloc on main/medium local rows while retaining much lower
post RSS. The fixed-8K audit showed the page substrate is the high-ROI speed
direction; its R3 API surface is now complete on Linux and must be judged as
an opt-in profile, not silently folded into the default.

The earlier `hz8-v2-mediumlocalfast` recheck remains NO-GO. Keep it as
reproducibility evidence only and do not add another active-run branch, mask,
or per-run fast state. The page8K substrate is the approved substrate change;
any broader medium expansion needs a new application-like gate.

The fixed-8K cost audit remains historical evidence rather than the next
behavior box. Its HZ10 substrate comparison explains why the page8K lane was
opened, but it does not authorize copying the HZ10 public entry wholesale.
The narrow 4097..8192 eligibility/served probe has now been measured and
closed as a speed-candidate NO-GO. Do not reopen it through cap or geometry
ladder tuning; keep the result as reproducible evidence only.

L0 measured HZ8 at 65.447M / 258.09 process cycles per logical fixed-8K
operation versus tcmalloc at 213.284M / 76.60 cycles. The audited HZ8 medium
alloc/free objects contain no locked RMW, while the static alloc/free bodies
remain large. This rules out atomic contention as the primary explanation.
The L1 active-block audit is now complete on Linux/WSL and native Windows:
250,000 same-owner active hits, 250,000 owner-matched frees, and zero active
misses on both diagnostic runs. Its stage totals are attribution aids only;
they do not authorize removing the mark-live, slot-state, mask, or generation
checks from the production path. Worker assembly review confirmed that the
largest measured `mark-live` stage is dominated by diagnostic residency,
retention, lock, and timer hooks; it is not a release hot-path cycle result.
No contract-free fixed-cost block was identified in the release active path.

The HZ10 substrate sibling is now connected only inside the audit tool. On the
same Windows fixed-8K repeat-3 it measured 184.485M / 94.74 cycles, versus HZ8
66.105M / 254.50 cycles and tcmalloc 216.546M / 75.10 cycles. This is a 2.79x
HZ8 improvement and about 85% of tcmalloc throughput. The substrate gate is
GO; incremental HZ8 medium-entry trimming is no longer the primary track.

The HZ8/HZ10 contract delta is fixed in
`docs/HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`. Preserve
MISS/VALID/INVALID, fail-closed stale/duplicate handling, owner generation,
bounded remote pending, owner-exit recovery, and low post-RSS. Do not copy the
HZ10 public entry wholesale or expose the shadow in the normal matrix.

Next decision: keep common-entry trimming closed unless a future assembly
review identifies a contract-free block with a reproducible release gain.
Treat the HZ10 substrate as an opt-in contract-import research lane, not as a
default merge. Do not add another cache, counter, atomic, or geometry ladder
for the fixed-8K row. Use the feature-off release assembly as the baseline for
any future audit; do not optimize from diagnostic timing totals.

The native boundary cleanup is now implemented: the HZ8 R3 page backend is the
only behavior path, while the old direct HZ10 adapter is named
`H8_MEDIUM_PAGE8K_HZ10_SHADOW_L1` and is evidence-only. The two flags cannot be
combined. Any future speed work must improve the HZ8-native backend behind the
existing route, ownership, remote, and RSS contracts.

The contract delta is now fixed in
`docs/HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`. Active implementation
order is complete through the opt-in fixed-8K behavior sibling. P0/P1/R1/R2
and R3 results are recorded in the linked contract and adapter documents.

Historical P0 classification shadow passed the Windows fixed 8K/16K valid rows
with 203,840/203,840 hits, zero miss, zero run mismatch, and zero exact-invalid.
The interior/duplicate smoke recorded two exact addresses and one interior
invalid while keeping run mismatch zero. Default builds preprocess all hooks
away. P0 is closed as GO evidence.

P1 detached state shadow is GO. Fixed 8K local reports 203,840 state
matches and zero mismatch. The interior/duplicate smoke reports two state
matches, one exact-invalid interior pointer, and zero mismatch. Fixed 8K MT
remote90 at T=8 reports 800,800 state matches, zero state/run mismatch,
effective remote 90.01%, and zero fallback/failure. This authorized the
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

MagazineTailReclaim is CLOSED at 1.0-1.2MiB maximum hypothetical tail bytes.
Active L0 sweeps hidden reuse at Windows fixed 1KiB/2KiB; 4KiB stays opt-in.
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

ReclaimAdapter L0 remains an accepted retirement witness; its live-owner
behavior variants are closed NO-GO. SpeedAdapter attribution led to the Mag16
owner-local reusable-span inventory. Keep both histories in
`docs/HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md` rather than this restart file.

`HZ8ReusableSpanMagazine-L1` Mag16 is now part of the HZ8 v2 default. It preserves
up to 16 displaced owner-local active hints per class and validates each hint
before reuse. Windows R5 improved 16..256 by 3.15x, 16..2048 by 2.56x, and
16..4096 by 4.11x. The 16..4096 peak fell about 58%, and remote90 median stayed
near-neutral while committed small-span bytes fell about 43%. Windows smokes
pass. Status: PROMOTED after Linux and Windows gates.
Design and task order:
`docs/HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md`.

Mag16 correctness and local gates pass on Windows and Linux; remote evidence
is near-neutral but uses differing effective remote ratios. Mag16 is default,
and `hz8-v2-nomag` remains the explicit research control. Detailed numbers are
in `docs/HZ8_REUSABLE_SPAN_MAGAZINE_L1.md`.

Runner policy: normal comparisons expose `hz8-v2` only. All control,
diagnostic, Mag32, Page8K, adaptive, and reclaim siblings require
`-IncludeHz8Research`; the Windows lane map is the status authority.

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
