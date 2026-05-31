# Current Task

HZ6 is now in active Windows/Linux implementation and benchmarking. HZ5 Linux
remains profile-stabilized; new HZ5 work should not blur the HZ6 contract.

## Latest Windows Matrix Read

Latest full matrix:

```text
private/allocators/hakozuna/docs/benchmarks/windows/20260601_045004_allocator_matrix.md
```

Read:

```text
balanced / wide_ws:
  HZ6 strict/speed/rss are far below HZ3 / HZ4 / mimalloc / tcmalloc.
  broad-cap rows improve throughput but raise RSS substantially.

larger_sizes:
  HZ6 strict/speed/rss stay very low-RSS and are still throughput weak.
  broad-cap rows recover throughput well and stay well below the mixed-row RSS
  blow-up seen on balanced / wide_ws.

large_slice_64k / 128k:
  HZ6 strict/speed/rss are strong and low-RSS on the exact large-slice rows.
  128k is particularly strong in the broad lanes and beats tcmalloc in the
  current run.

RSS:
  the matrix now carries peak RSS KB for every allocator row, including
  HZ3 / HZ4 / mimalloc / tcmalloc.
```

Current interpretation:

```text
strong:
  large_slice_64k
  large_slice_128k
  exact large-slice low-RSS lanes

weak:
  balanced
  wide_ws
  small/mixed rows under strict/speed/rss controls

tradeoff:
  broad-cap HZ6 lifts throughput on many rows, but it also raises RSS.
```

Paper runner coverage:

```text
RSS-aware summaries now cover:
  allocator matrix
  Larson paper runner
  Redis workload paper runner
  MT remote paper runner

Next validation order:
  1. rerun the RSS-aware paper runners with the new peak_kb columns
  2. keep the balanced / wide_ws weakness as the main HZ6 warning
  3. keep large_slice_64k / 128k as the strong proof points
```

Latest weakness read:

```text
Larson:
  worker-warmup is good.
  stress and appcap are still the weak lanes, with HZ6 failing or
  underperforming on the heavier T=8/T=16 rows.

Redis:
  HZ6 appcap is weak across SET / GET / LPUSH / LPOP / RANDOM.
  mimalloc and tcmalloc stay ahead on every Redis pattern in the latest run.

HZ6 owner-aware MT remote:
  throughput is usable and RSS is low.
  alloc_failures are still huge, so this remains a pressure lane rather than a
  clean win lane.
```

Next step:

```text
1. Keep the RSS-aware matrix as the current Windows comparison baseline.
2. Use the balanced / wide_ws weakness as the main low-ROI warning.
3. Treat large_slice_64k / 128k as the strong HZ6 proof points.
4. Add dedicated RSS sweeps only where the matrix still leaves ambiguity.
```

## HZ6 Windows Current Read

```text
Latest app-like matrix:
  results/windows-benchmark-suite/20260528_115721/app-like

Default HZ6 rows:
  hz6-strict / hz6-speed / hz6-rss
  small R1 capacities
  keep as smoke/control rows

Broad HZ6 rows:
  hz6-strict-broad / hz6-speed-broad / hz6-rss-broad
  same profiles with larger fixed capacities for broad working-set matrix rows
  use for large_slices and same-runner broad comparison

Appcap HZ6 rows:
  hz6-strict-appcap / hz6-speed-appcap / hz6-rss-appcap
  app-like fixed capacity rows for Larson/live-set separation

Capacity finding:
  the earlier 8K..64K Windows weakness was dominated by alloc_fail from
  descriptor/route/source capacity exhaustion.

Route finding:
  exact-route hash probing before fail-closed scan fallback restores much of
  the mid/large working-set performance while preserving invalid/interior
  pointer scan checks.
```

App-like repeat-3 read:

```text
Larson:
  HZ6 default and broad rows fail during warmup.
  HZ6 stats show descriptor exhaustion at default 64 and broad 4096.
  appcap rows run at T=1 with no alloc_fail, but throughput remains far below
  HZ3/tcmalloc/mimalloc, so the remaining issue is lifecycle/front-source
  behavior rather than raw live-set capacity.

Random mixed:
  small-cap HZ6 rows fail as capacity controls.
  broad-cap HZ6 rows run with low RSS but are slower than HZ3/HZ4/tcmalloc.

MT remote:
  HZ6 rows are extremely slow under the legacy remote runner.
  Treat as remote-contract mismatch until an HZ6 owner/remote-aware runner is
  available.

Redis workload:
  HZ6 default rows are strong on SET / GET / LPUSH.
  HZ6 broad rows are weak, so broad-cap is not a Redis default.
```

Latest HZ6 standalone Windows benchmark:

```text
results:
  private/allocators/hakozuna/hakozuna-hz6/private/raw-results/windows/
    hz6_benchmark_20260528_124050

local:
  strict 8192   ~60.16M ops/s
  strict 65536  ~68.05M ops/s
  speed  8192   ~47.57M ops/s
  rss    8192   ~52.44M ops/s

remote 131072:
  strict/speed/rss ~57.49M..60.44M ops/s

reuse 131072:
  strict/speed/rss ~58.30M..63.88M ops/s
```

Read:

```text
The standalone runner is a stable contract baseline for local/remote/reuse.
Remote/reuse are usable, but still profile-sensitive.

Treat the legacy mt_remote runner as contract-mismatched until the HZ6 owner-
aware remote runner exists.
The mt_remote paper runner is now timeout-bounded and skips HZ6 rows by
default. Use `-IncludeHz6Legacy` only to debug the mismatch; HZ6 remote verdicts
should come from the HZ6 standalone local/remote/reuse runner or the owner-
aware HZ6 mt-remote runner.
```

