# HZ10ShimInternalBinding-L0

Status: GO.

## Box

Added preload-only linker binding:

```text
SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions
```

All `libhz10*.so` preload artifacts now use `SHIM_LDFLAGS`. Smoke binaries and
standalone benches still use normal `LDFLAGS`.

## Gates

Functional:

```text
make -B -C hakozuna-hz10 preload preload-base preload-fine preload-front \
  preload-coarse preload-orphan-adoption preload-orphan-partial \
  smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

Passed.

Codegen:

```text
objdump -d hakozuna-hz10/libhz10.so |
  rg 'call.*<hz10_(malloc|free|page_drain_remote).*@plt>|<hz10_(malloc|free|page_drain_remote)@plt>'
```

No output.

Dynamic exports still include the expected interposition/API symbols:

```text
calloc
free
hz10_free
hz10_malloc
malloc
realloc
```

Perf sh6bench no longer reports internal `hz10_malloc@plt`,
`hz10_free@plt`, or `hz10_page_drain_remote@plt` samples.

## RUNS=5 Macro

Command:

```text
OUTDIR=bench_results/20260707T_shim_internal_binding_l0 \
ALLOCATORS_CSV=hz10 RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

Median summary:

```text
workload        wall_sec  rss_kb
cache_scratch  1.100     3968
larson         4.183     283008 current
mstress        0.210     204556
python_alloc   0.850     106664
redis_setget   0.540     8064
sh6bench       0.470     321024
xmalloc_test   2.000     13184
```

Comparison to HZ10ShimOwnerLookupInline-L0 / speed-stack baseline:

```text
python_alloc   0.860 -> 0.850
larson         4.179 -> 4.183
mstress        0.210 -> 0.210
sh6bench       0.510 -> 0.470
RSS            flat within row noise
```

Full all-allocator RUNS=5 guard:

```text
bench_results/20260707T_shim_internal_binding_l0_full/
```

Default HZ10 remains in the same comparison band:

```text
python_alloc   hz10 0.850s vs glibc 1.210s, tcmalloc 0.830s, mimalloc 0.690s
larson         hz10 4.187s / 284,404 KiB vs tcmalloc 4.153s / 278,784 KiB
mstress        hz10 0.210s / 204,416 KiB vs tcmalloc 0.160s / 218,368 KiB
sh6bench       hz10 0.480s / 318,976 KiB vs tcmalloc 0.320s and mimalloc 0.250s
```

## Verdict

GO. This is a small product-shim entry/binding win with no allocator semantic
changes. It removes internal PLT/interposition cost and captures the
calibrated sh6bench improvement.

Remaining sh6bench gap is now inside `hz10_malloc/free` and the unavoidable
host binary `malloc@plt/free@plt` boundary, not internal HZ10 PLT edges.
