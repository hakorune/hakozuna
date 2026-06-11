# HZ7 V3 Benchmarks

HZ7 v3 keeps its benchmark plumbing intentionally small and readable. The goal
is not a broad profile matrix yet, but a focused span-audit flow that makes the
4K / 8K / 16K path easy to inspect.

## Current Windows Probes

```text
win/run_win_hz7_v3_hotpath.ps1
  hotpath probe for the span-audit rows
  includes the route invariant helper rows

win/run_win_hz7_v3_size_slices.ps1
  filtered companion probe for 4K / 8K / 16K slices
  emits a companion summary name instead of the hotpath summary name
  accepts experimental span-cache cap overrides for isolated span/free-list
  trials, but keeps the allocator default unchanged

win/run_win_hz7_v3_batch_focus.ps1
  focused companion probe for the sensitive batch rows
  keeps the span/free-list watch surface smaller than the full hotpath probe

win/bench_hz7_v3_common.ps1
  shared median / formatting / captured-process helpers for the runner scripts
  shared hotpath-row parser used by the hotpath and batch-focus runners
  shared probe helper now owns the repeated build / run / summary plumbing
  shared summary-row parser also feeds the size-slices companion

win/bench_hz7_v3_rows.inc
  shared span-audit row labels and row wrapper helpers used by the driver

win/bench_hz7_v3_ops.inc
  benchmark operation helpers for individual row bodies

win/bench_hz7_v3_sequences.inc
  ordered benchmark scenario groups used by the hotpath driver

docs/HZ7_V3_BENCHMARK_COMPARISON.md
  one-page comparison of the current Windows probe snapshots

docs/HZ7_V3_V2_COMPARISON.md
  one-page summary of how v3 differs from the v2 closeout reference

docs/HZ7_V3_STRUCTURE.md
  one-page note for the current allocator and benchmark module boundaries
```

The hotpath driver walks the scenario registry instead of spelling every
sequence out in `main`.

## Current Artifact Roots

```text
docs/benchmarks/windows/hz7_v3_span_audit_probe/
  hotpath probe output

docs/benchmarks/windows/hz7_v3_size_slices_probe/
docs/benchmarks/windows/hz7_v3_size_slices_probe2/
  filtered size-slice companion outputs

docs/benchmarks/windows/hz7_v3_batch_focus_probe/
  focused batch companion outputs
```

## What v3 Is Measuring Right Now

```text
hotpath:
  route invariant helper
  span audit rows
  local 4K..16K span/free-list behavior
  direct retained 32K / 64K companion rows

size slices:
  4K / 8K / 16K span-audit companion rows
  filtered companion to the hotpath probe
```

## What v3 Is Not Doing Yet

```text
no broad profile matrix
no TLS ownership
no owner-aware remote handoff
no lock-free remote queue
no remote batching
```

The benchmark surface stays narrow until the span path is clearly understood.
Experimental knobs such as `EmptySpanCap` stay in the runner layer until a
specific span/free-list change proves itself worth promoting.
The batch-focus probe exists so the sensitive rows can be refreshed without
rerunning the whole hotpath matrix every time.
