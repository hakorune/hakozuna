# Cross-Platform Benchmark Conditions

Write the conditions before implementing a new benchmark lane or a tuning box.

## Rule

- Freeze the workload shape first.
- Record OS-specific launcher and preload details separately from the allocator question.
- If a result only holds on one platform or architecture, mark it as a platform tuning value instead of a shared default.
- Do not promote an arm64 win to Windows x64 or macOS without rerunning those lanes; cross-lane regressions are expected until proven otherwise.
- Prefer median-based results whenever the run-to-run variance is non-trivial.

## Common Baseline

Before implementation, make these items explicit:

| Item | What to pin down |
|---|---|
| Benchmark name | Which bench is being changed |
| Allocator list | Which allocators are compared |
| Shape | Threads, iters, working set, size range, remote ratio, ring size |
| Repeat rule | Single sweep, 3-run median, 5-run median, etc. |
| Metric | ops/s, requests/sec, RSS, or latency |
| Counter plan | Which existing counters explain the result |
| Result location | Which dated results doc receives the numbers |

## Platform Matrix

| Platform | Launcher | Preload / injection | Timing | Notes |
|---|---|---|---|---|
| macOS | `./mac/*.sh` | `DYLD_INSERT_LIBRARIES` | shell timing / `time.time_ns()` wrappers | Keep Mac-only tuning values separate until Linux/Windows confirm them; do not assume an arm64 Linux win is a Mac win |
| Ubuntu/Linux (x86_64) | `./linux/*.sh` or `gmake` | `LD_PRELOAD` | `/usr/bin/time -v` or `perf` when available | Use Linux canonical shapes before comparing against Mac or Windows; benchmark compare runs can prepare local `mimalloc` / `tcmalloc` caches via `linux/run_linux_bench_compare.sh`; revalidate any arm64-specific idea before promoting it here |
| Ubuntu/Linux (arm64) | `./linux/*.sh` or `gmake` | `LD_PRELOAD` | `/usr/bin/time -v` or `perf` when available | Record CPU arch + kernel in summaries; do not mix results with x86_64; use the explicit arm64 wrappers for the lane when you want a named entrypoint; keep any win lane-specific until Windows x64 confirms it |
| Windows | `./win/*.ps1` | native allocator EXEs / DLL path injection | PowerShell-native timing and Windows RSS metrics | Do not assume `LD_PRELOAD` semantics carry over; an arm64 Linux gain may still regress Windows x64 |

- For HZ4 Windows builds, keep `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` as the default safety boundary. Use `-ExtraDefines HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=0` only for perf-only rollback / A-B replay.
- The Windows safety boundary cost lands on the free route of small-free-heavy
  lanes first: the diagnostic HZ4 probe showed `seg_checks=free_calls` on the
  default-on path, while the rollback path shifted those calls back to the
  normal large-validate precheck. Use this as the SSOT explanation for the
  Windows `hz4` mixed-lane slowdown.
- For the Windows follow-up, keep `HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1` with the
  safety boundary: the wrapper now proves `foreign / seg-owned / large-owned`
  once and the core no longer repeats the same `seg_check` on seg-owned frees.
  Use `-ExtraDefines HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=0` only for rollback /
  bisect.

## Benchmark-Specific Conditions

### Larson

- Canonical SSOT shape: `5 4096 32768 1000 1000 0 {T}`
- Compare `system`, `hz3`, `hz4`, `mimalloc`, and `tcmalloc`
- On macOS, use `T=1,4,8` with 5 repeats and median
- Keep smoke shapes separate from ranking shapes
- If the run is slow, start with mid-path counters before adding new instrumentation

### MT Remote

- Canonical shape: `threads iters ws min max remote_pct ring_slots`
- On the current M1 Mac pass, `ring_slots=262144` is the platform tuning value
- Record `actual` and `fallback_rate` together with throughput
- Treat `ring_full_fallback` as a benchmark-pressure signal, not an allocator-internal metric
- Only promote a larger ring value to a shared default if other OSes show the same pressure pattern
- Keep `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` separate from `HZ4_MID_PREFETCHED_BIN_HEAD_BOX=1`: the former is the free-route box for the `guard` / `main` / `cross64` lane set, while the latter is the canonical MT small-path box for Larson and MT remote
- For the high-remote diagnostic lane, keep the shape fixed at
  `16 2000000 400 16 2048 90 262144` and treat any fallback above noise as
  pressure-bound until proven otherwise
