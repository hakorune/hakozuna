# HZ10OwnerRecordFootprint-L0 Macro Result

Command shape:

```text
ALLOCATORS_CSV=glibc,hz10,tcmalloc,mimalloc
RUNS=3 RUN_LARSON=1 REDIS_OPS=20000 PYTHON_LOOPS=80
LARSON_SECONDS=2 LARSON_CHUNKS=128
make -C hakozuna-hz10 bench-macro-matrix
```

Implementation:

* page owner token now points to a small persistent `Hz10OwnerRecord`;
* the large live `Hz10ThreadOwner` class-state cache is allocated separately
  and released by the pthread-key destructor;
* the destructor registration is always enabled, but orphan registry publish
  remains gated by `HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION`.

Median macro results:

```text
workload       allocator   wall      RSS/current
larson         glibc       4.134s    272,384 KiB
larson         hz10        4.175s    289,536 KiB
larson         tcmalloc    4.141s    279,040 KiB
larson         mimalloc    4.145s    284,016 KiB

python_alloc   glibc       1.220s     92,144 KiB
python_alloc   hz10        0.970s    116,844 KiB
python_alloc   tcmalloc    0.850s    104,320 KiB
python_alloc   mimalloc    0.710s    102,536 KiB

redis_setget   all         ~0.54s      7,936-8,192 KiB client
```

Direct pre-matrix probe:

```text
hz10 larson 5 runs: 284,288 - 290,432 KiB, throughput ~= 2.0956M/s
smaps at plateau:
  total RSS ~= 278,072 KiB
  8MiB pthread stack-cache VMAs ~= 261,956 KiB
  no 1MiB owner-record slab mass remains
```

Verdict: GO. This removes the HZ10-specific owner-record slab RSS cliff and
puts larson RSS/throughput in the same band as glibc/tcmalloc/mimalloc for
this thread-churn shape. Remaining python_alloc RSS is the earlier size-class
rounding/retention story, not owner records.

Gates:

```text
smoke-public-entry: green
smoke-public-entry-orphan-adoption: green
smoke-public-entry-orphan-partial: green
preload/preload-base: green
smoke-shim-api / smoke-shim-foreign: green
hz10-standalone-check: green
smoke-tsan-aslr-off: green
git diff --check: green
```
