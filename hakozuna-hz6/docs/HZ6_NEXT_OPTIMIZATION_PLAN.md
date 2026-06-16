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
     workload-control meta-off probe `hz6_workload_proxy_matrix_20260616_100846`
     lowers RSS but is speed-weak, so keep workload aliases unchanged

3. Fixed_4k speed-gap audit
   Goal:
     explain the remaining HZ3/tcmalloc gap without weakening selected RSS.
   First target:
     compare selected, meta-off, and existing fixed-boundary profiles with
     diagnostics focused on Toy class4 allocation/free shape.
   Rule:
     do not reopen active-map widening or raw frontcache push without new
     counter evidence.
   Added lane:
     `hz6-small-boundary-trusted-toy-map8192-external-meta-off-target` plus
     `run_hz6_fixed_external_meta_off_matrix.sh` for clean fixed-boundary
     external-vs-meta-off A/B without env-override label collisions.
   Current read:
     raw `hz6_fixed_external_meta_off_matrix_20260616_101836` keeps the
     focused/fixed RSS win, but workload proxy raw
     `hz6_workload_proxy_matrix_20260616_101836` rejects workload/default
     promotion; keep it as an explicit fixed-boundary RSS control.
     cross raw `hz6_fixed_gap_matrix_20260616_102020` shows the control is now
     the preferred fixed-boundary HZ6 RSS profile versus tcmalloc, while HZ3
     still keeps the lower fixed_4k/8k RSS floor.
     attribution raw `hz6_fixed_rss_gap_attribution_20260616_102348` shows
     logical MidPage payload is not the right next lever: external-meta-off can
     lower peak RSS while increasing logical all-local-free payload.
     static-table trim raw `hz6_static_table_trim_ab_20260616_102750` selects
     route16K as the next fixed-profile control: fixed_4k/8k/16k RSS drops
     about another 2.5 MiB with no failure counters in the stats guard.
     profile-frontier raw `hz6_preload_profile_frontier_20260616_102939`
     keeps the route16K RSS win on focused/fixed rows.
     cross raw `hz6_fixed_gap_matrix_20260616_103109` shows route16K now
     beats tcmalloc on fixed_4k/8k/16k speed and ops-per-MiB and reaches HZ3
     fixed_4k/fixed_8k balance territory, though HZ3/HZ4 still keep lower
     absolute RSS on some rows.
     workload proxy raw `hz6_workload_proxy_matrix_20260616_103109` keeps
     route16K out of workload/default promotion: capacity-narrow and
     descriptor-hybrid are still orders faster on large live-set cache proxies.

4. Real workload profile evidence
   Goal:
     decide whether capacity-narrow, descriptor-hybrid, or a future profile is
     worth documenting as an application-specific recommendation.
   Rule:
     proxy rows alone are not enough to change selected/default.

5. Wrapper profile audit only if needed
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
Fixed4K8KResidualRssGapAudit-L1:
  status: route16K profile/control implemented
  explain the remaining fixed-profile RSS/static-capacity tradeoff after
  external-meta-off-route16K
  continue from diagnostics/profile controls, not selected behavior:
    route table capacity safety by fixed/profile/workload shape
    frontcache table shape
    Toy/MidPage active-map storage

Why this first:
  SourceBlockMetaSlim-L1 and route16K are implemented and clean on fixed/focused
  rows.
  Workload proxy rejects default/workload promotion.
  Cross fixed-gap now shows the HZ6 route16K profile beats tcmalloc and is
  competitive with HZ3 on fixed_4k/8k ops-per-MiB, so the remaining question is
  fixed-profile safety/capacity, not hot-path behavior.
  Payload release/cold-retire is not the next fixed RSS lever from current
  evidence.
```
