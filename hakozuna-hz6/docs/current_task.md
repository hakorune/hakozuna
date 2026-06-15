# HZ6 Current Task

HZ6 is the active Windows/Linux allocator family. This file is the short
orientation ledger, not the chronological experiment log.

## Read First

```text
Selected/default:
  HZ6_SELECTED_FAMILY_SUMMARY.md
  HZ6_UBUNTU_SELECTED_BALANCE.md

Lane decisions:
  HZ6_LANE_GUIDE.md
  HZ6_UBUNTU_PRELOAD_LANES.md
  HZ6_UBUNTU_PROFILE_REGISTRY.md

Ubuntu MidPage closeouts:
  HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md
  HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md
  HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md

Hygiene/archive:
  HZ6_REPO_HYGIENE.md
  HZ6_SOURCE_MODULARIZATION.md
  archive/README.md
```

## Current Ubuntu State

```text
Selected/default:
  linux/hz6_preload_flags.sh
  linux/build_hz6_preload.sh
  out/linux/hz6_preload/libhakozuna_hz6_preload.so

Selected read:
  HZ6 is now a speed/RSS balance allocator.
  ToyTrustedDefault-L1 is selected/default.
  malloc_trim(0) is the selected quiescent RSS recovery API.
  Peak RSS remains mostly touched MidPage source-run payload residency.

Default position:
  Keep selected/default stable.
  Treat realloc/adaptive/calloc/aligned lanes as profile/control until they
  pass focused, fixed, stats/diagnostic, RSS, and cross-allocator guards.
```

## Active Profile Lanes

```text
Profile registry:
  HZ6_UBUNTU_PROFILE_REGISTRY.md

Standard frontier:
  toy-trusted / small-boundary-trusted / realloc-boundary split+adaptive
  aligned / calloc-direct / calloc-real / calloc-large-real

Explicit controls:
  small-boundary-trusted-toy-map8192 / toy-map-external / midpage-skip-transfer

Runners:
  fixed_boundary_profile_frontier / preload_profile_frontier
  fixed_cost_residency_matrix / fixed_quiescent_rss_matrix
  check_hz6_preload_profile_registry
```

## Latest Evidence

```text
Profile frontier:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_060739
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_065739
  private/raw-results/linux/hz6_midpage_skip_transfer_alias_smoke_20260616_065421

Realloc/adaptive profile repeats:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042441
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042628

Calloc large-real profile:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_044958
  private/raw-results/linux/hz6_preload_calloc_cross_20260616_045446

Calloc direct-HZ6 control:
  private/raw-results/linux/hz6_preload_calloc_audit_20260616_051752
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_051752

Recent fixed/profile repeats:
  private/raw-results/linux/hz6_fixed_boundary_profile_frontier_20260616_063106
  private/raw-results/linux/hz6_fixed_cost_residency_matrix_20260616_063918
  private/raw-results/linux/hz6_fixed_quiescent_rss_matrix_20260616_072153
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_{062305,064915,065310,065329}
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_063423

Details:
  archive/current_task_2026-06-16_{adaptive_profile_snapshot,calloc_profile_snapshot,fixed_boundary_profile_repeat}.md
```

## Do Not Reopen Casually

```text
Default no-go/control-only without substantially different evidence:
  route lookup header/body inline or macro expansion
  page-kind all-free selector
  page-hint/free-order hint table
  runroute-after-maps behavior
  MidPage active-map cap/probe widening or free fast-slot
  broad raw frontcache push
  same-owner trusted free default
  free-path cold-retire/source-block release default
  realloc-boundary slack/adaptive default
  calloc real-fallback/late-probe default
  source-run reuse/reclaim default
```

## Next Work

```text
1. Keep selected/default stable.
2. Fixed-floor selected/default is route32K + desc8192 + source1024 plus
   Toy16K/MidPage8K maps; fixed-row smoke sits near 79..80 MiB peak.
3. Treat adaptive-4k and adaptive-8k as fixed-boundary profile lanes.
4. Treat calloc-large-real as a large-calloc RSS/speed profile, not selected.
5. Keep calloc-direct default-off: thick focused+calloc repeat is mixed.
6. Cross-allocator refresh says HZ6 is strong on 4096..16384, fixed_8k, and
   fixed_16k; profile frontier refresh says fixed-boundary profile DSOs are
   still useful after the fixed-floor trims.
7. Current profile frontier:
   selected for broad focused/4096..16384; small-boundary-trusted/adaptive-4k
   for fixed_4k; small-boundary-trusted/adaptive-8k for fixed_8k; adaptive
   combined only for fixed_16k-heavy profile runs.
8. `hz6-small-boundary-trusted-toy-map8192-target` is the current fixed-boundary RSS profile:
   best HZ6 ops/MiB on fixed_4k/8k/16k; cross-allocator repeat keeps HZ3
   best on fixed_4k/8k, while HZ6 profile leads fixed_16k speed.
   It is still not a broad default because 16..4096/1024..4096 regress.
9. Do not promote realloc-boundary/adaptive to selected/default without a new
   guard that also preserves tiny, mixed-small, target, fixed, RSS, and stats.
10. Fixed-cost audit points to all-local-free payload plus static/map/frontcache
    floor; use FixedQuiescentRssMatrix-L1 before trim/release defaulting.
11. 8K run shrink under Toy-map8192 RSS profile is no-go/control.
12. MidPage skip-transfer is control/watch only: latest profile frontier loses
    16..4096 and 4096..16384 despite 1024..4096/fixed_16k signals.
13. Toy external-map and packed-frontcache combos remain runner-only RSS
    controls: they cut fixed RSS further but speed is mixed, so keep the
    existing Toy-map8192 DSO as the fixed-boundary RSS profile.
14. Keep this file below about 150 lines; archive completed evidence snapshots.
```
