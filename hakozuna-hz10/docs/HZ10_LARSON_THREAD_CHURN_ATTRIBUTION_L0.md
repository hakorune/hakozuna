# HZ10LarsonThreadChurnAttribution-L0

Status: scoped, implementation in progress.

Purpose: explain the expanded macro matrix larson result before designing a
fix. The first `HZ10MacroMatrixExpand-L0` run showed:

```text
larson current RSS:
  glibc/tcmalloc/mimalloc: ~272-284MB
  hz10:                   ~7.8GB
  hz10+fine:              ~9.2GB
```

That is large enough to open a thread-churn attribution box, but not enough
to choose a policy. This box measures only; it must not reclaim pages or
change thread-exit ownership.

## Hypotheses

The larson RSS can come from several mechanisms:

```text
H1 orphaned owner-thread pages:
  Worker threads exit with their TLS active/retired pages still resident.

H2 live working set:
  The benchmark genuinely keeps enough live objects to explain the RSS.

H3 page-pool retention:
  Pages are free but cached in the global page pool.

H4 metadata slab overhead:
  Page metadata/pending/spread nodes grow with page count.

H5 class rounding / fine-class spread:
  Fine classes reduce python_alloc rounding but can increase page count and
  retained footprint under threaded churn.
```

## Measurement Shape

Add a diagnostic-only thread-exit stats mode:

```text
HZ10_SHIM_THREAD_EXIT_STATS=1
```

When enabled, the shim marks threads that touch the allocator and registers
a pthread TLS destructor. The destructor dumps the same parseable
`hz10_shim_exit_stats` lines as the existing atexit path, but for the
exiting thread's TLS state.

Load-bearing rule:

```text
This is dump-only. It must not call
hz10_public_entry_flush_thread_cache_quiescent(), destroy pages, drain
remote frees, or change ownership. Automatic reclaim remains forbidden by
the flush contract.
```

The existing `HZ10_SHIM_EXIT_STATS=1` process atexit dump remains useful
for global pool/metadata summary and for the main thread. For larson
attribution, use both env vars on HZ10 lanes:

```text
HZ10_SHIM_EXIT_STATS=1 HZ10_SHIM_THREAD_EXIT_STATS=1
```

## Sweep

Run smaller larson shapes before returning to the full macro row:

```text
threads: 1, 2, 4
chunks/thread: 32, 64, 128
seconds: 1 or 2
min/max: 8..128
allocators: hz10, hz10+fine; external allocators only as RSS references
```

Required output:

```text
summary.tsv:
  allocator, threads, chunks, wall_sec, sampled_current_rss_kb,
  larson_throughput

thread_stats.tsv:
  allocator, threads, chunks, run, class_id, slot_size, slot_count,
  active_pages, retired_pages, page_bytes, evictions, retired,
  reclaimed_ready, reclaimed_sweep, reclaimed_local_free

totals:
  sum of thread active_pages/retired_pages/page_bytes, process atexit
  pool_cached_bytes, metadata live/slab bytes, sampled current RSS.
```

Interpretation:

```text
If summed thread page_bytes tracks current RSS:
  H1 is the main driver; open ownership/thread-exit handoff design.

If current RSS is much larger than summed thread page_bytes:
  inspect page pool and metadata; do not design handoff first.

If hz10+fine always worsens larson page_bytes/current RSS:
  fine classes stay a python/RSS diagnostic lane, not a broad shim opt-in.
```

## Gate

This box is complete when it can say which component explains the 7-9GB
larson RSS. A fix is a separate box.

## Implementation Record 20260707

Implemented:

```text
HZ10_SHIM_THREAD_EXIT_STATS=1:
  dump-only pthread TLS destructor that records the exiting thread's
  hz10_shim_exit_stats. It never calls quiescent flush and never reclaims.

HZ10_SHIM_EXIT_STATS_CLASSES=0:
  optional output limiter for high-churn runs. Class totals still print;
  per-class rows are suppressed.

scripts/run_hz10_larson_thread_churn_attribution.sh:
  shape sweep runner for glibc/hz10/hz10+fine, sampled current RSS,
  larson throughput, thread totals, and attribution_summary.tsv.
```

Validation:

```text
make -B -C hakozuna-hz10 preload preload-fine smoke-shim-api
bash -n scripts/run_hz10_larson_thread_churn_attribution.sh
standalone check
```

First sweep:
`bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/`
with `RUNS=1`, `threads=1,2,4`, `chunks/thread=32,64,128`,
`seconds=1`, `min/max=8..128`.

Key result from `attribution_summary.tsv`:

```text
allocator  threads  chunks  current RSS  thread page bytes  explain
hz10       1        32      1898MB       1788MB             0.942
hz10       2        32      3447MB       3247MB             0.942
hz10       4        32      5330MB       5021MB             0.942
hz10       4        64      4812MB       4536MB             0.943
hz10       4        128     5264MB       4963MB             0.943

hz10+fine  1        32      1918MB       1812MB             0.944
hz10+fine  2        32      3516MB       3321MB             0.945
hz10+fine  4        32      5434MB       5133MB             0.945
hz10+fine  4        128     5360MB       5067MB             0.945
```

All HZ10 rows had `retired_pages=0` in the summed thread totals. The RSS is
not primarily page-pool retention, retired backlog, or metadata. It is
active pages owned by short-lived larson worker threads and left resident
after those threads exit. The current RSS explanation ratio is consistently
about 94-95%; the remaining 5-6% is compatible with executable/stack/libc
baseline, metadata, and sampling noise.

Fine classes are worse on this row: they reduce dump count because each dump
covers a different class table shape, but they increase total active pages,
RSS, and reduce throughput relative to default HZ10.

Verdict:

```text
H1 confirmed: orphaned owner-thread active pages explain the larson RSS.
H3/H4 are not primary for this row.
H5 confirmed negative for larson: fine classes worsen thread-churn RSS and
throughput.
```

Next design box should be a thread-exit ownership/handoff design. It must
respect the existing flush contract: no automatic quiescent reclaim unless a
new protocol proves foreign frees cannot race the dying owner. Candidate
directions to review:

```text
1. ownership handoff of abandoned pages to a global orphan owner/drainer;
2. explicit thread-exit API for applications/benchmarks that can provide a
   true quiescent boundary;
3. conservative abandoned-page quarantine with remote-free-safe eventual
   reclamation.
```
