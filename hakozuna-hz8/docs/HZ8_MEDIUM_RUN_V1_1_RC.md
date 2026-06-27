# HZ8 MediumRun V1.1 RC

Status: **frozen current default / positioning record**.

This record closes the MediumRun-v1.1 default after `lazy128` residency,
`q64-v12-48k2` geometry, pure `LD_PRELOAD` compatibility, and the corrected
same-run allocator matrix.  It does not change the frozen small-v0 behavior.

MediumRun-v1.1 is the balanced HZ8 default: fail-closed pointer ownership,
bounded retained-empty residency, owner-exit hard drain, and preload ABI
compatibility are part of the contract.  It is not a tcmalloc-throughput clone.
Further attempts to close the absolute local/remote throughput gap belong in a
separate v2 throughput lane, not in this RC record.

## Identity

```text
baseline protocol / geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

current default:
  q64-v12-48k2 geometry
  lazy128 residency
  per-run mmap
  64KiB quantum directory
  owner queue remote protocol

current matrix:
  bench_results/hz8_v11_same_run_matrix_20260626T193636Z/

artifact superseded:
  bench_results/hz8_v11_same_run_matrix_20260626T192109Z/
  measured matrix control vectors as HZ8 post RSS
```

## Default Contract

```text
small:
  p2-v0 small-v0 remains frozen

medium classes:
  8K / 16K / 24K / 32K / 48K / 64K

medium geometry:
  8K  class:  64KiB run / 8 slots
  16K class:  64KiB run / 4 slots
  24K class:  64KiB run / 2 slots
  32K class:  64KiB run / 2 slots
  48K class: 128KiB run / 2 slots
  64K class: 128KiB run / 2 slots

remote:
  owner-attached foreign free publishes to owner medium queue
  pending bit is remote claim authority
  owner collector owns slot mutation
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY

residency:
  normal empty-resident budget remains bounded
  lazy128 persistent owner-attached reservation is default
  conservative retained-empty overhead contract is about 212MiB
  owner exit / detach / destroy are hard drain points
```

## Matrix Positioning

Corrected pure `LD_PRELOAD` matrix medians:

```text
guard_local0:
  HZ8      234.26M
  tcmalloc 333.41M
  HZ3      150.39M
  system   151.95M

small_interleaved_remote90:
  HZ8      13.16M
  tcmalloc 26.14M
  mimalloc 15.96M
  HZ3      12.35M
  HZ4      10.99M

medium_local0:
  HZ8      105.28M
  legacy   121.05M
  HZ3      172.14M
  system   158.08M
  tcmalloc 471.15M

medium_interleaved_r50:
  HZ8      9.07M
  legacy   9.24M
  HZ3      15.15M
  HZ4      8.84M
  tcmalloc 17.37M

main_local0:
  HZ8      121.71M
  legacy   127.66M
  HZ3      147.11M
  system   157.32M
  tcmalloc 313.96M

main_interleaved_r50:
  HZ8      9.82M
  legacy   9.48M
  HZ3      15.54M
  HZ4      12.11M
  tcmalloc 22.65M

main_interleaved_r90:
  HZ8      6.38M
  legacy   6.41M
  HZ3      11.85M
  HZ4      9.63M
  mimalloc 8.60M
  tcmalloc 13.24M
```

RSS highlights after harness teardown fix:

```text
HZ8 post RSS:
  guard_local0              about 1.9MiB
  small_interleaved_remote90 about 2.9MiB
  medium_interleaved_r50     about 3.8MiB
  main_interleaved_r90       about 4.4MiB

interpretation:
  RSS is a strength in the current matrix, not the weak lane.
```

## Closed Micro-Tuning Boxes

The following boxes do not justify reopening the remote owner-side protocol or
promoting another behavior change:

See also:

```text
docs/HZ8_NO_GO_LEDGER.md
```

