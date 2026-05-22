# HZ5 P43i / P43o Algorithm Consultation Draft

## Short Conclusion To Review

Current working judgment:

```text
P43i:
  keep as balanced / RSS-bounded candidate-watch
  not default yet

P43o:
  keep as source-churn evidence/control
  not a replacement for P43i

P44:
  keep as dry-run evidence
  do not implement actual trim under the P43i lockless contract

P43m/P43n:
  RSS lower-bound no-go controls
  do not use as implementation paths

P25a/P33:
  keep as practical speed/RSS baselines
```

The question is no longer "is P43i promising?" It is:

```text
What is the right algorithmic response to P43i's mixed-prelude state-pressure
tail without destroying the P25 bridge that gives P43i its speed?
```

## Important Terms

P25 bridge means the HZ5 lowpage64 hot batching topology:

```text
release_common()
  -> TLS relbuf
  -> global batch
  -> acquired stash
```

It is not HZ3/HZ4 fallback.

P43i is:

```text
P25 bridge
+ P33 stash128
+ P40 global age release
+ P43 2MiB segment slots as lower raw source
+ P43 fast/lockless active lookup
+ P43 prepared bridge
+ SpeedLane / CounterFree

disabled:
  slot decommit
  PAGE_NOACCESS
  runtime segment release
  direct descriptor release
  descriptor release buffer
```

## What The Data Says

P43i is strong enough to keep as candidate-watch:

```text
exact isolated 64K/a8192:
  P43i is fast
  RSS is low
  fallback load_count = 0

safety:
  focused exact repeat-80 clean
  fallback-loaded guard clean
  PAGE_NOACCESS diagnostic clean
  raw_mismatch = 0
```

But P43i is not default-ready:

```text
non-isolated mixed-prelude final 64K:
  low-tail remains

P43i-only repeat-10:
  final 64K min/median/max = 4.92M / 6.94M / 8.43M

isolated workload variant:
  final 64K min/median/max = 6.19M / 13.72M / 14.70M
  steady RSS = 29.69 MiB
```

So the exact route itself is good; same-process state pressure creates the tail.

## What Seems Ruled Out

### Not a simple fallback bug

Fallback-loaded guard is clean:

```text
load_count expected where fallback profile needs it
load_failed = 0
free_miss_unloaded = 0
exact HZ5 route keeps load_count = 0
```

### Not P25 bridge stash bloat

P43k snapshots show P43i does not retain more P25 stash than P25a/P33:

```text
P43k final snapshot:
  acquired_count_max lower than P25/P33
  stash_set_nodes_max lower than P25/P33
```

The extra state is source-side:

```text
p43_segments_current = 16
p43_slots_committed_current = 256
p43_slots_committed_free_current ~95
p43_slots_global_retained_current ~81
p43_slot_commits = 256
p40_release_nodes ~408
os_allocs higher than P25/P33
```

### Not removable by direct descriptor release

P43m/P43n lower RSS but collapse speed:

```text
P43m:
  direct prepared release into P43 source
  lower RSS
  64K speed collapses

P43n:
  descriptor-owned release buffer
  lower RSS
  still P43m speed tier
```

So the P25 bridge is a speed asset, not accidental waste.

### Not a good P44 actual trim target

P44 dry-run sees:

```text
p44_relbuf_candidate_current = 0
p44_candidate_current = 16
p44_p43_free_candidate_current = 16
```

The observed target is P43 committed-free source state, not P25 relbuf/global.
Actual trim would likely require decommit/release/direct source behavior, which
is outside the P43i lockless contract.

## P43o Control

P43o was added as a minimal source-admission control:

```text
P43o = P43i topology
  + BENCHLAB_HZ5_P40_SKIP_WHEN_P43_FREE_CAP=64
```

Algorithm:

```text
at P40 checkpoint:
  observe P43 committed-free slots
  if committed_free >= 64:
    skip P40 global release
  else:
    run normal P40 global release
```

It preserves:

```text
P25 bridge
prepared bridge
lockless contract
no direct descriptor release
no release buffer
no slot decommit
no PAGE_NOACCESS
no runtime segment release
```

