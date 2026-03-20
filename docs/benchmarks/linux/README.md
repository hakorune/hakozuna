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
