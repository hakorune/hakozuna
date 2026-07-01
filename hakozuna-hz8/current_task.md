# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

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
LargeDirectRSSPolicyShadow-L1

scope:
  keep HZ8 default as KeepRefill balanced default
  keep LargeDirect as opt-in/profile evidence
  add direct-large RSS/reuse policy counters
  do not add a retained large cache yet

target:
  understand whether cross128 throughput can be kept while lowering RSS

counters:
  direct-large alloc/free count
  payload bytes allocated/freed
  live payload bytes and peak
  size bucket alloc/free counts
  free-to-next-alloc reuse-distance buckets

non-goals:
  no default promotion
  no large cache behavior yet
  no medium/small behavior changes
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

## Hold

```text
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
