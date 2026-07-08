# HZ11 Current Task

```text
Active status:
  HZ11CentralPolicyCorrectness-L1 is measured as focused correctness GO.

Current stance:
  HZ11 remains a speed-first research line.
  libhz11_span_transfer.so is the remote/mixed microbench speed-lane candidate.
  libhz11_span_transfer_thread_exit.so fixes larson thread-churn RSS, but is
  not a macro default.

Macro promotion blockers:
  python_alloc and mstress pass under the thread-exit-cap candidate, but that
  is still a fixed-cap sibling rather than a final/default central policy.
  sh6bench remains far slower than tcmalloc and over the RSS guard.

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
docs/HZ11_NO_GO_LEDGER.md

NO-GO / attribution records:
  docs/no_go/README.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_GATE_L1.md
  docs/no_go/HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md
  docs/no_go/HZ11_SPAN_SOURCE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_L1.md

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
```

## Next Step

```text
Primary next box:
  HZ11MacroSpeedLaneGateWithThreadExitCap-L1

Goal:
  re-run the broader macro gate with hz11-thread-exit-cap after the focused
  correctness rows pass.

Boundary:
  measurement only; keep transfer/thread-exit/thread-exit-cap as opt-in
  candidate lanes.

Required evidence:
  confirm remaining blockers before deciding between sh6bench metadata-lock work
  and replacing fixed cap with a real central policy.

Secondary box:
  HZ11SpanReturnMetadataBatch-L1

Goal:
  if span-return is retried, remove or batch per-object metadata locking.

Reason:
  HZ11SpanSourceAttribution-L1 ruled out sweep/O(n) as sh6bench span-return's
  primary wall-time source.

Keep:
  transfer as the remote/mixed microbench lane only until macro correctness is
  stable.
```
