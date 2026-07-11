# HZ8 Reusable Span Magazine Mag32 Linux Gate (2026-07-11)

## Scope

- baseline: public-default Mag16
- candidate: `H8_REUSABLE_SPAN_MAG_CAP=32`
- Ubuntu x86_64
- default remains Mag16

## A/A Noise

Mag16 was run under two labels in alternating fresh processes.

- local 16..4096 repeat-10: median label delta -0.34%; paired median about
  -0.15%, with individual paired deltas from about -7.4% to +2.6%
- public small remote90 repeat-10: median label delta -0.59%

Small percentage differences are treated as noise unless paired shape and
effective remote work are controlled.

## Correctness

GCC and Clang `-Werror` builds and execution passed for Mag16 and Mag32:

- smoke and safety stress
- owner exit: 8
- handoff: 68
- remote frees: 8,192
- existing duplicate-claim and invalid-route fail-closed checks

No route, generation, pending, or owner-exit invariant failure was observed.

## Local Paired R5

Eight threads, 500,000 operations per thread, working set 1,024, alternating
fresh processes. Values are medians.

| Size range | Mag16 ops/s | Mag32 ops/s | Delta | Mag16 peak RSS | Mag32 peak RSS |
|---|---:|---:|---:|---:|---:|
| 16..256 | 366.35 M | 331.90 M | -9.4% | 3.62 MiB | 3.62 MiB |
| 16..2048 | 147.99 M | 278.33 M | +88.1% | 76.62 MiB | 17.62 MiB |
| 16..4096 | 11.05 M | 95.64 M | +765% | 1,957.50 MiB | 139.75 MiB |

All rows completed with zero allocation failures. A direct owner-exit local
probe reported approximately 1.6–1.8 MiB post RSS for both lanes.

## Remote Paired R5

The direct HZ8 bench used identical deterministic work in every process:

```text
remote enqueue: 360,317
local free:      39,683
effective remote: 90.07925%
```

| Metric | Mag16 | Mag32 |
|---|---:|---:|
| median ops/s | 31.37 M | 30.71 M (-2.1%) |
| median peak RSS | 8.125 MiB | 8.250 MiB |
| median post RSS | 4.734 MiB | 4.730 MiB |

The throughput difference is small relative to observed run variance and is
not a remote win. It remains a regression signal for promotion.

## Decision

```text
GCC/Clang correctness: GO
larger local size ranges: GO
16..256 local: NO-GO
remote: HOLD / near-neutral regression
Mag32 default promotion: HOLD
HZ8 public default: Mag16 unchanged
```

Linux confirms the capacity/phase-amplitude benefit for larger size mixes,
but a global capacity increase is not uniformly beneficial. The next box, if
pursued, should isolate class-7 capacity rather than promote Mag32 globally.
