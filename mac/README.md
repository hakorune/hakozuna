# Mac Entrypoints

This directory is the public macOS entrypoint layer for this repository.
The current Mac benchmark pass documented here was measured on an Apple Silicon M1 machine.

Use it to keep Mac build and smoke commands in one place without mixing:

- allocator core source
- Linux-specific preload details
- Windows-specific build lanes
- generated outputs

## What Lives Here

- [`check_mac_env.sh`](check_mac_env.sh): quick environment check for the local Mac toolchain
- [`build_mac_release_lane.sh`](build_mac_release_lane.sh): public build wrapper for the current Mac release lane
- [`build_mac_observe_lane.sh`](build_mac_observe_lane.sh): separate stats-enabled build wrapper for diagnosis
- [`build_mac_mid_candidate_lane.sh`](build_mac_mid_candidate_lane.sh): current Mac mid candidate wrapper for `HZ4_MID_FREE_BATCH_CONSUME_MIN=2`
- [`build_mac_mid_candidate_lane_segreg.sh`](build_mac_mid_candidate_lane_segreg.sh): segment-registry follow-up wrapper for the same mid candidate, with `--slots 65536` for the slot A/B
- [`run_mac_preload_smoke.sh`](run_mac_preload_smoke.sh): minimal `DYLD_INSERT_LIBRARIES` smoke runner
- [`run_bench_compare.sh`](run_bench_compare.sh): thin macOS frontend for the shared allocator compare runner
- [`run_mac_larson_compare.sh`](run_mac_larson_compare.sh): macOS Larson-style allocator comparison runner
- [`run_mac_mt_remote_compare.sh`](run_mac_mt_remote_compare.sh): macOS MT remote (malloc/free) compare runner
- [`run_mac_mimalloc_bench_subset.sh`](run_mac_mimalloc_bench_subset.sh): macOS mimalloc-bench subset runner
- [`bench_redis_workload_compare.c`](bench_redis_workload_compare.c): macOS Redis-style workload benchmark
- [`run_mac_redis_compare.sh`](run_mac_redis_compare.sh): build-and-run wrapper for the Redis-like Mac benchmark
- [`run_mac_paper_suite.sh`](run_mac_paper_suite.sh): canonical paper benchmark bundle runner for Larson, MT remote, redis-like, and mimalloc-bench

## Quick Start

```bash
cd /path/to/hakozuna
./mac/check_mac_env.sh
./mac/build_mac_release_lane.sh
```

## Current Direction

Mac bring-up is being added as a separate box from the existing Linux and Windows lanes.

Keep the repository roles separate:

- `hakozuna/`: hz3 allocator track
- `hakozuna-mt/`: hz4 allocator track
- `linux/`: Ubuntu/Linux entrypoints
- `win/`: Windows entrypoints
- `mac/`: macOS entrypoints

The Mac entry layer is intentionally thin. The next step is to normalize the build paths for `hakozuna` and `hakozuna-mt` so each platform can keep its own wrapper without cross-contamination.

See also:

- [`docs/MAC_DESIGN_BOXES.md`](/Users/tomoaki/git/hakozuna/docs/MAC_DESIGN_BOXES.md)
- [`docs/MAC_BUILD.md`](/Users/tomoaki/git/hakozuna/docs/MAC_BUILD.md)
- [`docs/benchmarks/GO_NO_GO_LEDGER.md`](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)
- [`bench/README.md`](/Users/tomoaki/git/hakozuna/bench/README.md)
- [`mac/bench_larson_compare.c`](/Users/tomoaki/git/hakozuna/mac/bench_larson_compare.c)

Default Mac build lane:

- `hz3`: `all_ldpreload_scale`
- `hz4`: `all_perf_lib` for now, until the hz4 test lane is Mac-ready

Current Mac design box:

- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` is the lane-specific live candidate
- use [`build_mac_mid_candidate_lane.sh`](build_mac_mid_candidate_lane.sh) to build it
- use [`build_mac_mid_candidate_lane_segreg.sh`](build_mac_mid_candidate_lane_segreg.sh)
  for the segment-registry follow-up lane
- the `malloc-large` research box stays on the existing subset runner; isolate
  it with `DO_CACHE_THRASH=0 DO_CACHE_SCRATCH=0 DO_MALLOC_LARGE=1 RUNS=5`
- keep it separate from the shared default until the remaining lower-remote spot-check is reconciled

Observe lane for slow-path diagnosis:

```bash
./mac/build_mac_observe_lane.sh
HZ3_SO=/Users/tomoaki/git/hakozuna/libhakozuna_hz3_obs.so \
HZ4_SO=/Users/tomoaki/git/hakozuna/hakozuna-mt/libhakozuna_hz4_obs.so \
  ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Use `--hz4-os-stats` only if the first pass still does not explain the slowdown.

Redis-like Mac benchmark:

```bash
./mac/run_mac_redis_compare.sh system
./mac/run_mac_redis_compare.sh mimalloc
./mac/run_mac_redis_compare.sh tcmalloc
```

Larson-style Mac benchmark:

```bash
ALLOCATORS=system ./mac/run_mac_larson_compare.sh 1 8 64 100 1 12345 2
ALLOCATORS=mimalloc ./mac/run_mac_larson_compare.sh 1 8 64 100 1 12345 2
ALLOCATORS=tcmalloc ./mac/run_mac_larson_compare.sh 1 8 64 100 1 12345 2
```

MT remote (malloc/free) Mac benchmark:

```bash
ALLOCATORS=system ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
ALLOCATORS=mimalloc ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
ALLOCATORS=tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 200000 400 16 2048 50 65536
```

mimalloc-bench subset (Mac):

```bash
./mac/run_mac_mimalloc_bench_subset.sh --runs 3
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mimalloc_bench_subset.sh --runs 3
```

Paper benchmark bundle:

```bash
./mac/run_mac_paper_suite.sh
./mac/run_mac_paper_suite.sh --include-segreg
```

## Notes

- Mac smoke uses `DYLD_INSERT_LIBRARIES`, not `LD_PRELOAD`.
- System-protected binaries may ignore library injection.
- Keep platform differences in wrappers and build flags, not in allocator hot paths.

## See Also

- [linux/README.md](/Users/tomoaki/git/hakozuna/linux/README.md)
- [win/README.md](/Users/tomoaki/git/hakozuna/win/README.md)
- [docs/REPO_STRUCTURE.md](/Users/tomoaki/git/hakozuna/docs/REPO_STRUCTURE.md)
