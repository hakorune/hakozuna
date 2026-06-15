# HZ6 Current Task

HZ6 is the active Windows/Linux allocator family. This file is the short
orientation ledger, not the chronological experiment log.

## Read First

```text
Current state:
  HZ6_SELECTED_FAMILY_SUMMARY.md
  HZ6_LANE_GUIDE.md
  HZ6_UBUNTU_PRELOAD_LANES.md
  HZ6_UBUNTU_SELECTED_BALANCE.md

Ubuntu MidPage design/closeouts:
  HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md
  HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md
  HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md

Hygiene and archive map:
  HZ6_REPO_HYGIENE.md
  HZ6_SOURCE_MODULARIZATION.md
  archive/README.md
```

## Current Ubuntu Selected State

```text
Selected/default:
  linux/hz6_preload_flags.sh
  linux/build_hz6_preload.sh
  out/linux/hz6_preload/libhakozuna_hz6_preload.so

Selected additions to remember:
  HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
  HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
  HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1
  malloc_trim(size_t pad) interpose for explicit quiescent RSS release

Default position:
  selected remains the broad balanced Ubuntu preload lane.
  Do not promote profile-only macro bundles into selected/default without
  focused, fixed, stats/diagnostic, RSS, and cross-allocator guard evidence.
```

## Latest Read

```text
Speed:
  HZ6 selected is now a speed/RSS balance allocator.
  Profile DSOs can be much faster than selected on focused/fixed rows.

RSS:
  Explicit malloc_trim/quiescent release is the selected RSS recovery path.
  Latest quiescent refresh lowered current RSS to roughly 27..28 MiB on
  focused/fixed rows.
  Peak RSS is still mostly touched MidPage source-run payload residency.

Residency diagnosis:
  fixed_16k still reports roughly 520 MiB logical 32K payload as
  all-local-free/frontcache-retained material with ref mismatch 0.
  Do not reopen free-path cold-retire or automatic purge default from this.
```

## Current Profile DSOs

```text
Broad small/fixed:
  hz6-small-boundary-trusted-target
  builder: linux/build_hz6_preload_small_boundary_trusted_target.sh

Lighter focused:
  hz6-toy-trusted-target
  builder: linux/build_hz6_preload_toy_trusted_target.sh

Profile controls:
  hz6-small-boundary-fast-target
  hz6-realloc-boundary-4k-target
  hz6-realloc-boundary-8k-target
  hz6-toy-target
  hz6-aligned-target

Alias/helper ownership:
  linux/hz6_preload_aliases.sh
  linux/hz6_preload_profile_builder.sh
  bench/lib/bench_common.sh
```

## Key Raw Results

```text
Selected:
  private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_011316
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_011443

Profiles:
  private/raw-results/linux/hz6_selected_vs_trusted_refresh_fixed_alias_20260616_024159
  private/raw-results/linux/hz6_selected_vs_trusted_stats_diag_fixed_alias_20260616_024313
  private/raw-results/linux/hz6_trusted_profile_decomp_refresh_20260616_024517
  private/raw-results/linux/hz6_toy_trusted_cross_20260616_025048
  private/raw-results/linux/hz6_toy_trusted_fixed_20260616_025117

RSS:
  private/raw-results/linux/hz6_rss_residency_audit_refresh_20260616_025309
  private/raw-results/linux/hz6_quiescent_rss_attribution_refresh_20260616_025320

More detail:
  archive/current_task_2026-06-16_profile_quiescent_snapshot.md
```

## Closed / Do Not Reopen Casually

```text
Default no-go or control-only unless new evidence is substantially different:
  route lookup header/body inline or macro expansion
  page-kind all-free selector
  page-hint/free-order hint table
  runroute-after-maps behavior
  MidPage active-map cap/probe widening or free fast-slot
  broad raw frontcache push
  same-owner trusted free default
  free-path cold-retire/source-block release default
  broad Toy direct-class default
  realloc-boundary slack default
  source-run reuse/reclaim default
```

## Next Work

```text
1. Keep selected/default stable.
   Use HZ6_UBUNTU_PRELOAD_LANES.md and HZ6_UBUNTU_SELECTED_BALANCE.md as the
   authoritative selected/control/no-go ledgers.

2. Continue measured profile or narrow code-shape optimization.
   Current best broad profile: hz6-small-boundary-trusted-target.

3. If adding a profile DSO:
   add builder under linux/
   add dash/underscore aliases to linux/hz6_preload_aliases.sh
   add resolver/hint in bench/lib/bench_common.sh
   smoke through selected-balance or fixed-size matrix, then update docs

4. If touching selected/default:
   require production no-stats, stats/diagnostics safety, focused/fixed matrix,
   RSS read, and docs update before commit.

5. Keep this file below 150 lines; archive long logs.
```
