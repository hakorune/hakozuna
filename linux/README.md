# Linux Entrypoints

This directory is the public Ubuntu/Linux entrypoint layer for this repository.

Use it to keep Linux build and smoke commands in one place without mixing:

- allocator core source
- private benchmark assets
- generated outputs

## What Lives Here

- [build_linux_release_lane.sh](/C:/git/hakozuna-win/linux/build_linux_release_lane.sh): public build wrapper for the current Ubuntu release lane
- [build_linux_arm64_release_lane.sh](/C:/git/hakozuna-win/linux/build_linux_arm64_release_lane.sh): explicit Ubuntu arm64 build wrapper
- [build_linux_bench_compare.sh](/C:/git/hakozuna-win/linux/build_linux_bench_compare.sh): build the Linux benchmark compare binary
- [build_linux_arm64_bench_compare.sh](/C:/git/hakozuna-win/linux/build_linux_arm64_bench_compare.sh): explicit Ubuntu arm64 benchmark build wrapper
- [build_linux_arm64_order_gate_release_lane.sh](/C:/git/hakozuna-win/linux/build_linux_arm64_order_gate_release_lane.sh): explicit Ubuntu arm64 order-gate build wrapper for experimental tuning
- [run_linux_preload_smoke.sh](/C:/git/hakozuna-win/linux/run_linux_preload_smoke.sh): minimal `LD_PRELOAD` smoke runner for `hz3` and `hz4`
- [run_linux_bench_compare.sh](/C:/git/hakozuna-win/linux/run_linux_bench_compare.sh): build, prepare allocators, and run the Linux benchmark compare lane
- [run_linux_arm64_bench_compare.sh](/C:/git/hakozuna-win/linux/run_linux_arm64_bench_compare.sh): explicit Ubuntu arm64 benchmark compare wrapper
- [run_linux_arm64_order_gate_compare.sh](/C:/git/hakozuna-win/linux/run_linux_arm64_order_gate_compare.sh): explicit Ubuntu arm64 order-gate compare wrapper for experimental tuning
- [prepare_linux_bench_allocators.sh](/C:/git/hakozuna-win/linux/prepare_linux_bench_allocators.sh): local `mimalloc` / `tcmalloc` cache prep for benchmark runs
- [run_bench_compare.sh](/Users/tomoaki/git/hakozuna/linux/run_bench_compare.sh): thin Linux frontend for the shared allocator compare runner

## Quick Start

```bash
cd /path/to/hakozuna-win
./linux/build_linux_release_lane.sh
./linux/build_linux_arm64_release_lane.sh
./linux/run_linux_preload_smoke.sh hz3 /bin/true
./linux/run_linux_preload_smoke.sh hz4 /bin/true
./linux/run_linux_bench_compare.sh
```

## Ubuntu Lane Split

Ubuntu/Linux is one entrypoint layer with two CPU lanes:

- `x86_64` (published reference lane)
- `arm64` (Ubuntu on ARM64, including Apple Silicon Linux VMs)

The same scripts are used for both lanes. Always record the CPU architecture in
benchmark summaries and keep Linux `arm64` results separate from Linux `x86_64`.

## Build Boxes

Default behavior:

- `hz3`: `make -C hakozuna clean all_ldpreload_scale`
- `hz4`: `make -C hakozuna-mt clean all_stable`

For the explicit arm64 lane, use:

```bash
./linux/build_linux_arm64_release_lane.sh
```

For benchmark compare runs on arm64, use:

```bash
./linux/run_linux_arm64_bench_compare.sh
```

For the experimental arm64 free-route order-gate tuning preset, use:

```bash
./linux/run_linux_arm64_order_gate_compare.sh
```

Keep this preset separate from the shared arm64 default.
The Linux arm64 follow-up on 2026-03-21 made it stable again, but the rebuilt
default lane still won on median throughput for the current compare workload.

You can select a different public `hz3` lane by passing a make target:

```bash
./linux/build_linux_release_lane.sh --hz3-target all_ldpreload_scale_r90_pf2_s97_2
```

Examples:

- `all_ldpreload_scale`
- `all_ldpreload_fast`
- `all_ldpreload_scale_r50`
- `all_ldpreload_scale_r90`
- `all_ldpreload_scale_r90_pf2_s97_2`

## GO / NO-GO And Conditions

- Quick GO / NO-GO status: [docs/benchmarks/GO_NO_GO_LEDGER.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)
- Shared workload conditions: [docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- Linux arm64 preload ownership fix results: [docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](/home/tomoaki/hakozuna/docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- Linux arm64 lane map: [docs/benchmarks/linux/ARM64_LANE_MAP.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/linux/ARM64_LANE_MAP.md)
- Linux arm64 profiling guide: [docs/benchmarks/linux/ARM64_PROFILING.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/linux/ARM64_PROFILING.md)
- macOS is tracked as a separate Apple Silicon M1 development lane; see [mac/README.md](/Users/tomoaki/git/hakozuna/mac/README.md) and [docs/MAC_BENCH_PREP.md](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md)

## Published Ubuntu Lane

Ubuntu/Linux is the already-published public release lane for this repository.

Current public references:

- [README.md](/C:/git/hakozuna-win/README.md)
- [docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md](/C:/git/hakozuna-win/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md)
- [docs/benchmarks/linux/README.md](/C:/git/hakozuna-win/docs/benchmarks/linux/README.md)
- [bench/README.md](/Users/tomoaki/git/hakozuna/bench/README.md)

## Historical Notes

Some older docs still mention paths such as `hakozuna/hz3` or runner scripts under `scripts/`.

In this repo snapshot:

- `hz3` builds are driven from [hakozuna/Makefile](/C:/git/hakozuna-win/hakozuna/Makefile)
- `hz4` builds are driven from [hakozuna-mt/Makefile](/C:/git/hakozuna-win/hakozuna-mt/Makefile)
- public Linux summaries should collect under [docs/benchmarks/linux](/C:/git/hakozuna-win/docs/benchmarks/linux)
- private raw logs and vendor trees should stay under [private](/C:/git/hakozuna-win/private)

## See Also

- [mac/README.md](/Users/tomoaki/git/hakozuna/mac/README.md)
- [win/README.md](/Users/tomoaki/git/hakozuna/win/README.md)
- [docs/REPO_STRUCTURE.md](/Users/tomoaki/git/hakozuna/docs/REPO_STRUCTURE.md)
