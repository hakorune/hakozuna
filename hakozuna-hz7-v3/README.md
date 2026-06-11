# Hakozuna HZ7 TinyRoute V3

HZ7 v3 is the performance-growth fork of the HZ7 tiny allocator line.

```text
hakozuna-hz7/
  v1 frozen cute TinyRoute baseline

hakozuna-hz7-v2/
  RemoteNatural-L1 closeout
  selected tiny route-safe reference

hakozuna-hz7-v3/
  new research folder
  starts from v2 code
  may spend more lines on performance experiments
```

V3 starts as a copy of v2 so it inherits the useful contract:

```text
API:
  h7_malloc(size)
  h7_calloc(count, size)
  h7_free(ptr)
  h7_route(ptr)
  h7_stats()

Route:
  MISS     foreign pointer / not owned
  VALID    currently active exact pointer
  INVALID  HZ7-owned-looking inactive/interior/retained pointer

Threading:
  coarse global lock
  cross-thread free is safe
  RemoteNatural-L1 preset exists for bounded route pressure

Shape:
  small/medium spans through 16KiB
  direct retained buckets for 32KiB and 64KiB
  larger direct OS regions
```

## Current Module Layout

V3 keeps the allocator easy to read by splitting the small route/span logic out
of the main translation unit:

```text
hz7.c
  common helpers, OS allocation, public API wiring

hz7_route.inc
  route table, retain buckets, route lookup helpers

hz7_span.inc
  span metadata, bitmap/free-list helpers, partial/empty span movement

hz7_big.inc
  direct allocation, malloc/free API wiring, stats and retained direct flow

win/bench_hz7_v3_hotpath.c
  benchmark driver and scenario sequencing

win/bench_hz7_v3_ops.inc
  operation helpers for the benchmark rows

win/bench_hz7_v3_rows.inc
  shared size-row definitions and row wrapper helpers

win/bench_hz7_v3_common.ps1
  shared runner helpers for hotpath and size-slices summaries

win/bench_hz7_v3_sequences.inc
  ordered benchmark scenario groups used by the hotpath driver
```

The hotpath driver now walks the scenario registry instead of spelling every
sequence out in `main`.

The Windows benchmark scripts share a common helper layer for median
formatting, captured-process handling, and summary generation. The hotpath
script and the size-slices companion keep separate summary names so the
comparison notes stay easy to read.

## V3 Goal

V3 is not a replacement for the v2 closeout reference. It is the place where we
can try one or two larger but still readable experiments without blurring v2.

Initial target:

```text
primary:
  improve local 4K..16K span path
  keep low RSS close to v2
  keep route safety and remote-free safety
  keep the route invariant helper and span/free-list cleanup easy to read

secondary:
  keep RemoteNatural-L1 available as a control
  keep implementation readable enough to remain a teaching allocator
```

## Current Benchmark Plumbing

V3 already has the first row set wired for the span-audit experiment:

```text
win/run_win_hz7_v3_hotpath.ps1
win/run_win_hz7_v3_size_slices.ps1

docs/benchmarks/windows/hz7_v3_span_audit_probe/
docs/benchmarks/windows/hz7_v3_size_slices_probe/
docs/benchmarks/windows/hz7_v3_size_slices_probe2/
```

The hotpath probe now carries the route invariant helper rows, and the size
slices companion keeps the experiment focused on 4K / 8K / 16K rows.
The benchmark driver itself now keeps the scenario sequence readable by moving
shared row helpers into `win/bench_hz7_v3_rows.inc`.

See also:

```text
docs/HZ7_V3_BENCHMARKS.md
docs/HZ7_V3_BENCHMARK_COMPARISON.md
docs/HZ7_V3_V2_COMPARISON.md
docs/HZ7_V3_STRUCTURE.md
```

## Guardrails

Allowed:

```text
span path trim
span class accounting cleanup
small/medium local reuse improvements
route helper cleanup when it supports the main experiment
benchmark plumbing for v3 rows
```

Not for the first V3 pass:

```text
owner-aware remote handoff
owner inbox
TLS ownership
lock-free remote queue
remote batching
HZ6-style profile matrix
production hot-path diagnostics
```

If a V3 experiment needs those features, it should become a different allocator
family instead of silently turning HZ7 into HZ6.

## First Task

Read:

```text
docs/HZ7_V3_TASKS.md
```

The first recommended experiment is `SpanPathAudit-L1`: measure and simplify the
4K..16K span path before adding new remote or TLS machinery.
