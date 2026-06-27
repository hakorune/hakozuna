# HZ8 v1.1 Release Notes

Status: **current recommended default**.

HZ8 v1.1 is the balanced Hakozuna allocator release line.  It freezes the
small-v0 contract, the MediumRun-v1.1 default, the lazy128 residency policy, and
the pure `LD_PRELOAD` malloc surface.

HZ8 v1.1 is not a throughput-only allocator.  Its release claim is:

```text
fail-closed ownership
bounded retained-empty residency
owner-exit hard drain
low post-workload RSS
remote-free correctness
practical general throughput
```

## User-Facing Recommendation

```text
Recommended allocator:
  HZ8

Internal lineage:
  HZ3 / HZ4 / HZ5 / HZ6 / HZ7

Experimental research:
  HZ9
```

Do not present HZ3..HZ9 as a user-facing selector.  HZ8 is the product line.
HZ3..HZ7 are design references, and HZ9 is an opt-in throughput research lane.

## Release Contract

```text
small-v0:
  frozen

medium range:
  4097..65536 bytes

medium classes:
  8K / 16K / 24K / 32K / 48K / 64K

medium geometry:
  8K  class:  64KiB run / 8 slots
  16K class:  64KiB run / 4 slots
  24K class:  64KiB run / 2 slots
  32K class:  64KiB run / 2 slots
  48K class: 128KiB run / 2 slots
  64K class: 128KiB run / 2 slots

medium identity:
  64KiB quantum directory
  owned pointer VALID / INVALID / MISS separation
  interior owned-looking pointers are INVALID

remote free:
  owner-attached pending queue
  pending bitmap as duplicate-claim authority
  qstate owner collector protocol

residency:
  normal empty-resident budget
  TLS active empty-live retention
  ctx-aware collector active-empty-live retention
  lazy128 persistent owner-attached reservation
  owner exit / detach / destroy hard drain

compatibility:
  malloc / free / calloc / realloc
  pure LD_PRELOAD matrix support
```

## Lazy128 Residency

`lazy128` is part of the default.  It keeps budget-rejected, owner-attached
empty medium runs resident under a 128MiB lazy reservation cap.

The conservative retained-empty overhead contract is:

```text
normal empty-resident budget:
  about 64MiB

lazy reservation:
  128MiB

active-empty LIVE structural bound:
  about 20MiB

conservative retained-empty overhead:
  about 212MiB
```

This is a bounded throughput-stability policy, not a minimum-RSS-at-all-times
policy.  Owner exit, detach, and destroy remain hard drain points.

## Matrix Positioning

Corrected pure `LD_PRELOAD` same-run matrix record:

```text
bench_results/hz8_v11_same_run_matrix_20260626T193636Z/
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
```

Summary:

```text
strengths:
  low post RSS across small / medium / main rows
  fail-closed owned pointer behavior
  medium r50 fault outlier resolved by lazy128
  remote free correctness gates retained
  realloc-compatible preload surface

weaknesses:
  medium_local0 trails tcmalloc/HZ3/system/legacy64k2
  medium_interleaved_r50 trails tcmalloc/HZ3
  main local and remote rows still trail throughput-first allocators
```

The remaining throughput gap is accepted for v1.1.  It feeds the v2 / HZ9
throughput research lane and should not reopen the frozen HZ8-v1.1 remote
protocol or residency policy without a new material evidence bucket.

## Frozen / Held Lanes

Frozen in v1.1:

```text
small-v0 behavior
q64-v12-48k2 medium geometry
lazy128 residency
owner queue / pending bitmap / qstate remote protocol
64KiB quantum directory
pure LD_PRELOAD realloc-compatible surface
```

Held for v1.1:

```text
owner lease redesign
remote queue protocol rewrite
ChunkArena default promotion
broad collect cadence changes
available-index promotion
demand-collect expansion
48K local-free decode default promotion
LocalFastTier behavior promotion
```

Research after v1.1:

```text
HZ8-v2 / HZ9 local throughput architecture
medium local magazine / local-remote split experiments
size-policy and geometry research
paper-quality cross-allocator matrix refresh
```

## Release Checklist

Before tagging a release:

```text
make smoke
make preload
make bench-release
./h8_smoke
git diff --check
```

Recommended release records:

```text
README.md
README.ja.md
docs/HZ8_V1_1_RELEASE.md
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
docs/ALLOCATOR_MATRIX.md
```
