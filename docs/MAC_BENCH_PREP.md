# Mac Bench Prep Order

This repo now has a Mac smoke path and allocator preload support.
The current Mac benchmark notes were gathered on an Apple Silicon M1 machine.
The next step is to widen coverage in the same order the paper and
Windows notes emphasize, while keeping the first runs cheap.

Before implementing any new Mac box, write the workload and platform
conditions first in:

- [Cross-platform benchmark conditions](/Users/tomoaki/git/hakozuna/docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [GO / NO-GO ledger](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)
- [Mac profiling recipe](/Users/tomoaki/git/hakozuna/docs/MAC_PROFILING.md)

## Current Status

- Smoke check: `bench_mixed_ws` is already usable on macOS
- Allocator replacements available on Mac:
  - `mimalloc`
  - `tcmalloc`
  - `hz3`
  - `hz4`
- The paper-grade benchmark set in this repo is centered on:
  - `bench_random_mixed_mt_remote_malloc`
  - redis-like workload
  - larson
  - mimalloc-bench subset

## Current Mac Facts

These are the conclusions from the first M1 Mac pass:

- `bench_mixed_ws` is only a smoke check; do not use it as the final ranking.
- `bench_random_mixed_mt_remote_malloc` needs `ring_slots=262144` on this machine shape to avoid fallback noise.
- MT remote gives us useful counters in the log:
  - `allocated`
  - `local_freed`
  - `remote_sent`
  - `remote_received`
  - `ring_full_fallback`
  - `overflow_sent`
  - `overflow_received`
  - `[EFFECTIVE_REMOTE]` with actual and fallback rate
- Mac `hz3` observe builds also emit `[HZ3_MACOS_MALLOC_SIZE]` so we can tell whether `malloc_size` stayed inside hz3 or had to fall back to the system zone lookup.
- `system_fallback` is for foreign pointers only; `arena_unknown` is the allocator-side bug signal if a hz3-range pointer was not recognized.
- `larson` should be rerun in the canonical hygienic SSOT shape before we compare it with Ubuntu or Windows.
- The current Mac larson wrapper only exposes workload shape and wrapper-level counts, so it is a smoke lane rather than an allocator-internal counter lane.
- Use `./mac/build_mac_observe_lane.sh` before adding new counters; the first box is to enable the existing `hz3` / `hz4` stats libraries in `mac/out/observe/` and switch runners with `HZ3_SO` / `HZ4_SO`.
- `mimalloc-bench` subset should start with `cache-thrash`, `cache-scratch`, and `malloc-large`.
- `larson-sized` and `mstress` stay deferred until the core three subset benches are stable.

## Current Mac Design Box

The first Mac correctness box is the foreign-pointer safety box, and it is now
part of the default Mac `hz4` lanes:

- `HZ4_MACOS_FOREIGN_SAFE=1`
- helper module: `hakozuna-mt/src/hz4_macos_foreign.[ch]`
- thin interpose shell: `hakozuna-mt/src/hz4_macos_interpose.c`
- lane wrapper: `mac/build_mac_foreign_safe_lane.sh` (compatibility alias)
- run order: smoke -> compare -> observe
- keep the helper boundary around while the compatibility wrapper remains
  available

The current Mac-specific tuning box is:

- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2`

Use [`mac/build_mac_mid_candidate_lane.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane.sh)
to build it, then compare it on:

- canonical Larson
- canonical MT remote
- the relevant segment-registry lane

