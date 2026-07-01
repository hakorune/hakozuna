# HZ8 LargeDirect Default Probe L1

## Status

```text
box:
  HZ8LargeDirectDefaultProbe-L1

state:
  build-target probe
  not default behavior yet
```

## Why

`cross128_r90` mixes small, medium, and 65,537..131,072 byte allocations.
The current HZ8 default owns small/medium but sends the large side to system
allocation.

Focused observation shows the weakness is at this large/direct boundary.

```text
cross128_r90 short R3:
  HZ8 default:
    ~70k ops/s

  HZ8 default + H8_LARGE_DIRECT_OWNED_L1:
    ~2.811M ops/s

cross128_r0 short R3:
  HZ8 default:
    ~263k ops/s

  HZ8 default + H8_LARGE_DIRECT_OWNED_L1:
    ~6.797M ops/s
```

Read this as evidence for a default candidate, not as a promotion decision.

## Scope

```text
added targets:
  preload-largedirectdefault
  bench-release-largedirectdefault

flags:
  HZ8_DEFAULT_CFLAGS
  + H8_LARGE_DIRECT_OWNED_L1

unchanged:
  small-v0
  medium q64-v12-48k2
  KeepRefill
  lazy128
  owner queue / pending / qstate authority
```

No allocator source behavior changes are made by this box. It only provides
official comparison artifacts for the existing LargeDirectOwned lane.

## Hot-Path Closure

The first probe exposed a medium hot-path tax: enabling LargeDirect made
`h8_free_inner()` call the direct-large exact lookup before the medium lookup,
even for medium-only rows.

The closure keeps the default build unchanged and limits direct lookup to
LargeDirect builds:

```text
default HZ8:
  no direct-large range guard in free/route/realloc hot paths

LargeDirect build:
  inline min/max range guard
  call direct-large lookup only when the pointer is inside the known
  direct-large address envelope

direct-large implementation:
  publishes min/max range in H8Global
  locked hash remains final authority
```

This preserves the cross128 fast route while avoiding the earlier medium-local
regression.

## Observed Gate

Primary R10 after hot-path closure:

```text
record:
  bench_results/20260630T230450Z_hz8_largedirect_probe_gate/

cross128_r90:
  baseline median:  62.940k ops/s
  candidate median: 2.835M ops/s
  ratio:            45.048x
  peak RSS:         150.17 MiB -> 260.07 MiB
  post RSS:         107.04 MiB -> 190.61 MiB

cross128_r0:
  baseline median:  259.749k ops/s
  candidate median: 5.848M ops/s
  ratio:            22.512x
  peak RSS:         5.93 MiB -> 6.86 MiB
  post RSS:         5.40 MiB -> 5.99 MiB
```

Regression checks:

```text
full R5:
  bench_results/20260630T230137Z_hz8_largedirect_probe_gate/

small_interleaved_remote90:
  median ratio: 1.013

main_interleaved_r90:
  median ratio: 1.031

medium_interleaved_r50:
  median ratio: 0.977
  noisy in the full R5 batch

medium_interleaved_r50 focused R20 fresh-process:
  bench_results/20260630T230432Z_hz8_largedirect_probe_gate/
  median ratio: 0.997
  p25 ratio:    1.015

medium_local0 focused R10:
  bench_results/20260630T230122Z_hz8_largedirect_probe_gate/
  median ratio: 1.019
  p25 ratio:    1.024
```

Smoke:

```text
h8_smoke:
  pass

h8_smoke_largedirect:
  pass
```

## Gates

Primary:

```text
cross128_r90:
  material improvement

cross128_r0:
  material improvement
```

Regression:

```text
small_interleaved_remote90:
  regression <= 2%

main_interleaved_r90:
  regression <= 2%

medium_interleaved_r50:
  regression <= 2%

guard_local0 / main_local0 / medium_local0:
  regression <= 2%
```

Safety / RSS:

```text
owned INVALID remains fail-closed
owner exit drains direct-owned allocations
peak RSS remains acceptable for large/direct rows
post RSS returns near baseline after worker exit
```

## Decision

