# HZ11 Current Task

```text
Active status:
  HZ11Sh6benchSpanPageFootprintWithBatch32-L1 is measured as attribution GO.

Current stance:
  HZ11 remains a speed-first research line.
  libhz11_span_transfer.so is the remote/mixed microbench speed-lane candidate.
  libhz11_span_transfer_thread_exit.so fixes larson thread-churn RSS, but is
  not a macro default.

Macro promotion blockers:
  python_alloc and mstress now pass under the thread-exit-cap candidate.
  sh6bench remains far slower than tcmalloc and over the RSS guard. Attribution
  shows batch32 is a useful wall lever with no obvious macro side effects, but
  sh6bench span/page footprint is now attributed to fresh arena-carved spans and
  missing span reuse.

Do not do yet:
  claim HZ11 generally beats tcmalloc
  promote thread-exit or span-return lanes to default
  promote no-bytes lanes to default without an RSS-cap replacement
  add checked-mode diagnostics to the default path
  claim HZ11 replaces HZ8 or HZ10
```

## Restart Pointers

```text
README.md
docs/README.md
docs/HZ11_TRANSFER_CACHE_CENTRAL_SPAN_L1.md
docs/HZ11_TRANSFER_PROMOTION_MATRIX_L1.md
docs/HZ11_CURRENT_SPAN_POOL_THREAD_EXIT_L1.md
docs/HZ11_CENTRAL_POLICY_CORRECTNESS_L1.md
docs/HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1.md
docs/HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1.md
docs/HZ11_SH6BENCH_TRANSFER_BATCH_VS_CAP_L1.md
docs/HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1.md
docs/HZ11_SH6BENCH_SPAN_PAGE_FOOTPRINT_WITH_BATCH32_L1.md
docs/HZ11_NO_GO_LEDGER.md

NO-GO / attribution records:
  docs/no_go/README.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_GATE_L1.md
  docs/no_go/HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md
  docs/no_go/HZ11_SPAN_SOURCE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_CAP_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_BATCH32_L1.md

Cleanup / baseline docs:
  docs/HZ11_TLS_FAST_PATH_L1.md
  docs/HZ11_CACHE_BYTE_ACCOUNTING_GATE_L1.md
  docs/HZ11_REMAINING_BODY_ATTRIBUTION_L0.md
  docs/HZ11_SYS_RESOLVER_SPLIT_L0.md
  docs/HZ11_TOKEN_HELPERS_SPLIT_L0.md
  docs/HZ11_PUBLIC_ENTRY_HELPERS_L0.md
```

## Latest Results