For the segment-registry follow-up lane, use
[`mac/build_mac_mid_candidate_lane_segreg.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane_segreg.sh)
so the lane-specific free-route box stays explicit.

Do not promote it to the shared default yet; the 50% remote spot-check still
needs to be reconciled.

The current paper-suite follow-up boxes are narrower than the full mid search:

- `hz4` `malloc-large` large-path
- segment-registry high-remote fallback / bench pressure

For `malloc-large`, keep the existing `run_mac_mimalloc_bench_subset.sh`
wrapper and isolate the workload with `DO_CACHE_THRASH=0`
`DO_CACHE_SCRATCH=0` `DO_MALLOC_LARGE=1 RUNS=5`.
The current treatment A/B is the large extent cache band/cap:
`HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400` and
`HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB`.

For the segment-registry follow-up, keep the slot A/B explicit by using
`build_mac_mid_candidate_lane_segreg.sh --slots 32768` and
`--slots 65536`.

Keep those separate from the mid candidate so the next pass can answer whether
the gap is allocator cost or measurement pressure.

## Recommended Order

### 1) Keep `bench_mixed_ws` as smoke

Use this for:
- environment checks
- quick allocator sanity
- regression detection before a bigger sweep

### 2) Add redis-like first

Why first:
- It is already documented as a paper-aligned workload.
- It is easier to run than the larger MT remote sweep.
- It gives a realistic allocator comparison on mixed key/value churn.

Target note:
- compare `CRT`, `hz3`, `hz4`, `mimalloc`, `tcmalloc`
- keep the benchmark single-binary and swap allocators by preload

### 3) Add larson next

Why next:
- It is another paper-aligned allocator stressor.
- It is time-based and complements the redis-like workload.
- It catches a different class of throughput / churn behavior.

### 4) Bring in `bench_random_mixed_mt_remote_malloc`

Why after the simpler workloads:
- it is the main MT remote benchmark used in the docs and paper results
- it is more sensitive to lane / ring / remote-fraction tuning
- it is best approached once the simpler Mac ports are stable

Current Mac tuning:
- use `ring_slots=262144` for the first M1 Mac comparison pass
- keep `65536` as the shared default unless another platform shows the same ring pressure

### 5) Add mimalloc-bench subset

Why last:
- it is broad and useful, but it is not the quickest way to get the Mac baseline moving
- it makes more sense after the core Mac allocator comparison path is stable

Current subset target:
- `cache-thrash`
- `cache-scratch`
- `malloc-large`
- For the current follow-up, run `malloc-large` alone before widening the subset
- If `malloc-large` stays slow, keep the research on the large extent cache band/cap

Deferred to a later box:
- `larson-sized`
- `mstress`

## Evidence From Existing Notes

- [Paper benchmark results](/Users/tomoaki/git/hakozuna/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md)
- [Windows redis workload note](/Users/tomoaki/git/hakozuna/docs/benchmarks/windows/paper/20260309_140418_paper_redis_workload_windows.md)
- [Windows larson note](/Users/tomoaki/git/hakozuna/docs/benchmarks/windows/paper/20260309_044450_paper_larson_windows.md)
- [MT remote work order](/Users/tomoaki/git/hakozuna/hakozuna/docs/PHASE_HZ3_S41_STEP4_MT_REMOTE_BENCH_WORK_ORDER.md)

## Mac Execution Style

Use a single benchmark binary and swap allocators with preload:

```bash
DYLD_INSERT_LIBRARIES=/opt/homebrew/opt/mimalloc/lib/libmimalloc.dylib ./your_bench
DYLD_INSERT_LIBRARIES=/opt/homebrew/opt/gperftools/lib/libtcmalloc.dylib ./your_bench
DYLD_INSERT_LIBRARIES=/Users/tomoaki/git/hakozuna/libhakozuna_hz3_scale.so ./your_bench
DYLD_INSERT_LIBRARIES=/Users/tomoaki/git/hakozuna/hakozuna-mt/libhakozuna_hz4.so ./your_bench
```

## Working Rule

- Do not treat the smoke result as the final ranking.
- Do not optimize `hz3` / `hz4` from a single quick run.
- Write the platform conditions first, then implement the box.
- Expand benchmark breadth first, then tighten run counts and medians.
- When comparing against Ubuntu or Windows, prefer the canonical workload shape for each benchmark before drawing cross-platform conclusions.
