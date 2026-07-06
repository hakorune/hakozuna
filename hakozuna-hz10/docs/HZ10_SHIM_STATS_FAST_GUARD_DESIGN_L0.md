# HZ10ShimStatsFastGuard-L0

Status: design for the next shim speed box after HZ10ShimTlsModelFix-L0.

## Problem

`hz10_shim_mark_thread_for_stats()` is called by every shim malloc/free entry:

```text
malloc/calloc/realloc alloc path -> hz10_shim_malloc_impl()
free/realloc free path          -> hz10_shim_free_impl()
```

The function is only useful when `HZ10_SHIM_THREAD_EXIT_STATS=1`, which is a
diagnostic lane. In normal product macro runs it should be completely cold.
Perf attribution after the TLS model fix still shows this function around 5%
self time on sh6bench, even when thread-exit stats are not requested.

The current fast path is logically cheap:

```c
if (!hz10_shim_thread_exit_stats || hz10_shim_in_stats_dump) return;
```

But it is still a real call site on every malloc/free in the LD_PRELOAD shim,
and it keeps a diagnostic branch in the product lane.

## Box Shape

Move the hot-path decision to one predictable guard and split the work into a
cold slow helper:

```text
static inline hz10_shim_mark_thread_for_stats_fast():
  if (likely(!hz10_shim_thread_exit_stats)) return
  hz10_shim_mark_thread_for_stats_slow()

noinline hz10_shim_mark_thread_for_stats_slow():
  if (hz10_shim_in_stats_dump) return
  pthread_once(...)
  if key ready and no marker:
    pthread_setspecific(...)
```

Implementation options, in order:

```text
S1. static inline fast guard in hz10_shim.c
    Minimal change. Keeps the same call sites but lets the compiler inline
    the disabled case into one global load + branch.

S2. static inline fast wrapper + noinline slow helper
    Stronger codegen guarantee:
      static inline fast wrapper:
        if (__builtin_expect(hz10_shim_thread_exit_stats, 0))
          slow();
      noinline slow helper:
        pthread_once / getspecific / setspecific
    This keeps call sites clean and makes the product hot path visibly free of
    a stats function call. `noinline` matters under `-flto`: without it, the
    compiler may inline the pthread_* slow body back into malloc/free. That is
    probably still correct with the unlikely branch, but it weakens the
    objdump check. `noinline` turns the intended shape into a codegen
    guarantee.

S3. optional compile-time no-stats preload sibling
    Not recommended for this box. It adds lane complexity for a runtime
    diagnostic knob that should be cheap enough with S1/S2.
```

Preferred implementation: S2. The boundary is clear: product path sees only a
single unlikely branch through a `static inline` wrapper; diagnostic path
remains centralized in one `noinline` slow helper.

## Correctness Contract

No allocator semantics change:

```text
1. HZ10_SHIM_THREAD_EXIT_STATS unset:
   No pthread key is created and no per-thread marker is installed, same as
   today.

2. HZ10_SHIM_THREAD_EXIT_STATS=1:
   First malloc/free on each participating thread still creates the key once
   and installs the marker, so the dump-only destructor still runs.

3. hz10_shim_in_stats_dump:
   Still suppresses recursive marking while stats are being printed.

4. Process exit stats, census, foreign-free policy, orphan adoption, and page
   ownership are untouched.
```

This box must not introduce a destructor reclaim path. It only changes whether
the diagnostic marker setup is called on the hot path.

## Observation Plan

Codegen checks:

```text
objdump -d libhz10.so:
  malloc/free wrappers should not contain pthread_once / pthread_getspecific /
  pthread_setspecific bodies. The disabled path should be one branch around a
  call to the noinline slow helper.

nm -D libhz10.so:
  hz10_shim_mark_thread_for_stats_slow is not exported; it remains an
  internal implementation detail.
```

Functional gates:

```text
make -B preload smoke-shim-api smoke-shim-foreign hz10-standalone-check

HZ10_SHIM_THREAD_EXIT_STATS=1:
  run a small threaded allocation program and verify hz10_shim_exit_stats
  lines still appear at thread exit.
```

Performance gates:

```text
RUNS=5 ALLOCATORS_CSV=hz10:
  sh6bench, python_alloc, mstress from bench-macro-matrix

perf check on sh6bench:
  hz10_shim_mark_thread_for_stats* should disappear from the flat profile or
  fall to noise level. This directly proves that the measured ~5% self-time
  attribution was real and recovered.

Expected:
  sh6bench small win if the perf attribution is real;
  no RSS movement;
  no regression above noise on python_alloc / mstress / larson.
```

GO:

```text
1. smoke/standalone green;
2. thread-exit stats diagnostic still works;
3. objdump confirms the slow pthread_* body is not in the malloc/free wrapper;
4. perf confirms `hz10_shim_mark_thread_for_stats*` is gone or noise-level;
5. sh6bench improves or stays flat;
6. no macro row regresses materially.
```

NO-GO:

```text
Any diagnostic behavior break, new crash, or measurable product-lane
regression. In that case, keep the TLS model fix as the last speed change and
move the next attribution target to owner lookup wrappers.
```

## Next If GO

After this box, rerun perf on sh6bench/mstress. If the owner lookup wrappers
still dominate, open a separate `HZ10ShimOwnerLookupInline-L0` box. Do not mix
that with stats fast-guarding; owner lookup touches allocator routing and
deserves its own proof and A/B.
