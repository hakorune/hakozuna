# HZ6 R1 Benchmark Snapshot (2026-05-28)

This is the first Linux benchmark snapshot after the modular HZ6 R1
LargeSpan CentralSpanPool seed.

This file is a development comparison note. It is not a paper-facing unified
allocator table yet.

## Provenance

```text
result root:
  hakozuna-hz6/private/raw-results/linux/hz6_benchmark_20260528_075731

git sha:
  bb92db8

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
bash hakozuna-hz6/linux/run_hz6_benchmark.sh --skip-build
```

The benchmark binary was built successfully with:

```bash
bash hakozuna-hz6/linux/build_hz6_benchmark.sh
```

The R1 smoke suite passed before this benchmark:

```bash
bash hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

## Scope

The HZ6 runner currently measures HZ6-only single-process lanes:

```text
local:
  malloc/free in the same allocator owner slot

remote:
  malloc in owner slot 0, remote-free through owner slot 1

reuse:
  malloc, remote-free, then malloc again and check pointer reuse
```

Default measured sizes:

```text
local:
  8192
  65536

remote:
  131072

reuse:
  131072
```

This is intentionally smaller than the HZ5 paper matrix. The HZ5 RUNS=10
paper dataset uses the hakmem multi-thread runner across `guard`, `main`,
`mid_only`, `cross128`, and `large128` lanes. Therefore the HZ6 numbers below
should be read as an R1 implementation signal, not as a direct replacement for
the HZ5 paper comparison table.

## HZ6 Median Results

RUNS=5 median.

| Mode | Profile | Size | Median ops/s | Median RSS | Reuse hits | Status |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `local` | `remote` | 65536 | 27.52M | 1.38 MiB | 0 | ok |
| `local` | `remote` | 8192 | 25.89M | 1.38 MiB | 0 | ok |
| `local` | `rss` | 65536 | 32.96M | 1.38 MiB | 0 | ok |
| `local` | `rss` | 8192 | 28.52M | 1.38 MiB | 0 | ok |
| `local` | `speed` | 65536 | 27.52M | 1.38 MiB | 0 | ok |
| `local` | `speed` | 8192 | 25.96M | 1.38 MiB | 0 | ok |
| `local` | `strict` | 65536 | 41.60M | 1.38 MiB | 0 | ok |
| `local` | `strict` | 8192 | 34.84M | 1.38 MiB | 0 | ok |
| `remote` | `remote` | 131072 | 36.12M | 1.38 MiB | 0 | ok |
| `remote` | `rss` | 131072 | 38.94M | 1.38 MiB | 0 | ok |
| `remote` | `speed` | 131072 | 37.02M | 1.38 MiB | 0 | ok |
| `reuse` | `remote` | 131072 | 37.81M | 1.38 MiB | 100000 | ok |
| `reuse` | `rss` | 131072 | 39.75M | 1.38 MiB | 100000 | ok |
| `reuse` | `speed` | 131072 | 37.98M | 1.38 MiB | 100000 | ok |

## Comparison Context

The relevant existing baseline is:

```text
hakozuna_paper/paper/results_hz5_linux_20260526_run10_unified
```

That dataset measured `hz3`, `hz4`, `mimalloc`, `tcmalloc`, `system`, and HZ5
profile rows in the same hakmem runner on the same machine. It remains the
paper-facing dataset for cross-allocator claims.

Important distinction:

```text
HZ5 paper data:
  multi-thread hakmem allocator comparison
  RUNS=10
  paper-facing

HZ6 R1 data:
  HZ6-only single-process development runner
  RUNS=5
  implementation sanity and direction check
```

The HZ6 `remote/reuse 131072` rows are encouraging because the
CentralSpanPool path is already reusing 128K spans consistently in the R1
runner. The `reuse` rows report 100000 / 100000 reuse hits for every measured
profile.

A broader HZ6-only R1 sweep was also recorded after this first snapshot:

```text
docs/HZ6_R1_BROAD_TRENDS_20260528.md
```

That sweep covers 1024, 4096, 8192, 32768, 65536, and 131072 byte rows across
local, remote, and reuse modes. Its current trend is that `strict` wins most
local-only rows while `rss` wins every remote/reuse row in the R1 runner.

The `ru_maxrss` values are very small and nearly flat because this is a short
single-process microbenchmark. Treat them as a smoke-level RSS check only. RSS
claims still need the hakmem-style matrix or a dedicated retention test.

## Engineering Read

Good signals:

- `remote 131072` and `reuse 131072` are already in the 36M-40M ops/s range.
- `reuse 131072` has full pointer reuse hit coverage in the current runner.
- `strict local` is faster than the transfer-oriented profiles on local-only
  8K / 64K rows, which matches the profile model.
- All rows completed with status `ok`.

Limits:

- This does not yet prove HZ6 beats HZ5, HZ4, HZ3, tcmalloc, or mimalloc.
- The runner is not the same as the HZ5 RUNS=10 paper runner.
- `>128KiB` LargeSpan classes are not implemented yet.
- The current RSS signal is too small to use for claims.

Working interpretation:

```text
HZ6 R1 has a good architecture signal.

The CentralSpanPool direction appears viable for the 128K seed:
  ACTIVE -> CENTRAL_FREE
  central pop -> ACTIVE
  full reuse hit rate in the R1 reuse lane

The next proof is not more local R1 timing.
The next proof is a broader runner:
  128K remote pressure
  256K / 512K / 1M LargeSpan classes
  HZ3 / HZ4 / HZ5 / tcmalloc / mimalloc comparison on the same harness
```

## Next Measurement Step

Recommended next benchmark step:

```text
1. Keep this HZ6-only runner as the R1 smoke benchmark.
2. Add a hakmem-compatible HZ6 row or wrapper.
3. Run the existing comparison matrix with:
     hz3
     hz4
     mimalloc
     tcmalloc
     best HZ5 rows
     HZ6 profiles
4. Keep RUNS=10 for paper-facing tables.
```

Until then, this result should be cited only as:

```text
HZ6 R1 CentralSpanPool development snapshot.
```
