# HZ8 Reusable Span Magazine Mag16 Linux Gate (2026-07-11)

## Box

- candidate: `H8_REUSABLE_SPAN_MAGAZINE_L1`
- baseline: HZ8 v2 public default
- compile-time opt-in; default remains unchanged
- GCC and Clang use separate Mag16 smoke/safety targets

## Correctness

Both GCC and Clang passed `-Werror` builds and execution for:

- `h8_smoke_reusable_span_mag16`
- `h8_safety_stress_reusable_span_mag16`

The safety stress completed with 8 owner exits, 68 handoffs, 8,192 remote
frees, and the existing duplicate-claim and invalid-route fail-closed checks.
No route, generation, pending, or owner-exit invariant failure was observed.

## Paired Local Churn

Eight threads, 500,000 operations per thread, working set 1,024, alternating
baseline/candidate fresh processes, repeat-5. Values are medians.

| Size range | HZ8 v2 ops/s | Mag16 ops/s | Ratio | HZ8 v2 peak RSS | Mag16 peak RSS |
|---|---:|---:|---:|---:|---:|
| 16..256 | 72.71 M | 327.03 M | 4.50x | 245.25 MiB | 8.00 MiB |
| 16..2048 | 9.49 M | 88.86 M | 9.37x | 2,403.75 MiB | 172.75 MiB |
| 16..4096 | 4.33 M | 10.03 M | 2.32x | 4,933.38 MiB | 2,068.50 MiB |

All rows completed with zero allocation failures.

## Public Remote Matrix

Established HZ8 same-run harness, 8 threads, 50,000 iterations, repeat-5.

| Row | HZ8 v2 ops/s | Mag16 ops/s | Delta | HZ8 v2 peak/post | Mag16 peak/post |
|---|---:|---:|---:|---:|---:|
| small interleaved remote90 | 11.70 M | 10.56 M | -9.7% | 20.75 / 2.34 MiB | 31.00 / 2.28 MiB |
| main interleaved remote90 | 4.69 M | 5.18 M | +10.4% | 56.88 / 3.33 MiB | 44.62 / 3.30 MiB |
| main local0 | 101.69 M | 107.07 M | +5.3% | 2.88 / 2.75 MiB | 2.88 / 2.75 MiB |

Post-workload RSS remains low. The small remote90 row regresses both
throughput and peak RSS, while the broader main remote90 row improves.

## Decision

```text
Linux correctness gate: GO
Linux local churn candidate: GO
Linux broad default promotion: HOLD
HZ8 public default: unchanged
```

Mag16 is a strong local/retention candidate, but the small remote90 regression
must be resolved or isolated to a workload-specific lane before promotion.

## Remote Follow-up

The first remote result exposed active-hint churn after the magazine reached
capacity. Local frees continued replacing the active span even though the
displaced span could not be added to the full inventory. The candidate now
retains the active span when Mag16 is full.

Focused fresh-process repeat-30 after the fix:

| Metric | HZ8 v2 | Mag16 |
|---|---:|---:|
| small remote90 ops/s | 31.627 M | 31.241 M (-1.22%) |
| peak RSS | 8.125 MiB | 8.125 MiB |
| post RSS | 4.746 MiB | 4.762 MiB |

Focused local repeat-5 improved from 259.88 M to 279.79 M ops/s (+7.66%).
The large initial remote regression and RSS increase are no longer reproduced,
but default promotion remains HOLD pending the full public matrix refresh.
