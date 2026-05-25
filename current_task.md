# Current Task

## Active Status

HZ5 Linux general allocator work is now in the profile-stabilization phase.

```text
MidPage:
  PageRun64 is the current strong keep.

LargeFront:
  128K r50 remote/free behavior is the remaining design target.

Adaptive128:
  first mapped-bytes-only source-batch policy is no-go.

LargeFront observe:
  phase counters did not show a clean cross128 vs large128 signal.

LargeFront Policy-L0:
  new control-plane observation lane.
  slow-path only; no malloc/free hot-path counter updates.
  name is policy-l0, not auto, because it does not select policy yet.

Cleanup checkpoint:
  LargeFront observe and Policy-L0 are intentionally separate.
  shared atomic counter helpers are allowed.
  source batch bucket classification is shared between observe and policy.
  state transitions, region map lookup, source refill lifetime, and owner drain
  ordering are not cleanup targets without a measurement task.
```

Primary registry:

```text
hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
```

## Saved Lanes

```text
hz5-linux-pagerun64-main:
  build alias:
    --linux-hz5-profile-pagerun64-main
  PageRun64 + empty retain cap 4096
  main / mid_only / cross64 candidate

hz5-linux-pagerun64-cross-size:
  build alias:
    --linux-hz5-profile-pagerun64-cross128
  PageRun64 + LargeFront takefirst + source batch16
  cross128 remote-heavy diagnostic

hz5-linux-pagerun64-large-only:
  build alias:
    --linux-hz5-profile-pagerun64-large128
  PageRun64 + LargeFront takefirst + source batch4
  Large-first free route + LargeFront exact-base fast map
  large128 remote-heavy diagnostic
```

## Current Decision

```text
Keep fixed split.
Do not promote mapped-bytes adaptive128.
Do not promote retained-payload scavenging.
Do not build another adaptive source-batch policy without a stronger signal.
Re-check fixed source-batch constants after the Large-first/free-map fix before
designing a new adaptive policy.
```

Reason:

```text
source batch is a real lever, but the best value reverses:
  cross128 wants batch16
  large128 wants batch4

The first adaptive policy used mapped bytes only and failed both rows.
```

## Latest Broad Read

Command/result root:

```text
private/raw-results/linux/hz5_hakmem_compare_pagerun64_broad_r3
RUNS=3
threads=2,4,8
lanes=main,mid_only,cross128,large128
remote=0,50,90
allocators=system,hz4,mimalloc,tcmalloc,pagerun64-main,cross128,large128
```

Read:

```text
HZ5 wins many r50/r90 main and mid_only rows, especially at t=4/t=8.
HZ5 keeps much lower RSS than tcmalloc/mimalloc in most MidPage rows.

Largest remaining gaps:
  main/mid_only r0:
    tcmalloc local fast path is still about 1.8x-2.0x faster.

  large128 r50/r90:
    first targeted fix improved large128 substantially.
    remaining gap is mainly r50 vs tcmalloc/HZ4 depending on thread count.
    perf still points at LargeFront source/refill, page touching, and remote
    handoff shape.

  cross128 r90:
    mostly close at t=4/t=8, but t=2 still has a large HZ4 gap.
```

## Next Work

First:

```text
finish source/lane cleanup:
  keep saved large128 aliases on the new batch helper
  keep rb32/rb64 as diagnostics only
  keep Policy-L0 out of speed lanes
  keep shared counter helpers and batch-bucket helper in largefront.c

Then:

add diagnostic LargeFront-L4 source-batch aliases:
  large128-batch8
  large128-batch16

run large128 r50/r90 batch sweep after the Large-first/free-map fix:
  batch4 saved profile
  batch8 diagnostic
  batch16 diagnostic

keep speed lanes free of stats and observation counters
```

If batch sweep does not explain r50:

```text
prototype LargeFront-L4 drain-budget lane:
  take-first remote span
  drain only a bounded number of extra remote spans to local cache
  avoid full inbox-to-local churn on r50
```

Current likely attack order:

```text
1. LargeFront-L4 source-batch sweep after Large-first/free-map.
2. LargeFront-L4 remote drain budget if batch sweep is not enough.
3. main/mid_only r0 instruction-path reduction only if we decide to keep
   chasing tcmalloc local-only throughput.
4. cross128 r90 t=2 after LargeFront is understood.
```

Latest LargeFront fix:

```text
change:
  LargeFront region exact-base fast map
  pagerun64-large128 uses Large-first free route

result:
  private/raw-results/linux/hz5_large128_largefirst_fastmap_r5

read:
  large128 r0 improved strongly.
  large128 r90 now wins at t=8 and is much closer at t=2/t=4.
  large128 r50 remains the next LargeFront gap.
```

LargeFront-L4 source-batch sweep:

