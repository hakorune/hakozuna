# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

## Restart Surface: HZ8 v3 Adaptive Integration L0

Keep HZ8 v2 / KeepRefill frozen as the public default. Begin v3 only as a
diagnostic research track inside the HZ8 family. The first box is
`HZ8AdaptiveTransferShadow-L0`: observe class-local refill, reusable-transfer,
remote-drain, and retained-byte pressure at existing slow-path checkpoints and
recommend BALANCED / TRANSFER_PRESSURE / RSS_PRESSURE without changing allocator
behavior.

Do not copy HZ11/HZ12 cores into HZ8. Import only the proven contracts: bounded
batch transfer from HZ11 and cold-path reclaim authority from HZ12. No hot-path
profile checks, production counters, or default-lane changes are allowed.
Design and gates: `docs/HZ8_V3_ADAPTIVE_INTEGRATION_L0.md`.

L0 result: ACCEPT on Windows. Feature-off smoke remains clean; the diagnostic
smoke observes slow-path recommendations. Same-owner balanced/wide_ws report no
transfer pressure, while remote90 R3 reports 10.4K..11.1K transfer
recommendations and 10.3K..11.0K remote collector checkpoints.

L1 feasibility result: CLOSED / NO-GO. HZ8 already bulk-splices pending bitmap
words into the owner free list. A larger collector budget was previously
throughput-neutral and increased RSS; an ownerless second sink and ownership
transfer violate the frozen HZ8 contract. Keep L0 as diagnostic evidence only.
Do not implement an adaptive transfer behavior or change the HZ8 v2 default.
HZ11 fine128 remains the broad-MT opt-in line.

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
