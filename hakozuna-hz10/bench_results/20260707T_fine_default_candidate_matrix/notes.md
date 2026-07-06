# HZ10 shim fine default candidate matrix

Purpose: re-measure `hz10+fine` after the corrected sibling wiring
(`orphan + partial adoption + fine`) and owner-record split. This is the
default-candidate gate for making fine size classes part of `make preload`.

Command:

```text
OUTDIR=hakozuna-hz10/bench_results/20260707T_fine_default_candidate_matrix \
ALLOCATORS_CSV=glibc,hz10,hz10+fine,tcmalloc,mimalloc \
RUNS=3 \
make -C hakozuna-hz10 bench-macro-matrix
```

Conditions:

```text
python_alloc: PYTHON_LOOPS=80
redis_setget: REDIS_OPS=20000, REDIS_CLIENTS=32
larson:       seconds=2 min=8 max=128 chunks=128 rounds=1 seed=12345 threads=4
```

Median summary:

```text
workload       allocator   wall      rss
python_alloc   glibc      1.190s     92,256 KiB
python_alloc   hz10       0.930s    116,756 KiB
python_alloc   hz10+fine  0.930s    106,788 KiB
python_alloc   tcmalloc   0.840s    104,576 KiB
python_alloc   mimalloc   0.690s    102,492 KiB

larson         glibc      4.135s    272,384 KiB
larson         hz10       4.176s    288,256 KiB
larson         hz10+fine  4.173s    281,856 KiB
larson         tcmalloc   4.142s    279,168 KiB
larson         mimalloc   4.142s    284,124 KiB

redis_setget   all rows   ~0.550s     7,936-8,192 KiB client RSS
```

Verdict: GO for shim-default fine classes. Fine reduced python RSS by
~10 MiB with unchanged wall, reduced larson RSS by ~6.4 MiB with unchanged
wall, and did not move redis. `make preload` now builds `libhz10.so` with
orphan + partial adoption + fine classes. Coarse rollback remains
`libhz10_orphan_partial.so`.