Post-route-hash large-slice signal on Windows:

```text
8K:
  hz6-strict-broad ~54.7M ops/s, near tcmalloc ~57.1M

16K / 32K / 64K:
  hz6-strict-broad beats or roughly matches tcmalloc in the latest local run

128K:
  hz6-speed-broad ~69.6M ops/s, above tcmalloc ~46.7M
```

Next HZ6 rule:

```text
Do:
  keep fixed broad-cap rows for reproducible broad comparison
  keep small-cap rows as controls
  inspect alloc_fail before interpreting a row
  expose HZ6 stats in app-like failure rows before adding new knobs

Do not:
  call hz6-*-broad the production/default lane yet
  hide small-cap controls
  add adaptive capacity to comparison rows before a dry-run/growth policy exists

Next order:
  1. Keep the HZ6 standalone local/remote/reuse runner as the first contract
     baseline.
  2. Compare HZ6 against HZ3 / HZ4 / HZ5 on the same machine and same runner.
  3. Keep HZ6 appcap rows as Larson capacity-separation evidence.
  4. Build an HZ6 owner/remote-aware runner for MT remote.
  5. Larson lifecycle/front-source diagnostic.
  6. Redis default-row mechanism readout.
  7. hz6-adaptive-cap-dryrun only after the above evidence is clean.

Research candidate:
  hz6-adaptive-cap-dryrun
  then hz6-adaptive-cap only if growth counters justify it
```

Latest capacity-breakdown read:

```text
results/windows-hz6-capacity-breakdown/20260528_105215_allocator_matrix.md

8K default HZ6:
  route_register_fail dominates alloc_fail.

64K default HZ6:
  descriptor_exhausted dominates alloc_fail.

Broad-cap rows:
  clear all three tracked exhaustion counters in the checked 8K/64K slices.
```

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

HZ6:
  keep separate from HZ5 implementation.
  design route-safe / transfer-first / RSS-aware contracts before writing code.
```

## HZ6 Design Seed

HZ6 docs now live under:

```text
hakozuna-hz6/
```

Read in this order:

```text
hakozuna-hz6/README.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
hakozuna-hz6/docs/current_task.md
```

Current HZ6 position:

```text
HZ3 lesson:
  thin local/TLS cache

HZ4 lesson:
  remote grouping and page/span reuse

HZ5 lesson:
  fail-closed descriptor ownership and low-RSS profile discipline

HZ6 plan:
  make RouteLayer, FrontCache, TransferLayer, SourceLayer, ScavengeLayer,
  OwnerLayer, and PolicyLayer explicit from the first implementation.
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
private/raw-results/linux/hz5_large128_draintrust_budget1_manual_r3_20260526_062121
private/raw-results/linux/hz5_large128_source16_draintrust_l0_compare_20260526_062558
private/raw-results/linux/hz5_large128_source16_draintrust_perf_20260526_062616
private/raw-results/linux/hz5_large128_source16_draintrust_perf_t4_20260526_062634
private/raw-results/linux/hz5_large128_source16_draintrust_median_r3_20260526_062644
private/raw-results/linux/hz5_large128_transfer128_smoke_r3_20260526_063953
private/raw-results/linux/hz5_large128_transfer128_flushmiss_r3_20260526_064056
private/raw-results/linux/hz5_large128_transfer128_tlsfirst_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_ownershard_r3
private/raw-results/linux/hz5_large128_transfer128_shard16_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_shard16_mask_smoke_r1
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
9. Draintrust shows the drain CAS cost matters. The latest RUNS=3 recheck is
   split: t8/r90 wins strongly with low RSS, but t8/r50 loses source16 and
   t4 rows remain far behind tcmalloc.
10. Draintrust+drainbulk composition is no-go in manual RUNS=3.
11. Draintrust+budget1 is a t4/r50-only local optimum: 29.56M on manual RUNS=3,
   but t4/r90 and t8 rows collapse. Do not add it as a named lane.
12. L0 source16/draintrust recheck shows draintrust increases source refill
    pressure on some rows. Example: t4/r90 source16 source_refill=392 versus
    draintrust source_refill=1143 in the L0 run. Trusted drain can reduce a
    state-transition slice, but can also perturb reuse/refill timing.
13. Perf recheck says instruction count is not always the only t8 limiter:
    at t8/r50 HZ5 and tcmalloc are close on aggregate instructions/op, while
    elapsed still differs. Treat parallel efficiency, owner-drain timing, and
    source/refill pressure as co-equal suspects.
14. Transfer128 class cache is a useful structural diagnostic: it improves
    t4/r50 with very low RSS, showing class-level transfer can cut owner-inbox
    cost. It is not broad yet; t8 rows regress, likely from global transfer
    contention or poor high-thread consumption order.
15. Transfer128-tlsfirst is no-go: producer-local retention starves consumer
    reuse and worsens RSS.
16. Transfer128-ownershard is no-go: routing by old owner slot preserves
    neither transfer128's t4/r50 signal nor source16's r90 behavior.
17. Transfer128-shard16 is no-go even after adding a nonempty mask to avoid
    empty-shard locks. Consumer-visible sharding loses the global transfer128
    t4/r50 win and does not recover t8.
18. Do not add another policy until a concrete hotspot explains the row split.
19. Keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters.
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
