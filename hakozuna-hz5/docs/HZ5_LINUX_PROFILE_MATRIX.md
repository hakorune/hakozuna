# HZ5 Linux Profile Matrix

This page is the short registry for the current HZ5 Linux general allocator
lanes. Detailed measurements stay in the result/design documents linked below.

## Current Read

```text
MidPage PageRun64:
  strong keep
  main / mid_only / cross64 are healthy
  32769..65536 timeout gap is fixed

LargeFront 128K remote:
  remaining design target
  fixed source batch choices split by workload

Adaptive128:
  first mapped-bytes-only implementation is no-go
```

Latest broad check:

```text
result root:
  private/raw-results/linux/hz5_hakmem_compare_pagerun64_broad_r3

RUNS=3:
  HZ5 wins many main/mid_only r50/r90 rows at t=4/t=8.
  HZ5 keeps strong RSS in MidPage rows.
  main/mid_only r0 remains tcmalloc's clear local-fast-path win.
  large128 r50/r90 is the largest actionable remaining gap.
```

## Saved Profiles

These aliases are the canonical build handles for the current reportable HZ5
Linux profile family. The expanded historical flags are kept only for
reproducibility in older notes.

### `hz5-linux-pagerun64-main`

Role:

```text
general MidPage throughput/RSS profile
```

Build:

```text
--linux-hz5-profile-pagerun64-main
```

Status:

```text
keep
best current default candidate for main/mid/cross64-style rows
```

### `hz5-linux-pagerun64-cross-size`

Role:

```text
cross-size remote-heavy diagnostic
prioritizes cross128 mixed rows
```

Build:

```text
--linux-hz5-profile-pagerun64-cross128
```

Status:

```text
saved fixed profile
not a universal default
```

### `hz5-linux-pagerun64-large-only`

Role:

```text
large128 remote-heavy diagnostic
prioritizes pure 65537..131072 traffic
```

Build:

```text
--linux-hz5-profile-pagerun64-large128
```

Status:

```text
saved fixed profile
useful evidence for source-batch/RSS tradeoff
```

## Diagnostic / No-Go Lanes

### `hz5-linux-pagerun64-adaptive128`

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-adaptive128
--linux-midpagefront-empty-retain-cap 4096
```

Result:

```text
no-go in first mapped-bytes-only form
```

Reason:

```text
Mapped bytes are too blunt as a phase signal. The policy lowers the 128K source
batch during cross128 phases that still want batch16, and it does not rescue
large128.
```

### `hz5-linux-pagerun64-scavenge`

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-scavenge
--linux-midpagefront-empty-retain-cap 4096
```

Intent:

```text
LargeFront-L3 diagnostic after adaptive128 no-go
```

Behavior:

```text
128K LargeFront spans only
retained owner-local payload is madvise(DONTNEED) after a small cap
span descriptor/prefix page remains mapped
reuse clears the scavenged flag
```

Status:

```text
no-go as first retained-payload form
```

Smoke result:

```text
RUNS=3, threads=8, ws=100, r90, iters=120000:

cross128:
  base_b16   27.91M /  69MB
  scav_b16    7.83M /  93MB
  base_b4    19.22M / 101MB
  scav_b4     7.14M /  66MB

large128:
  base_b16   18.35M / 128MB
  scav_b16    4.11M / 185MB
  base_b4     9.68M / 207MB
  scav_b4     3.59M / 129MB
```

Cap64 spot check:

```text
RUNS=2, same shape:
  scav64_b16 cross128  9.32M /  87MB
  scav64_b16 large128  4.50M / 179MB
  scav64_b4  cross128  8.96M /  69MB
  scav64_b4  large128  3.87M / 124MB
```

Decision:

```text
Retained payload madvise is too close to the remote/free path. It can reduce
RSS in some rows, but the throughput collapse is too large to promote.
Keep fixed source-batch split.
```