P43o confirms the interaction is real:

```text
p40_skip_p43_free_calls = 31 median
p40_skip_p43_free_nodes = 2250 median

p40_release_nodes:
  P43k 440 -> P43o 360

p43_source_alloc_calls:
  P43k 588 -> P43o 546
```

But P43o is not a fix:

```text
mixed-prelude final 64K:
  P43i 7.06M
  P43o 6.55M

exact/rss-bounded:
  r90:
    P43i 9.21M
    P43o 13.65M

  r99:
    P43i 13.30M
    P43o 12.52M

  plateau RSS:
    roughly tied
```

Interpretation:

```text
P40/P43 source-admission is a real control point.
Simple guard64 is too coarse to solve mixed-prelude.
More cap sweeps are probably low ROI.
```

## Algorithm-Level Questions

### 1. Is P43i's mixed-prelude issue best modeled as source-admission churn?

The current model is:

```text
P40 release demotes reusable P25/global bridge nodes into P43 source state.
P43 source then holds committed-free/global slots.
The final exact 64K row sees worse state/cache topology than isolated mode.
```

Is this the right algorithmic model, or should we suspect a different root
cause?

### 2. Should source admission be hysteretic rather than cap-threshold based?

P43o uses a simple threshold:

```text
if P43 committed_free >= 64:
  skip P40 release
```

Would a better algorithm be:

```text
track P43 reuse pressure over epochs:
  committed_free age
  source_alloc_calls per epoch
  free_slot_hits / global_hits ratio
  P40 release pressure

only release bridge/global nodes when:
  P43 committed_free is low
  and recent P43 reuse pressure is low
  and global retained is above hard pressure
```

Or is that too much policy surface for the current evidence?

### 3. Should P40 release target a different state transition?

Current P40 release eventually causes nodes to return to lower source behavior.

Alternative:

```text
P40 release does not immediately demote to P43 source
Instead it marks bridge/global nodes as "cooling" or "do-not-refill-source"
and only lets source reclaim them at a later checkpoint.
```

Would a two-phase source-admission state be cleaner than P43o's skip guard?

### 4. Should P43 source have hot/free admission classes?

Current P43 source state is mostly:

```text
TLS retained
global retained
committed-free
free slot
```

Would it be better to split source-side committed-free into:

```text
hot reusable committed-free:
  recently used by P25 bridge
  should be preferred by alloc

cold committed-free:
  only used after bridge/global misses
  candidate for future decommit/release under a different contract
```

This might avoid P43i's source cache being a flat bag of committed-free slots.

### 5. Should actual RSS trim remain forbidden under P43i?

Current P43i contract forbids:

```text
slot decommit
PAGE_NOACCESS
runtime segment release
direct descriptor release
```

Given P43i's speed comes from the bridge and no-decommit contract, should actual
trim remain blocked until a new contract is explicitly chosen?

Or is there a safe bridge-preserving trim that does not become P43m/P43n again?

### 6. Should we stop P43 tuning here?

Choices:

```text
A. freeze P43i as balanced candidate-watch and stop tuning
B. try one non-cap-sweep source-admission algorithm
C. redesign P43 segment source admission/topology
D. keep P43i/P43o/P44 as evidence and return to P25a/P33 practical baselines
```

## My Leaning

My current leaning is:

```text
freeze:
  P43i as balanced candidate-watch
  P43o as source-churn evidence/control
  P44 as dry-run evidence

do not:
  promote P43i to default
  continue P43o cap sweeps
  implement P44 actual trim under current P43i contract
  use P43m/P43n as implementation paths

if continuing:
  try exactly one non-cap-sweep source-admission design,
  preferably hysteretic/epoch based,
  and keep P25 bridge intact.
```

Question for review:

```text
Is a hysteretic source-admission algorithm the right next experiment,
or should P43i be frozen as candidate-watch and the project move to broader HZ5
design review?
```

## Update: P43p Bridge-Cold Stage1 Follow-Up

After the P43o/P44 review, we tried the non-cap-sweep source-admission path.

