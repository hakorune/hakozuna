# Repo Structure

This document is the SSOT for how this repository should separate:

- public vs private materials
- Ubuntu/Linux vs Windows entrypoints
- source vs generated artifacts vs benchmark assets

Use this before adding new benchmark lanes, release assets, or third-party trees.

## Current Release Stance

- Ubuntu/Linux already has a published GitHub release lane.
- Windows is still a bring-up lane and should be developed without mixing private assets into public release contents.
- Public docs may describe Windows bring-up status, but private benchmark assets and raw local traces must stay outside git.

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

- [`win`](/C:/git/hakozuna-win/win): Windows build, run, hook, and bench entrypoints
- `linux/`: Ubuntu/Linux build and run entrypoints
- allocator core stays shared under [`hakozuna`](/C:/git/hakozuna-win/hakozuna) and [`hakozuna-mt`](/C:/git/hakozuna-win/hakozuna-mt)

Rules:

- do not duplicate allocator source just to separate OSes
- keep OS-specific shims and launch paths near the platform entrypoints
- keep platform differences concentrated in build flags, wrappers, and compatibility layers
- avoid scattering `#ifdef _WIN32` decisions across hot allocator logic

## Benchmark Layout

Public benchmark code and public summaries may live in git.

Recommended split:

- `bench/src/`: public benchmark source
- `bench/configs/`: public benchmark parameter sets
- `docs/benchmarks/windows/`: public Windows summaries
- `docs/benchmarks/linux/`: public Ubuntu/Linux summaries
- `private/bench-assets/windows/`: private Windows third-party assets
- `private/bench-assets/linux/`: private Ubuntu/Linux third-party assets
- `private/raw-results/windows/`: raw Windows logs
- `private/raw-results/linux/`: raw Ubuntu/Linux logs

For this repo today:

- Windows public runners already live under [`win`](/C:/git/hakozuna-win/win)
- Windows public summaries already live under [`docs/benchmarks/windows`](/C:/git/hakozuna-win/docs/benchmarks/windows)
- local-only assets already belong under [`private`](/C:/git/hakozuna-win/private)
- Windows Redis and future memcached assets should migrate toward:
  - `private/bench-assets/windows/redis/...`
  - `private/raw-results/windows/redis/...`

## Release Policy

### Ubuntu/Linux

- treat the existing GitHub Ubuntu release as the public release lane
- do not backfill private benchmark assets into that release
- public release docs should only point at public artifacts and public summaries

### Windows

- keep Windows as a bring-up and release-prep lane until public assets are clean
- do not upload private benchmark trees, private vendor sources, or raw local result bundles
- when Windows is ready, prepare a separate Windows release lane instead of mixing it into private bring-up state

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

1. Keep Windows build and benchmark entrypoints under [`win`](/C:/git/hakozuna-win/win)
2. Add a `linux/` entrypoint layer when Ubuntu-side repo runners are organized
3. Keep private source recovery and third-party assets below [`private`](/C:/git/hakozuna-win/private)
4. Only publish summary-grade benchmark outputs under [`docs/benchmarks`](/C:/git/hakozuna-win/docs/benchmarks)
5. Treat Windows release preparation as a separate box from current Windows bring-up
6. Keep real app-bench source trees and raw logs under `private/bench-assets` and `private/raw-results`, even when the runner script itself is public
