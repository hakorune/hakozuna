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
```

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
  ../out_win_hz7_v3_hotpath/20260612_002922_hz7_v3_hotpath_windows.md

size-slices:
  ../out_win_hz7_v3_size_slices/20260612_002928_hz7_v3_size_slices_windows.md

batch-focus:
  ../out_win_hz7_v3_batch_focus/20260612_003905_hz7_v3_batch_focus_windows.md
```

## 4K / 8K / 16K Span Audit

The hotpath probe contains the route invariant rows plus the broader bench
sequence. The size-slices companions isolate the 4K / 8K / 16K rows. The
hotpath sequence also carries the direct retained 32K / 64K companion rows so
their retained-path behavior stays visible in the same registry.

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

## Latest Refresh

The most recent one-run smoke refresh keeps the same registry shape and updates
the current local evidence with a 20260612 timestamp.

| row | hotpath latest | size-slices latest |
| --- | ---: | ---: |
| malloc_free span4k | 31.746M pairs/s | 48.866M pairs/s |
| malloc_free span8k | 37.313M pairs/s | 48.579M pairs/s |
| malloc_free span16k | 41.322M pairs/s | 48.483M pairs/s |
| route_valid span4k | 123.457M ops/s | 123.065M ops/s |
| route_valid span8k | 123.457M ops/s | 123.037M ops/s |
| route_valid span16k | 123.457M ops/s | 121.864M ops/s |
| route_invalid span4k | 123.457M ops/s | 122.514M ops/s |
| route_invalid span8k | 125.000M ops/s | 122.465M ops/s |
| route_invalid span16k | 123.457M ops/s | 122.758M ops/s |
| free_retained_loop span4k | 49.751M pairs/s | 47.834M pairs/s |
| free_retained_loop span8k | 49.261M pairs/s | 48.157M pairs/s |
| free_retained_loop span16k | 49.505M pairs/s | 47.658M pairs/s |

## Direct Retained Companion

The hotpath companion rows now surface the direct retained 32K / 64K path
explicitly. That keeps the retained direct path visible alongside the span
audit rows without changing the companion split.

| row | hotpath latest |
| --- | ---: |
| free_batch direct32k | 956.297K ops/s |
| free_batch direct64k | 874.126K ops/s |
| free_retained_loop direct32k | 58.140M pairs/s |
| free_retained_loop direct64k | 58.139M pairs/s |
| malloc_batch direct32k | 513.716K ops/s |
| malloc_batch direct64k | 591.716K ops/s |
| malloc_free direct32k | 51.282M pairs/s |
| malloc_free direct64k | 51.546M pairs/s |
| route_invalid direct32k | 121.951M ops/s |
| route_invalid direct64k | 121.951M ops/s |
| route_valid direct32k | 125.000M ops/s |
| route_valid direct64k | 125.000M ops/s |

## Empty Span Cap Smoke

The runner-only `EmptySpanCap` knob was tested as an isolated span/free-list
experiment. The cap=8 smoke kept the registry shape readable, but it did not
beat the current default on the sensitive batch rows, so the knob stays
experimental.

| row | default hotpath | cap8 hotpath | default size-slices | cap8 size-slices |
| --- | ---: | ---: | ---: | ---: |
| malloc_batch span4k | 1.291M ops/s | 1.016M ops/s | 18.435K ops/s | 17.968K ops/s |
| malloc_batch span8k | 612.820K ops/s | 725.321K ops/s | 624.943K ops/s | 698.303K ops/s |
| malloc_batch span16k | 678.933K ops/s | 517.411K ops/s | 638.767K ops/s | 576.784K ops/s |
| free_batch span4k | 5.086M ops/s | 5.411M ops/s | 2.968M ops/s | 2.744M ops/s |
| free_batch span8k | 2.491M ops/s | 2.109M ops/s | 2.135M ops/s | 1.913M ops/s |
| free_batch span16k | 1.605M ops/s | 1.607M ops/s | 1.331M ops/s | 1.163M ops/s |
| malloc_free span4k | 31.746M pairs/s | 30.581M pairs/s | 48.866M pairs/s | 48.552M pairs/s |
| malloc_free span8k | 37.313M pairs/s | 36.765M pairs/s | 48.579M pairs/s | 48.433M pairs/s |
| malloc_free span16k | 41.322M pairs/s | 40.816M pairs/s | 48.483M pairs/s | 48.164M pairs/s |

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

direct retained 32K / 64K:
  now visible in the hotpath companion rows
  the retained direct path stays part of the same benchmark registry

free path reserved-slot lookup:
  uses the region header hint before the exact-base fallback
  latest 3-run smoke showed it was a no-go; exact-base lookup stays the
  simpler reference for now
```

## Batch Focus Probe

The dedicated batch-focus probe isolates the sensitive `malloc_batch` and
`free_batch` rows plus the adjacent retained rows, so future span/free-list
experiments can be compared without rerunning the whole hotpath matrix.

| row | batch-focus latest |
| --- | ---: |
| malloc_batch span4k | 1.339M ops/s |
| malloc_batch span8k | 662.691K ops/s |
| malloc_batch span16k | 597.086K ops/s |
| free_batch span4k | 5.126M ops/s |
| free_batch span8k | 2.513M ops/s |
| free_batch span16k | 1.689M ops/s |
| malloc_free span4k | 50.000M pairs/s |
| malloc_free span8k | 49.751M pairs/s |
| malloc_free span16k | 50.000M pairs/s |
| free_retained_loop span4k | 50.761M pairs/s |
| free_retained_loop span8k | 49.505M pairs/s |
| free_retained_loop span16k | 51.282M pairs/s |
| malloc_batch direct32k | 537.663K ops/s |
| malloc_batch direct64k | 595.664K ops/s |
| free_batch direct32k | 948.587K ops/s |
| free_batch direct64k | 876.501K ops/s |
| malloc_free direct32k | 59.524M pairs/s |
| malloc_free direct64k | 59.172M pairs/s |
| free_retained_loop direct32k | 59.524M pairs/s |
| free_retained_loop direct64k | 59.524M pairs/s |

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
  full span-audit sequence plus direct retained 32K / 64K companions

size-slices:
  filtered 4K / 8K / 16K companion rows

shared:
  median / formatting / captured-process / summary helpers
```

Those summaries are the current reference points for the companion split.
