# HZ10 macro width L0

Purpose: widen the product/shim evidence beyond the original
python_alloc / redis_setget / larson trio.

Command:

```text
OUTDIR=hakozuna-hz10/bench_results/20260707T_hz10_macro_width_l0 \
ALLOCATORS_CSV=glibc,hz10,tcmalloc,mimalloc \
RUNS=5 \
make -C hakozuna-hz10 bench-macro-matrix
```

New rows added to `scripts/run_hz10_macro_preload_matrix.sh`:

```text
xmalloc_test   xmalloc-test -w 4 -t 2 -s -1
cache_scratch  cache-scratch 4 5000 256 5000 4
mstress        mstress 8 80 3
sh6bench       sh6bench with input: call_count=100000, min=1, max=1000,
               threads=4 (the local binary still reports 16 threads)
```

Median summary:

```text
workload       allocator   wall      RSS/current RSS
cache_scratch  glibc      1.090s      3,456 KiB
cache_scratch  hz10       1.090s      3,968 KiB
cache_scratch  mimalloc   1.090s      5,724 KiB
cache_scratch  tcmalloc   1.100s      7,552 KiB

larson         glibc      4.144s    272,384 KiB
larson         hz10       4.184s    283,264 KiB
larson         mimalloc   4.150s    285,360 KiB
larson         tcmalloc   4.148s    278,784 KiB

mstress        glibc      0.210s    363,184 KiB
mstress        hz10       0.220s    207,136 KiB
mstress        mimalloc   0.170s    321,692 KiB
mstress        tcmalloc   0.160s    219,136 KiB

python_alloc   glibc      1.160s     92,448 KiB
python_alloc   hz10       0.950s    106,748 KiB
python_alloc   mimalloc   0.680s    102,616 KiB
python_alloc   tcmalloc   0.830s    104,576 KiB

redis_setget   all rows   0.550s      8 MiB client RSS

sh6bench       glibc      0.940s    424,576 KiB
sh6bench       hz10       0.800s    319,744 KiB
sh6bench       mimalloc   0.250s    273,868 KiB
sh6bench       tcmalloc   0.320s    271,872 KiB

xmalloc_test   glibc      2.040s    199,052 KiB
xmalloc_test   hz10       2.000s     13,952 KiB
xmalloc_test   mimalloc   2.000s     20,560 KiB
xmalloc_test   tcmalloc   2.030s    196,096 KiB
```

Verdict:

```text
GO for widened macro matrix as the new product-lane evidence surface.

HZ10 is now competitive on python_alloc, redis_setget, larson, cache_scratch,
and xmalloc_test. It has a strong RSS story on xmalloc_test, mstress, and
sh6bench. The honest remaining wall-time misses are mstress and sh6bench
against tcmalloc/mimalloc; keep those as product-lane deltas, not blockers
for the comparability claim.
```
