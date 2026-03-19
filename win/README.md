# Windows Entrypoints

This directory is the public Windows entrypoint layer for this repository.

Use it to keep Windows build, smoke, profile, and paper lanes in one place
without mixing:

- allocator core source
- private benchmark assets
- generated outputs
- OS-agnostic workload definitions

## What Lives Here

- [`check_windows_env.ps1`](check_windows_env.ps1): quick environment check for the local Windows toolchain
- [`build_win_min.ps1`](build_win_min.ps1): minimal `hz3` build lane
- [`build_win_bench_compare.ps1`](build_win_bench_compare.ps1): allocator compare build lane
- [`build_win_allocator_suite.ps1`](build_win_allocator_suite.ps1): full `crt` / `hz3` / `hz4` / `mimalloc` / `tcmalloc` suite builder
- [`build_win_larson_suite.ps1`](build_win_larson_suite.ps1): Windows Larson suite builder
- [`build_win_mt_remote_suite.ps1`](build_win_mt_remote_suite.ps1): Windows MT remote suite builder
- [`build_win_random_mixed_suite.ps1`](build_win_random_mixed_suite.ps1): Windows `random_mixed` suite builder
- [`build_win_redis_workload_suite.ps1`](build_win_redis_workload_suite.ps1): Windows Redis-workload suite builder
- [`run_win_allocator_suite.ps1`](run_win_allocator_suite.ps1): run the full allocator suite
- [`run_win_allocator_matrix.ps1`](run_win_allocator_matrix.ps1): reusable Windows allocator profile matrix
- [`run_win_paper_windows.ps1`](run_win_paper_windows.ps1): paper-aligned Windows allocator summary
- [`run_win_larson_paper.ps1`](run_win_larson_paper.ps1): paper-aligned Larson lane
- [`run_win_mt_remote_paper.ps1`](run_win_mt_remote_paper.ps1): paper-aligned MT remote lane
- [`run_win_random_mixed_paper.ps1`](run_win_random_mixed_paper.ps1): paper-aligned `random_mixed` lane
- [`run_win_redis_workload_paper.ps1`](run_win_redis_workload_paper.ps1): paper-aligned Redis-workload lane
- [`run_win_redis_real_profile_matrix.ps1`](run_win_redis_real_profile_matrix.ps1): real Redis profile matrix
- [`run_win_redis_real_matrix.ps1`](run_win_redis_real_matrix.ps1): real Redis allocator matrix
- [`run_win_redis_real_sanity.ps1`](run_win_redis_real_sanity.ps1): real Redis sanity run
- [`build_win_inject.ps1`](build_win_inject.ps1): injector build helper
- [`build_win_link.ps1`](build_win_link.ps1): link / hook build helper
- [`build_win_memcached_allocator_variants.ps1`](build_win_memcached_allocator_variants.ps1): memcached allocator variant builder
- [`build_win_memcached_min_main.ps1`](build_win_memcached_min_main.ps1): minimal memcached main builder
- [`build_win_memtier_benchmark.ps1`](build_win_memtier_benchmark.ps1): memtier benchmark builder
- `win/bench_*.c`: Windows benchmark harnesses
- `win/include/`: Windows compatibility headers
- `win/memcached/` and `win/memtier/`: recovered app sources and shims

## Quick Start

```powershell
powershell -ExecutionPolicy Bypass -File .\win\check_windows_env.ps1
powershell -ExecutionPolicy Bypass -File .\win\build_win_allocator_suite.ps1
powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_suite.ps1
```

## Boundary Rules

- Keep Windows toolchain and launcher details in `win/`
- Keep workload profile names aligned with Ubuntu/Linux where possible
- Keep Windows-only allocator knobs in named suite scripts or `hakozuna-mt/Makefile` lane presets
- Keep profile boundaries documented in [docs/WINDOWS_PROFILE_BOUNDARIES.md](../docs/WINDOWS_PROFILE_BOUNDARIES.md)
- Keep build and lane guidance in [docs/WINDOWS_BUILD.md](../docs/WINDOWS_BUILD.md)
- Keep public benchmark summaries under `docs/benchmarks/windows/`
- Keep private raw assets and recovered trees under `private/`

## GO / NO-GO And Conditions

- Quick GO / NO-GO status: [docs/benchmarks/GO_NO_GO_LEDGER.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)
- Shared workload conditions: [docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- macOS is tracked as a separate Apple Silicon M1 development lane; see [mac/README.md](/Users/tomoaki/git/hakozuna/mac/README.md) and [docs/MAC_BENCH_PREP.md](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md)

## See Also

- [linux/README.md](../linux/README.md)
- [mac/README.md](../mac/README.md)
- [docs/WINDOWS_BUILD.md](../docs/WINDOWS_BUILD.md)
- [docs/WINDOWS_PROFILE_BOUNDARIES.md](../docs/WINDOWS_PROFILE_BOUNDARIES.md)
- [bench/README.md](../bench/README.md)
- [docs/REPO_STRUCTURE.md](../docs/REPO_STRUCTURE.md)