The design goal was:

```text
Do not trim.
Do not decommit.
Do not runtime-release segments.
Do not bypass the P25 bridge.

Instead:
  treat P40 release as a bridge-cold / stage1 source-demotion intent.
```

### P43p.2 BridgeColdStage1

P43p.2 changed P40 checkpoint release:

```text
before:
  P40 release candidate -> source release / P43 committed-free

after:
  P40 release candidate -> bridge-cold stage1 queue
  acquire miss -> pop_all stage1
  return first node
  put the rest back into the hot-ish stash topology
```

Observed counters showed the queue was live:

```text
p43p_stage1_enqueue_nodes = 1976
p43p_stage1_acquire_nodes = 1920
p43p_stage1_current       = 56
```

But the behavior was mixed/no-go:

```text
exact/rss-bounded:
  improved some r99/RSS rows
  regressed r90

mixed-prelude:
  final 64K regressed versus P43i
```

Interpretation:

```text
bridge-cold is a real control point,
but putting all P40 release candidates into stage1 and then reinjecting them
wholesale is too blunt.
```

### P43p.3a Stage1LimitedAcquire

P43p.3a kept P43p.2 stage1 queueing but limited acquire-miss reinjection:

```text
stage1-pop1:
  acquire at most 1 stage1 node per miss

stage1-pop4:
  acquire at most 4 stage1 nodes per miss

remaining stage1 nodes:
  requeued, not pop_all/stash-reinjected
```

This still preserves the P43i contract:

```text
no trim
no slot decommit
no PAGE_NOACCESS
no runtime segment release
no direct descriptor release
no descriptor relbuf
P25 bridge remains intact
```

### P43p.3a repeat-10 readout

On `hz5-lowpage64-rss-bounded` repeat-10:

```text
P43i:
  64K/r90 14.30M
  64K/r99 11.85M
  plateau steady RSS 61.52 MiB

stage1-pop1:
  64K/r90 14.38M
  64K/r99 13.87M
  plateau steady RSS 61.12 MiB

stage1-pop4:
  64K/r90 10.68M
  64K/r99 11.26M
  plateau steady RSS 61.39 MiB
```

On `hz5-p43-mixed-prelude-rss64-rss256-4k8k` repeat-10:

```text
P43i final exact 64K:
  6.89M
  final/steady RSS 92.10 MiB

stage1-pop4 final exact 64K:
  7.06M
  final/steady RSS 92.72 MiB

stage1-pop1 final exact 64K:
  7.49M
  final/steady RSS 91.32 MiB

P43p.2 pop_all final exact 64K:
  7.88M
  final/steady RSS 93.17 MiB
```

On `hz5-a8192-guards` repeat-5:

```text
fallback state:
  load_count = 1 as expected
  load_failed = 0
  free_miss_unloaded = 0

stage1-pop4 guard concerns:
  8K/a4096  5.67M vs P43i 8.28M
  64K/a4096 6.59M vs P43i 7.58M

stage1-pop4 guard positives:
  2K/a8192   15.15M vs P43i 14.10M
  256K/a4096 5.34M vs P43i 5.41M, similar
```

### Updated Interpretation

The new read is:

```text
P43p.2 pop_all:
  no-go/control
  evidence that wholesale bridge-cold reinjection is blunt

P43p.3a pop1:
  exact/rss-bounded evidence
  improves exact r99/r90 in this repeat-10

P43p.3a pop4:
  mixed-prelude evidence
  modest final-64K improvement, but not enough
  weak on some a4096 guard rows

P43i:
  remains the balanced candidate-watch
```

So P43p.3a supports the algorithmic point that reinjection granularity matters,
but it does not cleanly beat P43i enough to become the next balanced lane.

## Updated Review Question

Given this P43p.3a evidence, which direction should HZ5 take next?

