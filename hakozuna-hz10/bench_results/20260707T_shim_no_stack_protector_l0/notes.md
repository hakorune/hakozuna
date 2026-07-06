# HZ10ShimNoStackProtector-L0

Status: NO-GO.

## Box

Probe adding `-fno-stack-protector` to `SHIM_CFLAGS` only, affecting preload
`libhz10*.so` artifacts but not debug/smoke/sanitizer/TSan lanes.

Motivation: post-HZ10ShimInternalBinding-L0 annotate showed visible canary cost
inside `hz10_free` due to stack route-result storage.

## Gates

Functional gate passed:

```text
make -B -C hakozuna-hz10 preload preload-base preload-fine preload-front \
  preload-coarse preload-orphan-adoption preload-orphan-partial \
  smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

Codegen gate passed:

```text
objdump -d hakozuna-hz10/libhz10.so --disassemble=hz10_free
```

No `%fs:0x28` canary load/store/check and no `__stack_chk_fail` edge.

## RUNS=5 Macro

Command:

```text
OUTDIR=bench_results/20260707T_shim_no_stack_protector_l0 \
ALLOCATORS_CSV=hz10 RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

Median summary:

```text
workload        wall_sec  rss_kb
cache_scratch  1.090     3968
larson         4.152     283264 current
mstress        0.210     204320
python_alloc   0.870     106800
redis_setget   0.540     8064
sh6bench       0.490     320896
xmalloc_test   2.000     13568
```

Comparison to HZ10ShimInternalBinding-L0:

```text
python_alloc   0.850 -> 0.870
larson         4.183 -> 4.152
mstress        0.210 -> 0.210
sh6bench       0.470 -> 0.490
RSS            flat
```

Perf confirmed the old canary instructions disappeared, but the target row got
slower. Likely cause is code layout / register allocation after removing stack
protector, not allocator semantics.

## Verdict

NO-GO. The Makefile change was reverted. Do not retry stack-protector removal
without new code-layout evidence. Next measured target is runtime division in
pagemap local route / interior validation.
