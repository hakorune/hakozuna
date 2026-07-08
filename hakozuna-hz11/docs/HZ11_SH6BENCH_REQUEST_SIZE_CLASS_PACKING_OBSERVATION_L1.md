# HZ11Sh6benchRequestSizeClassPackingObservation-L1

## Box

```text
Goal:
  Measure requested-size distribution against HZ11 power-of-two slot classes
  under batch32 before changing class packing or span/page policy.

Boundary:
  diagnostics only
  no optimization policy
  no macro promotion claim

Lane:
  libhz11_span_transfer_thread_exit_cap_batch32_live_diag.so
```

## Implementation

Extended the existing live-footprint diagnostic lane with requested-size
tracking. The diagnostic lane now stores the requested allocation size in a
lazy per-span slot side table, then subtracts the same size on free. This lets
the runner report requested bytes live/high-water and HZ11 slot bytes
live/high-water by class.

This is diagnostic-only. The side table increases live-diag RSS and wall time,
so the baseline batch32 row remains the RSS/wall reference.

## Evidence

Command:

```sh
RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_request_class_packing_observation.sh
```

Output:

```text
bench_results/hz11_sh6bench_request_class_packing_20260708T071205Z/summary.md
```

Sample medians:

```text
tcmalloc:
  wall 0.366s
  max RSS 263808 KiB

hz11-thread-exit-cap-batch32:
  wall 3.549s
  max RSS 350208 KiB

hz11-thread-exit-cap-batch32-live-diag:
  wall 10.149s
  max RSS 388352 KiB
```

The live-diag RSS is higher because of the requested-size side table. Use it
for class-packing attribution only.

## Class Packing

The requested payload high-water is much smaller than HZ11 slot high-water:

```text
requested bytes high-water:      211116531
HZ11 slot bytes high-water:      359344128
request-to-slot waste:           148227597
request-to-slot waste ratio:          41.2%
```

The previous span-footprint observation showed span-internal waste was only
about 1.7%. This observation shows the larger footprint is class rounding
inside the live HZ11 slot footprint, not unused span capacity.

Largest contributors:

```text
class 0, slot 16:
  requested high 28614062
  slot high 101025600
  request-to-slot waste 72411538
  waste ratio 71.7%

class 3, slot 128:
  requested high 44317569
  slot high 67206272
  request-to-slot waste 22888703
  waste ratio 34.1%

class 2, slot 64:
  requested high 44136750
  slot high 60304384
  request-to-slot waste 16167634
  waste ratio 26.8%
```

## Decision

GO for observation only. Do not promote any lane.

The sh6bench RSS gap is now attributed primarily to HZ11 power-of-two
class-packing waste. The next optimization box should be class packing / size
class policy, not central retention, span reuse, span-internal fragmentation, or
arena commit policy.

Any size-class policy change must remain opt-in until sh6bench wall/RSS and the
full macro gate pass.