```text
MediumLocalFreeRunCache-L1:
  HOLD
  same-owner directory skip did not improve medium/main rows enough

MediumCollectActiveRefillHint-L1:
  HOLD
  small medium-r50 signal but main_r90 regression

MediumOwnerAvailableRunIndex-L1:
  HOLD
  exact available work-set removed owner-list discovery but did not improve
  medium_r50 enough

MediumAvailableHitCostAttribution / inline owner allocation:
  HOLD
  helper call shape did not explain the medium_r50 residual

MediumActiveMissDemandCollect64K-L1:
  HOLD
  mechanism worked, but release medium_r50 gain was about 1.4%

MediumV12TwoSlotDecodeFastPath-L1:
  NO-GO as a broad 24K/48K specialization
  exact 24K/48K two-slot offset decode removed the target div path, but release
  R10 batches did not show stable local gains and regressed main_r50/main_r90
  later narrow result:
    24K local-free exact-offset decode is retained as default
    48K local-free exact-offset decode is HOLD

MediumLocalContractCostCeiling-L1:
  HOLD as behavior
  unsafe ceiling targets showed medium-only headroom, but no clean cross-row
  bucket:
    medium_local0 combined ceiling about +11.8%
    main_local0 combined ceiling about -4.8%
    freepending ceiling improved medium_r50 about +15.1% but regressed
    main_r50/main_r90
```

## Known Weaknesses

```text
throughput:
  medium_local0 trails tcmalloc/HZ3/system and legacy64k2
  medium_interleaved_r50 trails tcmalloc/HZ3
  main_local0 trails tcmalloc/system and is below HZ3
  main r50/r90 trail tcmalloc/HZ3/HZ4 in the corrected pure preload matrix

accepted interpretation:
  remaining gap is likely dominated by ownership, slot_state, pending, and
  fail-closed contract cost unless a new class-specific material bucket is
  found

phase:
  phase rows remain lifecycle / first-touch / RSS stress, not primary
  throughput ranking rows
```

## Freeze Decision

```text
freeze:
  MediumRun-v1.1 remote/local micro-tuning lane

keep default:
  q64-v12-48k2
  lazy128
  current owner queue / pending / qstate protocol

do not reopen without new evidence:
  owner lease redesign
  qstate / pending protocol
  broad collect cadence
  available-index promotion
  demand collect expansion
  broad v12 24K/48K two-slot decode specialization

future lanes:
  SameRun positioning / public RC record
  HZ8-v2 throughput architecture
  SizePolicy / rounded-byte policy as a separate lane
  ChunkArena / VMA and creation variance only with no-regression evidence
```

## V2 Boundary

The corrected matrix shows real throughput gaps:

```text
medium_local0:
  below tcmalloc, HZ3, system, and legacy64k2

medium_interleaved_r50:
  below tcmalloc and HZ3

small_interleaved_remote90:
  below tcmalloc and mimalloc
```

These are not good candidates for more v1.1 micro-tuning.  The recent
evidence boxes already failed to produce a clean cross-row bucket:

```text
local-free cache:
  flat

owner available index:
  removed discovery cost but did not improve medium_r50

demand collect:
  mechanism worked but release gain was about 1.4%

v12 decode:
  24K local-free retained, 48K and broad specialization held or rejected

active-empty charge elide:
  not stable enough for default

local contract ceilings:
  unsafe evidence only, with main-row regressions
```

The next meaningful throughput work should be architectural:

```text
Local Fast Tier:
  owner-local medium alloc/free path that batches or defers some accounting
  without weakening fail-closed route authority

Remote Transfer Tier:
  medium remote handoff / owner-side reuse policy that reduces episode cost
  without reopening lost-publish or duplicate-collect hazards

Throughput Profile:
  only if it has an explicit RSS and safety contract distinct from v1.1
```

Any v2 lane must keep v1.1 as the comparison baseline and must report when it
trades off:

```text
fail-closed invalid pointer handling
owner generation / lifetime safety
lazy128 retained-empty cap
owner-exit hard drain
pure LD_PRELOAD compatibility
```
