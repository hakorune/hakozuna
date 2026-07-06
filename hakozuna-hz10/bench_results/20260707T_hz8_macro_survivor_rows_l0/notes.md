# HZ8 Macro Survivor Rows Probe

Status: STOPPED BY HZ8 HANG.

Purpose: after HZ8 aborted in larson, disable both Redis and larson to see
whether the remaining macro rows can produce a usable RSS comparison.

Command shape:

```text
HZ10_EXT_ROOT=/mnt/workdisk/public_share/hakozuna_repo
RUNS=5 RUN_REDIS=0 RUN_LARSON=0
ALLOCATORS_CSV=hz8
RUN_XMALLOC=1 RUN_CACHE_SCRATCH=1 RUN_MSTRESS=1 RUN_SH6BENCH=1
```

Result:

```text
hz8 python_alloc run=1: 2.60s / 4,324,920 KiB.
hz8 xmalloc_test run=1: killed after more than 3 minutes.
```

`xmalloc-test` was invoked with `-t 2`, so the long-lived process is a
compatibility/liveness failure for this macro lane, not a valid timing sample.
