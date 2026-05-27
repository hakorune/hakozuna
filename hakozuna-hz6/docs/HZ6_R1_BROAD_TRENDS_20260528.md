# HZ6 R1 Broad Benchmark Trends (2026-05-28)

This note summarizes the first broad HZ6-only R1 sweep after the modular
LargeSpan CentralSpanPool seed.

This is still a development trend note. It is not a paper-facing
cross-allocator table.

## Provenance

```text
result root:
  hakozuna-hz6/private/raw-results/linux/hz6_benchmark_broad_20260528_0830

git sha:
  8a00754

host:
  Linux tomoaki-M5-PLUS 6.8.0-90-generic x86_64

runner:
  hakozuna-hz6/linux/run_hz6_benchmark.sh

runs:
  5

benchmark binary:
  hakozuna-hz6/out/linux/hz6_benchmark/hz6_allocator_bench
```

Command:

```bash
bash hakozuna-hz6/linux/run_hz6_benchmark.sh \
  --skip-build \
  --runs 5 \
  --local-sizes 1024,4096,8192,32768,65536,131072 \
  --remote-sizes 1024,4096,8192,32768,65536,131072 \
  --reuse-sizes 1024,4096,8192,32768,65536,131072 \
  --outdir hakozuna-hz6/private/raw-results/linux/hz6_benchmark_broad_20260528_0830
```

## Scope

The runner covers three HZ6-only single-process modes:

```text
local:
  malloc/free in the same allocator owner slot

remote:
  malloc in owner slot 0, remote-free through owner slot 1

reuse:
  malloc, remote-free, then malloc again and check pointer reuse
```

Measured sizes:

```text
1024
4096
8192
32768
65536
131072
```

Measured profiles:

```text
local:
  strict
  rss
  speed
  remote

remote / reuse:
  rss
  speed
  remote
```

## Best Profile Per Row

RUNS=5 median.

| Mode | Size | Best profile | Median ops/s | Runner RSS | Reuse hits |
| --- | ---: | --- | ---: | ---: | ---: |
| `local` | 1024 | `rss` | 33.19M | 1.38 MiB | 0 |
| `local` | 4096 | `strict` | 33.51M | 1.38 MiB | 0 |
| `local` | 8192 | `strict` | 34.13M | 1.38 MiB | 0 |
| `local` | 32768 | `strict` | 39.31M | 1.38 MiB | 0 |
| `local` | 65536 | `strict` | 42.22M | 1.38 MiB | 0 |
| `local` | 131072 | `strict` | 39.50M | 1.50 MiB | 0 |
| `remote` | 1024 | `rss` | 25.18M | 1.38 MiB | 0 |
| `remote` | 4096 | `rss` | 25.08M | 1.38 MiB | 0 |
| `remote` | 8192 | `rss` | 26.11M | 1.38 MiB | 0 |
| `remote` | 32768 | `rss` | 28.68M | 1.38 MiB | 0 |
| `remote` | 65536 | `rss` | 31.50M | 1.38 MiB | 0 |
| `remote` | 131072 | `rss` | 38.98M | 1.38 MiB | 0 |
| `reuse` | 1024 | `rss` | 27.21M | 1.38 MiB | 100000 |
| `reuse` | 4096 | `rss` | 28.42M | 1.38 MiB | 100000 |
| `reuse` | 8192 | `rss` | 28.19M | 1.38 MiB | 100000 |
| `reuse` | 32768 | `rss` | 31.36M | 1.38 MiB | 100000 |
| `reuse` | 65536 | `rss` | 33.37M | 1.38 MiB | 100000 |
| `reuse` | 131072 | `rss` | 39.49M | 1.38 MiB | 100000 |

## Full Median Matrix

