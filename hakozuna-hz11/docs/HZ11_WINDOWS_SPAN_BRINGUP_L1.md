# HZ11 Windows Span Bring-up L1

Status: Windows span/classify smoke passes.

This is the next rung after `HZ11_WINDOWS_BRINGUP_L0.md`. It verifies that the
HZ11 span/classify lane can be built and smoke-tested on Windows with a
Windows-native arena backing.

## Scope

```text
Goal:
  Build and run hz11_thread_cache_smoke with HZ11_CLASSIFY_SPAN=1 on Windows.

Backing:
  VirtualAlloc reserve+commit arena
  CRITICAL_SECTION returned-object locks
  INIT_ONCE span initialization

Still not included:
  HZ11_TRANSFER_CENTRAL_SPAN
  current-span thread-exit salvage
  Windows DLL/interposition
  fine128 Windows claim
  real-app claim
```

## Result

```text
command:
  powershell -ExecutionPolicy Bypass -File .\hakozuna-hz11\scripts\build_hz11_win64_smoke.ps1

outputs:
  hz11_thread_cache_smoke_win.exe
  hz11_thread_cache_smoke_span_win.exe

result:
  token smoke ok
  span smoke ok
```

Observed span smoke line:

```text
hz11_thread_cache_smoke[span] stats:
  span_create=6
  cached_bytes=4592
  ok
```

## Reading

```text
GO:
  HZ11 Windows span/classify can now build and pass smoke.

NOT CLAIMED:
  Linux fine128 parity
  transfer-cache behavior
  remote/mixed behavior
  RSS quality
  malloc replacement / DLL provider behavior
```

The next clean step is to connect an opt-in `hz11-span` row to the existing
Windows random_mixed runner and measure it as a Windows L1 diagnostic row.

## Follow-Up

`hz11-span` became the L1 Windows control row for random_mixed and the legacy
allocator matrix. It has since been superseded as the selected Windows row by
`hz11-span-cache256`.

```text
random_mixed:
  docs/benchmarks/windows/hz11_l1_span_random_mixed_connectivity/20260709_142042_paper_random_mixed_windows.md

allocator matrix:
  docs/benchmarks/windows/hz11_l1_allocator_matrix_connectivity/20260709_143810_allocator_matrix.md
```

`hz11-span-transfer` was also probed as a direct port of the Linux
transfer/central middle-end. It is a Windows NO-GO for selected-row promotion:
it builds and runs, but does not improve `balanced` throughput and greatly
increases peak RSS.

```text
record:
  docs/no_go/HZ11_WINDOWS_SPAN_TRANSFER_MATRIX_L2.md
```

`hz11-span-cache256` is the current selected Windows HZ11 bring-up row:

```text
record:
  docs/HZ11_WINDOWS_SPAN_CACHE256_L3.md
```
