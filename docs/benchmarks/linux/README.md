# Linux Benchmark Summaries

This directory is the public home for Ubuntu/Linux benchmark summaries.

Use it for:

- summary-grade markdown
- small reproducible tables
- public notes that can ship with the repository

Keep these elsewhere:

- private raw logs: [private](../../../private)
- vendor/source recovery trees: [private](../../../private)
- generated local outputs: ignored `out_*` and temporary files

## Ubuntu Linux Lanes

Linux benchmarks are published for two CPU lanes:

- `x86_64` (published reference lane)
- `arm64` (Ubuntu on ARM64)

Always label Linux summaries with the CPU architecture and keep arm64 results
separate from x86_64 in tables and comparisons.

For local preparation and compare runs, use the Linux entrypoints:

- `linux/prepare_linux_bench_allocators.sh`
- `linux/build_linux_bench_compare.sh`
- `linux/run_linux_bench_compare.sh`
- `linux/build_linux_hz5_preload_full.sh`
- `linux/run_linux_bench_compare_matrix.sh`
- `linux/run_linux_bench_remeasure_matrix.sh`
- `linux/build_linux_hz6_benchmark.sh`
- `linux/run_linux_hz6_benchmark.sh`
- `hakozuna-hz6/linux/build_hz6_preload_toy_target.sh`
- `hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh`
- `hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_target.sh`

HZ6 allocator names in shared compare matrices:

- `hz6`: selected/default LD_PRELOAD DSO
- `hz6-toy-target`: Toy/mid-small profile/control DSO
- `hz6-aligned-target`: real aligned-allocation fallback profile/control DSO
- `hz6-realloc-boundary-target`: fixed-boundary realloc-growth profile/control DSO
- `hz6-small-boundary-target`: small/fixed-boundary profile/control DSO
- `hz6-small-boundary-trusted-target`: preferred broad small/fixed-boundary profile/control DSO
- `hz6-small-boundary-fast-target`: raw-push comparison small/fixed-boundary profile/control DSO

Keep profile/control DSO results separate from selected-default HZ6 results.
They are useful for targeted workloads, but they are not the default HZ6 lane.

Template:

- [TEMPLATE_ARM64_COMPARE.md](./TEMPLATE_ARM64_COMPARE.md)

Lane map:

- [ARM64_LANE_MAP.md](./ARM64_LANE_MAP.md)

Profiling guide:

- [ARM64_PROFILING.md](./ARM64_PROFILING.md)

Saved result:

- [2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](../2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- [2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md](../2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- [2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md](../2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md)

For the experimental arm64 free-route order-gate tuning preset, use:

- `linux/build_linux_arm64_order_gate_release_lane.sh`
- `linux/run_linux_arm64_order_gate_compare.sh`

Keep this preset private and separate from the shared arm64 default.
The 2026-03-21 follow-up made it stable again on Linux arm64, but the rebuilt
default lane still won on median throughput for the current compare workload.

For the current arm64 profiling read, start with:

- `hz4_free`
- `hz4_large_header_valid`
- `hz4_os_seg_acquire`

## Current Public Ubuntu Reference

The published Ubuntu snapshot currently lives in:

- [docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md](../2026-02-18_PAPER_BENCH_RESULTS.md)

Add future public Ubuntu/Linux summaries here instead of mixing them into Windows-only folders.
