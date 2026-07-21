# Hakozuna Bench Lab

Hakozuna Bench Lab is a Windows-first desktop application for running,
comparing, and visualizing memory allocator benchmarks.

The application does not replace the existing benchmark executables. It gives
them a reproducible GUI entry point and turns their output into comparable
throughput, RSS, latency, and safety reports.

## First Release

```text
platform: Windows 10/11 x64 first; macOS next
UI: .NET 8 + Avalonia UI
execution: isolated child processes
allocators: system, HZ8, mimalloc, tcmalloc
presets: local, remote-heavy, mixed-size, RSS turnover
exports: JSON, CSV, Markdown
```

The first release is Windows-first, with macOS as the second desktop target.
Linux support should reuse the result protocol after the two desktop workflows
are stable; it must not delay the Windows MVP.

## Core Rule

The GUI must never load an allocator DLL into its own process. Each allocator
is exercised by a prepared benchmark executable or preload/interpose child
process. A crash, invalid DLL, or allocator bug must not take down the result
browser.

## Measurement Modes

| Mode | Purpose | UI behavior | Claim quality |
|---|---|---|---|
| Preview | Fast exploration | Live progress and approximate RSS chart | Directional only |
| Verified | Comparable result | No high-frequency polling; charts render after completion | Reportable |

Verified mode records the executable hash, allocator identity, arguments,
machine information, run order, exit code, and raw stdout/stderr. A result is
not reportable when required runs fail or allocator identity is ambiguous.

## Documentation

- [Product charter](docs/CHARTER.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Windows MVP](docs/MVP_WINDOWS.md)
- [Result protocol](docs/RESULT_PROTOCOL.md)
- [Current task](current_task.md)

## Non-Goals for the MVP

- Editing allocator source code from the GUI
- Silently downloading third-party allocator binaries
- Claiming cross-platform parity
- Showing diagnostic-counter builds as speed results
- Running arbitrary untrusted DLLs inside the GUI process
- Replacing paper-quality command-line runners

The product promise is simple: select allocators and a workload, run them under
the same conditions, and understand the speed/RSS tradeoff without reading
PowerShell scripts.
