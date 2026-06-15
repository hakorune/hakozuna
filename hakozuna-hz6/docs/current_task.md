# HZ6 Current Task

HZ6 is the active Windows/Linux allocator family. Keep this file short: it is
the orientation entry point, not the chronological experiment ledger.

## Read First

```text
Selected rows and current comparisons:
  HZ6_SELECTED_FAMILY_SUMMARY.md

Lane names, status, controls, and no-go boundaries:
  HZ6_LANE_GUIDE.md

Ubuntu LD_PRELOAD selected bundle, controls, and profile DSOs:
  HZ6_UBUNTU_PRELOAD_LANES.md

Ubuntu selected speed/RSS balance:
  HZ6_UBUNTU_SELECTED_BALANCE.md

Ubuntu MidPage design/closeouts:
  HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md
  HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md
  HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md

Repo hygiene and source/module cleanup:
  HZ6_REPO_HYGIENE.md
  HZ6_SOURCE_MODULARIZATION.md

Archived chronological ledgers:
  archive/current_task_2026-06_history.md
  archive/current_task_2026-06-16_pre_compaction.md
```

## Current Ubuntu Selected State

```text
Selected/default source of truth:
  linux/hz6_preload_flags.sh

Selected build:
  linux/build_hz6_preload.sh

Selected DSO:
  out/linux/hz6_preload/libhakozuna_hz6_preload.so

Current selected additions to remember:
  HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
  HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
  HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1
  malloc_trim(size_t pad) interpose for explicit quiescent RSS release

Default position:
  selected remains the broad balanced Ubuntu preload lane.
  Do not promote profile-only macro bundles into selected/default without
  focused, fixed, stats/diagnostic, and cross-allocator guard evidence.
```

## Latest Selected Reads

```text
Focused cross-allocator selected matrix:
  private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_011316

  4096..16384:
    hz6      45.315M / 94.25 MiB
    hz3      51.230M / 73.38 MiB
    tcmalloc 34.618M / 100.25 MiB

Fixed-size selected matrix:
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_011443

  fixed_4k  hz6 31.542M / 91.88 MiB
  fixed_8k  hz6 43.506M / 93.25 MiB
  fixed_16k hz6 45.586M / 93.25 MiB

RSS:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_012801
  private/raw-results/linux/hz6_fixed_rss_residency_refresh_20260616_021200

  malloc_trim keeps peak RSS flat but lowers current RSS:
    16..4096    79.88 MiB -> 27.27 MiB
    1024..4096  91.00 MiB -> 27.18 MiB
    4096..16384 94.25 MiB -> 28.52 MiB
    fixed_16k   93.38 MiB -> 28.38 MiB

Read:
  HZ6's current Ubuntu strength is speed/RSS balance plus explicit quiescent
  RSS recoverability. Peak RSS is still mostly touched MidPage source payload.
  Fixed-size residency refresh keeps the same diagnosis: fixed_16k reports
  520 MiB logical 32K payload as all-local-free/frontcache-retained material,
  with ref mismatch 0. Do not reopen free-path cold-retire default.
```

## Current Profile DSOs

```text
Preferred broad small/fixed profile:
  linux/build_hz6_preload_small_boundary_trusted_target.sh
  aliases:
    hz6-small-boundary-trusted-target
    hz6_small_boundary_trusted_target

  validation:
    private/raw-results/linux/hz6_small_boundary_trusted_alias_smoke_20260616_015323
    private/raw-results/linux/hz6_small_boundary_trusted_focused_20260616_015341
    private/raw-results/linux/hz6_small_boundary_trusted_position_20260616_015331
    private/raw-results/linux/hz6_trusted_profile_cross_20260616_020157
    private/raw-results/linux/hz6_trusted_profile_fixed_20260616_020211

  read:
    16..256      selected 57.497M -> trusted 78.031M
    16..4096     selected 34.912M -> trusted 42.391M
    1024..4096   selected 33.926M -> trusted 39.045M
    4096..16384  selected 41.774M -> trusted 45.363M
    fixed_4k     selected 31.654M -> trusted 46.761M
    fixed_8k     selected 40.717M -> trusted 45.079M
    fixed_16k    selected 42.377M -> trusted 46.385M

  comparison:
    trusted beats tcmalloc/HZ4 on 4096..16384, fixed_8k, and fixed_16k speed.
    trusted beats tcmalloc on fixed_4k speed in this read, but HZ3 remains the
    speed/RSS frontier for tiny and most fixed-mid rows.

Comparison/profile controls:
  hz6-small-boundary-fast-target
    Useful comparison lane; latest trusted profile is more stable overall.

  hz6-realloc-boundary-4k-target
  hz6-realloc-boundary-8k-target
    Exact fixed-boundary controls. Keep profile-only.

  hz6-toy-target
    Toy/mid-small profile/control. Keep profile-only.

  hz6-aligned-target
    Real aligned-allocation fallback profile/control. Refresh raw
    private/raw-results/linux/hz6_aligned_profile_refresh_20260616_020716
    keeps the dedicated aligned win, but focused/fixed guards
    hz6_aligned_profile_guard_20260616_020941 and
    hz6_aligned_profile_fixed_guard_20260616_020953 keep it profile-only.

Profile alias helper:
  linux/hz6_preload_aliases.sh
  Keep matrix alias build hooks centralized here.
```

## Closed / Do Not Reopen Casually

```text
Default no-go or control-only unless new evidence is substantially different:
  route lookup header inline
  route lookup body inline/macro expansion
  page-kind all-free selector
  page-hint/free-order hint table
  MidPage active-map cap/probe widening
  MidPage free fast-slot
  broad raw frontcache push
  same-owner trusted free default
  cold-retire/free-path source-block release default
  broad Toy direct-class default
  realloc-boundary slack default
  source-run reuse/reclaim default
```

## Next Work

```text
1. Keep selected/default stable.
   Use HZ6_UBUNTU_PRELOAD_LANES.md and HZ6_UBUNTU_SELECTED_BALANCE.md as the
   authoritative selected/control/no-go ledgers.

2. Continue optimization through measured profile or narrow code-shape lanes.
   Current best profile lane is hz6-small-boundary-trusted-target.

3. If adding a profile DSO:
   - add builder under linux/
   - add dash/underscore aliases to linux/hz6_preload_aliases.sh
   - add resolver/hint in bench/lib/bench_common.sh
   - smoke through selected-balance or fixed-size matrix
   - update HZ6_UBUNTU_PRELOAD_LANES.md and current_task.md

4. If touching selected/default:
   require production no-stats, stats/diagnostics safety, focused/fixed matrix,
   RSS read, and docs update before commit.

5. Keep this file short.
   Move long experiment logs to archive/ and keep only current orientation here.
```