### `hz5-linux-pagerun64-observe`

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-observe
--linux-midpagefront-empty-retain-cap 4096
```

Runtime:

```text
HZ5_LARGEFRONT_OBSERVE=1
```

Intent:

```text
LargeFront-L3 phase-signal observation
not for speed medians
```

Counters:

```text
per LargeFront class:
  alloc calls
  local free-list hits
  remote take-first returns
  remote drains to local free list
  global hits
  new spans
  local/remote/global frees
  source refill count and batch histogram
  owner-local free-list high water
```

Decision target:

```text
If cross128 and large128 differ cleanly in slow-path counters, adaptive source
batching may be redesigned. If not, keep the fixed split.
```

First observation:

```text
RUNS=1 observation smoke, threads=8, ws=100, r90, iters=120000:

cross128:
  observe_b16  23.87M /  75MB
  observe_b4   22.56M /  70MB

large128:
  observe_b16  15.54M / 134MB
  observe_b4   17.22M / 101MB
```

Class 0, 128K counters:

```text
cross128 b16:
  alloc_calls=239601 local_pop_hit=219683 remote_take_first=3461
  remote_to_local=196025 new_span=16247 source_refill=1016
  local_free_highwater=290

cross128 b4:
  alloc_calls=239601 local_pop_hit=220943 remote_take_first=3711
  remote_to_local=197628 new_span=14875 source_refill=3719
  local_free_highwater=398

large128 b16:
  alloc_calls=480202 local_pop_hit=444415 remote_take_first=2253
  remote_to_local=397697 new_span=32856 source_refill=2054
  local_free_highwater=842

large128 b4:
  alloc_calls=480202 local_pop_hit=451984 remote_take_first=3851
  remote_to_local=405014 new_span=24322 source_refill=6081
  local_free_highwater=907
```

Decision:

```text
The counter shape mostly scales with 128K traffic volume. It does not expose a
clean slow-path phase signal that would safely choose batch16 for cross128 and
batch4 for large128. Keep fixed split.
```

### LargeFront remote-batch

Build flag:

```text
--linux-largefront-remote-batch
```

Status:

```text
diagnostic only
can help one cross128 sweep, but regresses large-only remote rows and increases
RSS risk
```

## Key Results

RUNS=5, r90, iters=500000:

```text
cross128:
  pagerun64            21.53M / 421MB
  pagerun64-takefirst  25.00M / 319MB
  hz4                  28.01M / 333MB
  tcmalloc             16.16M / 401MB

large128:
  pagerun64            11.48M / 929MB
  pagerun64-takefirst  13.25M / 800MB
  hz4                   3.93M / 1703MB
  tcmalloc             17.35M / 500MB
```

Source batch sweep, PageRun64+takefirst, RUNS=3, r90:

```text
cross128:
  batch4   13.44M / 571MB
  batch8   22.12M / 345MB
  batch16  28.70M / 265MB

large128:
  batch4   18.35M / 420MB
  batch8   11.31M / 864MB
  batch16   9.65M / 1153MB
```

Adaptive128 first implementation, RUNS=5, r90:

```text
cross128:
  adaptive 13.50M / 567MB
  batch16  27.48M / 288MB
  batch4   17.02M / 455MB

large128:
  adaptive  8.90M / 1070MB
  batch16  13.04M / 779MB
  batch4    8.20M / 1060MB
```

## Next Design Options

Recommended order:

```text
1. Preserve fixed profile split.
2. Stop tuning fixed source-batch constants.
3. Do not promote retained-payload scavenging in its current form.
4. Do not redesign adaptive source batching without a colder/clearer phase
   signal.
5. Keep speed lanes free of HZ5_PRELOAD_STATS and diagnostic atomics.
```

Current attack order:

```text
1. LargeFront 128K source/refill and page-touch behavior.
2. main/mid_only r0 instruction-path reduction against tcmalloc, only if
   local-only throughput remains a priority.
3. cross128 r90 t=2 after LargeFront is understood.
```

## Detailed Docs

```text
docs/HZ5_MIDPAGEFRONT_C7_LANES.md
docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
docs/HZ5_LARGEFRONT_L1_DESIGN.md
docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```