```text
GO:
  cross128 fixes hold under RUNS=10
  regression rows clean
  RSS and safety gates clean

HOLD:
  cross128 improves but small/main/medium regress
  or cross128 RSS tradeoff is not acceptable for the public default

NO-GO:
  cross128 improvement is unstable
  direct-owned lifecycle or RSS gates fail
```

Current read:

```text
throughput:
  GO

small/main/medium regressions:
  GO after focused medium R20

RSS:
  needs explicit product decision
  cross128_r90 gains about 45x throughput but raises peak/post RSS

default promotion:
  reasonable if cross128 is part of the public MT matrix contract
  otherwise keep as opt-in LargeDirect profile
```

## RSS Policy Shadow

`LargeDirectRSSPolicyShadow-L1` adds counters without changing allocation
behavior.

```text
direct_large_shadow:
  alloc/free count
  alloc/free payload bytes
  live payload bytes
  peak live payload bytes
  cache hits / stores / retained bytes
  size buckets:
    0: <=80KiB
    1: <=96KiB
    2: <=112KiB
    3: <=128KiB
  free-to-next-alloc reuse distance:
    0..1
    2..7
    8..31
    32+
```

Initial linked-bench smoke:

```text
row:
  cross128_r90
  runs=2, threads=4, iters=5000

direct_large_shadow:
  alloc=19975
  free=19975
  alloc_bytes=1959095411
  free_bytes=1959095411
  live_bytes=0
  live_peak=9810770
  alloc_bucket=[5094,4967,5029,4885]
  free_bucket=[5094,4967,5029,4885]
  reuse_distance=[1968,8439,8716,842]
```

## RSS Policy Probes

After the throughput probe, three opt-in payload policies were measured:

```text
profiles:
  largedirectdefault:
    system malloc/free payload

  largedirectmmap:
    mmap/munmap payload

  largedirectpurgecache:
    mmap payload + bounded dead mapping cache + payload purge on free

  largedirectrecyclecache:
    mmap payload + bounded dead mapping cache without purge
```

R5 linked-bench results:

```text
record:
  bench_results/20260701T025517Z_large_direct_purgecache_probe/
  bench_results/20260701T025734Z_large_direct_recyclecache_probe/

cross128_r90:
  largedirectdefault:
    median:   3.286M ops/s
    post RSS: 125.87MiB
    peak RSS: 184.02MiB

  largedirectmmap:
    median:   150.0k ops/s
    post RSS: 10.23MiB
    peak RSS: 39.86MiB

  largedirectpurgecache:
    median:   506.2k ops/s
    post RSS: 14.84MiB
    peak RSS: 40.32MiB
    cache:    hit=1,999,842 store=2,000,234 bytes=40,554,905

  largedirectrecyclecache:
    median:   1.459M ops/s
    post RSS: 22.16MiB
    peak RSS: 335.23MiB
    cache:    hit=1,958,024 store=1,958,710 bytes=67,105,770

cross128_r0:
  largedirectdefault:
    median:   5.012M ops/s
    post RSS: 7.78MiB
    peak RSS: 8.03MiB

  largedirectmmap:
    median:   174.7k ops/s
    post RSS: 6.27MiB
    peak RSS: 7.54MiB

  largedirectpurgecache:
    median:   504.2k ops/s
    post RSS: 7.25MiB
    peak RSS: 8.76MiB

  largedirectrecyclecache:
    median:   2.794M ops/s
    post RSS: 7.73MiB
    peak RSS: 8.16MiB
```

Read:

```text
mmap/munmap:
  RSS improvement is real, but per-operation mapping cost destroys throughput.

purge cache:
  keeps RSS low, but purge/refault cost remains too high.

recycle cache:
  recovers part of the throughput, but still loses heavily to sysmalloc-backed
  LargeDirect and worsens remote-heavy peak RSS in this R5.
```

Current decision:

```text
LargeDirectOwned:
  profile evidence / paper probe

default promotion:
  HOLD

cache policies:
  HOLD
```

Next direction:

