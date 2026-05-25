# Current Task

HZ5 Linux is in profile-stabilization plus targeted tcmalloc-chase mode.

Long historical task notes were archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```

## Current Claim

```text
MidPage:
  PageRun64 remains the strong keep for general malloc main/mid/cross64 rows.

LargeFront:
  128K remote-heavy rows are the active tcmalloc chase area.
  No single LargeFront diagnostic is broad enough to replace saved profiles.

Documentation:
  keep current_task short.
  put long result logs in docs/archive or dedicated result docs.
```

## Current LargeFront Read

Saved / comparison lanes:

```text
large128-rss:
  source batch4, low-RSS fixed profile.

large128-source16:
  source batch16, current best r90/t8 comparison lane.
```

Diagnostics:

```text
large128-r50-hold8:
  wins t8/r50 in one focused run.
  loses source16 badly on r90.
  keep as r50 diagnostic only.

large128-direct-header:
  unsafe ptr-4096 header lookup upper bound.
  improves t4/r50 and wins t8/r90 in one run.
  not promotable because it is not fail-closed for foreign pointers.

large128-base-directmap:
  safe exact-base one-slot route cache.
  improves t4/r90 versus source16.
  loses source16 on t8 rows.
  no-promote diagnostic.

large128-r50-drain-directmap:
  combines drain1 and base-directmap.
  current RUNS=3 no-go: it does not beat either parent and r90 RSS/ops regress.

large128-ownerfast:
  enables LargeFront same-owner load/store state transition.
  current RUNS=3 no-go: t4/t8 r50 and t8/r90 regress versus source16.

large128-drainbulk:
  source16 plus bulk local-list commit during owner remote drain.
  diagnostic only: t4 rows show signal, but t8 rows regress versus source16.
  useful evidence that local_push overhead is only a slice of the remaining
  drain/refill gap.

large128-draintrust:
  source16 plus trusted load/store for owner-drained
  REMOTE_PENDING->LOCAL_FREE transitions.
  diagnostic only: t8/r50 beats tcmalloc in RUNS=3, but t8/r90 loses source16.
  useful evidence that drain state CAS is a real slice, but not the broad fix.
```

Recent result roots:

```text
private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802
private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345
private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731
private/raw-results/linux/hz5_large128_current_focus_r3_20260526_052851
private/raw-results/linux/hz5_large128_drain_directmap_r3_20260526_053047
private/raw-results/linux/hz5_large128_t4r50_perf_current_20260526_053300
private/raw-results/linux/hz5_large128_rb_current_r3_20260526_053400
private/raw-results/linux/hz5_large128_batch32_smoke_20260526_053458
private/raw-results/linux/hz5_large128_base_directmap4_r3_20260526_053547
private/raw-results/linux/hz5_large128_ownerfast_r3_20260526_053858
private/raw-results/linux/hz5_large128_direct_header_recheck_r3_20260526_055156
private/raw-results/linux/hz5_large128_source_populate_r3_20260526_055548
private/raw-results/linux/hz5_large128_l0_compare_20260526_060606
private/raw-results/linux/hz5_large128_drainbulk_r3_20260526_061000
private/raw-results/linux/hz5_large128_drainbulk_tail_r3_20260526_061107
private/raw-results/linux/hz5_large128_draintrust_r3_20260526_061614
private/raw-results/linux/hz5_large128_draintrustbulk_manual_r3_20260526_061931
```

## Next Engineering Direction

```text
1. Keep source16 and large128-rss as the current comparison pair.
2. Treat hold8/base-directmap/direct-header/drain-directmap as diagnostics.
3. Current t4/r50 perf gap is instruction/branch count plus page-fault/refill
   pressure, not cache-miss rate alone.
4. rb32/rb64, batch32, and ownerfast do not fix t4/r50; keep them
   diagnostic/no-go.
5. Direct-header recheck does not improve t4/r50; free lookup alone is not
   the next primary fix.
6. MAP_POPULATE source refill is a hard no-go; page-fault prepopulation
   explodes RSS and throughput collapses.
7. L0 confirms r50-drain republish churn is huge; it is not a clean t4/r50
   fix. Source16 has no republish but has many REMOTE_PENDING->LOCAL_FREE
   conversions.
8. Drainbulk reduces part of that conversion overhead and can improve t4, but
   t8 regressions mean local-push batching is not the broad fix.
9. Draintrust shows the drain CAS cost matters, especially t8/r50, but r90
   regressions mean trusted REMOTE_PENDING->LOCAL_FREE is not a broad profile.
10. Draintrust+drainbulk composition is no-go in manual RUNS=3.
11. Do not add another policy until a concrete hotspot explains the row split.
12. Keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters.
```

## Cleanup Status

Completed in this cleanup phase:

```text
HZ5_LINUX_PROFILE_MATRIX.md:
  shortened to current registry.
  history moved to docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md.

current_task.md:
  shortened to current state.
  history moved to docs/archive/current_task_2026-05-hz5-linux-post-largefront.md.

HZ5_LINUX_DEV_BRIEF.md:
  restored to a short entry point.
  history moved to docs/archive/HZ5_LINUX_DEV_BRIEF_HISTORY_2026-05.md.

HZ5_LINUX_ROUTE_LANE_MATRIX.md:
  shortened to current route/lane naming rules.
  history moved to docs/archive/HZ5_LINUX_ROUTE_LANE_MATRIX_HISTORY_2026-05.md.

HZ5_BENCH_RESULTS_INDEX.md:
  shortened to current active evidence index.
  history moved to docs/archive/HZ5_BENCH_RESULTS_INDEX_HISTORY_2026-05.md.

HZ5_P0_ALIGNED_RUN_8192.md:
  shortened to lifecycle index.
  history moved to docs/archive/HZ5_IMPLEMENTATION_LIFECYCLE_HISTORY_2026-05.md.

HZ5_LINUX_LOCAL2P_DESIGN.md:
  shortened to current Local2P profile boundary.
  history moved to docs/archive/HZ5_LINUX_LOCAL2P_DESIGN_HISTORY_2026-05.md.

HZ5_P43I_P43O_ALGO_CONSULT.md:
  shortened to current exact-route consultation summary.
  history moved to docs/archive/HZ5_P43I_P43O_ALGO_CONSULT_HISTORY_2026-05.md.

HZ5_SOURCE_CLEANUP_PLAN.md:
  added source-code split plan.
  active LargeFront/MidPageFront C files are intentionally not split until the
  current LargeFront tcmalloc-chase measurement direction stabilizes.

build_linux_hz5_standalone.sh:
  argument parser moved to linux/hz5_build_arg_parser.sh.

linux/hz5_build_arg_parser.sh:
  human-facing profile aliases moved to linux/hz5_build_profile_aliases.sh.
  parser is now a short dispatcher.
  low-level feature flags split into exact/midpage/front-end groups.

build_linux_hz5_standalone.sh:
  reduced below 1000 lines.
  long usage text, compound profile helpers, validation, and build-config
  output moved to focused linux/hz5_build_*.sh helpers.
```

Remaining cleanup candidates:

```text
hakozuna-hz5/largefront/hz5_largefront.c:
  large but acceptable during active optimization.
  only refactor after measurements stabilize.

hakozuna-hz5/midpagefront/hz5_midpagefront.c:
  large but active/stable enough to defer until allocator behavior settles.
```

## Reading Order

```text
1. current_task.md
2. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
3. hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
4. linux/HZ5_BUILD_SCRIPT_LAYOUT.md
5. hakozuna-hz5/docs/HZ5_SOURCE_CLEANUP_PLAN.md
6. hakozuna-hz5/docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md
7. hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```
