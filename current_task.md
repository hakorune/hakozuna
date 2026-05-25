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

LargeFront Policy-L1a:
  implemented earlier as a diagnostic.
  128K source refill batch selector only.
  slow path only; no remote batch cap adaptation yet.

LargeFront Policy-L7:
  implemented and measured as a first rule-based remainder policy.
  no-go for promotion:
    t4/r50 regressed hard.
    r90 did not recover enough.

LargeFront Policy-L8 shadow:
  implemented and smoke-tested.
  no behavior change.
  extends Policy-L0 owner-drain counters with slow-path-only shadow
  classification:
    heavy vs sparse drains
    local-like vs hold-like vs republish-like vs mixed drains
  goal:
    check whether r50-like and r90-like phases are separable before another
    runtime policy is attempted.
  first smoke:
    private/raw-results/linux/hz5_large128_policy_l8_shadow_smoke_r1
    t4/r50 and t4/r90 both show republish-like dominance.
    current L8 sink-dominance classifier is useful instrumentation, but not
    yet a clean selector.

LargeFront global-remote diagnostic:
  implemented and measured.
  128K remote frees bypass owner inbox and go to global recycle as LOCAL_FREE.
  purpose:
    test whether large128/t4/r90 is fundamentally owner-inbox churn.
  safety:
    keep fail-closed ACTIVE -> LOCAL_FREE CAS.
    global pop reassigns owner on reuse.
  result:
    private/raw-results/linux/hz5_large128_global_remote_r90_r3
    no-go.
    global lock/recycle is much slower than owner inbox/source16 on r90.

LargeFront remote-first diagnostic:
  implemented and measured.
  128K alloc checks owner inbox before local free list.
  purpose:
    test whether large128/t4/r90 is delayed by local-free reuse ahead of
    remote inbox drain.
  result:
    private/raw-results/linux/hz5_large128_remote_first_r90_r3
    no-go.
    t8/r90 improves slightly, but t4/r90 collapses.
    owner-inbox early drain has too much low-thread fixed cost.

LargeFront gated remote-first diagnostic:
  implemented and measured.
  128K alloc checks owner inbox before local free list only if inbox is
  nonempty.
  purpose:
    preserve t8/r90 remote-first benefit while avoiding t4 fixed-cost collapse.
  result:
    private/raw-results/linux/hz5_large128_remote_first_gated_r90_r3
    no-go.
    worse than source16 on both t4/r90 and t8/r90.

LargeFront chunk inbox diagnostic:
  implemented and measured.
  128K remote batch publishes span pointers as owner-inbox chunks instead of
  span linked lists.
  purpose:
    reduce source16 owner-inbox list traversal / remainder pointer chasing.
  result:
    private/raw-results/linux/hz5_large128_chunk16_r3
    no-go.
    chunk metadata/pool and drain overhead are heavier than source16 list
    inbox; RSS also regresses.

Lane naming cleanup:
  committed L7 as a diagnostic checkpoint.
  current cleanup adds human-facing aliases while preserving historical names:
    large128-rss
    large128-source16
    large128-r50-drain
    large128-r50-hold
    large128-policy-l7
    large128-policy-l8-shadow
  use the human aliases in new commands and reports.

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
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
```

## Latest Policy-L0 Run

Result root:

```text
private/raw-results/linux/hz5_largefront_policy_l0_r3
RUNS=3
profiles=batch4,batch16,rb64
lanes=cross128,large128
threads=2,4,8
remote=50,90
```

Read:

```text
Best profile still changes by row:
  cross128/t2:
    batch16 wins r50/r90

  cross128/t4:
    rb64 wins r50
    batch4 wins r90

  cross128/t8:
    rb64 wins r50
    batch16 wins r90

  large128:
    batch16 wins t2/t4/t8 r50
    rb64 wins t2/t8 r90
    batch4 wins t4 r90

Policy-L0 features are useful:
  batch4 has high source_refill count but low source batch size.
  batch16 lowers source_refill count but can overfetch large128.
  rb64 exposes remote_flush/cap_hit pressure directly.

