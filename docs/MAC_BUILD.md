# Mac Build

This repository includes a public macOS entrypoint layer under [`mac/`](/Users/tomoaki/git/hakozuna/mac).

The current Mac measurements and notes in this pass come from an Apple Silicon M1 machine.

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

See the compact status view in the [GO / NO-GO ledger](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md) before changing the Mac lane.

Default Mac lane:

```bash
./mac/build_mac_release_lane.sh
```

This runs:

- `hz3`: `gmake -C hakozuna clean all_ldpreload_scale`
- `hz4`: `gmake -C hakozuna-mt clean all_perf_lib`

The `hz4` default lane now includes `HZ4_MACOS_FOREIGN_SAFE=1`, so the foreign-pointer helper boundary is part of the normal Mac release path.

## Observe Build

For stats-enabled allocator libraries without touching the release lane, use:

```bash
./mac/build_mac_observe_lane.sh
```

This helper:

- builds `hz3` and `hz4` in a temporary tree
- enables existing allocator stats flags only
- copies the results into `mac/out/observe/`
- leaves the release libraries intact
- prints `HZ3_SO` / `HZ4_SO` export lines for the existing Mac runners

Typical use:

```bash
./mac/build_mac_observe_lane.sh
export HZ3_SO='/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_scale_observe.so'
export HZ4_SO='/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_observe.so'
ALLOCATORS=hz3,hz4 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

The default observe flags are:

- `hz3`: `HZ3_STATS_DUMP`, `HZ3_MEDIUM_PATH_STATS`, `HZ3_S203_COUNTERS`, `HZ3_S204_LARSON_DIAG`, `HZ3_S197_MEDIUM_INBOX_CAS_STATS`, `HZ3_LARGE_CACHE_STATS`
- `hz4`: `HZ4_OS_STATS`, `HZ4_MID_STATS_B1`, `HZ4_MID_LOCK_TIME_STATS`, `HZ4_ST_STATS_B1`, `HZ4_FREE_ROUTE_STATS_B26`, `HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27`

Override them with `HZ3_OBSERVE_DEFS=...` or `HZ4_OBSERVE_DEFS=...` if you need a narrower counter set.

When you need the `hz3` lane-batch refill summary (`[HZ3_S74]`) for the
current Mac investigation, build the observe lane with:

```bash
./mac/build_mac_observe_lane.sh --skip-hz4 --hz3-s74-stats \
  --hz3-lib /Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so
```

This keeps the release lane unchanged and turns on `HZ3_SCALE_S74_STATS=1`
cleanly, without fighting the scale preset defaults in the Makefile.

## Foreign-Pointer Safety Lane

The foreign-pointer helper is now in the default Mac release and observe lanes.
If you still want an explicit compatibility build, use the dedicated wrapper:

```bash
./mac/build_mac_foreign_safe_lane.sh
```

This lane:

- keeps the Mac interpose file thin
- moves the foreign-pointer boundary into `hz4_macos_foreign.[ch]`
- enables `HZ4_MACOS_FOREIGN_SAFE=1`
- mirrors the default helper boundary while leaving a dedicated alias in place
- emits a separate `hz4` observe library under `mac/out/observe/`

Use the resulting `HZ4_SO` together with an existing `HZ3_SO` from the normal
observe lane or from the release lane when you run the Mac compare scripts.

## Current Mac Design Box

The current Mac lane-specific tuning box is:

- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2`

Use [`mac/build_mac_mid_candidate_lane.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane.sh)
when you want a ready-to-run build of that candidate without changing the shared
default.

Use [`mac/build_mac_mid_candidate_lane_segreg.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane_segreg.sh)
for the segment-registry follow-up lane when you want the same box under the
free-route box. Pass `--slots 65536` when you want the slot A/B variant.

Keep that box lane-specific until the lower-remote spot-check is reconciled.

The current paper-suite follow-up boxes are now narrower:

- `hz4` `malloc-large` large-path box
- segment-registry high-remote fallback box

Keep those separate from the mid candidate so the next bench pass can answer
whether the remaining gap is large-path cost or bench-pressure noise.

The Mac `hz3` interpose path also prints a small `[HZ3_MACOS_MALLOC_SIZE]` summary at exit.
Use it when you need to confirm whether `malloc_size` calls are landing in hz3 itself or falling back to the system zone lookup.
`system_fallback` is intended only for clearly foreign pointers; `arena_unknown` is the signal that a hz3-range pointer was not recognized and needs allocator-side attention.

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

## MT Remote Benchmark (malloc/free)

The Mac wrapper compiles the shared MT-remote source with `HZ_BENCH_USE_HAKOZUNA=0`
and uses preload to compare allocators:

```bash
./mac/run_mac_mt_remote_compare.sh --build-only
ALLOCATORS=system ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
ALLOCATORS=mimalloc ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
ALLOCATORS=tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
```

## mimalloc-bench Subset (Mac)

This wrapper targets the paper subset (`cache-thrash`, `cache-scratch`, `malloc-large`)
using the shared mimalloc-bench source directory.

```bash
./mac/run_mac_mimalloc_bench_subset.sh --runs 3
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mimalloc_bench_subset.sh --runs 3
```

For the current `malloc-large` research box, use the thin capture alias so the
allocator stderr stays in the logs while the subset runner remains the
implementation box. The wrapper also builds a stats-enabled `hz4` observe lib
by default so the first-read counters are visible:

```bash
./mac/run_mac_malloc_large_research.sh --runs 5
```

The first treatment A/B was the large extent cache band/cap, and it is now a
no-go:

- current build
- `HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400`
- `HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB`

The next `malloc-large` hypothesis needs to be narrower and more local.

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
- For slowness triage, use the [Mac profiling recipe](/Users/tomoaki/git/hakozuna/docs/MAC_PROFILING.md) after the observe lane is in place.
- See [Mac benchmark prep](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md) for the current run order and [Mac benchmark results](/Users/tomoaki/git/hakozuna/docs/benchmarks/2026-03-19_MAC_BENCH_RESULTS.md) for the first measured numbers.
- For the current MT remote Mac pass, `ring_slots=262144` is the practical tuning value; do not promote it to a shared default yet.
