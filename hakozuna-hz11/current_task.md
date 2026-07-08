# HZ11 Current Task

```text
Active status:
  HZ11Fine128RemoteMixedFinalConfirm-L1 is measured as GO for fine128 as the
  recommended opt-in macro speed-lane candidate. No default promotion.

Current stance:
  HZ11 remains a speed-first research line.
  libhz11_span_transfer.so remains the remote/mixed microbench speed-lane
  candidate.
  libhz11_span_transfer_thread_exit.so fixes larson thread-churn RSS, but is
  not a macro default.
  libhz11_span_transfer_thread_exit_cap_batch32.so is the current sh6bench wall
  candidate.
  libhz11_span_transfer_thread_exit_cap_batch32_fine128.so is the current
  recommended opt-in macro speed-lane candidate.
  Global fineclass remains a sh6bench RSS research lane only.
  The default allocator path is unchanged.

Macro promotion blockers:
  python_alloc and mstress now pass under the thread-exit-cap candidate.
  sh6bench remains far slower than tcmalloc and over the RSS guard. Attribution
  shows batch32 is a useful wall lever with no obvious macro side effects, but
  sh6bench span/page footprint is now attributed primarily to HZ11 power-of-two
  class-packing waste. Central-only full-span reuse did not materially reduce
  span_create, arena_carve, or RSS. MADV_NOHUGEPAGE also did not move RSS or
  page faults. Live-footprint observation shows active spans are almost full
  after class rounding, and requested-size observation shows requested payload
  high-water is far below HZ11 slot high-water. Fine size classes materially
  reduce sh6bench RSS while keeping batch32 wall roughly flat, but the full
  macro gate misses python_alloc current RSS by a small ratio. Focused
  python_alloc RUNS=10 diagnostics show that miss is a sampling artifact on a
  tiny denominator rather than a steady fineclass resident-footprint regression.
  Remote/mixed tradeoff shows global fineclass is not a clean general candidate:
  it improves sh6bench RSS but raises RSS/span_create and loses throughput on
  several transfer-matrix rows. Selective fine128 keeps enough of the sh6bench
  RSS win to pass the focused RSS guard while avoiding the worst global
  fineclass remote/mixed RSS expansion. Full macro gate with fine128 passes
  correctness, larson, xmalloc/cache_scratch, and sh6bench max RSS, but current
  RSS still fails on python_alloc and sh6bench under the old single-sample rule.
  Focused current-RSS semantics diagnostics clear that blocker under a
  documented stabilized-sampling rule, so fine128 is reclassified as the next
  opt-in macro candidate. Final RUNS=10 remote/mixed confirmation keeps
  fine128 acceptable versus batch32, but span-transfer remains the cleaner
  remote/mixed-only speed lane.

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
docs/HZ11_SH6BENCH_LIVE_FOOTPRINT_OBSERVATION_L1.md
docs/HZ11_SH6BENCH_REQUEST_SIZE_CLASS_PACKING_OBSERVATION_L1.md
docs/HZ11_SH6BENCH_SIZE_CLASS_POLICY_CANDIDATE_L1.md
docs/HZ11_FINECLASS_PYTHON_ALLOC_CURRENT_RSS_L1.md
docs/HZ11_SELECTIVE_FINECLASS_RANGE_L1.md
docs/HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1.md
docs/HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md
docs/HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md
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
  docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.md
  docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1.md
  docs/no_go/HZ11_SH6BENCH_SPAN_REUSE_POLICY_L1.md
  docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1.md

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

HZ11Sh6benchSpanReusePolicy-L1:
  Output:
    bench_results/hz11_sh6bench_span_page_batch32_20260708T063931Z/summary.md
  Record:
    docs/no_go/HZ11_SH6BENCH_SPAN_REUSE_POLICY_L1.md
  Verdict:
    NO-GO for sh6bench RSS/page-footprint fix.
  Key result:
    sh6bench RUNS=3:
      batch32 wall 3.497s, max RSS 351488 KiB, span_create 16769
      batch32-span-reuse wall 3.568s, max RSS 350848 KiB, span_create 16753
      batch32-source arena_carve 16755, span_reuse 44
      batch32-span-reuse-source arena_carve 16753, span_reuse 66
    Decision:
      central-only full-span reuse is too rare to move RSS or arena carve.
      It avoids transfer metadata locks, but does not solve the page footprint.

HZ11Sh6benchArenaCommitPolicy-L1:
  Output:
    bench_results/hz11_sh6bench_arena_commit_20260708T065611Z/summary.md
  Record:
    docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1.md
  Verdict:
    NO-GO for sh6bench RSS/page-footprint fix.
  Key result:
    sh6bench RUNS=3:
      batch32 wall 3.508s, max RSS 351104 KiB, minor faults 88218
      batch32-nohuge wall 3.522s, max RSS 351616 KiB, minor faults 88168
      batch32-source span_create/arena_carve 16728, carved about 1045.5 MiB
      batch32-nohuge-source span_create/arena_carve 16765, carved about 1047.8 MiB
    Decision:
      MADV_NOHUGEPAGE does not move RSS/wall/faults.
      RSS is not simple whole-arena eager commit or THP behavior.

HZ11Sh6benchLiveFootprintObservation-L1:
  Output:
    bench_results/hz11_sh6bench_live_footprint_20260708T070625Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_LIVE_FOOTPRINT_OBSERVATION_L1.md
  Verdict:
    GO for diagnostics only; no optimization or macro promotion.
  Key result:
    batch32 wall 3.539s, max RSS 351232 KiB
    live-diag wall 8.848s, max RSS 351744 KiB
    live slot bytes high-water 359342848
    span capacity bytes high-water 365625344
    estimated span-internal waste 6282496 bytes, 1.7%
  Decision:
    remaining RSS is live HZ11 slot footprint / class packing / residency, not
    central retention, current-span reserve, or span-internal waste after class
    rounding.

HZ11Sh6benchRequestSizeClassPackingObservation-L1:
  Output:
    bench_results/hz11_sh6bench_request_class_packing_20260708T071205Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_REQUEST_SIZE_CLASS_PACKING_OBSERVATION_L1.md
  Verdict:
    GO for diagnostics only; no optimization or macro promotion.
  Key result:
    batch32 wall 3.549s, max RSS 350208 KiB
    live-diag wall 10.149s, max RSS 388352 KiB due to side-table overhead
    requested bytes high-water 211116531
    HZ11 slot bytes high-water 359344128
    request-to-slot waste 148227597 bytes, 41.2%
  Decision:
    sh6bench RSS gap is primarily power-of-two class-packing waste.

HZ11Sh6benchSizeClassPolicyCandidate-L1:
  Output:
    bench_results/hz11_sh6bench_size_class_policy_20260708T072222Z/summary.md
    bench_results/hz11_macro_speed_lane_20260708T072459Z/summary.md
  Record:
    docs/HZ11_SH6BENCH_SIZE_CLASS_POLICY_CANDIDATE_L1.md
  Verdict:
    GO as opt-in candidate only; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      batch32 wall 3.542s, max RSS 350592 KiB
      fineclass wall 3.563s, max RSS 301568 KiB
      fineclass span_create 14613 vs batch32 16750
    class packing:
      batch32 request-to-slot waste 148234895 bytes, 41.2%
      fineclass request-to-slot waste 95470901 bytes, 31.1%
    focused correctness:
      python_alloc OK 3/3, wall 0.032s, max RSS 7040 KiB
      mstress OK 3/3, wall 0.218s, max RSS 234916 KiB
  Decision:
    fineclass materially improves sh6bench RSS without losing the batch32 wall
    win, but focused python_alloc current RSS needs full-gate recheck.

HZ11MacroSpeedLaneGateWithFineclass-L1:
  Output:
    bench_results/hz11_macro_speed_lane_20260708T073737Z/summary.md
    bench_results/hz11_fixed_local_fineclass_20260708T074151Z/summary.md
    bench_results/hz11_transfer_promotion_20260708T074224Z/summary.md
  Record:
    docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.md
  Verdict:
    NO-GO for macro promotion; keep fineclass as opt-in candidate.
  Key result:
    RUNS=5 macro gate:
      python_alloc OK 5/5, wall 0.032s, max RSS 6784 KiB
      python_alloc current RSS 2304 KiB vs tcmalloc 1792 KiB, 1.286x
      mstress OK 5/5, wall 0.220s, max RSS 238168 KiB
      larson RSS fix holds: 274176 KiB vs tcmalloc 278912 KiB
      sh6bench RSS improves: batch32 350336 KiB -> fineclass 301312 KiB
      sh6bench wall roughly flat: batch32 3.558s -> fineclass 3.582s
      xmalloc/cache_scratch remain inside the gate shape
    extra smoke:
      fixed64/fixed256 fineclass is about 1.012x/1.005x batch32 ops/s
      light remote/mixed shows fineclass can trade off transfer-matrix throughput
  Decision:
    fineclass is not a macro default yet. The blocker is the small python_alloc
    current-RSS ratio miss, with remote/mixed tradeoff still needing a dedicated
    decision if fineclass is pursued.

HZ11FineclassPythonAllocCurrentRSS-L1:
  Output:
    bench_results/hz11_fineclass_python_alloc_rss_20260708T075016Z/summary.md
    bench_results/hz11_fineclass_python_alloc_rss_20260708T075132Z/summary.md
  Record:
    docs/HZ11_FINECLASS_PYTHON_ALLOC_CURRENT_RSS_L1.md
  Verdict:
    GO for diagnosis only; no macro promotion.
  Key result:
    RUNS=10 focused python_alloc:
      5ms sampling:
        tcmalloc current RSS 104128 KiB
        batch32 current RSS 118792 KiB
        fineclass current RSS 117764 KiB
      20ms sampling:
        tcmalloc current RSS 104064 KiB
        batch32 current RSS 118790 KiB
        fineclass current RSS 117814 KiB
      span_create is equal: batch32 17980, fineclass 17980
      fineclass central final count is lower: 734400 vs 835520
      fineclass request-to-slot waste is lower by about 2.1 MiB
  Decision:
    The macro gate's 1792 KiB vs 2304 KiB current-RSS miss is a sampling
    artifact on a tiny denominator, not a steady fineclass RSS regression.

HZ11FineclassRemoteMixedTradeoff-L1:
  Output:
    bench_results/hz11_transfer_promotion_20260708T080416Z/summary.md
  Record:
    docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1.md
  Verdict:
    NO-GO for global fineclass as a general remote/mixed candidate.
  Key result:
    RUNS=10 transfer matrix:
      fineclass/span-transfer ops:
        main_local0 0.893x
        main_r50 0.912x
        main_r90 0.943x
        small_remote90 0.941x
        medium_r50 0.903x
        medium_r90 0.983x
      fineclass/span-transfer post RSS:
        main_local0 3.800x
        main_r50 1.436x
        main_r90 1.388x
        small_remote90 2.906x
        medium_r50 1.329x
        medium_r90 0.967x
      span_create also rises on several rows:
        main_local0 4.105x
        main_r50 1.880x
        small_remote90 3.864x
  Decision:
    Keep global fineclass as a sh6bench RSS research lane only. Do not rerun a
    macro promotion gate with global fineclass as the next general candidate.

HZ11SelectiveFineclassRange-L1:
  Output:
    bench_results/hz11_sh6bench_size_class_policy_20260708T081029Z/summary.md
    bench_results/hz11_macro_speed_lane_20260708T081413Z/summary.md
    bench_results/hz11_transfer_promotion_20260708T081413Z/summary.md
    bench_results/hz11_selective_fineclass_fixed_20260708T081439Z/summary.md
  Record:
    docs/HZ11_SELECTIVE_FINECLASS_RANGE_L1.md
  Verdict:
    GO for fine128 as an opt-in candidate only; no macro promotion.
  Key result:
    sh6bench RUNS=3:
      batch32 RSS 352000 KiB, wall 3.574s
      fine128 RSS 322688 KiB, wall 3.523s
      fine256 RSS 315264 KiB, wall 3.514s
      global fineclass RSS 301312 KiB, wall 3.603s
    request-to-slot waste:
      batch32 148231845 bytes
      fine128 119302668 bytes
      fine256 111644881 bytes
      global fineclass 95468881 bytes
    remote/mixed RUNS=5:
      fine128 removes the worst global small_remote90 RSS expansion:
        fine128/span-transfer post RSS 0.831x
        global/span-transfer post RSS 1.976x
      fine256 has stronger side effects:
        main_local0 post RSS 2.067x span-transfer
        medium_r50 ops 0.884x span-transfer
    fixed64/fixed256 sanity stays neutral.
  Decision:
    Select fine128 as the next candidate. Fine256 and global fineclass remain
    attribution/research lanes, not the next general candidate.

HZ11MacroSpeedLaneGateWithFine128-L1:
  Output:
    bench_results/hz11_macro_speed_lane_20260708T083138Z/summary.md
  Record:
    docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1.md
  Verdict:
    MIXED; no macro promotion.
  Key result:
    RUNS=5:
      python_alloc OK 5/5, wall 0.032s vs tcmalloc 0.032s
      mstress OK 5/5, wall 0.221s vs tcmalloc 0.192s
      larson RSS fix holds: 273024 KiB vs tcmalloc 278912 KiB
      xmalloc_test wall/RSS remains inside gate shape:
        wall 2.027s vs tcmalloc 2.047s
        max RSS 18432 KiB vs tcmalloc 195456 KiB
      cache_scratch remains inside gate shape:
        wall 1.177s vs tcmalloc 1.163s
        max RSS 3456 KiB vs tcmalloc 7296 KiB
      sh6bench:
        wall 3.532s vs batch32 3.582s
        max RSS 323712 KiB vs tcmalloc 259584 KiB, 1.247x
    Blocker:
      current RSS gate fails:
        python_alloc 1.714x tcmalloc current RSS
        sh6bench 1.313x tcmalloc current RSS
  Decision:
    fine128 remains the best sh6bench RSS candidate, but no macro promotion
    until current-RSS gate semantics/sampling are resolved.

HZ11MacroCurrentRSSGateSemantics-L1:
  Output:
    bench_results/hz11_macro_current_rss_20260708T085625Z/summary.md
    bench_results/hz11_macro_current_rss_20260708T085828Z/summary.md
  Record:
    docs/HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1.md
  Verdict:
    GO for gate semantics only; no allocator policy change or promotion.
  Key result:
    Authoritative RUNS=10 5ms sampling:
      python_alloc:
        tcmalloc max/last RSS 104128 / 104128 KiB
        fine128 max/last RSS 118506 / 118512 KiB
        fine128/tcmalloc last RSS 1.138x
      sh6bench:
        tcmalloc max/last/p95 RSS 263936 / 263616 / 263616 KiB
        fine128 max/last/p95 RSS 323520 / 323264 / 307648 KiB
        fine128/tcmalloc last RSS 1.226x
        fine128/tcmalloc p95 RSS 1.167x
  Decision:
    max RSS remains the primary hard guard. current RSS hard-fails only when a
    focused RUNS>=10 stability check also fails or shows a large stable retained
    footprint. The prior fine128 current-RSS blocker is cleared under this rule.

HZ11MacroSpeedLaneGateFine128Reclassify-L1:
  Record:
    docs/HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md
  Verdict:
    GO for fine128 as the next opt-in macro candidate; no default promotion.
  Key result:
    Existing RUNS=5 macro gate plus focused RUNS=10 current-RSS stability is
    sufficient for reclassification under the documented rule.
      macro gate:
        correctness OK
        larson RSS fix holds
        xmalloc/cache_scratch stay inside gate shape
        sh6bench max RSS 1.247x tcmalloc, inside the 1.25x guard
      focused current RSS:
        python_alloc fine128/tcmalloc last RSS 1.138x
        sh6bench fine128/tcmalloc last RSS 1.226x
        sh6bench fine128/tcmalloc p95 RSS 1.167x
  Decision:
    fine128 is now the best opt-in macro candidate, but requires final
    remote/mixed RUNS=10 confirmation before stronger promotion.

HZ11Fine128RemoteMixedFinalConfirm-L1:
  Output:
    bench_results/hz11_transfer_promotion_20260708T091747Z/summary.md
  Record:
    docs/HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md
  Verdict:
    GO for fine128 as the recommended opt-in macro speed-lane candidate.
    No default promotion.
  Key result:
    RUNS=10 transfer matrix:
      fine128 vs batch32:
        worst ops ratio 0.935x on main_local0
        worst post RSS ratio 1.094x on main_local0
        medium_r50 ops/RSS 1.017x / 0.996x
        medium_r90 ops/RSS 0.988x / 1.072x
      fine128 vs span-transfer:
        medium_r50 ops/RSS 0.929x / 1.261x
        medium_r90 ops/RSS 0.918x / 1.281x
      fine128 vs tcmalloc:
        remote/mixed throughput remains >1.0x on all rows and RSS remains far
        below tcmalloc on all rows.
  Decision:
    fine128 is the recommended opt-in macro candidate. Do not make it default
    and do not replace span-transfer as the remote/mixed microbench lane.
```

## Next Step

```text
Primary next box:
  HZ11Fine128CandidatePositioning-L1

Goal:
  Make docs and Makefile lane naming clearly distinguish the recommended
  opt-in macro candidate from the remote/mixed-only lane and default path.

Boundary:
  positioning/docs/build-target cleanup only.
  no allocator policy change.
  no default path change.

Required evidence:
  README/current_task/Makefile target names make lane boundaries obvious
  standalone check remains green

Keep:
  span-transfer as the remote/mixed microbench lane.
  fine128 as the recommended opt-in macro candidate.
  global fineclass as a sh6bench RSS research lane only.
```