```text
HZ11CurrentSpanPoolThreadExit-L1:
  Output:
    bench_results/hz11_current_span_thread_exit_20260708T000125Z/summary.md
  Verdict:
    GO for larson RSS/thread-churn only.
  Key result:
    larson RUNS=3:
      transfer RSS 654080 KiB, wall 4.170s
      thread-exit RSS 273280 KiB, wall 4.138s
      tcmalloc RSS 278912 KiB, wall 4.144s
    current-span pool drop=0; span_create drops to 17..24.

HZ11MacroSpeedLaneGateWithThreadExit-L1:
  Output:
    bench_results/hz11_macro_speed_lane_20260708T001420Z/summary.md
  Record:
    docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_L1.md
  Verdict:
    NO-GO for macro promotion.
  Key result:
    larson RSS fix holds, xmalloc_test/cache_scratch remain good, but:
      python_alloc fails 5/5
      mstress fails 5/5
      sh6bench is 13.161x tcmalloc wall and 1.307x max RSS

HZ11CentralPolicyCorrectness-L1:
  Output:
    bench_results/hz11_central_policy_correctness_20260708T003310Z/summary.md
  Record:
    docs/HZ11_CENTRAL_POLICY_CORRECTNESS_L1.md
  Verdict:
    GO for focused correctness rows; no macro promotion.
  Key result:
    python_alloc:
      thread-exit fails 3/3
      thread-exit-cap passes 3/3, wall 0.032s, max RSS 6656 KiB
      tcmalloc wall 0.032s, max RSS 8192 KiB
    mstress:
      thread-exit fails 3/3
      thread-exit-cap passes 3/3, wall 0.220s, max RSS 236140 KiB
      tcmalloc wall 0.191s, max RSS 213504 KiB
    central high-water:
      python_alloc class 2 high_water 4848
      mstress class 0 high_water about 58.5K-59.0K

HZ11MacroSpeedLaneGateWithThreadExitCap-L1:
  Output:
    bench_results/hz11_macro_speed_lane_20260708T003638Z/summary.md
  Record:
    docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_CAP_L1.md
  Verdict:
    NO-GO for macro promotion.
  Key result:
    Candidate hz11-thread-exit-cap:
      python_alloc OK 5/5, wall 0.032s vs tcmalloc 0.033s
      mstress OK 5/5, wall 0.221s vs tcmalloc 0.192s
      larson RSS fix holds: 273152 KiB vs tcmalloc 278912 KiB
      xmalloc_test 0.985x tcmalloc wall, 0.083x max RSS
      cache_scratch 1.010x tcmalloc wall, 0.456x max RSS
    Blocker:
      sh6bench 12.848x tcmalloc wall, 1.358x max RSS, 1.306x current RSS

HZ11Sh6benchPathCostAttribution-L1:
  Output:
    bench_results/hz11_sh6bench_path_cost_20260708T021718Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1.md
  Verdict:
    GO for attribution; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      tcmalloc wall 0.359s, max RSS 265344 KiB
      span-transfer wall 4.546s, max RSS 351360 KiB
      thread-exit-cap wall 4.552s, max RSS 351872 KiB
      thread-exit-cap-source wall 4.648s, max RSS 351232 KiB
      span-return-source wall 18.913s, max RSS 352768 KiB
    thread-exit-cap totals:
      xfer_insert 502021151
      xfer_spill / central_insert 25573012
      central final count drains to 0, high_water only ~3K for class 0
      span_create 16680
    source diag:
      span_create == arena_carve; span_reuse is only 51
      live_current_span is small
      meta_lock is 0 for thread-exit-cap-source
      meta_lock is huge only in span-return-source
  Decision:
    sh6bench active blocker is transfer/central path volume and span footprint,
    not span-return metadata-lock. Do not jump straight to metadata batching as
    the candidate macro fix.

HZ11Sh6benchTransferCentralPathCost-L1:
  Output:
    bench_results/hz11_sh6bench_path_cost_20260708T043634Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1.md
  Verdict:
    GO for transfer/central path-cost attribution; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      tcmalloc wall 0.362s, max RSS 267136 KiB
      thread-exit-cap wall 4.542s, max RSS 352256 KiB
      thread-exit-cap-xferwide wall 3.627s, max RSS 352128 KiB
    xferwide:
      xfer_spill / central_insert 25462402 -> 0
      xfer_hit 31382475 -> 17121178
      xfer_miss 3315573 -> 862087
      span_create stays about 16.7K
    Decision:
      transfer/central trip volume is a real wall lever, but still leaves
      sh6bench at 10.019x tcmalloc wall and about 1.318x RSS.

HZ11Sh6benchTransferBatchVsCap-L1:
  Output:
    bench_results/hz11_sh6bench_transfer_batch_vs_cap_20260708T050133Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_TRANSFER_BATCH_VS_CAP_L1.md
  Verdict:
    GO for contribution attribution; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      tcmalloc wall 0.373s, max RSS 267520 KiB
      base wall 4.552s, max RSS 351744 KiB
      batch32 wall 3.505s, max RSS 350848 KiB
      cap8192 wall 4.660s, max RSS 351616 KiB
      xferwide wall 3.602s, max RSS 351488 KiB
    Decision:
      HZ11_TRANSFER_BATCH=32 is the wall lever.
      HZ11_TRANSFER_CAP=8192 eliminates xfer_spill/central_insert but does not
      improve wall by itself.
      RSS/span_create stay essentially unchanged.

HZ11Sh6benchTransferBatchGranularity-L1:
  Output:
    bench_results/hz11_sh6bench_transfer_batch_granularity_20260708T060304Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1.md
  Verdict:
    GO for batch granularity attribution; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      tcmalloc wall 0.354s, max RSS 262016 KiB
      batch16 wall 4.539s, max RSS 352128 KiB
      batch32 wall 3.506s, max RSS 350336 KiB
      batch64 wall 5.501s, max RSS 352128 KiB
      batch128 wall 9.312s, max RSS 352128 KiB
    Decision:
      batch32 is the smallest useful wall candidate.
      batch64/batch128 regress as xfer_insert and central_insert grow.
      RSS/span_create remain unresolved.

HZ11MacroSpeedLaneGateWithBatch32-L1:
  Output:
    bench_results/hz11_macro_speed_lane_20260708T061909Z/summary.md
  Record:
    docs/no_go/HZ11_MACRO_SPEED_LANE_BATCH32_L1.md
  Verdict:
    NO-GO for macro promotion.
  Key result:
    RUNS=5:
      python_alloc OK 5/5, wall 0.032s vs tcmalloc 0.032s
      mstress OK 5/5, wall 0.221s vs tcmalloc 0.188s
      larson RSS fix holds: 273024 KiB vs tcmalloc 278912 KiB
      xmalloc_test 0.991x tcmalloc wall, 0.094x max RSS
      cache_scratch 1.016x tcmalloc wall, 0.456x max RSS
      sh6bench improves vs thread-exit-cap:
        4.537s -> 3.495s
      sh6bench still blocks:
        9.929x tcmalloc wall
        1.374x max RSS
        1.407x current RSS

HZ11Sh6benchSpanPageFootprintWithBatch32-L1:
  Output:
    bench_results/hz11_sh6bench_span_page_batch32_20260708T063053Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_SPAN_PAGE_FOOTPRINT_WITH_BATCH32_L1.md
  Verdict:
    GO for RSS/page-footprint attribution; no macro promotion.
  Key result:
    sh6bench RUNS=3 batch32-source:
      wall 3.542s vs tcmalloc 0.357s
      max RSS 351744 KiB vs tcmalloc 265728 KiB
      span_create 16745
      arena_carve 16745
      span_reuse 70
      live_current_span 387 total across runs
      central final total 752, high-water max 3232
    Decision:
      RSS/page footprint is fresh arena-carved span creation with little reuse,
      not central retention or current-span reserve.
```

## Next Step

```text
Primary next box:
  HZ11Sh6benchSpanReusePolicy-L1

Goal:
  add or test a narrow reusable span/page source for sh6bench classes with
  batch32 in place.

Boundary:
  diagnostic/candidate sibling only; do not promote macro default until
  sh6bench wall/RSS and full macro gate pass.

Required evidence:
  A/B sh6bench wall/RSS and source counters vs batch32; span_create and
  arena_carve must drop without correctness or macro side effects.

Keep:
  transfer as the remote/mixed microbench lane only until sh6bench wall/RSS and
  the full macro gate pass.
```
