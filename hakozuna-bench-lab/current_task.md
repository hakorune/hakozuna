# Hakozuna Bench Lab Current Task

## Goal

Build a Windows-first GUI that runs existing allocator benchmarks in isolated
child processes and visualizes throughput, peak RSS, and post-workload RSS.

## Current Phase

```text
phase: documentation and skeleton preparation
platform: Windows 10/11 x64
stack: .NET 8 + WPF
measurement: existing native benchmark executables
```

## Implementation Order

1. Create the .NET solution and four project skeletons.
2. Implement allocator/workload manifest models and validation.
3. Implement the immutable run plan and result protocol.
4. Add the Windows child-process agent with timeout and tree cleanup.
5. Parse allocator matrix and MT remote output markers.
6. Build Run Setup and Result Detail before charts.
7. Add throughput/RSS charts and JSON/CSV/Markdown export.
8. Run an end-to-end system/HZ8/mimalloc/tcmalloc smoke.

## Guardrails

```text
GUI never loads allocator DLLs.
Preview and Verified samples never mix.
Diagnostic builds never appear as speed results.
Failed samples remain visible.
Existing command-line runners remain authoritative.
Linux UI work waits until the Windows MVP is stable.
```
