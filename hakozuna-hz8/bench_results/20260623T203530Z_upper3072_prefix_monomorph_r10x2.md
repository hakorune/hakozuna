# Upper3072 P2 Prefix Monomorph R10 x 2

Date: 2026-06-23

Purpose:

```text
test whether upper3072-v0 hot regression comes from generic class geometry
rather than from the 3072 class policy itself
```

Change under test:

```text
upper3072-v0:
  class <= 2048 uses direct p2 prefix geometry
  class 3072 uses the factor-3 path
  class 4096 uses direct shift=12 path
```

Rows:

```text
local:
  runs=10 x 2, threads=16, iters=100000, size=16..2048

interleaved:
  runs=10 x 2, threads=16, iters=100000, size=16..4096, remote=90

phase:
  runs=10 x 2, threads=16, iters=100000, size=16..4096, remote=90
```

## Combined 20-Run Median

| Row | p2 | upper3072 prefix | Delta |
| --- | ---: | ---: | ---: |
| local throughput | 400.89M/s | 409.80M/s | +2.22% |
| interleaved remote90 throughput | 55.51M/s | 53.53M/s | -3.56% |
| phase remote90 throughput | 3.42M/s | 3.69M/s | +7.69% |
| phase peak RSS | 3803.5MiB | 3466.4MiB | -8.88% |
| phase minor faults | 966,099 | 880,103 | -8.90% |

## Decision

```text
Upper3072P2PrefixMonomorph-L1:
  partially successful

local:
  recovered

interleaved remote90:
  still below the >=0.98 relative gate

upper3072-v0:
  HOLD as default
  keep as SizePolicy-v1 evidence only

p2-v0:
  remains the small-v0 / rc1 default
```

The next allocator architecture step is MediumRun-v1 / size-boundary design,
not another small class-map default attempt.

Raw logs:

```text
bench_results/20260623T203530Z_upper3072_prefix_monomorph_r10x2/
```
