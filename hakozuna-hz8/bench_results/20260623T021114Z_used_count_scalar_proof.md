# HZ8 UsedCountScalarProof-L1

Date: 2026-06-23T02:11:14Z

## Result

`used_count` is not converted to a plain scalar in this box.

The implementation now routes all code through explicit helpers:

```text
h8_used_count_load_owner_relaxed()
h8_used_count_store_owner_relaxed()
h8_used_count_load_cold_acquire()
h8_used_count_init()
```

This makes the current authority split visible without changing storage.

## Access Classes

Owner-hot writers:

```text
local allocation:
  h8_owner_used_add()

same-owner local free:
  h8_owner_used_sub()

owner remote collect:
  h8_used_count_sub()

span initialization:
  h8_used_count_init()
```

Cold / cross-lifecycle readers:

```text
active-span fullness check:
  h8_find_active_span()

owner list scan:
  h8_find_active_span()

orphan adoption dry-run and claim:
  h8_orphan_adoption_dry_run()
  h8_orphan_adopt_span()

owner exit:
  h8_owner_exit()

slot-state quiescent verifier:
  h8_slot_shadow_verify_span_impl()
```

## Constraint

`used_count` remains part of span lifecycle and pressure accounting:

```text
remote pending objects stay counted until owner collect
empty-span retire depends on used_count == 0
orphan adoption skips empty spans
slot shadow checks compare used_count against allocated/live state
```

Those readers are not all on the malloc/free leaf.  Converting the field to a
plain scalar would need a stronger proof that every non-owner read is either
quiescent, locked, or otherwise data-race-free.

## Next Safe Step

The next performance box should not change storage yet.  Candidate:

```text
UsedCountHotMirrorShadow-L1
```

Concept:

```text
release:
  keep atomic used_count authority

debug/evidence:
  add owner-only mirror accounting
  compare mirror and atomic at quiescent points
```

Promotion condition:

```text
mirror mismatch = 0
all cold readers occur under owner lock, publish gate close, or quiescent state
```

Only after that should a scalar authority cutover be considered.

## Short Release Check

Raw logs:

```text
bench_results/20260623T021114Z_used_count_local0_r3.log
bench_results/20260623T021114Z_used_count_interleaved_r90_r3.log
```

Observed:

```text
guard/local0 RUNS=3:
  median ~= 228.4M ops/s
  steady ~= 327.1M ops/s
  quiescent pending clean
  local zero gates clean

small_interleaved_remote90 RUNS=3:
  median ~= 40.9M ops/s
  steady ~= 44.4M ops/s
  work ~= 36.0ms
  tail ~= 18.7ms
  quiescent pending clean
  local zero gates clean
```