```text
LargeDirectHotColdCacheShadow-L2:
  GO as research

LargeDirect sysmalloc-backed default:
  HOLD

benchmark-row adaptive policy:
  HOLD
```

## Hot/Cold Cache Research Lane

The current four-policy probe shows a clean Pareto split:

```text
sysmalloc-backed LargeDirect:
  high throughput, higher RSS

mmap/munmap:
  low RSS, unacceptable throughput

purge-on-free cache:
  low RSS, still too much purge/refault cost

recycle-only cache:
  partial throughput recovery, but remote-heavy peak RSS regresses
```

The next research lane is not a default change. It is a shadow/evidence lane
for a two-tier direct-large cache:

```text
hot resident cache:
  exact or near-exact page-rounded size classes
  small per-class cap
  total hot cap
  no purge on immediate free

cold lazy cache:
  receives hot evictions
  may use MADV_FREE / MEM_RESET-style lazy purge
  hard cap

release:
  cold overflow is unmapped / released
```

Required counters:

```text
direct_large_cache_hit_bucket[class]
direct_large_cache_store_bucket[class]
direct_large_cache_reject_cap_bucket[class]

direct_large_cache_exact_hit
direct_large_cache_near_hit
direct_large_cache_oversize_bytes
direct_large_cache_scan_steps

direct_large_hot_bytes
direct_large_hot_peak_bytes
direct_large_cold_bytes
direct_large_cold_peak_bytes

direct_large_demote_hot_to_cold
direct_large_lazy_purge_count
direct_large_lazy_purge_bytes

direct_large_release_from_cold
direct_large_release_bytes

direct_large_alloc_raw_count
direct_large_alloc_cache_hit_count
direct_large_free_to_hot
direct_large_free_to_release
```

Observed shadow:

```text
record:
  bench_results/20260701T042501Z_large_direct_hotcold_shadow_l2/

model:
  8KiB-granularity classes from 72KiB through 128KiB
  hot total cap:       64MiB
  hot per-class cap:   8MiB
  cold total cap:      64MiB

cross128_r90:
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

cross128_r0:
  alloc:       2,000,735
  shadow_hit:  2,000,675
  raw_alloc:   60
  exact_hit:   2,000,484
  near_hit:    191
  hot_peak:    6,172,416 bytes
  cold_peak:   0
```

Read:

```text
hot/cold shape:
  strong hit-rate evidence

oversize reuse:
  low enough to remain plausible with 8KiB classes

cold tier:
  material in remote-heavy row, but fits inside the modeled cap

behavior:
  not implemented yet
  shadow target adds lock/counter overhead, so throughput is not a promotion
  metric
```

## Hot/Cold Cache Behavior Probe

The shadow model was implemented as an opt-in behavior target:

```text
target:
  bench-release-largedirecthotcoldcache

backend:
  mmap payload
  hot resident cache
  cold purged mapping cache
  cold hard release on cap overflow
```

R5 result:

```text
record:
  bench_results/20260701T043525Z_large_direct_hotcold_cache_l1/

cross128_r90:
  largedirectdefault:
    median:   3.294M ops/s
    post RSS: 94.02MiB
    peak RSS: 142.46MiB

  hotcoldcache:
    median:   1.354M ops/s
    post RSS: 27.88MiB
    peak RSS: 202.42MiB
    cache:    hit=1,952,706 store=2,000,234 bytes=133,795,840
    churn:    demote=47,667 release=46,481 raw_alloc=47,528

cross128_r0:
  largedirectdefault:
    median:   4.517M ops/s
    post RSS: 7.80MiB
    peak RSS: 8.08MiB

  hotcoldcache:
    median:   2.372M ops/s
    post RSS: 7.82MiB
    peak RSS: 8.00MiB
```

Decision:

```text
HotColdCache-L1:
  HOLD

why:
  high hit rate is real, but global lock + demote/purge/release churn costs too
  much
  r90 post RSS improves, but peak RSS regresses and throughput is far below
  sysmalloc-backed LargeDirect

next:
  do not promote this behavior
  if revisited, reduce churn before changing default:
    lower release frequency
    shard cache lock
    avoid purge on hot/cold oscillation
```