- For the segment-registry follow-up, compare `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS=32768`
  against `65536` before promoting anything; keep `ring_slots=262144` fixed
  while you read the slot sensitivity

### Redis-like

- Keep the key mix and allocator list stable before comparing results
- Record median requests/sec once the first pass is green
- Compare the same command shape across OSes before drawing conclusions

### mimalloc-bench Subset

- Keep the subset fixed to `cache-thrash`, `cache-scratch`, and `malloc-large`
- Compare `system`, `hz3`, `hz4`, `mimalloc`, and `tcmalloc`
- Use a median of 3 runs for Mac paper-suite spot checks
- Treat `malloc-large` as the large-path research box; do not fold it into the
  Redis or Larson conclusions
- For the current Mac follow-up, run `malloc-large` in isolation with
  `DO_CACHE_THRASH=0 DO_CACHE_SCRATCH=0 DO_MALLOC_LARGE=1 RUNS=5`
- For `malloc-large`, read `HZ4_OS_STATS_B16.ext_acq_hit`, `ext_acq_miss`,
  `ext_rel_hit`, `ext_rel_miss`, `ext_rel_drop_cap`, `ext_bytes_peak`, plus
  `large_validate_calls` and `large_validate_hits` first
- Use `mac/run_mac_malloc_large_research.sh` as the capture alias around the
  subset runner so the stats-enabled `hz4` observe lib stays visible in logs
- Treat `HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400` and
  `HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB` as a no-go combo for now; the next
  hypothesis needs to be narrower and more local
- If `hz4` remains a clear outlier, keep it as a dedicated large-path box
  instead of trying to explain it away as a mid-path effect

### Mid Fixed-Cost Boxes

- For `hz4`, prefer existing `HZ4_MID_STATS_B1` and `HZ4_MID_LOCK_STATS` before adding new counters
- For `HZ4_MID_PREFETCHED_BIN_HEAD_BOX`, compare the canonical Larson SSOT shape and the MT remote shape with `ring_slots=262144`, and record `prefetched_hint_probe`, `prefetched_hint_hit`, `prefetched_hint_miss`, and `malloc_lock_path` on both lanes before promoting it
- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` is the last live mid candidate on the Mac pass; compare it on canonical Larson, canonical MT remote, and the relevant segment-registry lane before treating it as a shared default
- If MT remote is sparse in mid counters, add `HZ4_SMALL_STATS_B19` and watch `malloc_refill_calls`, `malloc_refill_hit`, `malloc_refill_miss`, `small_free_remote`, `small_tcache_push`, `small_remote_keyed`, `small_remote_plain`, and `small_alloc_page_init_lite_*`
- Do not read `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` through the same small-path lens; on the MT lane set it is a free-route box whose first evidence is `large_validate_calls`, `malloc_page_create`, and lock timing
- Read `malloc_lock_path`, `malloc_owner_fast`, `malloc_page_create`, `free_owner_local`, `free_owner_remote`, and lock timing first
- If `HZ4_OS_STATS` is near zero, the bottleneck is probably not the OS / large path

## Implementation Gate

Do not implement a new box until the following are written down:

- workload shape
- allocator list
- repeat rule
- OS-specific launcher / preload differences
- counters to read first
- success criterion

## Where To Record Results

- Linux arm64 compare results: [docs/benchmarks/2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md](./2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- Mac results: [docs/benchmarks/2026-03-19_MAC_BENCH_RESULTS.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/2026-03-19_MAC_BENCH_RESULTS.md)
- Mac prep: [docs/MAC_BENCH_PREP.md](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md)
- Windows bench notes: [docs/benchmarks/windows/](/Users/tomoaki/git/hakozuna/docs/benchmarks/windows)
- Linux and allocator work orders: [`hakozuna-mt/docs/`](/Users/tomoaki/git/hakozuna/hakozuna-mt/docs)
