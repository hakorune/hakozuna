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
  bench_results/20260702T122433Z_hz9_owner_page_perf_gate

ownerpage_purelocal R3:
  medium_local0 0.823
  main_local0   0.968
  medium_r50    1.171
  main_r90      0.884
  small_r90     0.899

ownerpage perf gate R3:
  medium_local0 0.911
  main_local0   0.819
  medium_r50    0.984
  main_r90      0.881
  small_r90     1.053
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

OwnerPage debug read:

```text
medium_local0:
  alloc_call=960000
  alloc_hit=960000
  free_call=960000
  route_hit=960000
  mode_block=0

main_local0:
  alloc_call=840948
  alloc_hit=840948
  free_call=840948
  route_hit=840948
  mode_block=0

small_interleaved_remote90:
  alloc_call=0
  free_call=0
  route_attempt=0

read:
  Local rows are not losing to fallback or remote mode. They are losing even
  with 100% OwnerPage local hits. Small rows do not touch OwnerPage API in the
  debug sample. The next target is the OwnerPage local hit body: atomic
  local_free_bits mutation, route_thread scan/decode, or page allocation shape.
```

## OwnerPage Body Attribution

```text
box:
  HZ9OwnerPageOwnerFastBits-L1

scope:
  opt-in proof only
  pure local owner pages replace local_free_bits CAS/fetch_or with
  atomic load/store
  REMOTE_SEEN / DETACHED paths keep the existing atomic fetch_or fallback
  HZ8/HZ9 default behavior unchanged

log:
  bench_results/20260702T122959Z_hz9_candidate_gate

R3 ratios:
  fixed64_local0:
    purelocal 0.548
    ownerfast_bits 0.963
  medium_local0:
    purelocal 0.948
    ownerfast_bits 0.996
  main_local0:
    purelocal 0.946
    ownerfast_bits 0.967
  medium_r50:
    purelocal 0.984
    ownerfast_bits 0.860
  main_r90:
    purelocal 0.939
    ownerfast_bits 0.932
  small_interleaved_remote90:
    purelocal 1.027
    ownerfast_bits 1.022

read:
  local_free_bits atomic RMW is a real local body cost. Removing it nearly
  recovers fixed64/medium_local. The same proof is not a broad candidate:
  medium_r50/main_r90 regress badly. Treat this as attribution for a future
  substrate, not as a promotion path for OwnerPage.

follow-up:
  bench_results/20260702T123734Z_hz9_candidate_gate

  ownerfast_bits R5:
    medium_r50 0.929
    main_r90   0.987
    medium_local0 1.083
    main_local0   0.992

  ownerfast_bits_low32 R5:
    medium_r50 0.910
    main_r90   0.906
    medium_local0 1.026
    main_local0   0.998

  read:
    class-cutting the fast mutation to <=32K does not stabilize the shape.
    Full ownerfast_bits remains the better attribution proof, but still misses
    medium_r50 and has noisy local/main movement. Do not continue OwnerPage
    class-cut tuning for default.
```

## OwnerPage Disabled Fast Reject

```text
box:
  HZ9OwnerPageDisabledFastReject-L1

scope:
  opt-in proof only
  when a TLS OwnerPage state already disabled a class after REMOTE_SEEN,
  reject before h9_owner_page_ensure_thread_state()

logs:
  bench_results/20260702T124529Z_hz9_candidate_gate
  bench_results/20260702T124822Z_hz9_candidate_gate

debug read:
  medium_r50 disabled_fast_reject:
    alloc_call=960000
    alloc_mode_block=959744
    state_ensure=352

read:
  The mechanism removes the intended disabled-class ensure tax. Release gates
  still do not stabilize: disabled_fast_reject helps some remote rows but can
  hurt main/local/small, and combining it with ownerfast_bits is worse in the
  mixed gate. The remaining OwnerPage problem is broader code/branch/body
  shape, not only a single disabled-class call.
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
  ownerfast-bits explains much of the local body cost but does not produce a
  stable broad gate
  disabled-fast-reject removes state-ensure tax but also does not stabilize
  the broad gate
  keep OwnerPage local mutation proofs as attribution only
```
