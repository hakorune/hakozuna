# HZ10ShimStatsFastGuard-L0

Goal:
remove dump-only `HZ10_SHIM_THREAD_EXIT_STATS` marker setup from the normal
LD_PRELOAD malloc/free hot path.

## Implementation

`src/hz10_shim.c` now uses:

```text
static inline hz10_shim_mark_thread_for_stats_fast()
  if (__builtin_expect(hz10_shim_thread_exit_stats, 0))
    hz10_shim_mark_thread_for_stats_slow()

__attribute__((noinline)) hz10_shim_mark_thread_for_stats_slow()
  hz10_shim_in_stats_dump guard
  pthread_once / pthread_getspecific / pthread_setspecific
```

The default disabled path is a single branch. The pthread key work remains
centralized and cold.

## Gates

Green:

```text
make -B -C hakozuna-hz10 preload smoke-shim-api smoke-shim-foreign \
  hz10-standalone-check
git diff --check
```

Diagnostic smoke:

```text
HZ10_SHIM_THREAD_EXIT_STATS=1 HZ10_SHIM_EXIT_STATS_CLASSES=0 \
LD_PRELOAD=libhz10.so /tmp/hz10_thread_stats_smoke
```

Verified stderr still contains:

```text
hz10_shim_exit_stats summary ...
hz10_shim_exit_stats class_totals ...
hz10_shim_orphan_adoption_stats totals ...
```

Codegen:

```text
free:
  load hz10_shim_thread_exit_stats
  test
  jne slow
  normal free path

malloc:
  load hz10_shim_thread_exit_stats
  test
  jne slow
  normal malloc path
```

`hz10_shim_mark_thread_for_stats_slow` is internal only, not exported by
`nm -D libhz10.so`.

Perf:

```text
perf record/report, sh6bench, hz10:
  hz10_shim_mark_thread_for_stats* no longer appears in the filtered flat
  profile. Top remaining entries are hz10_free, hz10_malloc, free, malloc,
  and owner lookup wrappers.
```

## RUNS=5 Macro Subset

`ALLOCATORS_CSV=hz10 RUNS=5`

```text
workload       before TLS-fix retry   after stats guard
python_alloc   0.900s / 106,772 KiB   0.880s / 106,628 KiB
redis_setget   0.550s /   8,064 KiB   0.540s /   8,064 KiB
larson         4.186s / 283,776 KiB   4.182s / 286,592 KiB
xmalloc_test   2.000s /  13,312 KiB   2.000s /  14,080 KiB
cache_scratch  1.090s /   3,968 KiB   1.090s /   3,968 KiB
mstress        0.220s / 204,268 KiB   0.220s / 203,432 KiB
sh6bench       0.700s / 320,896 KiB   0.660s / 319,616 KiB
```

## Verdict

GO.

This is a small product-lane win with no semantic change. It validates the
perf attribution: the stats marker cost disappears from the flat profile and
sh6bench improves by about 6% over the TLS-fix full-retry median.

Next attribution target, if continuing speed work:

```text
HZ10ShimOwnerLookupInline-L0
```

Keep that as a separate box. Owner lookup touches allocator routing and should
not be mixed with diagnostic-path cleanup.
