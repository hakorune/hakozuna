# HZ11Sh6benchLiveFootprintObservation-L1

## Box

```text
Goal:
  Decide whether sh6bench RSS under batch32 is caused by live payload demand,
  span internal fragmentation, class packing, or page residency.

Boundary:
  diagnostics only
  no optimization policy
  no macro promotion claim

Lane:
  libhz11_span_transfer_thread_exit_cap_batch32_live_diag.so
```

## Implementation

Added an opt-in `HZ11_LIVE_FOOTPRINT_DIAG` sibling lane for the batch32
candidate. The lane counts in-arena malloc handouts and frees by size class and
dumps:

```text
class alloc count
class free count
class live current
class live high-water
class slot bytes live/current high-water
```

The normal batch32 candidate is unchanged. The live diagnostic lane also enables
existing span-source and central-class diagnostics so the runner can join live
slot high-water with span-create high-water by class.

## Evidence

Command:

```sh
RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_live_footprint_observation.sh
```

Output:

```text
bench_results/hz11_sh6bench_live_footprint_20260708T070625Z/summary.md
```

Sample medians:

```text
tcmalloc:
  wall 0.344s
  max RSS 268544 KiB

hz11-thread-exit-cap-batch32:
  wall 3.539s
  max RSS 351232 KiB
  xfer_insert 520974054
  xfer_spill / central_insert 26913790
  span_create 16760

hz11-thread-exit-cap-batch32-live-diag:
  wall 8.848s
  max RSS 351744 KiB
  span_create 16715
```

The live diagnostic lane is intentionally slower because it adds atomic counters
to the hot malloc/free path. Its RSS and span-create shape match batch32 closely,
so it is usable for footprint attribution only.

## Class Footprint

For active classes 0..6, live slot high-water nearly fills the span capacity
created for that class:

```text
total live slot bytes high-water:      359342848
total span capacity bytes high-water:  365625344
estimated span-internal waste:           6282496
estimated waste ratio:                     1.7%
```

Largest classes:

```text
class 0, slot 16:
  live bytes high 101025600
  span capacity 101122048
  waste ratio 0.1%

class 3, slot 128:
  live bytes high 67182336
  span capacity 67764224
  waste ratio 0.9%

class 5, slot 512:
  live bytes high 36951040
  span capacity 37421056
  waste ratio 1.3%
```

Class 9 has high fractional waste, but its capacity is only about 4 MiB and does
not explain the RSS gap.

## Decision

GO for observation only. Do not promote any lane.

The remaining sh6bench RSS is not primarily:

```text
central final retention
central transient high-water
per-class live current-span reserve
span-internal waste after HZ11 class rounding
simple huge-page/eager-commit behavior
```

The evidence points to HZ11 live slot footprint under power-of-two classes, plus
page residency, as the next explanation to separate. The next diagnostic should
measure requested-size distribution versus HZ11 slot size by class before trying
span-size, class-packing, or decommit policy changes.
