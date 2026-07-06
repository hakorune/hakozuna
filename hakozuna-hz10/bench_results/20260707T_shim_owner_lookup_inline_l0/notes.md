# HZ10ShimOwnerLookupInline-L0

Status: GO.

## Box

Inline the already-safe owner lookup helpers in `hz10_public_entry_owner.h`:

- `hz10_public_entry_current_owner_if_any()`: TLS pointer read only.
- `hz10_public_entry_owner_record()`: null guard plus field read only.
- `hz10_public_entry_current_owner()`: TLS pointer read, then cold slow helper
  only on first touch.

The slow helper remains out-of-line, hidden, and noinline. It keeps the owner
allocation and pthread-key destructor registration.

This does not change page ownership, adoption, registry, or thread-exit
semantics.

## Gates

Functional:

```text
make -B -C hakozuna-hz10 preload smoke-shim-api smoke-shim-foreign hz10-standalone-check
make -C hakozuna-hz10 smoke-tsan-aslr-off
```

Both passed.

Codegen:

```text
nm -D hakozuna-hz10/libhz10.so | rg 'hz10_public_entry_current_owner|hz10_public_entry_owner_record|hz10_current_owner'
```

No dynamic exports.

```text
readelf -r hakozuna-hz10/libhz10.so | rg 'TPOFF|DTP|__tls_get_addr|hz10_current_owner'
```

Shows only `R_X86_64_TPOFF64` TLS relocations for this path.

```text
objdump -d hakozuna-hz10/libhz10.so | rg 'hz10_public_entry_current_owner|hz10_public_entry_owner_record|__tls_get_addr'
```

Only the hidden `hz10_public_entry_current_owner_slow` symbol remains.

Perf:

```text
perf record/report, sh6bench, HZ10 preload
```

Filtered flat profile no longer shows
`hz10_public_entry_current_owner`,
`hz10_public_entry_current_owner_if_any`, or
`hz10_public_entry_owner_record`.

## RUNS=5 Macro

Command:

```text
OUTDIR=bench_results/20260707T_shim_owner_lookup_inline_l0 \
ALLOCATORS_CSV=hz10 RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

Median summary:

```text
workload        wall_sec  rss_kb
cache_scratch  1.090     3968
larson         4.179     283768 current
mstress        0.210     203052
python_alloc   0.860     106628
redis_setget   0.540     8064
sh6bench       0.510     321536
xmalloc_test   2.000     13312
```

Comparison to the previous HZ10ShimStatsFastGuard-L0 hz10-only median:

```text
python_alloc   0.880 -> 0.860
larson         4.182 -> 4.179
mstress        0.220 -> 0.210
sh6bench       0.660 -> 0.510
RSS            flat within row noise
```

## Verdict

GO. The box removes hot owner-lookup call boundaries from the preload product
lane without changing allocator routing or ownership contracts. The largest
measured win is sh6bench, which moves from 0.66s to 0.51s median after the
previous stats fast guard.
