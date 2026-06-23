# HZ8 UsedCountColdDerivationShadow-L1

Date: 2026-06-23T03:01:34Z

## Result

Behavior authority is unchanged: `used_count` remains the runtime lifecycle
count.  This box adds shadow derivation from `slot_state` at cold/quiescent
points and compares it with the current atomic `used_count` authority.

## Implementation Shape

```text
h8_slot_allocated_count_quiescent(span):
  scans slot_state
  counts ALLOCATED slots

owner_exit:
  behavior still uses used_count
  debug shadow compares derived count with used_count

adoption:
  pre-quiescent scan no longer reads used_count
  after candidate quiescence, debug shadow compares derived count with used_count

slot-state verifier:
  compares existing allocated scan, derived scan, and used_count
```

The derivation scan is debug-only at owner-exit/adoption call sites, so release
hot/lifecycle paths are not charged for this proof instrumentation.

## Raw Logs

```text
bench_results/20260623T030134Z_used_count_derivation_debug_local0.log
bench_results/20260623T030134Z_used_count_derivation_debug_interleaved_r90.log
bench_results/20260623T030134Z_used_count_derivation_release_local0_r3.log
bench_results/20260623T030134Z_used_count_derivation_release_interleaved_r90_r3.log
```

## Debug Proof

```text
debug local0:
  derived_mismatch = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

debug interleaved remote90:
  derived_mismatch = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean
```

## Release Smoke

```text
local0 RUNS=3:
  median ~= 194.7M ops/s
  steady ~= 276.2M ops/s
  zero gates clean

interleaved remote90 RUNS=3:
  median ~= 52.9M ops/s
  steady ~= 55.0M ops/s
  zero gates clean
```

## Interpretation

Cold derivation from `slot_state` matches the current count authority in covered
local/interleaved and owner-exit/quiescent proof paths.

Next candidate:

```text
UsedCountReleaseElision-L1
```

Planned shape:

```text
release:
  stop maintaining used_count on alloc/free/collect
  derive lifecycle counts from slot_state only at cold/quiescent points

debug:
  keep atomic used_count shadow
  compare shadow against slot_state-derived count
```
