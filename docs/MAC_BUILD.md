# Mac Build

This repository includes a public macOS entrypoint layer under [`mac/`](/Users/tomoaki/git/hakozuna/mac).

Mac bring-up is intentionally separated from the existing Linux and Windows lanes:

- `hakozuna/`: hz3 allocator track
- `hakozuna-mt/`: hz4 allocator track
- `linux/`: Ubuntu/Linux entrypoints
- `win/`: Windows entrypoints
- `mac/`: macOS entrypoints

## Prerequisites

The current Mac setup expects:

- Xcode Command Line Tools
- Homebrew
- GNU Make (`gmake`)
- Git

Quick environment check:

```bash
./mac/check_mac_env.sh
```

## Build

Default Mac lane:

```bash
./mac/build_mac_release_lane.sh
```

This runs:

- `hz3`: `gmake -C hakozuna clean all_ldpreload_scale`
- `hz4`: `gmake -C hakozuna-mt clean all_perf_lib`

## Smoke Test

```bash
./mac/run_mac_preload_smoke.sh hz3 /usr/bin/env true
```

You can also point the smoke runner at an absolute library path.

## Larson Compare

For a small Mac-native allocator comparison benchmark that mirrors the Windows
Larson family, use:

```bash
./mac/run_mac_larson_compare.sh --help
ALLOCATORS=system ./mac/run_mac_larson_compare.sh 3 8 1024 10000 1 12345 4
ALLOCATORS=mimalloc ./mac/run_mac_larson_compare.sh 3 8 1024 10000 1 12345 4
```

This benchmark uses CRT `malloc/free` in the source and compares allocators via
`DYLD_INSERT_LIBRARIES`.

## Redis-like Benchmark

The Mac redis-style benchmark lives in [`mac/bench_redis_workload_compare.c`](/Users/tomoaki/git/hakozuna/mac/bench_redis_workload_compare.c) and is run through [`mac/run_mac_redis_compare.sh`](/Users/tomoaki/git/hakozuna/mac/run_mac_redis_compare.sh).

Typical invocations:

```bash
./mac/run_mac_redis_compare.sh system 4 500 2000 16 256
./mac/run_mac_redis_compare.sh hz3 4 500 2000 16 256
./mac/run_mac_redis_compare.sh mimalloc 4 500 2000 16 256
./mac/run_mac_redis_compare.sh tcmalloc 4 500 2000 16 256
```

This benchmark keeps the workload on CRT malloc/free; allocator comparison is done by `DYLD_INSERT_LIBRARIES` in the runner.

## Current hz4 Limitation

`hakozuna-mt/Makefile` includes an `all_stable` lane that tries to build unit tests, but this checkout does not contain a `tests/` directory under `hakozuna-mt/`.

Observed failure:

```text
No rule to make target 'tests/hz4_segq_test.c'
```

That means:

- `all_stable` is not usable in this checkout yet
- `all_perf_lib` is the safe Mac default for `hz4`
- when `tests/` is restored or added, `all_stable` can be revisited

## Notes

- Mac smoke uses `DYLD_INSERT_LIBRARIES`, not `LD_PRELOAD`
- Protected system binaries may ignore injection
- Keep Mac-specific differences in wrappers and build flags, not allocator hot paths
