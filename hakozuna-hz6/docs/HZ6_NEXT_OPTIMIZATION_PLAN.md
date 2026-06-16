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
   Goal:
     confirm selected/default and profile controls after the script cleanup.
   Runner:
     linux/run_hz6_broad_guard.sh
   Acceptance:
     selected/default stays stable
     capacity-narrow + descriptor-hybrid remain workload controls
     no new fail/route/source regression

2. SourceBlockMetaSlim-L1 design refresh
   Goal:
     reduce static SourceBlock metadata RSS without touching preload hot paths.
   First target:
     source-run metadata split or lazy run metadata
   Acceptance:
     source lifetime/route semantics unchanged
     selected smoke clean
     static RSS win visible before any behavior promotion

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

Why this first:
  HZ6 speed is now competitive on selected rows.
  The remaining fixed-floor gap versus HZ3 is mostly static/payload floor.
  Hot-path free/malloc no-go evidence is already dense.
  Metadata layout can improve RSS without adding per-operation branches.
```
