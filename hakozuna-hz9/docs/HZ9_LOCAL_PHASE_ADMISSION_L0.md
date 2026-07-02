# HZ9 Local Phase Admission L0

## Purpose

```text
HZ9LocalPhaseAdmission-L0

goal:
  keep the LocalArena owner-fast local win
  avoid creating owner-fast pages in remote-heavy phases
```

This is the first candidate after DirectSlabUse proved SlabPage is only
remote/profile evidence.

## Evidence Basis

```text
LocalArena dense_ownerfast:
  medium/main local can win when owner-local mutation avoids atomic RMW/CAS
  remote-heavy rows regress after pages become remote-contaminated

LocalArena remote-first age:
  remote contamination usually happens after 1..3 local allocations

DirectSlabUse:
  remote rows win
  local rows lose even when all allocation/free operations hit SlabPage
```

## Candidate Rule

Instead of creating a LocalArena page immediately, require a short local-only
phase first.

```text
per thread/class:
  local_phase_alloc_count
  remote_seen_epoch

on allocation:
  if class has no remote evidence and local_phase_alloc_count >= threshold:
    allow owner-fast LocalArena page creation
  else:
    use HZ8 medium fallback

on local free:
  no change

on remote free:
  mark class remote-seen
  reset local_phase_alloc_count
  block owner-fast page creation for that class until a new local-only phase
```

Initial threshold:

```text
8 allocations
```

This intentionally pays a small warmup cost in local rows. The hypothesis is
that local rows recover it quickly, while remote-heavy rows never create most
owner-fast pages.

## Non-Goals

```text
do not use benchmark row identity
do not promote LocalArena dense_ownerfast directly
do not add another SlabPage route/admission variant
do not change HZ8 balanced default
do not add remote concurrent freelists
```

## Gate

```text
primary:
  medium_local0 >= baseline * 1.05
  main_local0 >= baseline * 1.00
  medium_r50 >= baseline * 0.98
  main_r90 >= baseline * 0.98

non-target:
  guard_local0 >= baseline * 0.98
  small_interleaved_remote90 >= baseline * 0.98

mechanism:
  local rows create pages after threshold
  remote-heavy rows create materially fewer pages than dense_ownerfast
  remote-heavy page_create_after_remote is near zero

safety:
  owned INVALID fail-closed
  remote duplicate claim accepted = 0
	  owner exit drains HZ9-owned pages
```

## Implementation

```text
variant:
  localarena_dense_ownerfast_phase8

build flags:
  H9_LOCAL_ARENA_LOCAL_PHASE_ADMISSION_L1
  H9_LOCAL_ARENA_LOCAL_PHASE_ALLOCS=8

notes:
  HZ8 medium fallback remote frees call h9_local_note_medium_remote_free().
  This lets remote-heavy rows mark LocalArena class history even before an
  arena page exists.
```

## Initial Result

```text
gate:
  bench_results/20260702T121109Z_localarena_phase8_r1

R1 ratios:
  medium_local0              0.816
  main_local0                0.823
  medium_r50                 0.611
  main_r90                   0.622
  guard_local0               1.007
  small_interleaved_remote90 0.953
```

## Decision

```text
NO-GO as broad HZ9 substrate.

read:
  Phase admission blocks remote-heavy page creation, but the added admission
  and LocalArena entry/body shape still regresses both local and remote medium
  rows. Do not tune the phase threshold wider without a new mechanism.
```

## Interpretation

```text
passes:
  continue LocalArena owner-fast substrate as HZ9 L1 candidate

local wins but remote still regresses:
  admission signal is too weak; do not tune page body further

local flat:
  warmup cost eats the local win; abandon LocalArena as broad substrate
```
