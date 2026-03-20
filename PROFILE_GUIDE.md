# Profile Guide

This is the profile-selection hub for the repository.

Use it to keep lane choice, profiling depth, and platform-specific knobs
separate. Do not collapse Linux arm64, Linux x86_64, macOS, and Windows into
one mixed profiling ledger.

## Start Here

- Linux arm64 lane map: [docs/benchmarks/linux/ARM64_LANE_MAP.md](docs/benchmarks/linux/ARM64_LANE_MAP.md)
- Linux arm64 profiling: [docs/benchmarks/linux/ARM64_PROFILING.md](docs/benchmarks/linux/ARM64_PROFILING.md)
- macOS profiling: [docs/MAC_PROFILING.md](docs/MAC_PROFILING.md)
- Windows profile boundaries: [docs/WINDOWS_PROFILE_BOUNDARIES.md](docs/WINDOWS_PROFILE_BOUNDARIES.md)
- Cross-platform benchmark conditions: [docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md](docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)

## Current Lane Guidance

- `hz3` remains the default local-heavy profile for most workloads.
- `hz4` is the remote-heavy / high-thread profile.
- Linux arm64 results must stay labeled as arm64 until Linux x86_64, Windows x64,
  and macOS are rechecked where relevant.
- A win on arm64 is lane-specific unless the other lanes confirm it.

## When To Profile

- Use the platform-specific profiling page first.
- Freeze workload shape before changing allocator code.
- Keep profile names comparable across OSes, but keep allocator knobs OS-specific.
- Record raw traces and logs under `private/` and publish only summary-grade results.

## Related Docs

- [Repo Structure](docs/REPO_STRUCTURE.md)
- [Linux Entrypoints](linux/README.md)
- [Benchmarks](docs/benchmarks/README.md)