```text
A. Stop the P43p bridge-cold track here.
   Keep:
     P43i = balanced candidate-watch
     P43p.2 = pop_all no-go/control
     P43p.3a = limited acquire evidence

B. Try exactly one age-gated stage1 dry-run.
   Goal:
     test whether young stage1 reuse and old stage1 retention/demotion would
     avoid both pop_all pollution and pop4 guard regressions.

C. Implement age-gated stage1 behavior directly.
   This would add epoch/age to stage1 nodes and avoid hot-stash reinjection
   of old stage1 nodes.

D. Move to broader HZ5 core design.
   Treat P43p as evidence that P25 bridge and P43 source need a cleaner
   long-term admission/control-plane separation.
```

My current leaning after repeat-10:

```text
P43i should remain the selected balanced watch lane.

P43p.3a is useful evidence but not a promotion path.

If continuing P43p at all, do only B:
  age-gated stage1 dry-run, not immediate behavior.

If the review thinks the signal is already sufficient, choose A/D and stop
adding P43p knobs.
```

## P43p.4 Age-Gated Stage1 Dry-Run

Implemented the one extra diagnostic requested above as:

```text
P43p.4 AgeStage1DryRun
  behavior unchanged
  extends P43p.1 bridge-cold dry-run
  rejects SpeedLane
  no stage1 queueing
  no trim/decommit/PAGE_NOACCESS/runtime release
  no direct descriptor release
  no descriptor relbuf
```

It adds projected stage1 age buckets and counters:

```text
p43p_age_stage1_items_total
p43p_age_young_current
p43p_age_old_current
p43p_age_would_reuse_young
p43p_age_would_keep_old
p43p_age_would_demote_old_open
p43p_age_would_block_old_drain
p43p_age_would_block_old_closed
p43p_age_hot_reinject_avoided
p43p_age_pop_all_pollution_projection
p43p_age_admission_flips
```

Short readout:

```text
mixed-prelude repeat-3
  run root:
    results/synthetic-sweep/20260522_094523_883

  final exact 64K row / HZ5_LOWPAGE64 medians:
    p43p_age_stage1_items_total = 528
    p43p_age_young_current = 8
    p43p_age_old_current = 296
    p43p_age_would_reuse_young = 252
    p43p_age_would_demote_old_open = 196
    p43p_age_would_block_old_drain = 1504
    p43p_age_would_block_old_closed = 864
    p43p_age_pop_all_pollution_projection = 9036

rss-bounded repeat-3
  run root:
    results/synthetic-sweep/20260522_094542_911

  final HZ5_LOWPAGE64 medians:
    p43p_age_stage1_items_total = 1264
    p43p_age_young_current = 8
    p43p_age_old_current = 664
    p43p_age_would_reuse_young = 632
    p43p_age_would_demote_old_open = 584
    p43p_age_would_block_old_drain = 5840
    p43p_age_would_block_old_closed = 3088
    p43p_age_pop_all_pollution_projection = 52644
```

### Updated Interpretation

P43p.4 gives a clearer algorithm-level signal:

```text
age separation exists
old stage1 projection is large
pop-all pollution risk is plausible
young stage1 current is small and stable
admission state still flips under some profiles
```

This supports the idea that P43p.2/P43p.3a were not merely noisy knobs:
the bridge-cold queue has a real temperature problem. However, P43p.4 is still
diagnostic-only, and P43p.3a did not cleanly beat P43i on guard/balanced
criteria.

### Updated Question

Given P43p.4's age projection, what should HZ5 do next?

```text
A. Stop P43p here.
   Keep:
     P43i = selected balanced candidate-watch
     P43p.2 = pop-all no-go/control
     P43p.3a = limited-acquire evidence
     P43p.4 = age/pollution diagnostic evidence
   Move to broader HZ5 core design.

B. Implement one age-gated behavior prototype.
   Only if the large old-stage1 projection is considered strong enough.
   It would avoid hot-stash reinjection of old stage1 nodes and keep behavior
   bridge-preserving.

C. Do more P43p diagnostics.
   My bias is NO: we likely have enough P43p evidence now.

D. Reframe P43p into the HZ5 core design:
   P25 bridge = speed layer
   P43 segment source = source layer
   P40 release = source-demotion intent
   admission/temperature control = control plane
```

