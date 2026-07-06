# HZ8 Public Matrix Recheck

Status: COMPLETED.

Purpose: recheck the intended HZ8 public low-RSS lane after an overly broad
HZ10 macro-probe comparison made HZ8 look worse than its documented scope.

Command shape:

```text
RUNS=3 THREADS=16 ITERS=50000
ALLOCATORS=hz8,mimalloc,tcmalloc,system
ROWS=small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,
     main_local0,medium_local0
```

Representative median post RSS:

```text
row                         hz8 post RSS     tcmalloc post RSS
small_interleaved_remote90   2,981,888 B      33,030,144 B
main_interleaved_r90         4,648,960 B      79,167,488 B
medium_interleaved_r50       4,046,848 B      75,628,544 B
main_local0                  3,538,944 B       9,437,184 B
medium_local0                3,014,656 B       9,568,256 B
```

Read: the HZ8 public claim still holds. It is slower than tcmalloc on raw
throughput, but far lower on post-workload RSS in these same-run rows.