```text
result:
  private/raw-results/linux/hz5_large128_l4_batch_sweep_r3
  private/raw-results/linux/hz5_large128_l4_batch16_confirm_r5
  private/raw-results/linux/hz5_large128_l4_batch16_r0_r5

read:
  batch16 helps several large128 r50/r90 rows after the Large-first/free-map
  fix, especially high-thread rows.
  batch8 is not consistently better than batch4/batch16.
  source batch is still a real lever, but the row-to-row optimum remains noisy.

RUNS=5 confirmation:
  batch16 wins or nearly wins r90 and improves lower-thread r50.
  batch4 still wins t=8 r50 and remains competitive on r0.
  Do not replace the saved batch4 large128 profile yet.
  Keep batch16 as a high-remote/high-thread candidate-watch.
```

LargeFront-L4 drain-budget diagnostic:

```text
result:
  private/raw-results/linux/hz5_large128_l4_drain1_r3

lane:
  hz5-pagerun64-large128-b16-drain1

read:
  no-go as a broad profile.
  t=2 r50 improves, but r90 and t=8 regress badly.
  Do not promote drain1.
```

LargeFront-L4 design rule:

```text
Do not replace the saved large128 profile until a diagnostic lane beats it
broadly on r50/r90 without losing the RSS advantage.

Use fixed diagnostic lanes first. Adaptive policy only comes after a clear
source-batch or drain-budget signal.
```

Next L4 diagnostic:

```text
remote batch cap sweep on the batch16 candidate:
  hz5-pagerun64-large128-b16-rb32
  hz5-pagerun64-large128-b16-rb64

purpose:
  check whether remote publish cost, not drain-to-local budget, is still a
  visible r50/r90 bottleneck.

result:
  private/raw-results/linux/hz5_large128_l4_remote_batch_cap_r3
  private/raw-results/linux/hz5_large128_l4_remote_batch_cap_fixed_r3

read:
  first result invalidated as a remote-batch-cap test because the aliases did
  not enable LargeFront remote batching.
  fixed rerun:
    rb64 wins t=4 r90 and t=8 r90.
    rb32 wins t=8 r50.
    lower-thread r50 remains weak.
  conclusion:
    remote batch cap is another phase-sensitive lever, not a broad replacement.
```

LargeFront Policy-L0 plan:

```text
purpose:
  collect source/refill/remote/drain features needed for a future L1 selector
  without adding learning or counters to malloc/free hot paths.

build:
  --linux-hz5-profile-pagerun64-large128-policy-l0

runtime:
  HZ5_LARGEFRONT_POLICY_L0=1

smoke:
  private/raw-results/linux/hz5_policy_l0_smoke.log

features:
  source_empty
  source_refill / source_spans / batch4/8/16
  mapped_spans_proxy
  remote_batch_flush / spans / cap_hit
  owner_drain / spans / takefirst / to_local / republish

rule:
  L0 is diagnostic only.
  No speed medians from L0.
  L1 source-batch selector only after L0 separates batch4 vs batch16 rows.
```

Do not:

```text
add hot malloc/free counters
mix HZ5_PRELOAD_STATS into speed runs
enable unbounded per-owner span/object caches
touch Windows P43i/P45
change MidPage PageRun64 without a clear regression reason
```

## Active Implementation

```text
LargeFront-L4 source-batch diagnostic:
  flag:
    --linux-hz5-profile-pagerun64-large128-batch8
    --linux-hz5-profile-pagerun64-large128-batch16

  base:
    PageRun64
    LargeFront takefirst
    Large-first free route
    LargeFront exact-base fast map

  behavior:
    source batch changes only for 128K LargeFront profile comparison
    no runtime counters
    no stats in speed lane

  reason:
    old source-batch results were measured before the Large-first/free-map fix
    r50 may now prefer a different batch than the saved batch4 profile

  first result:
    batch16 is the only promising fixed diagnostic.
    drain1 is no-go.
    RUNS=5 confirms batch16 is useful but not broad enough to replace batch4.
```

LargeFront Policy-L0 observation:

```text
flag:
  --linux-hz5-profile-pagerun64-large128-policy-l0

behavior:
  compile slow-path policy counters only
  print with HZ5_LARGEFRONT_POLICY_L0=1
  no alloc/free hot-path updates
  not for performance medians

next:
  run L0 on batch4/batch16/rb64 style rows and compare shadow features against
  measured best fixed profile.

smoke:
  output includes source_empty/source_refill/mapped_spans_proxy and owner_drain
  without HZ5_LARGEFRONT_OBSERVE hot-path counters.
```

## Reading Order

```text
1. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
2. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
3. hakozuna-hz5/docs/HZ5_LARGEFRONT_L1_DESIGN.md
4. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_C7_LANES.md
5. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```

## Last Commits

```text
7d45a08 Optimize LargeFront large128 free route
4f6d157 Record PageRun64 broad comparison read
41c0be8 Update hakmem runner for PageRun64 profiles
f8b767d Add LargeFront adaptive128 diagnostic
3a204d9 Expose LargeFront source batch tuning
```