My current leaning after P43p.4:

```text
P43p has produced enough evidence.
P43i remains the balanced candidate-watch.
Do not promote P43p.
Either stop P43p and move to HZ5 core design, or do exactly one age-gated
behavior prototype if Pro thinks the old-stage1 projection is strong enough.
```

## Post-Review Decision: Stop P43p

After an extra guard dry-run:

```text
guard repeat-3
  run root:
    results/synthetic-sweep/20260522_095654_192

  a4096 / 65537 guard rows:
    p43p_age_* = 0
    p43p_p40_release_candidates = 0
```

This changes the final read:

```text
P43p.4 explains:
  mixed/rss-bounded old-stage1 exposure
  pop-all pollution plausibility

P43p.4 does not explain:
  P43p.3a pop4 a4096 guard regression
```

Therefore P43p should stop here:

```text
P43i:
  selected balanced candidate-watch

P43p.2:
  pop-all no-go/control

P43p.3a:
  limited-acquire evidence

P43p.4:
  age/pollution diagnostic evidence

Do not:
  implement age-gated behavior inside P43p
  continue popN/age knob sweeps
  promote P43p
```

Next design direction:

```text
HZ5 control plane / P45 BridgeSourceControl

P25 bridge:
  speed layer

P43 segment source:
  source layer

P40 release:
  source-demotion intent under P43i contract

Admission / temperature:
  core control-plane concern, not another P43p local knob
```

## Lane Cleanup After P43p

Use this framing for follow-up work:

```text
selected balanced lane:
  P43i / hz5-p43-balanced-watch

speed baseline:
  P25a / hz5-speed-current

RSS-bounded baseline:
  P33 / hz5-rss-bounded-current

closed evidence:
  P43o/P43p source-admission controls
  P43p.2 pop-all no-go
  P43p.3a limited-acquire evidence
  P43p.4 age/pollution diagnostic evidence

next design:
  P45 / HZ5 Control Plane / BridgeSourceControl
```

P43p should not receive more local popN / age-gated behavior knobs unless a new
design review explicitly reopens it. The useful result is the algorithm lesson:
P25 bridge is the speed layer, P43 segment slots are the source layer, and P40
release under the P43i contract is source-demotion intent rather than OS
memory return.

## P45 Refined-Gate Behavior Follow-Up

P45r1 is now implemented as the first small behavior probe for the HZ5 control
plane.

Commit:

```text
db24098 Add P45 refined gate behavior
```

P45r1 keeps the P43i/P25 bridge topology:

```text
release_prepared
  -> release_common
  -> TLS relbuf
  -> global batch
  -> acquired stash
```

It also keeps the P43i restrictions:

```text
no slot decommit
no PAGE_NOACCESS
no runtime segment release
no direct prepared release
no descriptor relbuf
```

This is a diagnostic/control-plane prototype, not a SpeedLane promotion lane.

### Behavior

During P40 checkpoint, P45r1 stages only refined blockers.

```text
demote_intent =
  p40 release candidates from observed global over soft cap

bridge_current =
  observed_global + stash_count + relbuf_count

bridge_residual_after_demote =
  max(bridge_current - demote_intent, 0)

stage1 if:
  admission state != OPEN
  or bridge_residual_after_demote > P40 global soft cap

allow existing P40 demotion if:
  admission state == OPEN
  and bridge_residual_after_demote <= P40 global soft cap
```

Important fix:

```text
stage1 backlog is not included in bridge_current.
```

The first implementation accidentally included stage1 backlog in bridge
pressure, causing self-amplifying stage1 queueing. That was fixed before the
readouts below. Stage1 backlog is now reported separately.

### Smoke after the accounting fix

Mixed-prelude smoke:

```text
results/synthetic-sweep/20260522_112206_535

final exact 64K:
  p45rg_demote_intent = 272
  p45rg_allowed_open_bridge_soft = 256
  p45rg_stage1_any = 16
  p45rg_stage1_bridge_excess = 16
  p45rg_stage1_enqueue_nodes = 16
  p45rg_stage1_acquire_nodes = 16
  p45rg_stage1_current = 0
```

