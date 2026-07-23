# Windows MVP and macOS Follow-up

## Technology

```text
.NET 8
Avalonia UI
System.Text.Json
Windows Job Objects for child cleanup
GetProcessMemoryInfo for working-set/private metrics
existing C benchmark executables and PowerShell build scripts
```

Avalonia is selected so the Windows-first UI can be reused on macOS without a
second screen implementation. Windows remains the first validation target; the
agent's process-control layer is isolated so macOS can later use its own
process-group and memory-query implementation.

## Platform Sequence

```text
1. Windows: existing PowerShell/native runners and Windows memory metrics
2. macOS: same Avalonia screens and result schema, native runner adapter
3. Linux: reuse protocol after desktop workflows are stable
```

## Screens

1. **Run Setup**: allocator cards, workload preset, run count, verified/preview.
2. **Run Monitor**: queue state, current sample, failures, cancel control.
3. **Comparison**: throughput bars, peak/post RSS bars, Pareto scatter.
4. **Result Detail**: arguments, hashes, raw output, individual samples.
5. **Export**: JSON bundle, CSV samples, Markdown summary.

The visual direction should feel like a compact laboratory instrument, not a
generic admin dashboard: warm paper background, ink/oxide colors, tabular
numerals, and restrained motion used only for run-state transitions.

## Initial Allocators

| Allocator | Source | MVP status |
|---|---|---|
| Windows system allocator | benchmark's system build | required |
| HZ8 selected default | `out_win_suite` / `out_win_mt_remote` | required |
| mimalloc | prepared benchmark artifact | required |
| tcmalloc | prepared benchmark artifact | required |
| External DLL | explicit user manifest | stretch |

External DLL support must validate architecture and use a child process. The
application must never call allocator exports directly.

Provider installation and side-by-side versioning are defined in
`DISTRIBUTION_AND_PROVIDERS.md`. System and HZ8 ship with the application;
mimalloc and tcmalloc use explicit downloadable provider packs.

## Initial Presets

| Preset | Purpose | Existing basis |
|---|---|---|
| Local Mixed | same-thread general throughput | allocator matrix |
| Remote 90 | cross-thread free pressure | MT remote runner |
| Medium Mixed | medium-size throughput/RSS | allocator matrix |
| RSS Turnover | peak and post-workload recovery | turnover/control lanes |

## Connected Preview Slice

The first connected GUI slice intentionally stays narrow:

| Allocator | Preset | Status |
|---|---|---|
| Windows system allocator | Local Mixed, Medium Mixed | runnable Preview |
| HZ8 selected Windows build | Local Mixed, Medium Mixed | runnable Preview |
| mimalloc / tcmalloc | all | requires an installed provider pack |
| any allocator | Remote 90, RSS Turnover | requires a dedicated runner adapter |

Preview stages file-backed runners into a unique local temporary directory before
launching them. This avoids executing a benchmark directly from a network or
mapped drive and keeps the GUI process separate from allocator code. The common
runner emits `ops/s` and peak RSS; post-workload RSS remains unavailable until a
turnover-specific adapter is connected.

## MVP Acceptance

- Four allocators can be discovered and validated from manifests.
- Four presets complete from the GUI with fresh child processes.
- Verified mode supports configurable repeat count and alternating order.
- Throughput, peak RSS, post RSS, failures, and run count are visible.
- JSON, CSV, and Markdown exports round-trip through tests.
- Cancel and timeout terminate the full child process tree.
- Diagnostic builds are rejected from Verified speed comparisons.
- One result bundle contains enough command/hash/environment data to rerun it.

## Deferred

- Linux UI
- Remote-machine execution
- Background or silent allocator downloads
- ETW allocation tracing
- Plugin marketplace
- Paper-style statistical analysis beyond median and quartiles
