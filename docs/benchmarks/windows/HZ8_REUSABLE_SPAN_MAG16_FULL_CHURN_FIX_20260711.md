# HZ8 Mag16 Full-Magazine Churn Fix: Windows Follow-up

Date: 2026-07-11  
Source commit: `652fa283`  
Machine: existing Windows Hakozuna benchmark machine

## Local Long-Lived A/B

Alternating baseline/candidate order, R5, fresh process per run:

```text
threads=8
iters_per_thread=5000000
working_set=16384
```

| Size | HZ8 v2 | Mag16 | Ratio | HZ8 v2 peak | Mag16 peak |
|---|---:|---:|---:|---:|---:|
| 16..256 | 134.49M | 329.35M | 2.45x | 3416.09 MiB | 1397.40 MiB |
| 16..2048 | 18.58M | 24.77M | 1.33x | 26500.74 MiB | 22421.15 MiB |
| 16..4096 | 3.34M | 9.63M | 2.88x | 50588.16 MiB | 48179.32 MiB |

The absolute RSS is specific to this harsh long-lived shape. The valid claim
is the same-session A/B direction, not comparison with older absolute values.

## Fixed MT Remote R5

```text
threads=16
iters=2000000
working_set=400
size=16..2048
remote_pct=90
ring_slots=65536
```

| Allocator | Median ops/s | Actual remote | Fallback | Median peak RSS |
|---|---:|---:|---:|---:|
| HZ8 v2 | 124.891M | 79.70% | 11.45% | 32.20 MiB |
| Mag16 | 131.737M | 77.43% | 13.97% | 18.71 MiB |

All five recorded runs per allocator completed. Effective remote ratios differ,
so this is a regression/neutrality gate rather than a remote throughput claim.

## Decision

```text
Windows candidate:
  GO

cross-platform correctness/local candidate:
  GO

default promotion:
  PROMOTED after broader Windows R5
```

Broader Windows R5 medians:

```text
balanced:     18.60M -> 52.85M (2.84x)
wide_ws:      32.86M -> 55.15M (1.68x)
larger_sizes: 14.27M -> 22.75M (1.59x)
```
