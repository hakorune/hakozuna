# HZ10 Partial Orphan Adoption Default-Candidate Matrix

Command shape:

```text
ALLOCATORS_CSV=glibc,hz10+orphan,hz10+orphan-partial,tcmalloc,mimalloc
RUNS=3 RUN_LARSON=1 REDIS_OPS=20000 PYTHON_LOOPS=80
LARSON_SECONDS=2 LARSON_CHUNKS=128
make -C hakozuna-hz10 bench-macro-matrix
```

The earlier unfiltered matrix attempted `hz10+fine` on larson and was OOM
killed. This filtered run keeps the decision-relevant rows only.

Median summary:

```text
workload       allocator              wall/rss
python_alloc   hz10+orphan            0.920s / 116,848 KiB
python_alloc   hz10+orphan-partial    0.940s / 116,876 KiB
redis_setget   hz10+orphan            0.540s / 8,064 KiB client
redis_setget   hz10+orphan-partial    0.540s / 8,064 KiB client
redis_server   hz10+orphan            7,712 KiB current
redis_server   hz10+orphan-partial    7,644 KiB current
larson         hz10+orphan            4.305s / 2,687,104 KiB
larson         hz10+orphan-partial    4.215s /   601,216 KiB
```

Competitor context:

```text
larson glibc       272,384 KiB, 2.095M throughput
larson tcmalloc    278,912 KiB, 2.095M throughput
larson mimalloc    285,456 KiB, 2.096M throughput
larson partial     601,216 KiB, 2.086M throughput
```

Verdict: GO for LD_PRELOAD shim default. The source compile-time default
remains off so unrelated public-entry/front-cache lanes stay isolated; the
`preload` target now builds `libhz10.so` with orphan + partial adoption, and
`preload-base` builds the former no-orphan shim as `libhz10_base.so`.
