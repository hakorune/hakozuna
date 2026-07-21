# Architecture

## Process Boundary

```text
HakozunaBenchLab (Avalonia UI)
  |
  +-- validates allocator/workload manifests
  +-- creates immutable run plan
  +-- starts HakozunaBenchAgent.exe
        |
        +-- applies process affinity and priority policy
        +-- launches existing benchmark executable
        +-- captures stdout/stderr and exit status
        +-- records process memory at the allowed cadence
        +-- writes one normalized result bundle
  |
  +-- reads completed bundles
  +-- renders charts and exports reports
```

The agent exists so benchmark execution can later be reused by CLI and Linux
front ends. The GUI must not parse allocator-specific output directly. The
process-control implementation is platform-specific; the run plan and result
model are shared.

## Proposed Projects

```text
src/HakozunaBenchLab.App/       Avalonia desktop application
src/HakozunaBenchLab.Core/      manifests, plans, result model, statistics
src/HakozunaBenchLab.Agent/     child-process execution and capture
tests/HakozunaBenchLab.Tests/   parser, validation, and report tests
manifests/allocators/            allocator identities and artifact paths
manifests/workloads/             versioned workload presets
```

## Measurement Isolation

Verified mode uses fresh child processes and alternating allocator order. The
GUI does not animate live charts during a timed sample. High-frequency process
polling, diagnostic counters, and background compilation are prohibited.

Preview mode may poll RSS for an approximate live chart, but its result bundle
is marked `preview` and cannot be merged with Verified samples.

## Failure Boundary

The agent reports timeout, crash, allocation failure, malformed output, and
identity mismatch as explicit sample states. A nonzero exit does not crash the
GUI and does not disappear from the report.

## Extensibility

Allocator and workload additions should normally require manifests and output
adapters, not new UI code. Linux support should implement another execution
backend while preserving the same run-plan and result schemas.
