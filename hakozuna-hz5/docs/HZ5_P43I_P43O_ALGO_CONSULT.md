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
