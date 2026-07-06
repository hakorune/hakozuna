# HZ10 Shim Default Partial Adoption Confirmation

Command shape:

```text
ALLOCATORS_CSV=hz10,hz10-base RUNS=2 RUN_LARSON=1
REDIS_OPS=10000 PYTHON_LOOPS=40 LARSON_SECONDS=2 LARSON_CHUNKS=128
make -C hakozuna-hz10 bench-macro-matrix
```

This confirms that `make preload` / `libhz10.so` now uses orphan + partial
adoption by default, while `make preload-base` / `libhz10_base.so` preserves
the former no-orphan behavior.

Median summary:

```text
workload       allocator    wall/rss
larson         hz10         4.181s /   602,752 KiB
larson         hz10-base    4.205s / 9,216,704 KiB
python_alloc   hz10         0.495s / 116,754 KiB
python_alloc   hz10-base    0.615s / 116,910 KiB
redis_setget   hz10         0.270s / 8,064 KiB client
redis_setget   hz10-base    0.275s / 8,064 KiB client
```

Verdict: default shim wiring is correct. Keep `libhz10_base.so` as rollback
and A/B reference.
