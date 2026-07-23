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
allocators: system, HZ8, plus explicit mimalloc/tcmalloc provider packs
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
- [Distribution and provider packs](docs/DISTRIBUTION_AND_PROVIDERS.md)
- [Build provider packs](docs/PROVIDER_PACK_BUILD.md)
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

The Compare tab runs the repository's `benchlab compare batch` command for a
suite of allocator manifests and displays the selected profile scorecard. The
presubmit scorecard currently includes system, mimalloc, and tcmalloc. HZ8's
dedicated Windows Preview remains in the Run tab until its runner contract is
adapted to the CLI manifest worker. The CLI remains the source of truth for
raw records and scoring; the GUI is the safe visualization and control
surface.

## Connected Preview

The current Windows Preview can run the repository's `system` and `HZ8` Local
Mixed and Medium Mixed executables from the Run tab. Each runner is staged to a
unique local temporary directory, and its reported throughput and peak RSS are
shown in the UI. Remote 90 and RSS Turnover require dedicated adapters;
mimalloc and tcmalloc require provider packs. These boundaries are intentional:
the GUI never substitutes a different workload and never presents a Preview as
a verified comparison.

## Build The Windows Portable Archive

```powershell
pwsh hakozuna-bench-lab/scripts/build_windows_portable.ps1
```

Extract the generated ZIP and launch `HakozunaBenchLab.App.exe`. Allocator
packs are managed from the Providers tab and stored per user.
