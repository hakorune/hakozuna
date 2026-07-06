# HZ10PostRssResidualAttribution-L0

Status: MEASURED.

## Purpose

Treat HZ10 as a separate line from HZ8 and measure whether HZ10's post-workload
RSS can be reduced cleanly.

Do not try to make HZ10 "be HZ8" in one step. HZ8 remains the mature low-RSS
public allocator. HZ10 is the active macro/shim line. This box only asks:

```text
Where does HZ10's extra post RSS live after HZ8 public-matrix rows?
```

## Scope

Rows:

```text
small_interleaved_remote90
main_interleaved_r90
medium_interleaved_r50
main_local0
medium_local0
```

Reference harness:

```text
hakozuna-hz8/bench/bench_matrix_malloc.c
THREADS=16 ITERS=50000 RUNS=3
```

This intentionally matches the HZ8 public matrix lane rather than the HZ10
macro application matrix.

## Observability

Use existing HZ10 shim diagnostics first:

```text
HZ10_SHIM_EXIT_STATS=1
HZ10_SHIM_EXIT_STATS_CLASSES=0
```

Those dump:

```text
class_totals active_pages / retired_pages / page_bytes
pool_cached_bytes
metadata_bytes / live_nodes / free_nodes
orphan adoption totals
```

For short rows, process-exit stats are better than `HZ10_SHIM_CENSUS_SEC=N`
because the census sampler may not wake before the benchmark exits.

## Attribution Model

For each row:

```text
page_bytes = (active_pages + retired_pages) * 64KiB
pool_bytes = pool_cached * 64KiB
metadata_bytes = metadata_slabs * metadata_slab_bytes
accounted_bytes = page_bytes + pool_bytes + metadata_bytes
unaccounted_bytes = post_rss - accounted_bytes
```

Read:

- page_bytes high: tune page retention, retired handling, or post-workload
  quiescent/purge policy.
- pool_bytes high: tune page-pool retention.
- metadata_bytes high: tune metadata slab sizing/density.
- unaccounted high: inspect pagemap leaves, large allocations, loader/runtime,
  stacks, or external allocator state.

## Gates

This is attribution only. No allocator behavior changes.

GO for opening a follow-up RSS trim box requires:

```text
one dominant bucket explains most of the HZ8/HZ10 post-RSS gap
and the implied fix does not weaken fail-closed ownership or remote-free safety
```

NO-GO for immediate implementation if the gap is mostly mixed runtime noise or
requires a geometry rewrite without a clear target.

## Measurement

Log:

```text
bench_results/20260707T_hz10_post_rss_residual_attribution_l0/
```

Command shape:

```text
HZ10_SHIM_EXIT_STATS=1
HZ10_SHIM_EXIT_STATS_CLASSES=0
LD_PRELOAD=libhz10.so
THREADS=16 ITERS=50000 RUNS=3
```

Median first-pass attribution, before adding orphan registry depth:

```text
row                         post RSS    active+retired pages  metadata  residual
small_interleaved_remote90   47.3MB      0.59MB               0.72MB    46.0MB
main_interleaved_r90         93.5MB      0.59MB               2.82MB    90.0MB
medium_interleaved_r50       62.3MB      0.59MB               3.15MB    58.5MB
main_local0                  35.4MB      0.59MB               0.59MB    34.2MB
medium_local0                 6.4MB      0.59MB               0.20MB     5.6MB
```

That made the residual look unaccounted. It was not. The missing bucket is the
orphan adoption registry depth printed by `hz10_shim_orphan_adoption_stats`.
Those pages are no longer in live class lists, so they do not appear in
`class_totals`, but they remain mapped candidates for future adoption.

Median with orphan registry depth:

```text
row                         post RSS    orphan depth  orphan capacity bytes
small_interleaved_remote90   47.3MB       700          45.9MB
main_interleaved_r90         93.5MB     2,731         179.0MB
medium_interleaved_r50       62.3MB     3,030         198.6MB
main_local0                  35.4MB       570          37.4MB
medium_local0                 6.4MB       160          10.5MB
```

The orphan capacity bytes can exceed RSS because not every retained quantum is
resident at sampling time, but the direction is decisive: HZ10's post-RSS gap
on these rows is dominated by orphan-registry retention, not active pages,
retired pages, page-pool cache, or metadata slabs.

## Verdict

The gap is cleanly attributable.

HZ10 is retaining exited-owner pages as adoption candidates. That is useful for
larson/thread-churn throughput and for avoiding the old orphan RSS cliff, but
it conflicts with the HZ8-style post-workload RSS contract measured by the HZ8
public matrix.

This does not mean HZ10 should copy HZ8 wholesale. It means the next RSS box is
a policy box around orphan registry retention:

```text
HZ10OrphanRegistryTrimPolicy-L0
```

Promising shapes:

```text
1. explicit post-workload purge API:
   release orphan registry pages at an application-defined quiescent boundary.

2. environment/product profile:
   HZ10_ORPHAN_REGISTRY_CAP_BYTES or cap-per-class, default tuned separately
   for RSS-first vs throughput-first lanes.

3. idle-age purge:
   keep adoption candidates briefly, then release old orphan pages when the
   registry exceeds a byte cap.
```

Guardrails:

```text
- never destroy pages still reachable from a live owner;
- registry pop remains the exclusive transfer operation;
- purge only fully orphaned registry pages, with pagemap release before
  munmap;
- benchmark both HZ8 public rows and larson/thread-churn macro rows, because
  the same retention is also what makes adoption fast.
```