| Mode | Profile | Size | Median ops/s | Runner RSS | Reuse hits |
| --- | --- | ---: | ---: | ---: | ---: |
| `local` | `strict` | 1024 | 33.16M | 1.38 MiB | 0 |
| `local` | `strict` | 4096 | 33.51M | 1.38 MiB | 0 |
| `local` | `strict` | 8192 | 34.13M | 1.38 MiB | 0 |
| `local` | `strict` | 32768 | 39.31M | 1.38 MiB | 0 |
| `local` | `strict` | 65536 | 42.22M | 1.38 MiB | 0 |
| `local` | `strict` | 131072 | 39.50M | 1.50 MiB | 0 |
| `local` | `rss` | 1024 | 33.19M | 1.38 MiB | 0 |
| `local` | `rss` | 4096 | 33.28M | 1.38 MiB | 0 |
| `local` | `rss` | 8192 | 29.31M | 1.38 MiB | 0 |
| `local` | `rss` | 32768 | 34.03M | 1.38 MiB | 0 |
| `local` | `rss` | 65536 | 32.33M | 1.38 MiB | 0 |
| `local` | `rss` | 131072 | 39.22M | 1.38 MiB | 0 |
| `local` | `speed` | 1024 | 31.75M | 1.38 MiB | 0 |
| `local` | `speed` | 4096 | 31.67M | 1.38 MiB | 0 |
| `local` | `speed` | 8192 | 25.69M | 1.38 MiB | 0 |
| `local` | `speed` | 32768 | 28.75M | 1.38 MiB | 0 |
| `local` | `speed` | 65536 | 27.23M | 1.38 MiB | 0 |
| `local` | `speed` | 131072 | 39.05M | 1.38 MiB | 0 |
| `local` | `remote` | 1024 | 31.92M | 1.38 MiB | 0 |
| `local` | `remote` | 4096 | 31.79M | 1.38 MiB | 0 |
| `local` | `remote` | 8192 | 26.07M | 1.38 MiB | 0 |
| `local` | `remote` | 32768 | 28.85M | 1.38 MiB | 0 |
| `local` | `remote` | 65536 | 26.66M | 1.38 MiB | 0 |
| `local` | `remote` | 131072 | 38.90M | 1.38 MiB | 0 |
| `remote` | `rss` | 1024 | 25.18M | 1.38 MiB | 0 |
| `remote` | `rss` | 4096 | 25.08M | 1.38 MiB | 0 |
| `remote` | `rss` | 8192 | 26.11M | 1.38 MiB | 0 |
| `remote` | `rss` | 32768 | 28.68M | 1.38 MiB | 0 |
| `remote` | `rss` | 65536 | 31.50M | 1.38 MiB | 0 |
| `remote` | `rss` | 131072 | 38.98M | 1.38 MiB | 0 |
| `remote` | `speed` | 1024 | 23.14M | 1.38 MiB | 0 |
| `remote` | `speed` | 4096 | 23.43M | 1.38 MiB | 0 |
| `remote` | `speed` | 8192 | 23.73M | 1.38 MiB | 0 |
| `remote` | `speed` | 32768 | 25.62M | 1.38 MiB | 0 |
| `remote` | `speed` | 65536 | 25.88M | 1.38 MiB | 0 |
| `remote` | `speed` | 131072 | 36.35M | 1.38 MiB | 0 |
| `remote` | `remote` | 1024 | 23.61M | 1.38 MiB | 0 |
| `remote` | `remote` | 4096 | 23.58M | 1.38 MiB | 0 |
| `remote` | `remote` | 8192 | 23.66M | 1.38 MiB | 0 |
| `remote` | `remote` | 32768 | 25.17M | 1.38 MiB | 0 |
| `remote` | `remote` | 65536 | 25.35M | 1.38 MiB | 0 |
| `remote` | `remote` | 131072 | 37.54M | 1.38 MiB | 0 |
| `reuse` | `rss` | 1024 | 27.21M | 1.38 MiB | 100000 |
| `reuse` | `rss` | 4096 | 28.42M | 1.38 MiB | 100000 |
| `reuse` | `rss` | 8192 | 28.19M | 1.38 MiB | 100000 |
| `reuse` | `rss` | 32768 | 31.36M | 1.38 MiB | 100000 |
| `reuse` | `rss` | 65536 | 33.37M | 1.38 MiB | 100000 |
| `reuse` | `rss` | 131072 | 39.49M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 1024 | 26.25M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 4096 | 25.56M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 8192 | 24.45M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 32768 | 26.87M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 65536 | 26.56M | 1.38 MiB | 100000 |
| `reuse` | `speed` | 131072 | 37.12M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 1024 | 25.71M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 4096 | 26.01M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 8192 | 24.33M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 32768 | 27.31M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 65536 | 26.13M | 1.38 MiB | 100000 |
| `reuse` | `remote` | 131072 | 36.77M | 1.38 MiB | 100000 |

## Trend Read

Good signals:

- `strict` is the best local-only profile for every measured size except
  1024, where `rss` is only slightly ahead.
- `rss` is the best remote and reuse profile for every measured size.
- `reuse` reports 100000 / 100000 reuse hits for every measured size and
  profile, so the current transfer / central reuse paths are functionally
  working in this runner.
- 128K is the strongest broad row: about 39M ops/s for local, remote, and
  reuse in the best profile. This supports the ownerless CentralSpanPool
  direction.
- Remote and reuse throughput generally improves as size increases under
  `rss`, which suggests fixed route / transfer overhead dominates more for
  small objects in the current R1 implementation.

Limits:

- The current `speed` and `remote` profiles are not faster than `rss` in this
  single-process runner. Treat the names as profile intents, not validated
  performance claims.
- `ru_maxrss` is still too small and flat to support RSS claims. It is useful
  only as a smoke-level check here.
- This is not a HZ3 / HZ4 / HZ5 / tcmalloc / mimalloc comparison. The result
  should not be used in paper-facing tables yet.

Working interpretation:

```text
HZ6 R1 is structurally viable.

The immediate performance winner is not the profile named speed.
The immediate remote/reuse winner is the simpler RSS profile.

That implies the current speed/remote knobs are adding overhead in the R1
single-process runner, likely through sharded transfer policy or larger
slow-path structures that do not pay off at this scale.
```

## Next Engineering Questions

```text
1. Why does rss dominate remote/reuse?
   Compare profile configuration, especially transfer backend policy,
   transfer capacity, and source_batch.

2. Does strict remain best for local after larger working sets?
   The current runner is too small to expose retention and refill costs.

3. Does 128K CentralSpanPool hold up in the hakmem-style runner?
   This is the first row worth comparing against HZ3 / HZ4 / HZ5 / tcmalloc.

4. Can 256K / 512K / 1M use the same LargeSpan backend without special cases?
   This is the next implementation proof before broad large-object claims.
```

## Recommended Next Measurement

```text
1. Keep this broad sweep as the HZ6 R1 trend baseline.
2. Add larger working-set / retention tests so RSS becomes meaningful.
3. Add 256K / 512K / 1M LargeSpan classes.
4. Add a hakmem-compatible HZ6 row for RUNS=10 comparison.
```