Decision:
  L1 cannot be a single static replacement.
  Next L1 should start as a conservative slow-path selector:
    source_batch = 4/8/16 from source/refill pressure and mapped proxy
    remote cap adaptation remains diagnostic until the low-thread rows are safe
```

## Policy-L1a Scope

```text
Name:
  LargeFront Policy-L1a

Lane:
  --linux-hz5-profile-pagerun64-large128-policy-l1a

Scope:
  LargeFront 128K class only.
  Source refill batch count only.

Selector:
  high mapped/source pressure:
    batch4

  mid mapped/source pressure:
    batch8

  low mapped/source pressure:
    batch16

Still diagnostic:
  remote batch cap rb32/rb64
  owner drain policy
  payload scavenging

No hot-path policy:
  malloc/free must not read or update policy counters.
```

## Policy-L1a Next Step

```text
Problem:
  first L1a smoke keeps cross128/t8/r90 healthy, but large128/t8/r90 is weak.

Likely cause:
  L1a falls from batch16 toward batch8/batch4 too early for high-thread
  large128 remote-heavy rows.

Next implementation:
  raise L1a mapped-span thresholds before adding remote-cap adaptation.

Still not doing:
  rb32/rb64 automatic switching.
  owner drain policy changes.
  hot-path policy reads.

Result:
  raised thresholds:
    mid spans 4096
    high spans 8192

  short r2 smoke:
    large128/t8/r90 improves versus first L1a smoke.
    cross128/t4/r90 regresses.

Read:
  source-batch-only L1a is useful as a diagnostic, but not enough for a
  broad replacement.
  Next attack should add remote-cap policy observation/selector separately,
  still outside malloc/free hot paths.
```

## Policy-L1b Prep

```text
Goal:
  prepare remote batch cap control without adding a global hot-path policy read.

Implementation step:
  cache LargeFront remote batch cap in TLS.
  initialize it from HZ5_LARGEFRONT_REMOTE_BATCH_CAP.
  remote batch push checks the TLS value.

Later selector:
  update TLS cap only at slow boundaries:
    batch flush
    owner drain
    source refill / checkpoint

Still diagnostic:
  actual rb32/rb64 automatic switching is not enabled until low-thread rows are
  safe.
```

## Policy-L1b Scope

```text
Name:
  LargeFront Policy-L1b

Lane:
  --linux-hz5-profile-pagerun64-large128-policy-l1b

Scope:
  Policy-L1a source selector plus 128K remote batch cap selector.

Selector:
  starts cap16.
  cap-hit flush raises toward cap32/cap64.
  small owner/class-switch flush lowers back toward cap16.

Boundary:
  cap is cached in TLS.
  updates happen at remote flush boundaries.
  malloc/free do not read a global policy object.

Smoke:
  private/raw-results/linux/hz5_policy_l1b_smoke_r2

Read:
  large128/t4/r90 improves versus L1a v2.
  cross128/t8/r90 regresses badly.

Decision:
  L1b is useful as a diagnostic, but not promotable yet.
  Remote cap selection needs a stronger guard before cap growth in cross-size
  high-thread rows.

Follow-up guard variants:
  cap32:
    recovers cross128/t8/r90 versus initial L1b, but hurts cross128/t4/r90
    and large128/t4/r90.

  cap32 + two-hit growth:
    cross128/t8/r90 improves to 26.14M, but large128/t4/r90 collapses
    to 7.18M.

Decision:
  stop L1b micro-tuning for now.
  Runtime remote-cap adaptation is too unstable with the current feature set.
  Keep fixed rb32/rb64 lanes as diagnostics, not as an auto policy.

Next attack:
  either improve L0/L1 features before another L1b attempt, or return to the
  stronger fixed profile split plus source-batch-only L1a.
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

## Latest Large128 Source/Remote Sweep

