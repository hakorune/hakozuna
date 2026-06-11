# HZ7 V3 Tasks

HZ7 v3 starts from the HZ7 v2 RemoteNatural-L1 closeout code, but it is a new
research folder. V2 remains the selected tiny reference. V3 is allowed to grow
past the v2 line count when the added code directly supports a measured
performance experiment.

## Current Identity

```text
HZ7 v1:
  frozen cute TinyRoute baseline

HZ7 v2:
  closeout reference
  low-RSS route-safe allocator
  coarse-lock remote-free safe
  RemoteNatural-L1 bounded remote pressure evidence

HZ7 v3:
  performance-growth fork
  starts from v2
  first target is local span path improvement
```

## Active Board

```text
done:
  V3Bootstrap-L1
    copied hakozuna-hz7-v2 into hakozuna-hz7-v3
    excluded generated out/ artifacts
    preserved smoke/tests/scripts as the starting point

  RouteInvariantHelper-L1
    one helper now owns VALID / INVALID / MISS checks for user pointers

  SpanFreeListCleanup-L1
    span free-list transitions now use explicit full / empty helpers

  BenchmarkRowRegistry-L1
    span-audit row registry now has a named row group and explicit row count

  BenchmarkRunnerCommon-L1
    hotpath and size-slices now share median / formatting / capture helpers

  BenchmarkComparisonNote-L1
    benchmark comparison note now records shared runner helpers and companion naming

  BenchmarkLatestSnapshot-L1
    comparison note now points at the latest hotpath and size-slices summary snapshots

active:
  SpanPathAudit-L1
    inspect 4K..16K span path before adding new policy
    keep route safety unchanged
    keep RemoteNatural-L1 as a control preset
    keep the route/span/big helper split easy to follow

next:
  extend the v3 hotpath rows to cover 4K, 8K, and 16K span-audit slices
  keep the v3 size-slice wiring as the filtered companion to the hotpath probe
  run HZ7 v3 hotpath and size-slice probes
```

Note:

```text
v3 hotpath:
  local script exists

v3 size slices:
  wired as a filtered companion around the v3 hotpath probe
  keep it focused on the 4K / 8K / 16K span-audit rows
```

Current artifacts:

```text
docs/benchmarks/windows/hz7_v3_span_audit_probe/
  hotpath probe output with route invariant helper rows

docs/benchmarks/windows/hz7_v3_size_slices_probe/
docs/benchmarks/windows/hz7_v3_size_slices_probe2/
  filtered 4K / 8K / 16K size-slice companion runs

docs/HZ7_V3_BENCHMARKS.md
  single place to read the current v3 benchmark surface

docs/HZ7_V3_BENCHMARK_COMPARISON.md
  one-page comparison of the current probe snapshots

docs/HZ7_V3_V2_COMPARISON.md
  one-page summary of how v3 differs from the v2 closeout reference

docs/HZ7_V3_STRUCTURE.md
  one-page note for the current module boundaries

win/bench_hz7_v3_rows.inc
  shared span-audit row definitions and row wrapper helpers

win/bench_hz7_v3_ops.inc
  benchmark operation helpers for individual row bodies

win/bench_hz7_v3_sequences.inc
  ordered benchmark scenario groups used by the hotpath driver
```

The hotpath driver now walks a tiny scenario registry instead of spelling every
sequence out in `main`.

## SpanPathAudit-L1

Motivation:

```text
HZ7 v2 closeout showed:
  direct 16K..32K retained path is strong
  route validation is not the local bottleneck
  4K..16K span-covered range is the main local watch item
```

Scope:

```text
allowed:
  audit h7_small_alloc_from_span
  audit h7_small_free
  add a route invariant helper for alloc/free transition checks
  audit partial/empty span list movement
  audit bitmap/free-list work for 4K / 8K / 16K classes
  add diagnostic-only smoke/probe rows if needed

not allowed in L1:
  TLS cache
  owner-aware remote free
  lock-free remote queue
  remote batching
  broad profile matrix
```

Acceptance:

```text
safety:
  Windows smoke passes
  Linux smoke passes
  route_register_fail = 0 in smoke
  remote-natural smoke remains clean

performance target:
  4K..16K size slice improves by at least 5%
  random_mixed small/medium/mixed does not regress materially
  RSS remains close to v2 baseline

design:
  changes stay understandable
  no v2 closeout docs are rewritten as if v2 were obsolete
```

## V2 Archive In This Folder

The copied `HZ7_V2_*` docs are seed/reference documents. They are intentionally
left in place for now so V3 can cite the exact baseline it forked from. New V3
decisions should go into this file first.
