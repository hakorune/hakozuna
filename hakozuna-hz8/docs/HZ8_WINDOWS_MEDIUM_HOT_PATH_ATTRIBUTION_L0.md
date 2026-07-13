# HZ8 Windows Medium Hot-Path Attribution L0

## Purpose

OwnerWitness is closed as a performance track. Its final Windows gate proved
the lifetime shape but did not improve the medium-heavy row. The next step is
therefore attribution, not another cache, ownership shortcut, or policy knob.

This box asks where native Windows exact-medium cost remains:

```text
allocation:
  common entry / class decode
  current-run hit
  refill or new-run path
  source commit

free:
  pointer classification
  owner / generation / state validation
  slot-state and availability publication
  lock or atomic cost
```

## Lane Boundary

```text
name:
  WindowsMediumHotPathAttribution-L0

sizes:
  8192 control
  16384 medium
  32768 medium

allocators:
  HZ8 v2 public default
  HZ10 substrate shadow
  tcmalloc

status:
  diagnostic-only
  excluded from the normal allocator matrix
```

The existing release-equivalent object disassembly is reused. The diagnostic
active-block timer remains fixed8K-only and is not compared directly with
release throughput. Exact 16K/32K rows use process cycles and throughput from
release binaries, without adding counters to allocator hot paths.

## Tool

```powershell
.\hakozuna-hz8\scripts\audit_medium_fixed8k_windows.ps1 `
  -Sizes 8192,16384,32768 `
  -Runs 5
```

Required outputs:

```text
build_manifest.txt
function_stats.csv
call_targets.csv
medium_size_repeat.csv
phase_repeat.csv
phase_hz8_diag.csv
path_audit.txt
HZ8_WINDOWS_MEDIUM_HOT_PATH_ATTRIBUTION.md
```

## Rules

- no production counter, atomic, or timing hook;
- no OwnerWitness behavior change;
- no removal of MISS / VALID / INVALID, generation, pending, or owner-exit
  checks;
- static instruction count is supporting evidence, not a cycle result;
- compare fresh-process runs and record the exact build manifest;
- do not promote a lane from this audit alone.

## Decision

```text
alloc-side dominates:
  open one current-run/refill experiment

free-side classify/authority dominates:
  open an HZ8-native contract adapter over the existing HZ10 page substrate

contract checks dominate:
  publish the fail-closed cost boundary and stop incremental check removal

result is unstable or layout-sensitive:
  add one cycle-level microbench; do not add behavior
```

Acceptance for attribution is a reproducible direction across 16K and 32K,
not a throughput promotion threshold. Any later behavior box must define its
own paired speed, RSS, safety, and cross-platform gate.

## Native Windows Result

Release repeat-5, four threads:

| Size | Allocator | Median ops/s | Process cycles/op |
|---:|---|---:|---:|
| 8192 | HZ8 v2 | 63.190M | 258.34 |
| 8192 | HZ10 substrate shadow | 179.180M | 94.64 |
| 8192 | tcmalloc | 204.940M | 79.85 |
| 16384 | HZ8 v2 | 20.760M | 801.63 |
| 16384 | HZ10 substrate shadow | 45.330M | 374.24 |
| 16384 | tcmalloc | 30.200M | 561.50 |
| 32768 | HZ8 v2 | 5.520M | 2947.60 |
| 32768 | HZ10 substrate shadow | 13.770M | 1211.21 |
| 32768 | tcmalloc | 15.100M | 1120.08 |

Single-thread batched phase repeat-5:

| Size | Allocator | Alloc ns/op | Free ns/op |
|---:|---|---:|---:|
| 8192 | HZ8 v2 | 26.00 | 11.07 |
| 8192 | HZ10 substrate shadow | 7.72 | 2.94 |
| 8192 | tcmalloc | 4.91 | 2.48 |
| 16384 | HZ8 v2 | 94.86 | 11.10 |
| 16384 | HZ10 substrate shadow | 23.60 | 3.00 |
| 16384 | tcmalloc | 5.48 | 2.46 |
| 32768 | HZ8 v2 | 356.44 | 12.90 |
| 32768 | HZ10 substrate shadow | 83.87 | 2.86 |
| 32768 | tcmalloc | 6.00 | 2.91 |

The free phase is nearly size-independent. HZ8 allocation grows sharply with
the number of runs required by a 256-object live batch. A separate
counter-bearing sibling showed no budget rejection and only initial
decommitted activation. Resident reuse succeeds.

| Size | Runs created | Owner scans | Owner steps | Average steps/scan |
|---:|---:|---:|---:|---:|
| 8192 | 32 | 33,729 | 540,222 | 16.0 |
| 16384 | 64 | 68,545 | 2,194,014 | 32.0 |
| 32768 | 128 | 138,177 | 8,843,934 | 64.0 |

The average owner-list depth doubles with each size step. Global scans occur
only during initial run creation. This identifies alloc-side run discovery and
the common medium substrate as the primary boundary; decommit/recommit churn is
not the explanation.

## Final Decision

```text
WindowsMediumHotPathAttribution-L0:
  COMPLETE

another cache/tier:
  NO-GO

MediumAvailableIndex revival:
  NO-GO
  the existing experiment removed discovery but did not improve medium_r50

free/classifier-only trim:
  not the next box

next:
  GeneralMediumPageSubstrateExpansion-L0
  extend the already proven HZ8-native R3 substrate contract to exact 16K/32K
  geometry in a diagnostic shadow before any behavior change
```
