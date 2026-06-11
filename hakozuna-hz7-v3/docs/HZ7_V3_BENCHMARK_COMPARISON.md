# HZ7 V3 Benchmark Comparison

This note keeps the current v3 Windows probe results in one place so the span
audit story stays easy to read.

All runs below are diagnostic snapshots, not promotion claims.

## Probe Set

```text
hotpath:
  docs/benchmarks/windows/hz7_v3_span_audit_probe/

size-slices:
  docs/benchmarks/windows/hz7_v3_size_slices_probe/
  docs/benchmarks/windows/hz7_v3_size_slices_probe2/

The hotpath and size-slices runners now share their median, formatting, and
captured-process helpers, but they keep separate summary names so the companion
probe stays easy to identify.

Current runner shape:

```text
hotpath summary:
  bench_hz7_v3_hotpath

size-slices summary:
  bench_hz7_v3_size_slices

shared helper:
  win/bench_hz7_v3_common.ps1
```

The shared helper preserves the per-row rate unit, so the size-slices summary
still prints `pairs/s` and `ops/s` in the same table shape as the hotpath
summary.

Latest runner snapshots:

```text
hotpath:
  out_win_hz7_v3_hotpath/20260611_235109_hz7_v3_hotpath_windows.md

size-slices:
  out_win_hz7_v3_size_slices/20260611_235108_hz7_v3_size_slices_windows.md
```
```

## 4K / 8K / 16K Span Audit

The hotpath probe contains the route invariant rows plus the broader bench
sequence. The size-slices companions isolate the 4K / 8K / 16K rows.

| row | hotpath | size_slices_probe | size_slices_probe2 |
| --- | ---: | ---: | ---: |
| malloc_free span4k | 30.675M pairs/s | 48.766M pairs/s | 48.681M pairs/s |
| malloc_free span8k | 35.971M pairs/s | 48.498M pairs/s | 47.984M pairs/s |
| malloc_free span16k | 40.816M pairs/s | 48.705M pairs/s | 48.239M pairs/s |
| route_invariant span4k | 27.027M ops/s | 27.277M ops/s | 26.976M ops/s |
| route_invariant span8k | 27.027M ops/s | 27.245M ops/s | 27.088M ops/s |
| route_invariant span16k | 27.027M ops/s | 27.123M ops/s | 27.062M ops/s |
| route_valid span4k | 121.951M ops/s | 121.445M ops/s | 121.726M ops/s |
| route_valid span8k | 121.951M ops/s | 121.407M ops/s | 121.839M ops/s |
| route_valid span16k | 120.482M ops/s | 122.046M ops/s | 121.414M ops/s |
| route_invalid span4k | 121.951M ops/s | 121.285M ops/s | 121.686M ops/s |
| route_invalid span8k | 121.951M ops/s | 121.331M ops/s | 121.482M ops/s |
| route_invalid span16k | 120.482M ops/s | 121.572M ops/s | 121.641M ops/s |
| malloc_batch span4k | 1.379M ops/s | 17.577K ops/s | 17.981K ops/s |
| malloc_batch span8k | 717.463K ops/s | 702.440K ops/s | 718.389K ops/s |
| malloc_batch span16k | 527.537K ops/s | 527.258K ops/s | 597.856K ops/s |
| free_batch span4k | 5.081M ops/s | 2.844M ops/s | 2.923M ops/s |
| free_batch span8k | 2.327M ops/s | 1.108M ops/s | 2.095M ops/s |
| free_batch span16k | 1.521M ops/s | 818.488K ops/s | 1.252M ops/s |
| free_retained_loop span4k | 49.751M pairs/s | 47.259M pairs/s | 47.027M pairs/s |
| free_retained_loop span8k | 49.505M pairs/s | 47.393M pairs/s | 46.974M pairs/s |
| free_retained_loop span16k | 49.751M pairs/s | 47.494M pairs/s | 47.845M pairs/s |

## What Stands Out

```text
route_valid / route_invalid:
  stable across all three probe sets
  the route invariant helper remains readable and cheap

malloc_free:
  size-slice probes show a much cleaner 4K/8K/16K span path than the broad
  hotpath snapshot

malloc_batch and free_batch:
  these remain the more sensitive rows in the span-audit story
  they are the rows to watch if the next span/free-list cleanup changes behavior

free_retained_loop:
  stays in the same band across all three probe sets
  retained-path behavior remains the steady reference point
```

## Current Reading

The current v3 structure is still aligned with the original goal:

```text
hz7.c
  assembly point for the public API and shared helpers

hz7_route.inc
  route table and retain bucket helpers

hz7_span.inc
  span metadata and free-list helpers

hz7_big.inc
  direct allocation and free / retained flow

win/bench_hz7_v3_hotpath.c
  scenario sequencing and row execution

win/bench_hz7_v3_rows.inc
  shared row definitions and row wrappers
```

The benchmark comparison note is intentionally small: it records what is already
known, and leaves the next span-audit change to the active task board.

The current runner split is intentionally small too:

```text
hotpath:
  full span-audit sequence

size-slices:
  filtered 4K / 8K / 16K companion rows

shared:
  median / formatting / captured-process / summary helpers
```

Those summaries are the current reference points for the companion split.
