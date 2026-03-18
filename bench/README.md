# Benchmark Entrypoints

This directory holds the shared benchmark compare core.

The layout is intentionally split into:

- common compare logic in `bench/`
- thin OS-specific frontends in `linux/` and `mac/`

That keeps allocator comparison logic in one place while letting each platform
own its preload and toolchain details.

## Current Scope

- compare `system`, `hz3`, `hz4`, `mimalloc`, and `tcmalloc`
- resolve allocator libraries from environment variables first
- fall back to platform-specific discovery when possible
- use `LD_PRELOAD` on Linux and `DYLD_INSERT_LIBRARIES` on macOS

## Entry Points

- [`run_compare.sh`](run_compare.sh): shared runner
- [`../linux/run_bench_compare.sh`](../linux/run_bench_compare.sh): Linux frontend
- [`../mac/run_bench_compare.sh`](../mac/run_bench_compare.sh): macOS frontend

## Usage

The frontends pass through arguments to the shared runner.

Examples:

```bash
./linux/run_bench_compare.sh --help
./mac/run_bench_compare.sh --help
```

Typical environment overrides:

- `BENCH_BIN`: benchmark binary to run
- `BENCH_ARGS`: benchmark arguments
- `ALLOCATORS`: comma-separated allocator list
- `RUNS`: number of repetitions
- `OUTDIR`: output directory

## Notes

- Keep workload definitions and reporting in the shared core.
- Keep installation and preload mechanics in the OS-specific wrappers.
