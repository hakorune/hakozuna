# HZ10ShimThreadStatsCompileGate-L0

Status: GO.

## Question

Can the product preload lane stop paying any hot-path cost for dump-only
per-thread exit stats?

`HZ10ShimStatsFastGuard-L0` already moved marker setup behind a noinline slow
helper and an unlikely branch. Post-leaf-split attribution still shows the
exported `malloc`/`free` shim wrappers carrying a frame and a cold guard for a
diagnostic path that is normally disabled.

## Shape

Add a compile-time gate:

```c
HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
```

Default:

```text
HZ10_ENABLE_SHIM_THREAD_EXIT_STATS=0
```

The default `libhz10.so` compiles out:

- `HZ10_SHIM_THREAD_EXIT_STATS` env parsing
- pthread key / marker / destructor registration
- `hz10_shim_mark_thread_for_stats_slow()`
- the hot malloc/free marker call body

Diagnostic sibling:

```text
libhz10_thread_stats.so
make preload-thread-stats
```

This sibling uses the same shim default feature set as `libhz10.so`
(orphan + partial adoption + fine size classes), plus
`HZ10_ENABLE_SHIM_THREAD_EXIT_STATS=1`. Only this sibling honors
`HZ10_SHIM_THREAD_EXIT_STATS=1`.

## Result

Implemented as designed.

```text
make preload-thread-stats
libhz10_thread_stats.so
```

Default `libhz10.so` no longer contains `hz10_shim_mark_thread_for_stats*` or
thread-stats symbols. The diagnostic sibling still prints per-thread
`hz10_shim_exit_stats` lines with `HZ10_SHIM_THREAD_EXIT_STATS=1`, while the
default lane ignores that env knob.

RUNS=5 macro guard:

```text
sh6bench      0.410s / 318592 KiB
python_alloc  0.840s / 106772 KiB
mstress       0.210s / 205856 KiB
larson        4.171s / 283516 KiB current RSS
redis_setget  0.540s / 8192 KiB
```

Decision: GO. This is the last small sh6bench speed box; close that loop and
return the product story to RSS/macro evidence breadth.

## Contract

No allocator semantics change:

- malloc/free/calloc/realloc/alignment behavior is unchanged.
- process exit stats (`HZ10_SHIM_EXIT_STATS=1`) stay available in default.
- census (`HZ10_SHIM_CENSUS_SEC=N`) stays available in default.
- thread-exit stats remain dump-only in the diagnostic sibling and still must
  not reclaim, flush, or destroy pages from pthread destructors.

Default `libhz10.so` intentionally ignores `HZ10_SHIM_THREAD_EXIT_STATS=1`.
This is a lane selection now, not a runtime product knob.

## Gates

Correctness:

```text
make -B preload preload-thread-stats smoke-shim-api smoke-shim-foreign
make hz10-standalone-check
git diff --check
```

Diagnostic:

```text
LD_PRELOAD=libhz10_thread_stats.so HZ10_SHIM_THREAD_EXIT_STATS=1 ...
```

must still print per-thread `hz10_shim_exit_stats` lines.

Codegen:

- default `libhz10.so` should not contain exported/hot references to
  `hz10_shim_mark_thread_for_stats*`
- default malloc/free wrappers should not carry pthread key setup bodies
- `libhz10_thread_stats.so` should still contain the slow helper

Performance:

Run at least:

```text
RUNS=5 ALLOCATORS_CSV=hz10
RUN_SH6BENCH=1 RUN_MSTRESS=1 RUN_LARSON=1
```

GO if default macro rows improve or stay flat with no RSS regression.

## Strategic Boundary

This is the last small, clean shim-speed box for sh6bench. After this gate,
close the sh6bench speed loop unless a new macro-level workload changes the
target. HZ10 already exceeds the original balanced-throughput target on this
row; the product story should move back to RSS and macro evidence breadth.
