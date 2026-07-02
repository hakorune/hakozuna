# HZ9 Substrate Cost Matrix L0

## Purpose

```text
HZ9SubstrateCostMatrix-L0

goal:
  compare the current HZ9 substrate families under one candidate gate
  separate profile wins from broad HZ9 default candidates
```

This box is evidence-only. It does not add allocator behavior.

## Compared Variants

```text
baseline:
  HZ8-derived HZ9 default baseline

slabdirectuse:
  Direct SlabPage proof
  strong remote/profile signal, local body remains suspect

localarena_dense_ownerfast_phase8:
  LocalArena owner-fast with local phase admission
  tests whether page creation gating rescues LocalArena

ownerpage_purelocal:
  OwnerPage pure-local API path
  tests HZ9-owned page route/API cost against baseline
```

## Command

```bash
RUNS=1 hakozuna-hz9/scripts/run_hz9_substrate_cost_matrix.sh
```

For stronger evidence:

```bash
RUNS=3 hakozuna-hz9/scripts/run_hz9_substrate_cost_matrix.sh
```

## Rows

```text
medium_local0:
  medium local substrate cost

main_local0:
  broad local no-regression

medium_r50:
  medium remote-heavy value

main_r90:
  main remote-heavy value

guard_local0 / small_interleaved_remote90:
  small-v0 non-target safety rows
```

## Decision Rule

```text
behavior candidate:
  medium_local0 >= 1.03
  main_local0 >= 1.00
  medium_r50 >= 1.05
  main_r90 >= 0.98
  small rows >= 0.98

profile evidence:
  wins remote-heavy rows but misses local/small gates

NO-GO:
  loses both local and remote medium/main rows
```

## Read

```text
The next HZ9 implementation should not be another threshold-only admission
tune. It needs a substrate shape that wins local rows or a deliberate profile
mode that does not pretend to be the broad default.
```

## Initial Result

```text
log:
  bench_results/20260702T121943Z_hz9_substrate_cost_matrix

R1 ratios:
  slabdirectuse:
    medium_local0 0.841
    main_local0   0.954
    medium_r50    1.589
    main_r90      1.573
    small_r90     0.908

  localarena_dense_ownerfast_phase8:
    medium_local0 0.616
    main_local0   0.690
    medium_r50    0.844
    main_r90      0.661
    small_r90     0.996

  ownerpage_purelocal:
    medium_local0 0.920
    main_local0   0.973
    medium_r50    0.916
    main_r90      0.967
    small_r90     0.768
```

Focused repeat:

```text
log:
  bench_results/20260702T122124Z_ownerpage_focus_r3

ownerpage_purelocal R3:
  medium_local0 0.823
  main_local0   0.968
  medium_r50    1.171
  main_r90      0.884
  small_r90     0.899
```

OwnerPage code-shape read:

```text
log:
  bench_results/20260702T122205Z_hz9_code_shape_audit

public shape:
  h8_malloc_inner            unchanged
  h8_malloc_non_small_inner  unchanged
  h8_free_inner              unchanged
  h8_free_non_arena_inner    unchanged

read:
  OwnerPage small-row regression is not explained by public free/non-arena
  route growth. Treat it as purelocal build/body/cache-shape evidence until a
  stronger attribution points elsewhere.
```

## Initial Decision

```text
SlabDirectUse:
  profile/remote evidence only
  remote rows are strong, but local/small gates fail

LocalArena phase8:
  NO-GO
  threshold admission does not recover LocalArena broad cost

OwnerPage purelocal:
  closest local substrate shape so far
  still not default due to medium_local0, main_r90, and small_remote movement
  next OwnerPage work must explain purelocal body/text cost before page-body
  tuning
```
