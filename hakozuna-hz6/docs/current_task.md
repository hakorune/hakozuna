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
  toy-map8192(+external) / workload-capacity(+narrow/map8192) / descriptor-overflow/hybrid / toy-map-external / midpage-skip-transfer

Runners:
  broad_guard / fixed_boundary_profile_frontier / preload_profile_frontier
  fixed_gap_matrix / fixed_cost_residency_matrix / fixed_quiescent_rss_matrix
  workload_proxy_matrix / workload_capacity_{frontier,gap_diag,narrow_ladder} / workload_descriptor_{overflow,hybrid,hybrid_narrow,hybrid_depot}_ladder
  check_hz6_preload_profile_registry
```

## Latest Evidence

```text
Profile frontier:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_{060739,081847}
  private/raw-results/linux/hz6_broad_guard_20260616_{082927,085900,090638,091214,091829,092222}
  private/raw-results/linux/hz6_midpage_skip_transfer_alias_smoke_20260616_065421

Realloc/adaptive profile repeats:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042441
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042628

Calloc large-real profile:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_044958
  private/raw-results/linux/hz6_preload_calloc_cross_20260616_045446

Calloc direct-HZ6 control: private/raw-results/linux/hz6_preload_calloc_audit_20260616_051752 and hz6_preload_profile_frontier_20260616_051752

Recent fixed/profile repeats:
  private/raw-results/linux/hz6_fixed_boundary_profile_frontier_20260616_072931
  private/raw-results/linux/hz6_fixed_cost_residency_matrix_20260616_063918
  private/raw-results/linux/hz6_fixed_gap_matrix_20260616_082042
  private/raw-results/linux/hz6_fixed_quiescent_rss_matrix_20260616_072153
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_{062305,064915,065310,065329,072333,074342,074414}
  private/raw-results/linux/hz6_workload_capacity_gap_diag_20260616_{083459,083910}
  private/raw-results/linux/hz6_workload_descriptor_{overflow_ladder_20260616_084807,hybrid_ladder_20260616_085253}
  private/raw-results/linux/hz6_workload_descriptor_hybrid_narrow_ladder_20260616_090407
  private/raw-results/linux/hz6_workload_descriptor_hybrid_depot_ladder_20260616_090934 and hz6_workload_capacity_narrow_ladder_20260616_091541
  private/raw-results/linux/hz6_workload_{capacity_frontier_20260616_081537,proxy_matrix_20260616_{080227,084249,084440,085632}}
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_073231
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
6. Fixed-gap refresh says HZ6 fixed RSS profiles beat tcmalloc balance on
   fixed_4k/8k/16k; external beats HZ3 speed on fixed_4k/8k, while HZ3 keeps
   the lower RSS floor.
7. Current profile frontier:
   latest short broad guard keeps selected/default stable; adaptive/realloc
   remain speed-leaning fixed controls; Toy-map8192/external remain explicit
   fixed RSS profiles, not broad defaults.
8. `hz6-small-boundary-trusted-toy-map8192-external-target` is the current
   lower-RSS fixed-boundary profile: best ops/MiB on fixed_4k/8k in the latest
   gap matrix, while Toy-map8192 is stronger on fixed_16k speed/ops-per-MiB.
   It is still not a broad default because focused mixed-small rows regress.
9. Do not promote realloc-boundary/adaptive to selected/default without a new
   guard that also preserves tiny, mixed-small, target, fixed, RSS, and stats.
10. Fixed-cost/quiescent audits point to peak-RSS payload/static floor, while
    `malloc_trim(0)` recovers current RSS; do not default free-time release.
11. 8K run shrink under Toy-map8192 RSS profile is no-go/control.
12. MidPage skip-transfer is control/watch only: latest profile frontier loses
    16..4096 and 4096..16384 despite 1024..4096/fixed_16k signals.
13. Toy-map8192 external is now an explicit lower-RSS fixed-boundary profile;
    packed-frontcache/sourceblock combos remain runner-only controls/no-go.
14. Capacity-gap diag says selected workload proxy collapse is descriptor-table
    exhaustion/prefill fallback; capacity-lite is faster, while descriptor-overflow 2048 keeps lower RSS with selected static tables.
15. Narrow hybrid moves explicit hybrid to desc10k/source1280/route40k plus depot1024: lower RSS than old hybrid, with only small mixed-small cost.
16. Broad workload guard now defaults to capacity-narrow + descriptor-hybrid as current explicit workload controls.
```