```text
result root:
  private/raw-results/linux/hz5_large128_batch_sweep_after_l1b_nogo_r3

scope:
  RUNS=3
  threads=4,8
  lane=large128
  remote=50,90
  allocators=hz4,tcmalloc,large128,b8,b16,b16-drain1,b16-rb32,b16-rb64
```

Read:

```text
large128/t4/r50:
  tcmalloc wins at 28.88M.
  HZ5 best is b16-drain1 at 24.36M with very low RSS.

large128/t4/r90:
  saved large128 batch4 wins at 22.25M.
  tcmalloc is 15.30M.

large128/t8/r50:
  saved large128 batch4 wins at 30.04M.
  tcmalloc is 23.45M.

large128/t8/r90:
  batch16 wins at 24.53M.
  tcmalloc is 14.73M.
```

Decision:

```text
fixed source-batch profiles explain the remaining large128 rows better than
L1b runtime remote-cap adaptation.

Keep:
  large128 batch4 as large-only default candidate
  batch16 as high-thread/r90 diagnostic
  b16-drain1 as t4/r50 diagnostic

Do not:
  promote L1b remote-cap auto policy yet
  keep tuning cap16/32/64 without stronger features
```

Next attack:

```text
focus on the one remaining gap:
  large128/t4/r50 versus tcmalloc.

Candidate:
  make drain1 less workload-specific:
    source batch16
    take-first remote span
    bounded extra drain only when remote inbox is already hot
    preserve r90 rows by avoiding extra local churn under r90
```

## LargeFront Take-Only Diagnostic

```text
lane:
  --linux-hz5-profile-pagerun64-large128-b16-takeonly

result root:
  private/raw-results/linux/hz5_large128_b16_takeonly_smoke_r3

scope:
  RUNS=3
  threads=4,8
  lane=large128
  remote=50,90
```

Read:

```text
large128/t4/r50:
  takeonly wins at 23.07M with 23MB RSS.

large128/t4/r90:
  takeonly collapses to 6.71M.

large128/t8/r50:
  takeonly is behind batch16 and tcmalloc.

large128/t8/r90:
  takeonly collapses to 7.49M.
```

Decision:

```text
take-first-only confirms the t4/r50 drain direction, but it is not promotable.
It starves r90 reuse and increases RSS in r90.

Keep as diagnostic only.
Do not add more take-only variants unless perf shows a specific t4/r50 paper
claim needs it.
```

## LargeFront t4/r50 Perf Read

```text
result root:
  private/raw-results/linux/hz5_large128_t4r50_perf_20260526_032727

scope:
  large128/t4/r50
  tcmalloc vs batch16 vs b16-drain1 vs b16-takeonly
```

Perf stat read:

```text
tcmalloc:
  27.70M ops/s
  254.9 instructions/op
  52.1 branches/op

HZ5 b16-drain1:
  19.40M ops/s
  362.2 instructions/op
  76.8 branches/op

HZ5 b16-takeonly:
  17.38M ops/s
  397.9 instructions/op
  86.2 branches/op

HZ5 batch16:
  12.14M ops/s
  453.9 instructions/op
  100.5 branches/op
```

Perf report read:

```text
largest HZ5 self hotspot:
  hz5_largefront_drain_remote_class_budget

next visible HZ5 costs:
  hz5_largefront_free
  hz5_largefront_alloc
  hz5_largefront_span_for_ptr

Interpretation:
  the large128/t4/r50 gap is not primarily cache misses.
  HZ5 still pays too much remote drain and ownership lookup path length.
```

## LargeFront PopBudget Diagnostic

```text
lane:
  --linux-hz5-profile-pagerun64-large128-b16-popbudget1

result root:
  private/raw-results/linux/hz5_large128_b16_popbudget1_smoke_r3

idea:
  instead of exchange(NULL) + tail traversal + republish remainder, pop only
  the needed remote spans from the owner inbox with CAS and leave the rest in
  the inbox.
```

Read:

```text
large128/t4/r50:
  popbudget1 15.08M
  batch16    17.77M
  drain1     17.20M
  tcmalloc   25.27M

large128/t8/r90:
  popbudget1 15.21M
  batch16    19.85M
  tcmalloc   15.60M
```