RSS-bounded smoke:

```text
results/synthetic-sweep/20260522_112206_517

plateau:
  p45rg_demote_intent = 1104
  p45rg_allowed_open_bridge_soft = 992
  p45rg_stage1_any = 112
  p45rg_stage1_state = 64
  p45rg_stage1_bridge_excess = 48
  p45rg_stage1_enqueue_nodes = 112
  p45rg_stage1_acquire_nodes = 64
  p45rg_stage1_current = 48
```

Initial interpretation:

```text
P45r1 is not all-stage1.
Mixed-prelude drains stage1.
RSS plateau can leave bounded stage1 backlog.
```

### Repeat-3 / guard follow-up

RSS-bounded repeat-3:

```text
results/synthetic-sweep/20260522_163246_969

P45r1 plateau median:
  p45rg_demote_intent = 960
  p45rg_allowed_open_bridge_soft = 848
  p45rg_stage1_any = 104
  p45rg_stage1_state = 88
  p45rg_stage1_bridge_excess = 24
  p45rg_stage1_enqueue_nodes = 104
  p45rg_stage1_acquire_nodes = 56
  p45rg_stage1_requeue_nodes = 188
  p45rg_stage1_current = 48
  p45rg_stage1_current_max = 48
  p43g_raw_mismatch = 0
```

Mixed-prelude repeat-3:

```text
results/synthetic-sweep/20260522_163320_319

final exact 64K median:
  p45rg_demote_intent = 320
  p45rg_allowed_open_bridge_soft = 272
  p45rg_stage1_any = 48
  p45rg_stage1_state = 24
  p45rg_stage1_bridge_excess = 48
  p45rg_stage1_enqueue_nodes = 48
  p45rg_stage1_acquire_nodes = 48
  p45rg_stage1_requeue_nodes = 88
  p45rg_stage1_current = 8
  p45rg_stage1_current_max = 24
  max run stage1_current = 40
  p43g_raw_mismatch = 0
```

Guard repeat-3:

```text
results/synthetic-sweep/20260522_163320_339

All a8192 / a4096 / 65537 guard rows:
  p45rg_demote_intent = 0
  p45rg_stage1_any = 0
  p45rg_stage1_current = 0
  p45rg_stage1_current_max = 0
  p45rg_enqueue_nodes = 0
  p45rg_acquire_nodes = 0
  p43g_raw_mismatch = 0

Fallback-loaded rows:
  load_count = 1 expected
  load_failed = 0
  free_miss_unloaded = 0
```

### Current interpretation

```text
P45r1:
  KEEP as refined-gate mechanism evidence

Promotion:
  NO

Stage1 current = 48 on plateau:
  bounded watch item
  not immediate no-go

Guard/fallback:
  clean in repeat-3
  no refined stage1 leakage into a4096 / 65537 rows
```

P45r1 avoids the older P43p failure mode:

```text
not all P40 demotion intent becomes stage1
stage1 backlog does not self-amplify in repeat-3
guard rows do not enter the refined stage1 path
```

The remaining open question is the plateau backlog:

```text
rss-bounded plateau keeps stage1_current = 48
mixed-prelude final exact drains to low current
```

### Next review question

Should the next step be a diagnostic-only stage1 drain/checkpoint dry-run?

Candidate counters:

```text
p45dr_stage1_current_at_checkpoint
p45dr_stage1_age_bucket0
p45dr_stage1_age_bucket1
p45dr_stage1_age_old

p45dr_would_drain_to_bridge
p45dr_would_keep_cold
p45dr_would_demote_open
p45dr_would_block_drain_closed
```

My current leaning:

```text
P45r1 should be kept as HZ5 control-plane prototype evidence.
Do not try acquire-limit 8 yet.
Do not add a new speed knob.
Do not add actual trim, decommit, PAGE_NOACCESS, or runtime segment release.
Next should be a dry-run that determines whether plateau stage1_current=48 is
natural bounded bridge-cold retention or backlog that needs a checkpoint/drain
rule.
```
