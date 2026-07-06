# HZ10Hz8MacroRssCheck-L0 No-Redis Attempt

Status: STOPPED BY HZ8 ABORT.

Purpose: compare HZ8 against the current HZ10 product shim on macro RSS rows
after the first all-row attempt hit a Redis SIGSEGV under HZ8.

Command shape:

```text
HZ10_EXT_ROOT=/mnt/workdisk/public_share/hakozuna_repo
RUNS=5 RUN_REDIS=0
ALLOCATORS_CSV=glibc,hz8,hz10,tcmalloc,mimalloc
RUN_LARSON=1 RUN_XMALLOC=1 RUN_CACHE_SCRATCH=1 RUN_MSTRESS=1 RUN_SH6BENCH=1
```

Result:

```text
glibc: completed RUNS=5 across enabled rows.
hz8 python_alloc run=1: 2.60s / 4,325,240 KiB.
hz8 larson run=1: abort after 1.226s / sampled RSS 913,664 KiB.
stderr: h8_platform_commit: Cannot allocate memory.
```

Read: HZ8 is not a stronger product macro RSS lane under this workload shape.
Even with Redis disabled, it hit a large python_alloc RSS blow-up and then
aborted in the thread-churn row before a RUNS=5 median could be formed.
