# HZ8 Largeish Route-Miss Boundary

Status:

```text
separate issue
not part of ActiveFullDefer4 RC promotion
do not use as a Defer4 no-go row
```

## Why This Boundary Exists

The Defer4 RC work targets small/medium remote pressure:

```text
small pending pressure:
  active-full defer

medium capacity pressure:
  finite capacity-miss collect budget
```

The `largeish_remote50` row currently exercises a different path.  The only
record in the broader gate shows that it is dominated by route misses rather
than remote-pressure collection.

Record:

```text
bench_results/hz8_defer8_rc_broad_gate_20260630T115242/largeish_remote50.log
```

Observed:

```text
largeish_remote50:
  size = 32768..131072
  remote_pct = 50
  throughput median = 220129 ops/s
  peak RSS median = 62.79 MiB

stats:
  pending_enqueue = 0
  pending_dequeue = 0
  local = 0
  remote = 0
  miss = 1600287

remote_pressure_collect:
  source_active_hit_full = 0
  source_active_miss = 0
  source_owner_exit = 0
  small_call = 0
```

Read:

```text
the row is not exercising the Defer4 small path
the row is not exercising the MediumCapacity collect path
the row is mostly sys/route-miss or direct/large boundary behavior
```

## Decision

```text
keep largeish rows outside the Defer4 RC gate
do not tune ActiveFullDefer or MediumCapacityCollectBudget for largeish rows
open a separate large/sys route lane only if largeish becomes the next priority
```

## Future Largeish Lane

If HZ8 needs to claim larger mixed-size throughput, use a separate box:

```text
LargeishRouteBoundary-L1
```

The first questions should be:

```text
1. Which sizes leave the current owned medium path?
2. Are route misses intentional platform/direct fallback or missing HZ8 coverage?
3. Does the direct/large path preserve MISS / VALID / INVALID separation?
4. Can the large path be improved without touching small remote pressure policy?
```

Required evidence:

```text
largeish route result attribution
direct/large allocation count
platform fallback count
owned-looking invalid fallback = 0
same-run default/candidate comparison
```
