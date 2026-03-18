# Mac Bench Prep Order

This repo now has a Mac smoke path and allocator preload support.
The next step is to widen coverage in the same order the paper and
Windows notes emphasize, while keeping the first runs cheap.

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

### 5) Add mimalloc-bench subset

Why last:
- it is broad and useful, but it is not the quickest way to get the Mac baseline moving
- it makes more sense after the core Mac allocator comparison path is stable

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
- Expand benchmark breadth first, then tighten run counts and medians.
