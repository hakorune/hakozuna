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
  Treat realloc-boundary/adaptive/calloc/aligned lanes as profile/control
  until they pass focused, fixed, stats/diagnostic, RSS, and cross-allocator
  guard evidence.
```

## Active Profile Lanes

```text
Preferred broad fixed-boundary profile:
  hz6-small-boundary-trusted-target

Light small/fixed16 profile:
  hz6-toy-trusted-target

Exact realloc-boundary profiles:
  hz6-realloc-boundary-4k-target
  hz6-realloc-boundary-8k-target

Adaptive realloc-boundary profiles:
  hz6-realloc-boundary-adaptive-4k-target
  hz6-realloc-boundary-adaptive-8k-target
  hz6-realloc-boundary-adaptive-target

Other controls:
  hz6-aligned-target
  hz6-calloc-real-target
  hz6-calloc-large-real-target

Profile runner:
  linux/run_hz6_preload_profile_frontier.sh
```

## Latest Evidence

```text
Profile frontier:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_043329

Realloc copy attribution:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042441

Adaptive realloc-boundary profile repeat:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042628

Adaptive alias smoke:
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_042846

Calloc large-real profile:
  private/raw-results/linux/hz6_preload_calloc_audit_20260616_044759
  private/raw-results/linux/hz6_preload_profile_frontier_20260616_044958

Details:
  archive/current_task_2026-06-16_adaptive_profile_snapshot.md
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
2. Treat adaptive-4k and adaptive-8k as fixed-boundary profile lanes.
3. Treat calloc-large-real as a large calloc-heavy RSS/speed profile, not a
   selected/default lane.
4. Before any selected/default change, update stable docs and archive long logs.
```
