# Windows MVP

## Technology

```text
.NET 8
WPF
System.Text.Json
Windows Job Objects for child cleanup
GetProcessMemoryInfo for working-set/private metrics
existing C benchmark executables and PowerShell build scripts
```

WPF is selected for the first release because it is small, stable, easy to run
without packaging infrastructure, and integrates directly with Windows process
controls. Cross-platform UI work is deferred.

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

## Initial Presets

| Preset | Purpose | Existing basis |
|---|---|---|
| Local Mixed | same-thread general throughput | allocator matrix |
| Remote 90 | cross-thread free pressure | MT remote runner |
| Medium Mixed | medium-size throughput/RSS | allocator matrix |
| RSS Turnover | peak and post-workload recovery | turnover/control lanes |

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

- Linux/macOS UI
- Remote-machine execution
- Automatic allocator downloads
- ETW allocation tracing
- Plugin marketplace
- Paper-style statistical analysis beyond median and quartiles