## Hot/Cold Counter Follow-up

Additional diagnostic counters were added after the first behavior result:

```text
record:
  bench_results/20260701T044331Z_large_direct_hotcold_counter_l2/

cross128_r90:
  throughput median: 708.7k ops/s
  lock_acquire:      5,874,298
  lock_wait_ns:      55.508s
  lock_hold_ns:      2.436s
  hot misses:
    empty:           73,823
    too_small:       21,961
  cold misses:
    empty:           94,643
    too_small:       66
  demote:            95,256
  release:           93,649
  purge_ns:          0.642s
  release_ns:        0.662s
  raw_alloc_ns:      0.845s

cross128_r0:
  lock_acquire:      5,876,056
  lock_wait_ns:      36.560s
  lock_hold_ns:      0.721s
  demote/release:    0 / 0
```

Read:

```text
primary blocker:
  global direct-large cache lock contention

secondary blocker:
  hot/cold churn in remote-heavy row

cold tier:
  almost no useful cold hits in the measured row
  most cold activity becomes purge/release overhead

timing counters:
  diagnostic only; they make the measured build slower
```

Next design implication:

```text
do not continue tuning the current global-lock HotColdCache-L1 shape

if the lane is reopened:
  first split or shard the hot cache
  avoid eager hot->cold purge/release oscillation
  consider hot-only bounded cache before restoring a cold tier
```

Promotion gate for a future behavior candidate:

```text
cross128_r90:
  throughput >= 60..70% of sysmalloc-backed LargeDirect
  or at least 10x over default HZ8 baseline

RSS:
  peak/post RSS must stay within an explicit low-RSS product contract

regression:
  small_interleaved_remote90 <= 2%
  main_interleaved_r90 <= 2%
  medium_interleaved_r50 <= 2%
  guard/main/medium local <= 2%

safety:
  owned INVALID fail-closed
  interior/stale/double free rejected
  exact pointer free only
```

Paper framing:

```text
HZ8 default:
  balanced low-RSS allocator

LargeDirectOwned:
  shows cross128 is solvable at the large/direct boundary

default promotion:
  remains a product RSS/throughput tradeoff, not a correctness blocker
```

## Current Disposition

```text
HZ8 default:
  unchanged
  KeepRefill balanced default remains the public Linux default

LargeDirectOwned:
  keep as opt-in / profile / paper evidence
  do not promote to default without a measured RSS/throughput Pareto point

HotColdCache-L1:
  HOLD
  high cache hit rate is real
  global-lock contention and hot/cold churn make this shape non-promotable

current global-lock HotCold cache:
  closed as a default lane
  keep build targets for diagnostics and regression evidence only

next possible research:
  LargeDirectShardedHotCacheShadow-L1
  start with a sharded or thread-local hot resident cache
  add a cold tier only after hot-cache lock contention is solved
```

Do not tune the current global-lock HotColdCache-L1 shape further. The counter
follow-up shows that the primary blocker is not hit rate; it is serialized cache
traffic. A future candidate should first remove that serialization, then measure
whether a cold tier is still useful.

## Sharded Hot Cache Follow-up

The sharded hot-cache lane was added as an opt-in profile experiment after the
global-lock HotCold cache was closed.

```text
shadow target:
  bench-release-largedirectshardedhotshadow

cap sweep targets:
  bench-release-largedirectshardedhot128_32
  bench-release-largedirectshardedhot128_64
  bench-release-largedirectshardedhot192_32

behavior target:
  bench-release-largedirectshardedhotcache
```

Cap sweep read:

```text
best profile-shaped cap:
  128MiB total / 32MiB per shard

why:
  materially better hit rate than 64/16
  128/64 is worse, so per-shard balance matters
  192/32 adds retained-hot budget for only modest hit-rate improvement
```

Behavior light read:

```text
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
```

Decision:

```text
ShardedHotCache-L1:
  HOLD for default
  keep as opt-in evidence target

why:
  mechanism works and cache hit rate is high
  but mmap-backed sharded cache is slower than sysmalloc-backed LargeDirect
  and carries a heavier retained-hot RSS contract
```
