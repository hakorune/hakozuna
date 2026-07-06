# HZ10ShimThreadStatsCompileGate-L0

Status: GO.

## Shape

Default `libhz10.so` now builds with:

```text
HZ10_ENABLE_SHIM_THREAD_EXIT_STATS=0
```

The dump-only per-thread exit stats path is compiled out of the product
preload lane. A diagnostic sibling keeps the old runtime behavior:

```text
make preload-thread-stats
libhz10_thread_stats.so
HZ10_ENABLE_SHIM_THREAD_EXIT_STATS=1
```

## Correctness

Passed:

```text
make -B preload preload-thread-stats smoke-shim-api smoke-shim-foreign
```

Diagnostic lane check:

```text
LD_PRELOAD=libhz10_thread_stats.so HZ10_SHIM_THREAD_EXIT_STATS=1 larson ...
```

printed per-thread `hz10_shim_exit_stats` lines.

Default lane check:

```text
LD_PRELOAD=libhz10.so HZ10_SHIM_THREAD_EXIT_STATS=1 larson ...
```

printed no thread-exit stats lines, as designed.

## Codegen

`nm -a libhz10.so` shows no `hz10_shim_mark_thread_for_stats*` or
`thread_stats` symbols. `libhz10_thread_stats.so` still contains:

```text
hz10_shim_mark_thread_for_stats_slow
hz10_shim_thread_stats_destructor
hz10_shim_init_thread_stats_key
```

Default `free` no longer pays the thread-stats guard/frame. The diagnostic
sibling still has the expected guard and slow-helper call.

## Macro Guard

RUNS=5, `ALLOCATORS_CSV=hz10`, larson + mstress + sh6bench enabled:

```text
workload      median wall/RSS
sh6bench      0.410s / 318592 KiB
python_alloc  0.840s / 106772 KiB
mstress       0.210s / 205856 KiB
larson        4.171s / 283516 KiB current RSS
redis_setget  0.540s / 8192 KiB
```

Compared with the previous post-NO-GO guard baseline, sh6bench moved from
about 0.420s to 0.410s while the other guarded rows stayed flat or slightly
better. RSS did not regress.

## Decision

GO. Keep per-thread exit stats as a diagnostic sibling, not a default runtime
knob.

This closes the small sh6bench speed-loop. Further speed work on sh6bench
should require a new macro-level reason. The product story should move back to
RSS and macro evidence breadth.
