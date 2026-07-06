# HZ8 vs HZ10 Same HZ8 Harness Recheck

Status: COMPLETED.

Purpose: correct the prior HZ8 comparison scope by running HZ8 and HZ10
preload libraries through the same HZ8 public `bench_matrix_malloc` harness.

Command shape:

```text
RUNS=3 THREADS=16 ITERS=50000
allocators: hz8, hz10
rows:
  small_interleaved_remote90
  main_interleaved_r90
  medium_interleaved_r50
  main_local0
  medium_local0
```

Median results:

```text
row                         allocator   ops/s       post RSS       peak RSS
small_interleaved_remote90   hz8        13.057M      3.17 MiB      28.12 MiB
small_interleaved_remote90   hz10       14.689M     41.25 MiB      41.25 MiB

main_interleaved_r90         hz8         6.590M      4.51 MiB      66.00 MiB
main_interleaved_r90         hz10       12.671M     93.75 MiB      93.75 MiB

medium_interleaved_r50       hz8         9.128M      4.12 MiB      61.62 MiB
medium_interleaved_r50       hz10       19.797M     64.25 MiB      64.25 MiB

main_local0                  hz8       116.986M      3.38 MiB       3.38 MiB
main_local0                  hz10      111.057M     33.62 MiB      33.75 MiB

medium_local0                hz8       100.964M      2.62 MiB       3.00 MiB
medium_local0                hz10      158.531M      6.12 MiB       6.25 MiB
```

Read:

- HZ8 remains the low-post-RSS allocator on its intended public matrix rows.
- HZ10 is frequently faster, especially medium/remote, but it is not a blanket
  replacement for HZ8's RSS recommendation yet.
- Keep HZ8 public-matrix and HZ10 macro/product claims separated.
