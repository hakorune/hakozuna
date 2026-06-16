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
  HZ6_NEXT_OPTIMIZATION_PLAN.md

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
  toy-map8192(+external) / workload-capacity(+narrow/hybrid/map8192) / descriptor-overflow/hybrid / toy-map-external / midpage-skip-transfer
  source-run-meta-off / external-meta-off fixed-boundary control / external-meta-off-route16k fixed-boundary control

Runners:
  broad_guard / fixed_boundary_profile_frontier / preload_profile_frontier
  fixed_gap_matrix / fixed_cost_residency_matrix / fixed_quiescent_rss_matrix
  route16k_capacity_guard
  workload_proxy_matrix / workload_profile_guard / workload_capacity_{frontier,gap_diag,profile_gap_diag,pair_focus,shape_sweep,narrow_ladder,narrow_map_ladder,hybrid_depot_ladder} / workload_descriptor_{overflow,hybrid,hybrid_narrow,hybrid_depot}_ladder
  check_hz6_preload_profile_registry
```

## Latest Evidence

```text
Archived profile/control repeats:
  profile_frontier/broad_guard/realloc/calloc raws before 20260616_060000 are
  summarized in HZ6_UBUNTU_PRELOAD_LANES.md and archive snapshots.

Recent fixed/workload/profile repeats:
  private/raw-results/linux/hz6_fixed_boundary_profile_frontier_20260616_072931
  private/raw-results/linux/hz6_fixed_cost_residency_matrix_20260616_063918
  private/raw-results/linux/hz6_fixed_gap_matrix_20260616_082042
  private/raw-results/linux/hz6_fixed_quiescent_rss_matrix_20260616_072153
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_{062305,064915,065310,065329,072333,074342,074414}
  private/raw-results/linux/hz6_workload_capacity_gap_diag_20260616_{083459,083910}
  private/raw-results/linux/hz6_workload_descriptor_{overflow_ladder_20260616_084807,hybrid_ladder_20260616_085253}
  private/raw-results/linux/hz6_workload_descriptor_hybrid_narrow_ladder_20260616_090407
  private/raw-results/linux/hz6_workload_descriptor_hybrid_depot_ladder_20260616_090934, hz6_workload_capacity_narrow_ladder_20260616_091541, hz6_workload_capacity_narrow_map_ladder_20260616_093456, hz6_workload_profile_gap_diag_20260616_093918, and hz6_workload_proxy_matrix_20260616_094129
  private/raw-results/linux/hz6_broad_guard_20260616_{094818,100224,100509}
  private/raw-results/linux/hz6_workload_{capacity_frontier_20260616_081537,proxy_matrix_20260616_{080227,084249,084440,085632}}
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_073231
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_100113
  private/raw-results/linux/hz6_workload_proxy_matrix_20260616_100846
  private/raw-results/linux/hz6_fixed_external_meta_off_matrix_20260616_{101654,101836}
  private/raw-results/linux/hz6_workload_proxy_matrix_20260616_101836
  private/raw-results/linux/hz6_fixed_gap_matrix_20260616_{102020,104546}
  private/raw-results/linux/hz6_fixed_rss_gap_attribution_20260616_102348
  private/raw-results/linux/hz6_static_table_trim_ab_20260616_{102718,102750}
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_102939
  private/raw-results/linux/hz6_{workload_proxy_matrix,fixed_gap_matrix}_20260616_103109
  private/raw-results/linux/hz6_route16k_capacity_guard_20260616_{103616,103858}
  private/raw-results/linux/hz6_static_table_trim_ab_20260616_{104235,104302}
  private/raw-results/linux/hz6_workload_profile_guard_20260616_{105102,105644,111620}, hz6_workload_capacity_{pair_focus_20260616_112040,shape_sweep_20260616_{112816,113139}}, hz6_workload_descriptor_hybrid_depot_ladder_20260616_105953, hz6_workload_capacity_profile_gap_diag_20260616_{110830,111503}, hz6_workload_capacity_narrow_ladder_20260616_111216, and hz6_workload_capacity_narrow_map_ladder_20260616_111243
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
2. Use HZ6_NEXT_OPTIMIZATION_PLAN.md as the forward plan.
3. Broad selected/profile guard refreshed in hz6_broad_guard_20260616_094818:
   selected/profile is stable; workload proxy still needs capacity controls.
4. SourceBlockMetaSlim-L1 + route16K fixed RSS control: static metadata/table
   RSS without malloc/free hot-path branches. Current aliases:
   `hz6-source-run-meta-off-target`,
   `hz6-small-boundary-trusted-toy-map8192-external-meta-off-target`, and
   `hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target`.
   Route16K cuts another about 2.5 MiB on fixed/focused rows and now makes HZ6
   fixed_4k/8k ops-per-MiB competitive with or ahead of HZ3/tcmalloc in the
   latest fixed-gap matrix. Thick route16K guard `103858` keeps stats
   failure-free and fixed-gap `104546` keeps the cross-allocator position:
   tcmalloc beaten on fixed rows, HZ3 matched/near on 4K/8K and beaten on 16K
   ops-per-MiB.
5. Prefer `capacity-hybrid` as the workload-capacity recommendation name while
   keeping capacity-narrow paired; raws `111620`, pair-focus `112040`, and
   shape-sweeps `112816`/`113139` show row/shape-specific splits and a
   WS16384 cliff. Depot raw `105953` is mixed, so keep hybrid depot1024; pair
   wrapper is `run_hz6_workload_capacity_pair_focus.sh`.
6. Keep Toy-map8192 external as explicit fixed-boundary RSS profile.
7. Next likely attack: WS16384 capacity/lookup cliff diagnostic; route16K
   +map/source/frontcache trims are control-only after raw `104302`.
8. Do not reopen cold-retire, active-map widening, page-kind/free-order tables,
   packed metadata, or route inline work without new diagnostics.
```
