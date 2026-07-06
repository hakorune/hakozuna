# HZ10 Macro RSS Confirm

Status: COMPLETED.

Purpose: rerun the current HZ10 product shim with the same no-Redis macro
shape used for the HZ8 comparison attempts.

Command shape:

```text
HZ10_EXT_ROOT=/mnt/workdisk/public_share/hakozuna_repo
RUNS=5 RUN_REDIS=0
ALLOCATORS_CSV=hz10
RUN_LARSON=1 RUN_XMALLOC=1 RUN_CACHE_SCRATCH=1 RUN_MSTRESS=1 RUN_SH6BENCH=1
```

Median result:

```text
workload        wall     RSS/current RSS
python_alloc    0.840s   106,796 KiB
larson          4.183s   282,624 KiB current
xmalloc_test    2.000s    13,184 KiB
cache_scratch   1.100s     4,096 KiB
mstress         0.220s   204,524 KiB
sh6bench        0.420s   319,872 KiB
```

Read: the current HZ10 product shim remains stable in the macro rows that HZ8
failed or could not finish in this check.