Decision:

```text
no-go.
Tail traversal/republication is not the main issue.
Small CAS-pop drains are also too expensive and do not preserve the strong
batch16/t8/r90 row.
```

## Next Design: LargeFront-L6 RemoteHold

```text
Goal:
  keep the takefirst win, but avoid both bad remainder choices:
    republish remainder every miss
    convert too many remote spans to LOCAL_FREE

Design:
  owner TLS keeps bounded remote_hold[class] list.
  held spans remain REMOTE_PENDING.
  alloc miss checks remote_hold before owner inbox.
  inbox drain takes one span ACTIVE and stashes remainder up to a small cap.

Why:
  b16-drain1 is closest to tcmalloc on large128/t4/r50.
  takeonly proves takefirst can be fast but r90 starves.
  popbudget1 proves small CAS-pop from the inbox is not the answer.
  remote_hold keeps exchange-based inbox drain but avoids hot republish/local
  churn for the remainder.

First lanes:
  b16-remotehold4:
    take first ACTIVE
    stash remainder up to 4
    no immediate local_free conversion

  b16-drain1-hold4:
    take first ACTIVE
    convert one extra span to LOCAL_FREE
    stash remainder up to 4
```

Result:

```text
root:
  private/raw-results/linux/hz5_large128_remotehold4_smoke_r3

large128/t4/r50:
  remotehold4       12.97M
  drain1-hold4      12.64M
  batch16           16.41M
  tcmalloc          29.45M

large128/t4/r90:
  remotehold4        8.42M
  drain1-hold4       9.53M
  batch16           22.11M
  tcmalloc          23.25M

large128/t8/r50:
  drain1-hold4      25.32M
  tcmalloc          24.30M
  batch16           21.87M

large128/t8/r90:
  remotehold4        9.21M
  drain1-hold4       8.95M
  batch16           19.82M
  batch4            23.00M
```

Decision:

```text
RemoteHold cap4 is not a broad answer.
It helps t8/r50 in drain1-hold4, but it badly hurts t4/r50 and r90.
Do not promote.

Read:
  keeping REMOTE_PENDING spans in owner TLS can help some producer pressure
  rows, but r90 needs more immediate reusable local/free spans.
  The remaining t4/r50 gap is not solved by a simple owner-local remote stash.
```

## Latest Policy-L0 Owner Drain Detail

```text
code change:
  Policy-L0 owner drain counters now include:
    owner_hold
    owner_orphan
    owner_state_fail

result roots:
  private/raw-results/linux/hz5_largefront_policy_l0_ownerdetail_20260526_041526
  private/raw-results/linux/hz5_largefront_policy_l0_ownerdetail_variants_20260526_041555
```

Key rows:

```text
saved large128 batch4, t4/r50:
  owner_drain=2998
  owner_drain_spans=111632
  owner_take_first=2998
  owner_to_local=108634
  owner_republish=0

b16-drain1, t4/r50:
  owner_drain=55161
  owner_drain_spans=1821037
  owner_take_first=55161
  owner_to_local=55092
  owner_republish=1710784

b16-drain1, t4/r90:
  owner_drain=99700
  owner_drain_spans=5238351
  owner_take_first=99700
  owner_to_local=99623
  owner_republish=5039028

b16-drain1-hold4, t4/r50:
  owner_drain=19402
  owner_drain_spans=691535
  owner_take_first=19402
  owner_to_local=19361
  owner_hold=77217
  owner_republish=575555

b16-drain1-hold4, t4/r90:
  owner_drain=30156
  owner_drain_spans=1686100
  owner_take_first=30156
  owner_to_local=30099
  owner_hold=119935
  owner_republish=1505910
```

Read:

```text
The large128 drain hotspot is dominated by repeated remote remainder churn.
b16-drain1 repeatedly exchanges large inboxes, takes/locals one or two spans,
then republishes most of the list.

RemoteHold reduces churn but does not eliminate it, and its REMOTE_PENDING hold
starves r90 reuse.

Next design should avoid repeatedly reprocessing the same remainder list.
Do not keep tuning hold cap alone.
```

