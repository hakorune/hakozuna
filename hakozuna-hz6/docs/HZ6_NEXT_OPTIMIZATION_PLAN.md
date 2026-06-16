# HZ6 Next Optimization Plan

This is the short forward plan after the 2026-06-16 Ubuntu preload/profile
cleanup. Keep detailed raw evidence in `HZ6_UBUNTU_PRELOAD_LANES.md`; this file
only states what to build next and what not to reopen.

## Current Read

```text
HZ6 selected/default is stable.
The default lane is now a speed/RSS balance allocator, not a max-speed-only row.
Fixed-boundary and workload profiles are useful, but remain explicit profiles.
The latest workload repeat keeps capacity-narrow and descriptor-hybrid paired:
  capacity-narrow wins some proxy rows
  descriptor-hybrid wins others
  RSS is effectively tied
Do not collapse them into a single broad default without new real workload data.
```

## Next Build Order

```text
1. Broad selected/profile refresh
   Status:
     done in `private/raw-results/linux/hz6_broad_guard_20260616_094818`
   Goal:
     confirm selected/default and profile controls after the script cleanup.
   Runner:
     linux/run_hz6_broad_guard.sh
   Read:
     selected/default remains stable on focused/fixed rows
     selected still has the known workload-proxy capacity cliff
     capacity-narrow + descriptor-hybrid both recover collapsed workload rows
     no alloc_fail regression was observed in the workload proxy summary

2. SourceBlockMetaSlim-L1 design refresh
   Goal:
     reduce static SourceBlock metadata RSS without touching preload hot paths.
   First target:
     `HZ6_SOURCE_RUN_INLINE_META_L1=0` profile/control DSO
   Acceptance:
     source lifetime/route semantics unchanged
     selected smoke clean
     static RSS win visible before any behavior promotion
   Current implementation:
     selected keeps inline metadata enabled
     `hz6-source-run-meta-off-target` compiles it out for A/B measurement
   Initial evidence:
     raw `hz6_preload_profile_frontier_20260616_100113` shows about 2.5 MiB
     lower peak RSS on focused/fixed rows, with fixed_4k slightly weaker and
     other rows flat/positive
     stats-on smoke shows `static_table_bytes 14017920 -> 11273600`
     short broad guard `hz6_broad_guard_20260616_100224` confirms the RSS win;
     cross fixed-gap still leaves fixed_4k behind HZ3/tcmalloc
     workload-only guard `hz6_broad_guard_20260616_100509` keeps alloc_fail=0
     but confirms meta-off does not solve selected workload-capacity cliffs

3. Real workload profile evidence
   Goal:
     decide whether capacity-narrow, descriptor-hybrid, or a future profile is
     worth documenting as an application-specific recommendation.
   Rule:
     proxy rows alone are not enough to change selected/default.

4. Wrapper profile audit only if needed
   Goal:
     keep calloc/realloc/aligned controls available, but do not promote them
     without a focused + broad + cross-allocator guard.
```

## Default Guardrails

```text
Keep selected/default stable.
Keep workload-capacity-narrow and descriptor-hybrid as paired controls.
Keep Toy-map8192 external as an explicit fixed-boundary RSS profile.
Keep realloc/adaptive/calloc/aligned as profiles or controls.
Do not default free-time release, cold-retire, active-map widening, page-kind
tables, packed metadata, or route inline work without new diagnostics.
```

## Immediate Implementation Candidate

```text
SourceBlockMetaSlim-L1:
  create a Linux-visible experiment flag for source-run metadata storage shape
  keep allocator behavior identical
  measure static table/RSS and selected smoke before touching free/malloc paths
  keep the meta-off profile explicit until focused/fixed/broad guards are clean

Why this first:
  HZ6 speed is now competitive on selected rows.
  The remaining fixed-floor gap versus HZ3 is mostly static/payload floor.
  Hot-path free/malloc no-go evidence is already dense.
  Metadata layout can improve RSS without adding per-operation branches.
```
