# Repo Structure

This document is the SSOT for how this repository should separate:

- public vs private materials
- Ubuntu/Linux, macOS, and Windows entrypoints
- source vs generated artifacts vs benchmark assets

Use this before adding new benchmark lanes, release assets, or third-party trees.

## Current Release Stance

- Ubuntu/Linux already has a published GitHub release lane.
- macOS has public development and benchmark entrypoints under `mac/`, and it is treated as a live development lane rather than a published release lane.
- The current GO / NO-GO snapshot for benchmark and lane status is recorded in [`docs/benchmarks/GO_NO_GO_LEDGER.md`](benchmarks/GO_NO_GO_LEDGER.md), and shared workload conditions live in [`docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md`](benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md).
- Mac-specific tuning boxes are documented in [`docs/MAC_DESIGN_BOXES.md`](MAC_DESIGN_BOXES.md).
- Windows-native build and benchmark entrypoints are public and documented under [`win/README.md`](../win/README.md) and [`docs/WINDOWS_BUILD.md`](WINDOWS_BUILD.md).
- Windows benchmark summaries may be published under [`docs/benchmarks/windows`](benchmarks/windows), while private raw assets and third-party recovery trees still stay outside git.
- Public docs may describe Windows bring-up status, but private benchmark assets and raw local traces must stay outside git.
- The first-stop platform entry docs are [`linux/README.md`](../linux/README.md), [`mac/README.md`](../mac/README.md), and [`win/README.md`](../win/README.md).

## Separation Rules

### 1. Public source stays in git

Keep these in the repository:

- allocator source
- public benchmark source
- public build and runner scripts
- public documentation
- small config files needed to reproduce public lanes

### 2. Private assets stay under `private/`

Keep these out of git:

- source recovery trees for external software
- vendor checkouts not prepared for public release
- prebuilt third-party binaries
- large raw benchmark logs
- failure history, dumps, local traces, and working notes
- unpublished paper scratch material
- local paper workspace under `private/paper/`
- local allocator caches and package extracts under `private/bench-assets/linux/allocators/`
- local source package extracts under `private/bench-assets/linux/source-packages/`
- current task memo under `private/current_task.md`
- agent roster and handoff notes under `private/agents.md`
- private journal history under `private/journal/`

### 3. Generated artifacts stay disposable

Build outputs should remain generated and ignored:

- `out_*`
- `*.exe`
- `*.dll`
- `*.lib`
- `*.pdb`
- local benchmark logs and dump files

Do not treat generated outputs as source-of-truth documentation.

## Platform Layout

Platform separation should happen at the build and runner layer first.

Recommended split:

- `win/`: Windows build, run, hook, and bench entrypoints
- `linux/`: Ubuntu/Linux build and run entrypoints (x86_64 and arm64 lanes)
- `mac/`: macOS build and run entrypoints
- allocator core stays shared under [`hakozuna`](../hakozuna) and [`hakozuna-mt`](../hakozuna-mt)

Rules:

- do not duplicate allocator source just to separate OSes
- keep OS-specific shims and launch paths near the platform entrypoints
- keep platform differences concentrated in build flags, wrappers, and compatibility layers
- avoid scattering `#ifdef _WIN32` decisions across hot allocator logic

## Profile Layout

Platform wrappers should own toolchain and launcher details. Profile and lane presets should own workload shape and allocator boxes.

Rules:

- keep workload names aligned where possible across OSes
- keep OS-specific allocator knobs inside OS wrappers or named lane presets
- use `hakozuna-mt/Makefile` lane presets and `docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md` for the profile ledger
- treat a lane win as lane-specific until Linux x86_64, Windows x64, and macOS are rechecked where relevant
- do not promote an arm64 optimization to a shared default without a deliberate regression check on the other target lanes
- avoid mixing launcher decisions with lane decisions

## Benchmark Layout

Public benchmark code and public summaries may live in git.

Recommended split:

- `bench/src/`: public benchmark source
- `bench/configs/`: public benchmark parameter sets
- `bench/bench_mixed_ws.c`: canonical shared mixed working-set compare source
- `docs/benchmarks/windows/`: public Windows summaries
- `docs/benchmarks/linux/`: public Ubuntu/Linux summaries
- `docs/benchmarks/macos/`: public macOS summaries once that lane is promoted
- `private/bench-assets/windows/`: private Windows third-party assets
- `private/bench-assets/linux/`: private Ubuntu/Linux third-party assets
- `private/raw-results/windows/`: raw Windows logs
- `private/raw-results/linux/`: raw Ubuntu/Linux logs

For this repo today:

- Windows public runners already live under [`win`](../win)
- Windows public summaries already live under [`docs/benchmarks/windows`](benchmarks/windows)
- local-only assets already belong under [`private`](../private)
- Windows Redis and future memcached assets should migrate toward:
  - `private/bench-assets/windows/redis/...`
  - `private/raw-results/windows/redis/...`

## Release Policy

### Ubuntu/Linux

- treat the existing GitHub Ubuntu release as the public release lane
- do not backfill private benchmark assets into that release
- public release docs should only point at public artifacts and public summaries

### Windows

- treat Windows-native source, scripts, and summary-grade benchmark docs as public repo content
- do not upload private benchmark trees, private vendor sources, or raw local result bundles
- keep Windows release assets source-only unless a binary lane is explicitly prepared and documented

## Practical Git Policy

Safe to commit:

- source code
- runner scripts
- reproducible public benchmark summaries
- public release notes
- public docs about build steps and structure

Do not commit:

- `private/`
- vendor drops not cleared for release
- redis/memcached source recovery trees
- raw local benchmark outputs
- temporary binaries, dumps, and traces

## Immediate Direction

1. Keep Windows build and benchmark entrypoints under [`win`](../win)
2. Add a `linux/` entrypoint layer when Ubuntu-side repo runners are organized
3. Keep private source recovery and third-party assets below [`private`](../private)
4. Only publish summary-grade benchmark outputs under [`docs/benchmarks`](benchmarks)
5. Treat Windows release preparation as a separate box from private raw-asset handling
6. Keep real app-bench source trees and raw logs under `private/bench-assets` and `private/raw-results`, even when the runner script itself is public
