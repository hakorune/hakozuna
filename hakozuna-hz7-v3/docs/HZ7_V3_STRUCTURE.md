# HZ7 V3 Structure Note

This note records the current module boundaries for HZ7 v3 so the code stays
easy to read as the experiment grows.

## Allocator Side

```text
hz7.c
  common helpers, OS allocation, public API assembly

hz7_route.inc
  route table, retain buckets, route lookup and user-ptr validation helpers

hz7_span.inc
  span metadata, bitmap/free-list helpers, partial/empty span movement
  span slot take/release helpers keep the alloc/free path compact
  small-span acquire helper keeps the partial/empty choice readable
  free-side reclassify helper keeps the partial/empty/release path readable
  route invariant helper feeds span/user-ptr validation

hz7_big.inc
  direct allocation, free path, retained direct flow, stats
```

## Benchmark Side

```text
win/bench_hz7_v3_hotpath.c
  benchmark driver and top-level orchestration

win/bench_hz7_v3_ops.inc
  benchmark operation helpers for individual row bodies

win/bench_hz7_v3_rows.inc
  shared span-audit row definitions and row wrapper helpers

win/bench_hz7_v3_common.ps1
  shared runner helpers for the Windows benchmark scripts
  shared hotpath-row parser keeps the runner scripts thinner
  shared probe helper now owns build / run / summary plumbing for hotpath-style
  runners
  shared filtered-companion helper now owns the size-slices summary wiring too
  shared span-audit key list keeps the size-slices ordering out of the runner

win/bench_hz7_v3_sequences.inc
  ordered scenario groups and registry-driven execution
```

## Current Shape

The hotpath driver is intentionally small:

```text
main
  -> scenario registry
    -> scenario group helpers
      -> row helpers
        -> operation helpers
```

The allocator itself follows the same pattern:

```text
public API
  -> route helpers
    -> route invariant helper
  -> span helpers
  -> big helpers
```

## Why This Exists

The v3 objective is not just to make the allocator faster. It is to keep the
experiment readable while growing the span path and the benchmark coverage.
This note acts as the current source-of-truth for the code layout.

See also:

```text
docs/HZ7_V3_V2_COMPARISON.md
```
