# HZ11 Current Task

```text
Active status:
  HZ11Sh6benchPathCostAttribution-L1 is measured as attribution GO.

Current stance:
  HZ11 remains a speed-first research line.
  libhz11_span_transfer.so is the remote/mixed microbench speed-lane candidate.
  libhz11_span_transfer_thread_exit.so fixes larson thread-churn RSS, but is
  not a macro default.

Macro promotion blockers:
  python_alloc and mstress now pass under the thread-exit-cap candidate.
  sh6bench remains far slower than tcmalloc and over the RSS guard. Attribution
  now points to transfer/central path volume plus span footprint, not the
  span-return metadata-lock regression.

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
docs/HZ11_NO_GO_LEDGER.md

NO-GO / attribution records:
  docs/no_go/README.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_GATE_L1.md
  docs/no_go/HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md
  docs/no_go/HZ11_SPAN_SOURCE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_CAP_L1.md

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
```

## Next Step

```text
Primary next box:
  HZ11Sh6benchTransferCentralPathCost-L1

Goal:
  reduce sh6bench transfer/central path volume or span footprint for the
  thread-exit-cap candidate.

Boundary:
  candidate sibling only; do not promote macro default until sh6bench improves
  and the full gate is rerun.

Required evidence:
  sh6bench wall improves materially without RSS regression; track transfer
  inserts/spills, central high-water, span_create, and RSS.

Keep:
  transfer as the remote/mixed microbench lane only until macro correctness is
  stable.
```
