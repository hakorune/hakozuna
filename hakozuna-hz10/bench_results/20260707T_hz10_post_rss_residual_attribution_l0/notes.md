# HZ10PostRssResidualAttribution-L0

Status: COMPLETED.

Purpose: measure why HZ10 keeps more post-workload RSS than HZ8 on the HZ8
public matrix rows.

Command shape:

```text
HZ10_SHIM_EXIT_STATS=1
HZ10_SHIM_EXIT_STATS_CLASSES=0
LD_PRELOAD=libhz10.so
THREADS=16 ITERS=50000 RUNS=3
```

Harness:

```text
hakozuna-hz8/bench/bench_matrix_malloc.c
```

Median first-pass attribution:

```text
row                         post RSS    live pages  metadata  first residual
small_interleaved_remote90   47.3MB      0.59MB     0.72MB    46.0MB
main_interleaved_r90         93.5MB      0.59MB     2.82MB    90.0MB
medium_interleaved_r50       62.3MB      0.59MB     3.15MB    58.5MB
main_local0                  35.4MB      0.59MB     0.59MB    34.2MB
medium_local0                 6.4MB      0.59MB     0.20MB     5.6MB
```

The missing bucket is orphan registry depth: pages published by exited owners
for future adoption no longer appear in live class lists, but remain mapped.

Median with orphan registry:

```text
row                         post RSS    orphan depth  orphan capacity bytes
small_interleaved_remote90   47.3MB       700          45.9MB
main_interleaved_r90         93.5MB     2,731         179.0MB
medium_interleaved_r50       62.3MB     3,030         198.6MB
main_local0                  35.4MB       570          37.4MB
medium_local0                 6.4MB       160          10.5MB
```

Read: HZ10's HZ8-matrix post-RSS gap is dominated by orphan-registry retention,
not active pages, retired pages, page-pool cache, or metadata slabs. The next
RSS box should be an orphan registry trim policy, with larson/thread-churn as
the regression guard.
