# Upper1p5CacheShapeAudit-L1

Behavior change:

```text
none to default p2-v0
```

Builds:

```text
make bench-release bench-release-upper1p5
```

Rows:

```text
guard_local0:
  size=16..2048, remote_pct=0, RUNS=5

small_interleaved_remote90:
  size=16..4096, remote_pct=90, interleaved=1, live_window=4096, RUNS=5

small_phase_remote90:
  size=16..4096, remote_pct=90, interleaved=0, RUNS=5
```

Raw logs:

```text
bench_results/20260623T190034Z_upper1p5_cache_shape_p2_local_r5.log
bench_results/20260623T190034Z_upper1p5_cache_shape_upper1p5_local_r5.log
bench_results/20260623T190034Z_upper1p5_cache_shape_p2_interleaved_r5.log
bench_results/20260623T190034Z_upper1p5_cache_shape_upper1p5_interleaved_r5.log
bench_results/20260623T190034Z_upper1p5_cache_shape_p2_phase_r5.log
bench_results/20260623T190034Z_upper1p5_cache_shape_upper1p5_phase_r5.log
```

Objdump:

```text
bench_results/20260623T190034Z_upper1p5_cache_shape_p2_objdump.txt
bench_results/20260623T190034Z_upper1p5_cache_shape_upper1p5_objdump.txt
```

## Throughput

| Row | p2-v0 | upper1p5 | Delta |
|---|---:|---:|---:|
| `guard_local0` | 391.08M | 284.81M | -27.2% |
| `small_interleaved_remote90` | 54.43M | 53.25M | -2.2% |
| `small_phase_remote90` | 3.46M | 3.74M | +8.1% |

## RSS / Faults

| Row | p2-v0 peak | upper1p5 peak | Delta |
|---|---:|---:|---:|
| `small_interleaved_remote90` | 42.47MiB | 20.98MiB | -50.6% |
| `small_phase_remote90` | 3803.55MiB | 3381.71MiB | -11.1% |

Phase minor faults:

```text
p2-v0:
  966,934 median

upper1p5:
  859,480 median

delta:
  -11.1%
```

## Code Shape

P2-v0 remains the compact bit-width / shift path.

Upper1p5 adds:

```text
malloc:
  size-class compare chain
  shift table load
  factor-3 adjustment for 1536 / 3072 classes

free:
  shift table load
  magic multiply / division-by-3 check for factor-3 classes
```

Hard checks:

```text
__tls_get_addr in malloc/free leaf:
  none

target h8_malloc_inner / h8_free_inner div/idiv:
  none
```

The full objdump has unrelated divide instructions outside the hot leaf, so do
not use whole-file `div` counts as the leaf verdict.

## Decision

```text
upper1p5 default:
  HOLD / NO-GO for small-v0 style default

reason:
  phase RSS and phase throughput improve, but guard_local0 regresses by 27%

interleaved remote:
  acceptable regression band in this R5, but not enough to offset local loss
```

The useful insight is that `1536` is expensive because it is common in
`16..2048` local workloads and forces non-power-of-two decode on a hot row.

Next candidate should preserve the p2-v0 path for `<=2048` and only test upper
refinement above 2048, such as a `3072-only` candidate, before any broader
class-map redesign.
