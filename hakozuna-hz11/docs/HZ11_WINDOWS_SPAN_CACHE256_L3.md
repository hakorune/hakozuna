# HZ11 Windows Span Cache256 L3

Status: GO as the selected Windows HZ11 bring-up row.

## Question

Can the Windows `hz11-span` row improve the legacy allocator-matrix `balanced`
pressure signal without importing the Linux transfer/central middle-end?

## Diagnosis

`hz11-span-diag` showed that `balanced` was dominated by cache overflow and
returned-object reuse:

```text
hz11-span-diag balanced:
  malloc_hit              28669
  refill                  987139
  overflow                30818
  flush_items             986176
  returned_push           986176
  returned_pop_hit        960795
  returned_pop_miss        26344
```

That means the 32-object per-class front cache was too small for this Windows
matrix shape. The slow path repeatedly flushed to the global returned-object
sink and then reacquired from it.

## Candidate

```text
hz11-span-cache256:
  HZ11_CLASSIFY_SPAN=1
  HZ11_CACHE_CAP=256
  no HZ11_TRANSFER_CENTRAL_SPAN
  no fine128 claim
  no DLL replacement claim
```

## Evidence

### Allocator Matrix

```text
summary:
  docs/benchmarks/windows/hz11_l3_span_cache_tls_matrix_probe/20260709_172413_allocator_matrix.md

balanced single-run probe:
  hz11-span                  9.042M ops/s, peak RSS 42392KB
  hz11-span-tlsfast         10.601M ops/s, peak RSS 41076KB
  hz11-span-cache256        11.271M ops/s, peak RSS 38712KB
  hz11-span-tlsfast-cache256 11.335M ops/s, peak RSS 40100KB
```

Follow-up repeat-5 direct balanced probe:

```text
median ops/s:
  hz11-span                  10.986M
  hz11-span-tlsfast          12.287M
  hz11-span-cache256         13.771M
  hz11-span-tlsfast-cache256 13.001M
```

### Diagnostic Comparison

```text
summary:
  docs/benchmarks/windows/hz11_l3_span_cache256_diag_probe/20260709_172549_allocator_matrix.md

hz11-span-diag:
  ops/s                   10.503M
  overflow                 30818
  flush_items             986176
  malloc_hit               28669

hz11-span-cache256-diag:
  ops/s                   15.776M
  overflow                  3314
  flush_items             848384
  malloc_hit              161584
```

### Random Mixed

```text
summary:
  docs/benchmarks/windows/hz11_l3_span_cache256_random_mixed_probe/20260709_172650_paper_random_mixed_windows.md

RUNS=3:
  small:
    hz11-span          148.208M, RSS 4632KB
    hz11-span-cache256 156.195M, RSS 4252KB

  medium:
    hz11-span          146.162M, RSS 4992KB
    hz11-span-cache256 154.459M, RSS 5016KB

  mixed:
    hz11-span          148.021M, RSS 5044KB
    hz11-span-cache256 154.560M, RSS 5124KB
```

## Decision

```text
GO:
  hz11-span-cache256 becomes the selected Windows HZ11 bring-up row.

KEEP:
  hz11-span remains the L1 control row.
  hz11-span-diag / hz11-span-cache256-diag remain diagnostic-only.

NO:
  do not promote hz11-span-transfer.
  do not claim Linux fine128 parity.
  do not claim Windows DLL replacement behavior.
```

## Reading

The Windows pressure issue was primarily returned-object overflow pressure from
an undersized front cache, not lack of transfer/central span policy. A simple
cache-cap increase keeps the design local, avoids the transfer RSS cliff, and
improves both the legacy matrix and paper-aligned random_mixed rows.