## RemoteHold Drain-Budget Consumption

```text
change:
  remote_hold is no longer consumed as ACTIVE-only.
  alloc miss now drains remote_hold with the same local budget:
    take one REMOTE_PENDING -> ACTIVE
    convert up to HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET to LOCAL_FREE

result root:
  private/raw-results/linux/hz5_large128_remotehold4_drainbudget_smoke_r3
```

Result:

```text
large128/t4/r50:
  drain1-hold4 25.09M /  34MB
  tcmalloc     26.53M /  53MB

large128/t4/r90:
  drain1-hold4  7.63M / 139MB
  batch16      22.93M /  41MB
  tcmalloc     25.41M /  59MB

large128/t8/r50:
  drain1-hold4 24.84M /  66MB
  tcmalloc     23.66M /  93MB

large128/t8/r90:
  drain1-hold4 10.70M / 185MB
  batch16      20.42M / 108MB
  tcmalloc     14.12M / 163MB
```

Policy-L0 detail:

```text
root:
  private/raw-results/linux/hz5_largefront_policy_l0_drainbudget_hold4_20260526_042346

drain1-hold4 t4/r50:
  owner_drain=18261
  owner_drain_spans=499743
  owner_to_local=18222
  owner_hold=72655
  owner_republish=390605

drain1-hold4 t4/r90:
  owner_drain=31474
  owner_drain_spans=1183875
  owner_to_local=31427
  owner_hold=125224
  owner_republish=995750
```

Decision:

```text
keep as r50 diagnostic/candidate.
It nearly closes t4/r50 and wins t8/r50, with lower RSS than tcmalloc.

Do not use for r90.
r90 still needs batch16/batch4 style immediate reuse; hold still leaves too
much republish/reprocess and delays reuse.
```

## Next Implementation: LargeFront-L7 Policy

```text
Goal:
  move from fixed large128 r50/r90 lanes toward one runtime policy lane.

Scope:
  LargeFront 128K class only.
  Slow drain path only.
  Do not add malloc/free hot-path counters.

First rule:
  use the owner inbox drain itself as the observation point.

  small/moderate remainder:
    use drain1-hold4 behavior
    keep takefirst
    convert one extra span to LOCAL_FREE
    hold up to 4 REMOTE_PENDING spans

  large remainder:
    use immediate reuse behavior
    convert the remainder to LOCAL_FREE instead of holding/republishing

Reason:
  r50 likes drain1-hold4 because it avoids repeated remote remainder churn.
  r90 needs immediate reusable spans; hold delays reuse and collapses.
  The owner inbox remainder length is available on the slow drain path and
  does not require hot-path counters.

Initial threshold:
  if remainder >= 32 spans, switch that drain to immediate local conversion.
  otherwise use hold4.

Lane:
  --linux-hz5-profile-pagerun64-large128-b16-policy-l7
```

Result:

```text
root:
  private/raw-results/linux/hz5_large128_policy_l7_smoke_r3

large128/t4/r50:
  policy-l7 11.36M /  81MB
  drain1    20.19M /  32MB
  tcmalloc  30.09M /  50MB

large128/t4/r90:
  policy-l7 14.64M /  63MB
  batch4    29.08M /  27MB
  tcmalloc  26.42M /  61MB

large128/t8/r50:
  policy-l7 24.42M /  74MB
  batch16   23.64M /  76MB
  tcmalloc  26.67M /  81MB

large128/t8/r90:
  policy-l7 14.64M / 147MB
  batch16   19.21M / 109MB
  tcmalloc  14.55M / 179MB
```

Decision:

```text
no-go as a broad policy.
Remainder length threshold 32 is too crude:
  it hurts t4/r50
  it does not recover r90 enough
  it only helps t8/r50 modestly

Next L7 attempt needs a stronger phase signal than current drain remainder
length alone, or should remain a fixed profile split.
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
