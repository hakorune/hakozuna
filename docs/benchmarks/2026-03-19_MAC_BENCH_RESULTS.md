# Mac Bench Results (2026-03-19)

This is the first macOS measurement pass for the new entrypoint layer.
It captures the smoke-to-paper bridge in the order we prepared it:

1. redis-like
2. larson
3. `bench_random_mixed_mt_remote_malloc`
4. mimalloc-bench subset

## Environment

- Host: Mac mini (Apple Silicon)
- macOS: `Darwin 25.3.0`
- Allocator switching: `DYLD_INSERT_LIBRARIES`

## 2026-03-20 Canonical Paper Suite

Command:

```bash
./mac/run_mac_paper_suite.sh
./mac/run_mac_paper_suite.sh --skip-build --include-segreg
```

This is the canonical Mac paper bundle from the current entrypoint layer.
The main result figure is saved at:

- [paper_suite_speedups.svg](/Users/tomoaki/git/hakozuna/mac/out/paper_suite_20260320_061621/paper_suite_speedups.svg)

Key takeaways:

- `Larson`: `hz3` is the clearest win at `175.18M ops/s`, followed by `tcmalloc` at `112.50M ops/s`, `hz4` at `69.46M ops/s`, and `mimalloc` at `26.60M ops/s`.
- `MT remote`: `hz3` leads at `126.60M ops/s`, `hz4` follows at `115.49M ops/s`, and `mimalloc` / `tcmalloc` are both around `92M ops/s` with zero fallback in the canonical `50%` remote run.
- `Redis-like`: mixed. `hz4` leads `SET` and `LPUSH`, `mimalloc` leads `GET` and `LPOP`, and `tcmalloc` leads `RANDOM`.
- `mimalloc-bench` subset: mostly near parity with `system`, except `hz4` on `malloc-large`, which is the clear slow outlier.
- Optional high-remote `hz4` segment-registry run hit `36.13M ops/s` at `77.7%` actual remote with `12.32%` fallback, so it is useful as a separate diagnostic lane but not apples-to-apples with the canonical `50%` remote case.

Follow-up hypothesis from the current read:

- `malloc-large` is probably a large extent cache band / cap problem, so the next A/B should isolate `malloc-large` with the existing subset runner and compare current `hz4` against `HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400` / `HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB`.
- The segment-registry 90% remote lane still looks bench-pressure sensitive, so compare `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS=32768` against `65536` with `ring_slots=262144` fixed before promoting any allocator change.

## 2026-03-20 Focused Follow-up A/B

The first focused follow-up confirmed two things:

### `malloc-large`

Command:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_observe.so \
  ALLOCATORS=hz4 RUNS=5 DO_CACHE_THRASH=0 DO_CACHE_SCRATCH=0 DO_MALLOC_LARGE=1 \
  OUTDIR=/Users/tomoaki/git/hakozuna/mac/out/research_malloc_large_current \
  ./mac/run_mac_mimalloc_bench_subset.sh

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_large_extent_400_obs.so \
  ALLOCATORS=hz4 RUNS=5 DO_CACHE_THRASH=0 DO_CACHE_SCRATCH=0 DO_MALLOC_LARGE=1 \
  OUTDIR=/Users/tomoaki/git/hakozuna/mac/out/research_malloc_large_treatment \
  ./mac/run_mac_mimalloc_bench_subset.sh
```

Median time:

| Build | Median |
|---|---:|
| current `hz4` | 768.102 ms |
| extent-cache treatment | 2502.354 ms |

Conclusion:

- The first extent-cache expansion treatment regressed `malloc-large` badly.
- This specific `400 pages / 1GiB` treatment is a no-go for now.
- If `malloc-large` is still worth pursuing later, it needs a narrower or different large-path hypothesis.

### Segment-registry high-remote

Command:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_slots32768.so \
  ALLOCATORS=hz4 OUT_DIR=/Users/tomoaki/git/hakozuna/mac/out/research_mt_segreg_32768 \
  ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_slots65536.so \
  ALLOCATORS=hz4 OUT_DIR=/Users/tomoaki/git/hakozuna/mac/out/research_mt_segreg_65536 \
  ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144
```

Observed single-run result:

| Slots | ops/s | actual remote | fallback rate |
|---|---:|---:|---:|
| 32768 | 15.96M | 63.1% | 26.94% |
| 65536 | 11.52M | 50.3% | 39.71% |

Conclusion:

- The larger slot table did not help on the 90% remote diagnostic lane.
- `65536` was worse than `32768` here, so the lane still reads as pressure-sensitive rather than allocator-win sensitive.

## Redis-like

Command:

```bash
./mac/run_mac_redis_compare.sh <allocator> 4 500 2000 16 256
```

This is a first-pass run, not a median sweep.

| Pattern | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| SET | 11.12M | 11.78M | 11.78M | 12.41M | 10.55M |
| GET | 1047.39M | 1053.46M | 904.57M | 677.39M | 1060.45M |
| LPUSH | 11.36M | 11.33M | 10.77M | 11.88M | 10.48M |
| LPOP | 1691.33M | 1658.37M | 1777.78M | 1755.93M | 1770.69M |
| RANDOM | 46.47M | 46.09M | 34.19M | 49.08M | 50.91M |

## Redis-like Canonical Median

Command:

```bash
./mac/run_mac_redis_compare.sh <allocator> 4 500 2000 16 256
```

Each allocator was measured 5 times and reduced with median.

| Pattern | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| SET | 10.45M | 11.36M | 11.10M | 12.14M | 10.56M |
| GET | 889.68M | 872.22M | 861.70M | 975.61M | 916.17M |
| LPUSH | 10.50M | 10.87M | 10.79M | 11.62M | 10.24M |
| LPOP | 1588.56M | 1675.04M | 1530.22M | 1766.78M | 1736.86M |
| RANDOM | 44.28M | 44.76M | 44.16M | 45.73M | 48.85M |

Notes:
- `mimalloc` is best on SET / LPUSH / GET in this Mac pass.
- `tcmalloc` is best on RANDOM.
- `hz3` / `hz4` stay close to `system` on this workload, but do not dominate it.

## Larson

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_larson_compare.sh 3 8 1024 10000 1 12345 4
```

| Allocator | ops/s |
|---|---:|
| system | 16,385,000 |
| hz3 | 16,356,033 |
| hz4 | 16,200,539 |
| mimalloc | 145,192,161 |
| tcmalloc | 104,373,949 |

Note:
- This Larson pass is a Mac smoke run for the custom wrapper (`runtime=3`, `8..1024`, `chunks=10000`, `rounds=1`).
- Treat the absolute ranking as preliminary until we rerun the canonical hygienic SSOT shape from the historical docs: `5 4096 32768 1000 1000 0 {T}` with median-based repeats.

## Larson Canonical Rerun

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 {T}
```

This is the canonical hygienic SSOT shape from the historical larson notes.
Each allocator was measured 5 times at `T=1,4,8`, then reduced with median.

| Allocator | T=1 | T=4 | T=8 |
|---|---:|---:|---:|
| system | 15.97M | 15.65M | 20.00M |
| hz3 | 16.22M | 16.21M | 22.08M |
| hz4 | 16.43M | 16.22M | 21.39M |
| mimalloc | 14.30M | 29.50M | 24.16M |
| tcmalloc | 44.99M | 112.10M | 113.63M |

Notes:
- This is the larson shape to trust for cross-platform comparison.
- On this Mac pass, `tcmalloc` is far ahead at all thread counts, `mimalloc` becomes strong at 4+ threads, and `hz3` / `hz4` stay in the same general band as `system`.
- The smoke run above is useful for wrapper sanity, but not for allocator ranking.

### Larson Mac Follow-up

The same canonical shape was then rerun on the current Mac release setup with 5 repeats.

Command:

```bash
./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Median ops/s:

| Allocator | ops/s |
|---|---:|
| system | 19,032,444 |
| hz3 | 19,041,043 |
| hz4 | 68,528,677 |
| mimalloc | 28,865,739 |
| tcmalloc | 111,732,814 |

Observe notes:

- `HZ4_MID_STATS_B1` showed `malloc_calls=50,501,595`, `malloc_alloc_cache_hit=48,812,237`, `malloc_lock_path=1,689,358`, `malloc_page_create=3,306`, and `malloc_free_batch_hit=6,380,955`.
- `HZ4_MID_LOCK_STATS` showed `lock_enter=1,689,358`, `prelock_alloc_run_hit=42,431,282`, and `prelock_alloc_run_miss=8,070,313`.
- `HZ4_OS_STATS` stayed near zero, so this Larson shape is overwhelmingly a `mid`-path workload on this M1 Mac pass.

Notes:
- `hz4` is not collapsing here; it is well above `system` / `hz3` and also ahead of `mimalloc`.
- `tcmalloc` still leads on this Mac pass, so the remaining gap looks like `mid` fixed-cost rather than a large/OS-path issue.
- Existing observe counters are enough for this diagnosis; no new instrumentation was needed for this pass.

### Larson Mid Profile A/B: owner-remote-queue + owner-local-stack

To test whether the live non-default mid profile helps the canonical Larson shape on macOS, `hz4` was rebuilt to a separate custom library path with:

```bash
gmake -C /Users/tomoaki/git/hakozuna/hakozuna-mt \
  /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_owner_stack.so \
  LIB_TARGET=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_owner_stack.so \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

The benchmark command stayed canonical:

```bash
./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Each allocator was measured 5 times and reduced with median. Only `hz4` was overridden with `HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_owner_stack.so`.

| Allocator | median ops/s |
|---|---:|
| system | 15,514,526 |
| hz3 | 16,949,520 |
| hz4 custom mid profile | 50,015,106 |
| mimalloc | 28,193,770 |
| tcmalloc | 115,924,409 |

Raw `hz4` custom runs:
- `49,807,481`
- `50,271,204`
- `49,344,489`
- `50,015,106`
- `50,308,894`

Comparison against the current default-release canonical Larson median:
- current default `hz4`: `68,528,677`
- custom mid profile `hz4`: `50,015,106`
- delta: `-18,513,571 ops/s` (`-27.02%`)

Notes:
- The build succeeded with no archived-knob failure and no compile-time guard stop.
- On this M1 Mac pass, the `owner_remote_queue + owner_local_stack + remote_gate` profile is clearly unhelpful for canonical Larson.
- This result argues against taking the owner-local-stack line forward as the next Mac `mid` box for Larson.
- If we revisit this direction later, the next evidence should come from focused `HZ4_MID_STATS_B1` / `HZ4_MID_LOCK_STATS` counter comparison between default `hz4` and this custom profile, not from allocator code changes.

### Larson Mid Profile A/B: prefetched-bin-head

To test whether the live opt-in B37 hint path helps the canonical Larson shape on macOS, `hz4` was rebuilt to a separate custom library path with:

```bash
gmake -C /Users/tomoaki/git/hakozuna/hakozuna-mt \
  /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_prefetch_bin_head.so \
  LIB_TARGET=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_prefetch_bin_head.so \
  HZ4_DEFS_EXTRA='-DHZ4_MID_PREFETCHED_BIN_HEAD_BOX=1'
```

The benchmark command stayed canonical:

```bash
./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Each allocator was measured 5 times and reduced with median. Only `hz4` was overridden with `HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_prefetch_bin_head.so`.

| Allocator | median ops/s |
|---|---:|
| system | 19,674,516 |
| hz3 | 17,505,958 |
| hz4 custom B37 profile | 75,504,262 |
| mimalloc | 30,221,182 |
| tcmalloc | 116,697,186 |

Raw `hz4` custom runs:
- `75,504,262`
- `73,799,584`
- `75,841,790`
- `74,957,552`
- `75,698,164`

Observe run notes:

- `HZ4_MID_STATS_B1` showed `malloc_calls=43,676,047`, `malloc_alloc_cache_hit=42,221,503`, `malloc_lock_path=1,454,544`, `malloc_page_create=3,289`, and `malloc_free_batch_hit=5,521,570`.
- `HZ4_MID_LOCK_STATS` showed `lock_enter=1,454,544`, `lock_wait_samples=5,680`, `lock_hold_samples=5,680`, `hold_binscan_steps_sum=5,672`, and `hold_page_create_calls=8`.
- `prefetched_hint_probe=51`, `prefetched_hint_hit=48`, `prefetched_hint_miss=3`, so the hint path was live and mostly effective on this Mac pass.
- `HZ4_OS_STATS` was not enabled in this observe build, but the lock-path counters stayed in the mid lane rather than the large/OS path.

Comparison against the current default-release canonical Larson median:
- current default `hz4`: `68,528,677`
- custom B37 `hz4`: `75,504,262`
- delta: `+6,975,585 ops/s` (`+10.18%`)

Notes:
- The build succeeded with no archived-knob failure and no compile-time guard stop.
- On this M1 Mac pass, `HZ4_MID_PREFETCHED_BIN_HEAD_BOX=1` looks promising on canonical Larson.
- This is still a Larson-only result, so the next evidence should be a focused counter pass with `HZ4_MID_STATS_B1` and `HZ4_MID_LOCK_TIME_STATS` to confirm that `prefetched_hint_hit` is non-trivial and that the uplift is coming from the intended lock-path scan reduction.

## MT Remote

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 65536
```

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 42,671,884.83 | 50.1% | 0.00% |
| hz3 | 44,516,843.24 | 50.1% | 0.00% |
| hz4 | 45,254,795.56 | 50.1% | 0.00% |
| mimalloc | 86,347,501.48 | 47.4% | 2.66% |
| tcmalloc | 85,591,924.49 | 45.6% | 4.42% |

Notes:
- `hz3` and `hz4` hit the target remote ratio cleanly.
- `mimalloc` and `tcmalloc` were faster on this pass, but both showed fallback due to ring pressure, so this is not an apples-to-apples final ranking.
- On this M1 Mac pass, `ring_slots=65536` was too small for this workload shape. Treat `262144` as the Mac tuning value for now, not as a shared cross-platform default.

### MT Remote rerun with larger ring

Fastest fix for the fallback was to increase `ring_slots` from `65536` to `262144` and rerun the same workload:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

This removed fallback completely on the same workload shape.

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 45,260,065.18 | 50.1% | 0.00% |
| hz3 | 45,434,366.16 | 50.1% | 0.00% |
| hz4 | 47,075,553.39 | 50.1% | 0.00% |
| mimalloc | 92,035,597.92 | 50.1% | 0.00% |
| tcmalloc | 81,151,348.31 | 50.1% | 0.00% |

Notes:
- This rerun is the apples-to-apples ranking for the current MT remote shape.
- `hz4` moved slightly ahead of `hz3` here, but both are still far behind `mimalloc` on raw throughput.
- The bigger ring looks like a Mac-specific queue-depth correction for this allocator/workload mix. Keep the shared benchmark default unchanged unless Linux/Windows reruns show the same pressure pattern.

### Post-hz4-fix release rerun

After rebuilding the default `libhakozuna_hz4.so` from the current source, the same canonical shapes were rerun without `HZ4_SO` overrides.

#### Larson

Command:

```bash
./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Single sweep, release build:

| Allocator | ops/s |
|---|---:|
| system | 16,511,805 |
| hz3 | 18,459,849 |
| hz4 | 74,217,248 |
| mimalloc | 28,510,168 |
| tcmalloc | 111,032,756 |

Notes:
- `hz4` is now well above `system` / `hz3` on this rerun.
- `tcmalloc` still leads on this Mac pass.
- This is a single sweep, not a median sweep.

#### MT Remote

Command:

```bash
./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Single sweep, release build:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 31,251,148.90 | 50.1% | 0.00% |
| hz3 | 26,663,919.08 | 50.1% | 0.00% |
| hz4 | 56,334,818.24 | 50.1% | 0.00% |
| mimalloc | 65,527,183.25 | 50.1% | 0.00% |
| tcmalloc | 63,488,035.94 | 50.1% | 0.00% |

Notes:
- `hz4` now runs cleanly at `ring_slots=262144` with no fallback.
- On this rerun it is above `system` and `hz3`, and below `mimalloc` / `tcmalloc`.
- The default release library is now aligned with the current source, so future reruns no longer need `HZ4_SO` overrides.

### Segment-registry release rerun

After rebuilding `libhakozuna_hz4.so` with `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1`, the same MT shape was rerun again.

Command:

```bash
./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Single sweep, release build:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 43,744,937.46 | 50.1% | 0.00% |
| hz3 | 43,992,120.22 | 50.1% | 0.00% |
| hz4 | 89,544,005.37 | 50.1% | 0.00% |
| mimalloc | 91,374,494.89 | 50.1% | 0.00% |
| tcmalloc | 87,769,835.06 | 50.1% | 0.00% |

Notes:
- This release A/B uses `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1`.
- In the observe lane, the same box reduced `large_validate_calls` from `4,000,448` to `4`, with `seg_hits=4,000,444`.
- On the release rerun, `hz4` moved to near parity with `mimalloc` and ahead of `tcmalloc` on this M1 Mac pass.
- This looks promising enough to keep as the next MT tuning candidate, but it should still be median-tested before becoming a shared default.

### Segment-registry median sweep

The same MT remote shape was measured 3 times with `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` and reduced with median.

Command:

```bash
./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Median over 3 runs, release build:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 45,007,194.89 | 50.1% | 0.00% |
| hz3 | 45,117,419.76 | 50.1% | 0.00% |
| hz4 | 104,055,393.68 | 50.1% | 0.00% |
| mimalloc | 95,666,253.25 | 50.1% | 0.00% |
| tcmalloc | 96,222,252.10 | 50.1% | 0.00% |

Notes:
- `hz4` stays ahead of `system` / `hz3` and remains near the fast allocator band on this M1 Mac pass.
- The median confirms the earlier single-sweep improvement was not just noise.
- This is the current MT remote comparison to trust for the segment-registry box.

### Segment-registry lane sweep

The same release library was then checked on `guard`, `main`, and `cross64` with the Mac tuning ring value.

Commands:

```bash
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Single sweeps, release build:

| Lane | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| guard_r0 | 43,425,377.07 | 46,048,693.39 | 46,693,439.53 | 84,674,906.28 | 83,185,545.32 |
| main_r50 | 20,022,877.05 | 15,587,487.24 | 29,028,592.51 | 9,120,421.95 | 23,640,593.64 |
| cross64_r90 | 3,191,339.46 | 3,303,426.22 | 20,713,511.31 | 13,962,117.44 | 14,168,676.95 |

Notes:
- `hz4` stays ahead of `system` and `hz3` on all three lanes.
- `main_r50` and `cross64_r90` show the largest relative gains from `segment_registry`.
- These are single sweeps. If we want to promote the box, the next step is to median-sweep the lane set.

### Segment-registry lane median sweep

The same lane set was then measured 3 times each and reduced with median.

Commands:

```bash
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144
./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Median ops/s, release build:

| Lane | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| guard_r0 | 43,897,410.44 | 44,255,272.81 | 69,530,357.65 | 78,438,186.21 | 94,353,278.94 |
| main_r50 | 18,141,900.77 | 16,565,923.46 | 26,119,555.28 | 14,934,254.24 | 28,317,873.93 |
| cross64_r90 | 3,200,043.11 | 3,167,677.25 | 22,934,516.58 | 11,270,548.07 | 13,819,465.53 |

Median actual/fallback:

- `guard_r0`: system `87.7% / 2.33%`, hz3 `89.1% / 0.90%`, hz4 `88.3% / 1.68%`, mimalloc `86.1% / 3.93%`, tcmalloc `90.0% / 0.00%`
- `main_r50`: system `90.0% / 0.00%`, hz3 `89.3% / 0.72%`, hz4 `90.0% / 0.00%`, mimalloc `89.5% / 0.47%`, tcmalloc `90.0% / 0.00%`
- `cross64_r90`: system `63.4% / 26.61%`, hz3 `64.3% / 25.75%`, hz4 `90.0% / 0.00%`, mimalloc `89.6% / 0.43%`, tcmalloc `90.0% / 0.00%`

Notes:
- `guard_r0` still shows some fallback in the medians, but `hz4` is clearly ahead of `system` / `hz3`.
- `main_r50` is mostly clean; only small fallback remains on `hz3` and `mimalloc`.
- `cross64_r90` is the strongest win for `hz4`: `system` and `hz3` still suffer heavy fallback, while `hz4` clears it and stays far ahead.

### MT Remote Follow-up: prefetched-bin-head

To sanity-check whether the same B37 mid profile that helped canonical Larson also holds on MT remote, `hz4` was rerun from the custom release library path with the same MT shape and Mac tuning ring value.

Command:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_prefetch_bin_head.so ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Single sweep, release build:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 42,653,895.49 | 50.1% | 0.00% |
| hz3 | 44,485,058.67 | 50.1% | 0.00% |
| hz4 custom B37 profile | 104,813,665.26 | 50.1% | 0.00% |
| mimalloc | 94,132,781.40 | 50.1% | 0.00% |
| tcmalloc | 94,338,849.20 | 50.1% | 0.00% |

Notes:
- This is a safety check, not a promoted default yet.
- The B37 mid profile stayed clean on MT remote and outpaced the other allocators on this pass.
- The result suggests B37 may generalize beyond canonical Larson, so the next evidence should be a focused counter comparison before changing defaults.
- The MT observe lane for this same B37 build stayed sparse in the Larson-style counters (`malloc_calls=25`, `malloc_lock_path=25`, `prefetched_hint_probe=0`), so the release win is still a safety-check result rather than a fully explained one.

### MT Remote Route Observe Comparison

The route-stat observe lane was then rebuilt for both the default release path and the B37 path to compare the free-route and refill counters directly on the canonical MT remote shape.

Commands:

```bash
./mac/build_mac_observe_lane.sh --skip-hz3 --hz4-route-stats --hz4-lib /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_obs.so
./mac/build_mac_observe_lane.sh --skip-hz3 --hz4-route-stats --hz4-lib /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_obs.so --hz4-extra '-DHZ4_MID_PREFETCHED_BIN_HEAD_BOX=1'
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_obs.so ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_obs.so ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Throughput:

| Allocator | ops/s |
|---|---:|
| default observe hz4 | 21,109,187.65 |
| B37 observe hz4 | 24,115,225.34 |

Counter comparison:

| Counter | default observe | B37 observe |
|---|---:|---:|
| `HZ4_FREE_ROUTE_STATS_B26.free_calls` | 4,000,448 | 4,000,448 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 4,000,448 | 4,000,448 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_hits` | 4 | 4 |
| `HZ4_FREE_ROUTE_STATS_B26.mid_magic_hits` | 20 | 20 |
| `HZ4_FREE_ROUTE_STATS_B26.small_page_valid_hits` | 4,000,424 | 4,000,424 |
| `HZ4_ST_STATS_B1.free_fallback_path` | 4,000,424 | 4,000,424 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 38,466 | 35,641 |
| `HZ4_ST_STATS_B1.refill_direct_calls` | 0 | 0 |
| `HZ4_MID_STATS_B1.malloc_calls` | 25 | 25 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 25 | 25 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_refill_objs` | 36 | 31 |
| `HZ4_MID_STATS_B1.prefetched_hint_probe` | n/a | 0 |
| `HZ4_MID_LOCK_STATS.lock_enter` | 25 | 25 |

Notes:
- The free-route counters are effectively identical between default and B37 on this MT remote shape.
- The only meaningful counter delta is `HZ4_ST_STATS_B1.refill_to_slow`, which drops by about 2.8k samples on B37.
- The MT remote throughput win is therefore only partially explained by the current observe counters; the large release gain still needs a follow-up box if we want a complete mechanism story.

### MT Remote Small Observe Comparison

To explain the remaining MT remote delta, the same route observe lane was rebuilt again with `HZ4_SMALL_STATS_B19=1` so the small-path refill pressure could be compared directly.

Commands:

```bash
./mac/build_mac_observe_lane.sh --skip-hz3 --hz4-route-stats --hz4-lib /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so --hz4-extra '-DHZ4_SMALL_STATS_B19=1'
./mac/build_mac_observe_lane.sh --skip-hz3 --hz4-route-stats --hz4-lib /Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_small_obs.so --hz4-extra '-DHZ4_SMALL_STATS_B19=1 -DHZ4_MID_PREFETCHED_BIN_HEAD_BOX=1'
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_small_obs.so ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Throughput:

| Allocator | ops/s |
|---|---:|
| default observe hz4 | 9,789,164.97 |
| B37 observe hz4 | 11,502,422.02 |

Counter comparison:

| Counter | default observe | B37 observe | delta |
|---|---:|---:|---:|
| `HZ4_ST_STATS_B1.refill_to_slow` | 37,805 | 33,637 | -4,168 |
| `HZ4_SMALL_STATS_B19.malloc_refill_calls` | 37,805 | 33,637 | -4,168 |
| `HZ4_SMALL_STATS_B19.malloc_refill_hit` | 37,805 | 33,637 | -4,168 |
| `HZ4_SMALL_STATS_B19.malloc_bin_hit` | 3,962,806 | 3,966,974 | +4,168 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_refill_objs` | 36 | 31 | -5 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 4,000,448 | 4,000,448 | 0 |
| `HZ4_FREE_ROUTE_STATS_B26.small_page_valid_hits` | 4,000,424 | 4,000,424 | 0 |
| `HZ4_ST_STATS_B1.free_fallback_path` | 4,000,424 | 4,000,424 | 0 |

Notes:
- The `refill_to_slow` drop now lines up exactly with the `malloc_refill_calls` / `malloc_refill_hit` drop in `HZ4_SMALL_STATS_B19`.
- `malloc_bin_hit` increases by the same count, so B37 is moving work from slow refill to direct bin hit on this MT remote lane.
- This explains most of the observed MT remote improvement on the small-stat observe lane, though there is still a little release-vs-observe gap left to account for.

### MT Remote Segment-Registry Lane Follow-up: route+small observe

The same `route+small` observe libs were then run over the segment-registry lane set to see whether the canonical MT small-path story generalizes. It mostly does not: the lane set is a free-route box, not a small-path box.

Commands:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_segment_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_segment_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_segment_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Throughput:

| Lane | default observe | segment-registry observe | delta |
|---|---:|---:|---:|
| guard_r0 | 9,232,303.87 | 9,600,505.79 | +3.99% |
| main_r50 | 10,616,722.38 | 10,794,197.97 | +1.67% |
| cross64_r90 | 9,702,756.48 | 11,473,637.90 | +18.25% |

`main_r50` counter deltas:

| Counter | default | segment-registry | delta |
|---|---:|---:|---:|
| `HZ4_FREE_ROUTE_STATS_B26.seg_checks` | 0 | 16,001,732 | +16,001,732 |
| `HZ4_FREE_ROUTE_STATS_B26.seg_hits` | 0 | 16,001,358 | +16,001,358 |
| `HZ4_FREE_ROUTE_STATS_B26.seg_misses` | 0 | 374 | +374 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 16,001,732 | 374 | -16,001,358 |
| `HZ4_SMALL_STATS_B19.malloc_bin_hit` | 976,173 | 976,245 | +72 |
| `HZ4_SMALL_STATS_B19.malloc_refill_calls` | 17,226 | 17,154 | -72 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 17,226 | 17,154 | -72 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 4,261,112 | 4,420,473 | +159,361 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_hit` | 10,747,409 | 10,588,048 | -159,361 |
| `HZ4_MID_STATS_B1.malloc_page_create` | 240,499 | 193,295 | -47,204 |
| `HZ4_MID_LOCK_STATS.lock_wait_samples` | 16,638 | 17,261 | +623 |
| `HZ4_MID_LOCK_STATS.lock_hold_ns_sum` | 19,644,000 | 12,289,000 | -7,355,000 |
| `HZ4_MID_LOCK_STATS.hold_page_create_calls` | 924 | 738 | -186 |
| `HZ4_MID_STATS_B1.supply_resv_refill` | 32,098 | 25,802 | -6,296 |
| `[EFFECTIVE_REMOTE].fallback_rate` | 0.00% | 0.00% | unchanged |

`cross64_r90` counter deltas:

| Counter | default | segment-registry | delta |
|---|---:|---:|---:|
| `HZ4_FREE_ROUTE_STATS_B26.seg_checks` | 0 | 16,001,732 | +16,001,732 |
| `HZ4_FREE_ROUTE_STATS_B26.seg_hits` | 0 | 15,935,225 | +15,935,225 |
| `HZ4_FREE_ROUTE_STATS_B26.seg_misses` | 0 | 66,507 | +66,507 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 16,001,732 | 66,507 | -15,935,225 |
| `HZ4_SMALL_STATS_B19.malloc_bin_hit` | 488,457 | 488,483 | +26 |
| `HZ4_SMALL_STATS_B19.malloc_refill_calls` | 8,523 | 8,497 | -26 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 8,523 | 8,497 | -26 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 5,361,034 | 5,063,880 | -297,154 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_hit` | 10,081,151 | 10,378,305 | +297,154 |
| `HZ4_MID_STATS_B1.malloc_page_create` | 468,521 | 254,391 | -214,130 |
| `HZ4_MID_LOCK_STATS.lock_wait_ns_sum` | 184,714,000 | 77,218,000 | -107,496,000 |
| `HZ4_MID_LOCK_STATS.lock_hold_ns_sum` | 19,337,000 | 10,046,000 | -9,291,000 |
| `HZ4_MID_LOCK_STATS.hold_page_create_calls` | 1,816 | 971 | -845 |
| `HZ4_MID_STATS_B1.supply_resv_refill` | 62,498 | 33,950 | -28,548 |

Guard quick read:

- `guard_r0` improves modestly, but the big change there is still ring pressure / fallback related rather than the small-path refill story.
- `segment_registry` on the guard lane reduces `ring_full_fallback` and raises `actual` remote utilization, so it is still a useful MT box for that lane.

Mechanism:

- `main_r50`
  - Main effect is the near-removal of `large_validate_calls`
  - The small-path refill counts shift only by `72`, so this lane is not a B37-style small-path story
  - `malloc_page_create` and `supply_resv_refill` drop, but `malloc_lock_path` rises, so the throughput win is modest

- `cross64_r90`
  - Main effect is again the large reduction in `large_validate_calls`
  - Small-path refill changes are tiny
  - `malloc_lock_path` drops, `malloc_alloc_cache_hit` rises, and both lock wait / hold time collapse
  - This lane gets the biggest throughput win from the free-route box

- `guard_r0`
  - This lane is still dominated by ring pressure / fallback improvements
  - The small-path refill story does not explain it

Conclusion:
- `segment_registry` is a **free-route box** on the MT lane set
- `B37` is a **canonical MT small-path box** and should not be mixed into the segment-registry lane set
- `cross64_r90` is the strongest free-route win, with the improvement propagating into mid lock/page-create costs
- `main_r50` improves only a little because some mid-side cost remains

### MT Remote Lane Set Follow-up: B37 small observe

The same `route+small` observe libs were then run over the segment-registry lane set to check whether the canonical MT small-path story generalizes.

Commands:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_b37_route_small_obs.so ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Throughput:

| Lane | default observe | B37 observe | delta |
|---|---:|---:|---:|
| guard_r0 | 10,370,890.32 | 11,297,588.33 | +8.94% |
| main_r50 | 5,796,318.27 | 5,492,771.49 | -5.24% |
| cross64_r90 | 5,124,334.19 | 4,608,172.56 | -10.07% |

`main_r50` counter deltas:

| Counter | default | B37 | delta |
|---|---:|---:|---:|
| `HZ4_SMALL_STATS_B19.malloc_bin_hit` | 975,934 | 975,704 | -230 |
| `HZ4_SMALL_STATS_B19.malloc_refill_calls` | 17,465 | 17,695 | +230 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 17,465 | 17,695 | +230 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 4,833,591 | 4,869,923 | +36,332 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_hit` | 10,174,930 | 10,138,598 | -36,332 |
| `HZ4_MID_STATS_B1.malloc_page_create` | 232,299 | 317,201 | +84,902 |
| `HZ4_MID_STATS_B1.prefetched_hint_probe` | n/a | 39,682 | +39,682 |
| `HZ4_MID_STATS_B1.prefetched_hint_hit` | n/a | 20,331 | +20,331 |

`cross64_r90` counter deltas:

| Counter | default | B37 | delta |
|---|---:|---:|---:|
| `HZ4_SMALL_STATS_B19.malloc_bin_hit` | 488,257 | 488,356 | +99 |
| `HZ4_SMALL_STATS_B19.malloc_refill_calls` | 8,723 | 8,624 | -99 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 8,723 | 8,624 | -99 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 6,507,166 | 6,345,312 | -161,854 |
| `HZ4_MID_STATS_B1.malloc_alloc_cache_hit` | 8,935,019 | 9,096,873 | +161,854 |
| `HZ4_MID_STATS_B1.malloc_page_create` | 558,204 | 571,191 | +12,987 |
| `HZ4_MID_STATS_B1.prefetched_hint_probe` | n/a | 15,479 | +15,479 |
| `HZ4_MID_STATS_B1.prefetched_hint_hit` | n/a | 7,828 | +7,828 |

Notes:
- `guard_r0` improves, but the big change there is still ring pressure / fallback related rather than the small-path refill story.
- `main_r50` regresses, even though B37 does fire the hint path and the small-path counters move.
- `cross64_r90` also regresses, so the canonical MT small-path explanation does not generalize to the segment-registry lane set.
- On this follow-up, B37 is a canonical-MT candidate, not a segment-registry-lane candidate.

## mimalloc-bench subset

Command:

```bash
RUNS=3 ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mimalloc_bench_subset.sh
```

Logs:
- `/tmp/mimalloc_bench_subset_mac_20260319_043136/README.log`
- `/tmp/mimalloc_bench_subset_mac_20260319_043136/cache-thrash.log`
- `/tmp/mimalloc_bench_subset_mac_20260319_043136/cache-scratch.log`
- `/tmp/mimalloc_bench_subset_mac_20260319_043136/malloc-large.log`

Times below are median run time in milliseconds. Lower is better.

### cache-thrash

| Threads | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| 1 | 205.105 | 210.648 | 210.064 | 211.988 | 213.173 |
| 8 | 71.213 | 78.212 | 78.770 | 80.341 | 81.137 |

### cache-scratch

| Threads | system | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|---:|
| 1 | 203.381 | 210.345 | 210.481 | 219.168 | 213.704 |
| 8 | 71.800 | 78.320 | 80.623 | 80.011 | 80.365 |

### malloc-large

| Allocator | median ms |
|---|---:|
| system | 819.865 |
| hz3 | 820.325 |
| hz4 | 816.081 |
| mimalloc | 833.438 |
| tcmalloc | 877.882 |

## B70 Chunk Pages Sweep

This pass kept the segment-registry lane set fixed and swept
`HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES` on `main_r50` and `cross64_r90`.

Baseline observe run on `main_r50`:

| Throughput | malloc_lock_path | malloc_page_create | supply_resv_refill | lock_wait_ns_sum | lock_hold_ns_sum |
|---|---:|---:|---:|---:|---:|
| 6,574,617.14 ops/s | 4,927,437 | 158,170 | 21,121 | 119,302,000 | 19,287,000 |

Chunk sweep on `main_r50`:

| chunk_pages | throughput | malloc_page_create | supply_resv_refill | lock_wait_ns_sum | lock_hold_ns_sum |
|---|---:|---:|---:|---:|---:|
| 4 | 10,783,606.29 ops/s | 185,833 | 49,581 | 55,643,000 | 13,473,000 |
| 8 | 8,402,368.51 ops/s | 361,103 | 48,178 | 131,719,000 | 37,627,000 |
| 16 | 10,816,489.35 ops/s | 171,730 | 11,481 | 65,983,000 | 13,743,000 |

Notes:
- `main_r50` is still a mid-side workload, not a scan-heavy one.
- `chunk8` is the worst of the tested values in this pass.
- `chunk4` is the best throughput point on `main_r50`, but `chunk16` wins on `supply_resv_refill` and lock wait.
- `cross64_r90` also prefers `chunk16` on throughput and on the lock/refill counters.
- The current default candidate for B70 is therefore `chunk16`, with `chunk4` remaining a throughput-only tie on `main_r50`.

### Post-B70 default=16 fresh rerun

After promoting `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES` to 16, the same canonical Larson and MT remote shapes were rerun on the current release build.

#### Larson

Command:

```bash
./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Single sweep, release build:

| Allocator | ops/s |
|---|---:|
| system | 19,531,515 |
| hz3 | 17,971,477 |
| hz4 | 201,981,103 |
| mimalloc | 145,306,417 |
| tcmalloc | 110,815,125 |

Notes:
- `hz4` now clears `mimalloc` and `tcmalloc` on this fresh canonical Larson rerun.
- This is a single sweep, not a median sweep.
- Keep the earlier B37 comparison as pre-B70 history; this fresh rerun is the current default-release read.

#### MT Remote

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Single sweep, release build:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 43,298,153.65 | 50.1% | 0.00% |
| hz3 | 43,680,293.96 | 50.1% | 0.00% |
| hz4 | 111,643,154.79 | 50.1% | 0.00% |
| mimalloc | 91,787,565.25 | 50.1% | 0.00% |
| tcmalloc | 89,668,855.44 | 50.1% | 0.00% |

Notes:
- `hz4` stays clean at `ring_slots=262144`.
- This fresh rerun puts `hz4` ahead of `mimalloc` and `tcmalloc` on the MT remote shape as well.
- This is the current default-release ranking to trust after the B70 `chunk16` promotion.

### B37 on live chunk16 default

To check whether the earlier prefetched-bin-head candidate still helped on the new live default, the same canonical Larson shape was rerun with `HZ4_SO` pointing at the B37 custom library.

Command:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/libhakozuna_hz4_prefetch_bin_head.so ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Single sweep, release build:

| Allocator | ops/s |
|---|---:|
| system | 19,026,041 |
| hz3 | 17,399,672 |
| hz4 B37 | 73,662,382 |
| mimalloc | 28,693,969 |
| tcmalloc | 109,160,480 |

Notes:
- On the live `chunk16` default, B37 is far below the default-release `hz4` ranking.
- This is a single sweep, but the gap is large enough that B37 should stay historical on the new baseline.

### B37 Release-Only Median Recheck

To make the prefetched-bin-head comparison less noisy, the same canonical Larson shape was rerun 5 times with separate release-only library builds:

- default: [`libhakozuna_hz4_default.so`](/Users/tomoaki/git/hakozuna/hakozuna-mt/libhakozuna_hz4_default.so)
- B37: [`libhakozuna_hz4_b37.so`](/Users/tomoaki/git/hakozuna/hakozuna-mt/libhakozuna_hz4_b37.so)

Median ops/s:

| Build | ops/s |
|---|---:|
| default | 69,810,342 |
| B37 | 70,664,542 |
| delta | +1.2% |

Notes:
- This recheck is much smaller than the earlier pre-B70 uplift, so B37 is not a default-worthy win on its own.
- It still confirms that the hint path is live, but the remaining gain is only marginal on this Mac release path.

### B37 Current-Tree Sweep on the Live `chunk16` Baseline

To re-check the prefetched-bin-head branch against the current default-release
build, the same lane shapes were rerun with three release libraries:

- default: [`libhakozuna_hz4_default_current.so`](/Users/tomoaki/git/hakozuna/mac/out/release/libhakozuna_hz4_default_current.so)
- B37 `PREV_SCAN_MAX=2`: [`libhakozuna_hz4_prefetch_bin_head_current.so`](/Users/tomoaki/git/hakozuna/mac/out/release/libhakozuna_hz4_prefetch_bin_head_current.so)
- B37 `PREV_SCAN_MAX=1`: [`libhakozuna_hz4_prefetch_bin_head_scan1_current.so`](/Users/tomoaki/git/hakozuna/mac/out/release/libhakozuna_hz4_prefetch_bin_head_scan1_current.so)

`hz4` comparison only:

| Shape | current default | B37 `PREV_SCAN_MAX=2` | B37 `PREV_SCAN_MAX=1` |
|---|---:|---:|---:|
| `main_r50` | 43,165,756.70 | 41,638,743.60 | 32,018,422.34 |
| `cross64_r90` | 17,883,213.47 | 23,810,382.05 | 16,425,738.92 |

Notes:
- `PREV_SCAN_MAX=2` is the current-tree cross/high-remote specialist box: it
  loses a little on `main_r50` but gains strongly on `cross64_r90`.
- `PREV_SCAN_MAX=1` is no-go on both lanes.
- On the live `chunk16` baseline, B37 is not the next Larson mid default; keep
  it separate from the `FREE_BATCH_CONSUME_MIN=2` lane-specific candidate.

### HZ4_MID_FREE_BATCH_CONSUME_MIN=2 Release Recheck

The last still-live mid knob was rebuilt both as a plain release candidate and as a segment-registry-enabled release candidate, then rerun on the same Mac pass:

- Larson canonical SSOT shape: `./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4`
- MT remote canonical shape: `./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144`
- Segment-registry high-remote shape: `./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 2048 90 262144`

Release-only medians / single sweeps:

| Shape | default | `FREE_BATCH_CONSUME_MIN=2` | delta |
|---|---:|---:|---:|
| Larson canonical SSOT | 68,551,811 | 71,274,629 | +4.0% |
| MT remote canonical | 103,795,990.13 | 110,982,040.74 | +6.9% |
| Segment-registry high-remote | 51,138,878.63 | 72,778,698.46 | +42.3% |

Additional spot check:

- On the same segment-registry build, a 50% remote MT spot-check moved from `83,933,161.47` to `82,824,603.54` ops/s, so this knob is not universally positive.

Notes:
- This is now the strongest remaining live mid candidate on the Mac pass.
- The candidate is promising on canonical Larson, canonical MT remote, and the high-remote segment-registry lane, but it still needs lane-specific judgment before becoming a global default.

### HZ4_MID_FREE_BATCH_CONSUME_MIN=2 Observe Verification

The Mac observe lane was then rebuilt with the same candidate knob so the current
Mac design box could be checked with counters enabled. This is not a ranking
run; it is a "did the box behave as expected?" run.

Candidate observe libs:

- Larson / MT remote: [`libhakozuna_hz4_mid_free_batch2_observe.so`](/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_observe.so)
- Segment-registry lane: [`libhakozuna_hz4_mid_free_batch2_segreg_observe.so`](/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_observe.so)

Observed counters:

- canonical Larson:
  - `malloc_calls=45377319`
  - `lock_enter=6412`
  - `supply_resv_refill=234`
  - `free_fallback_path=16`
  - `refill_to_slow=14`
  - `segment_unknown=0`
- canonical MT remote:
  - `malloc_calls=25`
  - `lock_enter=25`
  - `supply_resv_refill=8`
  - `free_fallback_path=4000424`
  - `refill_to_slow=34002`
  - `large_validate_calls=4000448`
  - `small_page_valid_hits=4000424`
- segment-registry `cross64_r90`:
  - `seg_hits=4000444`
  - `large_validate_calls=4`
  - `free_fallback_path=4000424`
  - `refill_to_slow=72392`

Notes:

- The observe build is for counter validation, so the throughput numbers in this
  pass are not the ranking source of truth.
- The counters confirm that the current Mac box is live on the expected lanes
  and still splits cleanly between canonical Larson / MT remote and the
  segment-registry free-route lane.

### HZ4_MID_FREE_BATCH_CONSUME_MIN=2 Release Confirmation

The same candidate was then rebuilt as release libraries and rerun against the
current release baselines:

- canonical Larson baseline:
  - default release: `74,422,842 ops/s`
  - candidate release: `74,742,838 ops/s`
  - delta: `+0.4%`
- canonical MT remote baseline:
  - default release: `112,479,016.05 ops/s`
  - candidate release: `117,476,864.54 ops/s`
  - delta: `+4.4%`
- segment-registry `cross64_r90` baseline:
  - default release: `67,019,473.02 ops/s`
  - candidate release: `53,236,485.27 ops/s`
  - delta: `-20.6%`

Notes:

- The candidate is still a good Mac lane-specific box for canonical Larson and
  canonical MT remote.
- It is not a blanket default because the segment-registry high-remote lane
  regresses materially in release form.
- Keep the box lane-specific until the segment-registry path has a separate
  fix or a different candidate.

### Fresh hz3 Rebuild Verification

After the shared-core Mac cleanup work, `hz3` was rebuilt from scratch on the
same current working tree and rerun before touching any more `hz3` design
knobs. This was meant to answer a narrow question: were the earlier weak Mac
`hz3` numbers real, or were they coming from stale artifacts / mismatched lane
state?

Fresh release rebuild:

```bash
./mac/build_mac_release_lane.sh --skip-hz4
```

Fresh observe rebuild:

```bash
./mac/build_mac_observe_lane.sh --skip-hz4
```

Canonical Larson, fresh `hz3` release rerun:

```bash
for i in 1 2 3; do
  ALLOCATORS=hz3 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
done
```

Runs:

- `149,380,406 ops/s`
- `173,933,717 ops/s`
- `170,619,781 ops/s`

Median:

- `170,619,781 ops/s`

Canonical MT remote, fresh `hz3` release rerun:

```bash
for i in 1 2 3; do
  ALLOCATORS=hz3 ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
done
```

Runs:

- `102,102,613.40 ops/s`
- `96,271,095.52 ops/s`
- `112,607,287.45 ops/s`

Median:

- `102,102,613.40 ops/s`

### Post-`hz3` medium-run fast-path rerun

The next shared-core `hz3` step was to reduce fixed cost inside
`hz3_slow_alloc_from_segment_locked()` by:

- adding an early current-segment run allocation fast path
- collapsing the repeated page-tag / page-bin / `sc_tag` commit work into one
  shared helper

Accepted numbers below were taken as **serial** reruns. An earlier low MT
remote spot-check that ran in parallel with Larson was discarded as noisy.

Canonical Larson, serial `hz3` release rerun:

```bash
for i in 1 2 3; do
  ALLOCATORS=hz3 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
done
```

Runs:

- `190,414,408 ops/s`
- `196,339,575 ops/s`
- `172,992,193 ops/s`

Median:

- `190,414,408 ops/s`

Delta versus the fresh current-tree baseline:

- `+11.6%` versus `170,619,781 ops/s`

Canonical MT remote, serial `hz3` release rerun:

```bash
for i in 1 2 3; do
  ALLOCATORS=hz3 ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
done
```

Runs:

- `114,667,589.17 ops/s`
- `119,130,812.56 ops/s`
- `124,218,497.44 ops/s`

Median:

- `119,130,812.56 ops/s`

Delta versus the fresh current-tree baseline:

- `+16.7%` versus `102,102,613.40 ops/s`

Takeaway:

- the current `hz3` shared-core fast path is a real win on Mac
- the gain is clearest on canonical Larson, which matches the
  medium-segment-burst refill story from the observe lane
- the same patch did not hurt canonical MT remote; it lifted that median too,
  so this is safe to keep as the new current-tree reference point

Fresh observe sanity check on canonical Larson:

```bash
HZ3_SO=/Users/tomoaki/git/hakozuna/libhakozuna_hz3_obs.so \
ALLOCATORS=hz3 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Observed counters:

- `[HZ3_MACOS_MALLOC_SIZE] calls=199 hz3_hit=199 system_fallback=0 arena_unknown=0 zero_return=0`
- `[HZ3_MEDIUM_PATH] slow=1957 inbox_hit=0 central_hit=0 central_miss=1957 central_objs=0 segment_hit=1957 segment_objs=7328 segment_fail=0`
- no `[HZ3_NEXT_FALLBACK]` line was emitted on this run

Notes:

- The earlier weak Mac `hz3` read did not reproduce on a fresh rebuild.
- On the current tree, `hz3` is back in the "broad and strong" band that lines
  up with the paper-era intuition much better.
- The absence of `HZ3_NEXT_FALLBACK` traffic on canonical Larson weakens the
  theory that Mac `hz3` is slow because it frequently escapes to the system
  allocator.
- The current runner still resolves `hz3` from the same path
  (`ROOT_DIR/libhakozuna_hz3_scale.so`), so this does not look like an
  allocator-selection bug in `bench_find_allocator_library`.
- A temp old-style Mac build that tried to re-enable the generic `hz3_shim`
  path alongside `hz3_macos_interpose.o` failed with duplicate
  `malloc_usable_size` / `memalign` symbols, which supports the idea that the
  current Mac lane depends on the newer shim/interpose split and that the old
  weak read likely came from an older library artifact rather than today's
  runner configuration.
- The next `hz3` task should be to explain the old weak read and then profile
  the remaining hot path, not to assume that M1 itself is hostile to `hz3`.

### Fresh Synchronized Sweep on Current Tree

To compare the current working-tree libraries without mixing old result blocks,
the same canonical Larson and canonical MT remote shapes were rerun once with
the live `system / hz3 / hz4 / mimalloc / tcmalloc` set.

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Single sweep, current tree:

| Allocator | ops/s |
|---|---:|
| system | 15,423,223 |
| hz3 | 147,411,854 |
| hz4 | 68,239,230 |
| mimalloc | 27,452,374 |
| tcmalloc | 101,228,024 |

Command:

```bash
ALLOCATORS=system,hz3,hz4,mimalloc,tcmalloc ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Single sweep, current tree:

| Allocator | ops/s | actual remote | fallback |
|---|---:|---:|---:|
| system | 34,428,607.10 | 50.1% | 0.00% |
| hz3 | 100,774,873.10 | 50.1% | 0.00% |
| hz4 | 92,454,151.85 | 50.1% | 0.00% |
| mimalloc | 80,620,090.06 | 50.1% | 0.00% |
| tcmalloc | 45,296,279.42 | 50.1% | 0.00% |

Notes:

- This synchronized sweep is the best current quick ranking for the exact
  working tree on disk today.
- On this pass, `hz3` is ahead of the other allocators on both canonical
  Larson and canonical MT remote.
- That ranking is very different from the earlier weak Mac `hz3` blocks, so the
  old low numbers should now be treated as stale or mismatched until they can
  be intentionally reproduced.

## Quick Read

- the latest fresh `hz3` rebuild changes the Mac story materially: on the
  current tree, `hz3` is back in the broad/strong band and the earlier weak Mac
  `hz3` numbers did not reproduce.
- The current Larson command is a smoke shape, not the canonical `4KB..32KB` SSOT benchmark.
- The canonical larson rerun confirms the gap is still there, and it gets much larger for `tcmalloc` on this Mac.
- The redis-like median rerun gives a steadier baseline than the first-pass table; `mimalloc` wins the write-heavy paths while `tcmalloc` wins RANDOM.
- `MT remote` needed a larger `ring_slots` value to become apples-to-apples; `262144` removed fallback cleanly.
- The `segment_registry` box is the MT free-route box on `guard`, `main`, and `cross64`; `cross64_r90` is the strongest win, while `main_r50` still carries some mid-side cost.
- `guard_r0` still carries a little fallback, but the segment-registry path is no longer a small-path story.
- After the B70 `chunk16` promotion, the fresh release rerun puts `hz4` ahead of `mimalloc` and `tcmalloc` on both canonical Larson and MT remote.
- Larson on the current Mac release build is now also medianed, but the new default-release rerun shows that the live `chunk16` baseline has overtaken the earlier B37 result. Re-measure any further Larson mid box against the live default before promoting it.
- The direct B37 rerun on the live `chunk16` default came in far below the default-release `hz4`, so B37 is historical on the new baseline rather than the next Larson mid box.
- The MT remote route and small-stat observe comparisons show that free-route counters stay unchanged, while `HZ4_ST_STATS_B1.refill_to_slow` drops and `HZ4_SMALL_STATS_B19.malloc_bin_hit` rises by the same amount. That now explains most of the MT observe improvement, though a little release-vs-observe gap is still left.
- The same small-path story does not generalize to the segment-registry lane set: `guard_r0` improves, but `main_r50` and `cross64_r90` regress.
- `hz3`'s early Mac abort was not a benchmark-speed problem; it was a `malloc_size` interpose gap. The current observe lane now prints `[HZ3_MACOS_MALLOC_SIZE]` and keeps objc-facing foreign pointers alive.
- the fresh `hz3` observe rerun also showed no `HZ3_NEXT_FALLBACK` traffic on canonical Larson, so a "system fallback explains everything" theory is not supported by the current tree.
- For this M1 Mac pass, `ring_slots=262144` is the practical tuning value. Do not treat it as a universal default without checking the other OSes.
- In the mimalloc-bench subset, `system` was best or tied on the cache-thrash/scratch runs, while `tcmalloc` was slowest on `malloc-large`.
- `B70` chunk-page tuning is now the live mid-side default: `chunk16` is the best cross-lane compromise, while `chunk4` keeps a tiny throughput edge on `main_r50` only.
- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` is the last live mid candidate. It improves canonical Larson, canonical MT remote, and the high-remote segment-registry lane, but it is not a blanket default yet because a 50% remote segment-registry spot-check regressed slightly.

### Fresh `hz3` Observe Split With `S74`

To resolve the current `hz3` hot path more cleanly, the observe lane was
rebuilt with `HZ3_S74_STATS=1`.

Build:

```bash
./mac/build_mac_observe_lane.sh --skip-hz4 --hz3-s74-stats \
  --hz3-lib /Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so
```

Canonical Larson:

```bash
HZ3_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so \
ALLOCATORS=hz3 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Observed counters:

- `[HZ3_S196_REMOTE_DISPATCH] medium_calls=2992 medium_objs=2992`
- `[HZ3_S74] refill_calls=1961 refill_burst_total=7328 refill_burst_max=8 lease_calls=1961`
- `[HZ3_MEDIUM_PATH] slow=1957 inbox_hit=0 central_hit=0 central_miss=1957 central_objs=0 segment_hit=1957 segment_objs=7328 segment_fail=0`
- throughput `172,624,394 ops/s`

Interpretation:

- canonical Larson is a **medium segment-burst refill** box
- central and inbox are not the story on this current Mac lane
- the steady cost is shared-core `hz3_alloc_slow -> segment refill`, not
  Mac-only glue

Canonical MT remote:

```bash
HZ3_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so \
ALLOCATORS=hz3 ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Observed counters:

- `[HZ3_S196_REMOTE_DISPATCH] small_calls=1998167 small_objs=1998167 direct_n1_small_calls=1998167`
- `[HZ3_S97_REMOTE] ... groups=1998167 ... saved_calls=0 nmax=1`
- `[HZ3_S74] refill_calls=9 refill_burst_total=52 refill_burst_max=8 lease_calls=9`
- `[HZ3_MEDIUM_PATH] slow=5 ... segment_hit=5 segment_objs=52`
- throughput `100,498,754.65 ops/s`

Interpretation:

- canonical MT remote is a **small direct remote-dispatch** box
- remote groups are effectively all `n=1`
- alloc-side medium refill is almost irrelevant here

Takeaway:

- the current Mac `hz3` story is a two-box split, not one global bottleneck
- Larson and MT remote should be tuned separately

### Current-Source `hz4` Segment-Registry High-Remote Observe Check

The specialist free-route lane was re-checked on current-source observe
libraries.

Default route observe:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_obs_default_route.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Key output:

- `[HZ4_FREE_ROUTE_STATS_B26] seg_checks=0 seg_hits=0 large_validate_calls=16001732 large_validate_hits=62771 mid_magic_hits=15442168 small_page_valid_hits=496793`
- `[HZ4_ST_STATS_B1] free_fallback_path=496793 refill_to_slow=8658`
- `ops/s=4,360,481.85`, `actual=90.0%`, `fallback_rate=0.05%`

Segment-registry observe:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_obs_segreg_route.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Key output:

- `[HZ4_FREE_ROUTE_STATS_B26] seg_checks=16001732 seg_hits=15724114 seg_misses=277618 large_validate_calls=277618 large_validate_hits=62771 mid_magic_hits=15442168 small_page_valid_hits=496793`
- `[HZ4_ST_STATS_B1] free_fallback_path=496793 refill_to_slow=8593`
- `ops/s=4,527,136.81`, `actual=89.5%`, `fallback_rate=0.52%`

Interpretation:

- `segment_registry` still clearly short-circuits the heavy free-route
  validation path on high-remote MT
- the route box remains a specialist win
- this particular observe run also showed ring-pressure noise, so release
  medians stay the ranking source

### Current-source `segment_registry` table A/B

After the current-source `hz3` pass stabilized, the `hz4` specialist lane was
checked again on current-source observe and release builds.

Observe A/B on `cross64_r90`:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_segreg_mid_obs.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_segreg_slots64k_obs.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_segreg_probe16_obs.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Key outputs:

| Variant | seg_misses | large_validate_calls | supply_resv_refill | ops/s |
|---|---:|---:|---:|---:|
| current `segment_registry` | 100,415 | 100,415 | 26,582 | 11,404,674.21 |
| `slots=65536` | 63,430 | 63,430 | 25,321 | 11,957,062.43 |
| `probe=16` | 102,841 | 102,841 | 28,290 | 11,566,976.54 |

Takeaway:

- the remaining `cross64_r90` specialist cost is still partly registry-table
  pressure
- doubling the registry slots helped more than doubling the probe depth
- `probe=16` is not the right next box on this Mac pass

Release confirmation with `slots=65536`:

```bash
for i in 1 2 3; do
  HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/release/libhakozuna_hz4_segreg_slots64k_release.so \
  ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
done

for i in 1 2 3; do
  HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/release/libhakozuna_hz4_segreg_slots64k_release.so \
  ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 90 262144
done
```

Release medians versus the current-source `segment_registry` baseline:

| Lane | baseline | `slots=65536` | delta |
|---|---:|---:|---:|
| `cross64_r90` | 18,222,858.58 | 19,899,284.51 | +9.2% |
| `main_r50` | 27,558,514.57 | 27,412,778.20 | -0.5% |

Interpretation:

- `slots=65536` is a real **specialist** candidate for `cross64/high-remote`
- it is not a blanket promotion for the whole `segment_registry` lane family,
  because `main_r50` stays essentially flat
- the next `hz4` question remains mid-side cost on `main_r50`, while
  `cross64_r90` now has a concrete registry-table box to keep testing

### Shared-core guarded free-batch consume box

The next shared-core implementation step was to add a generic guard for
mid free-batch consume:

- new config: `HZ4_MID_FREE_BATCH_CONSUME_SC_MAX`
- default: all mid classes allowed
- intent: keep the mechanism shared across Mac / Linux / Windows while allowing
  lane-specific cutoffs during A/B

First Mac specialist candidate:

```bash
-DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1 \
-DHZ4_MID_FREE_BATCH_CONSUME_MIN=2 \
-DHZ4_MID_FREE_BATCH_CONSUME_SC_MAX=127
```

Clean single-run checks on the current tree:

| Lane | baseline | guarded candidate | note |
|---|---:|---:|---|
| `main_r50` | `27,558,514.57` | `15,131,337.46` | clear regression |
| `cross64_r90` | `18,222,858.58` | `18,703,371.66` | small uplift, but not enough to offset `main_r50` |

Takeaway:

- the **box framework is good** and should stay shared-core
- this first value choice (`MIN=2`, `SC_MAX=127`) is **NO-GO on the current Mac specialist lane**
- the next try should keep the shared-core guard, but choose a different cutoff
  or a different condition instead of promoting this exact tuple

Second Mac specialist check:

```bash
-DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1 \
-DHZ4_MID_FREE_BATCH_CONSUME_MIN=2 \
-DHZ4_MID_FREE_BATCH_CONSUME_SC_MIN=85 \
-DHZ4_MID_FREE_BATCH_CONSUME_SC_MAX=127
```

Clean single-run checks:

| Lane | baseline | `SC_MIN=85, SC_MAX=127` | note |
|---|---:|---:|---|
| `main_r50` | `27,558,514.57` | `23,564,299.64` | improved over the first guarded try, but still regresses |
| `cross64_r90` | `18,222,858.58` | `21,336,132.53` | stronger than baseline |

Interpretation:

- the `SC_MIN` gate is a better direction than `SC_MAX` alone
- but this tuple is still not a shared `main_r50 + cross64_r90` answer
- if we keep pushing this family, it should be treated as a **cross/high-remote
  specialist branch**, while `main_r50` still needs a different mid-side box

Single-shot SC-window sweep on the same family:

| Window | `main_r50` | `cross64_r90` |
|---|---:|---:|
| `96..127` | `19,958,766.02` | `20,312,196.43` |
| `104..127` | `19,409,002.72` | `23,299,801.98` |
| `112..127` | `20,555,141.62` | `25,536,731.60` |
| `120..127` | `22,828,248.87` | `18,685,587.74` |

Sweep takeaway:

- none of the tested windows recovered the `main_r50` baseline
  (`27,558,514.57 ops/s`)
- `112..127` is the strongest cross/high-remote specialist among the tested
  windows
- `120..127` begins to give back too much cross benefit

Decision:

- keep the shared-core `SC_MIN/SC_MAX` mechanism
- do **not** add a separate size-gate box yet
- on this allocator, `size` and `sc` are already a direct mapping through
  `HZ4_MID_ALIGN=256` and `hz4_mid_size_to_sc()`, so a size gate would be a
  duplicate expression of the same boundary rather than a meaningfully new box

### p32 Clean Preset Check

To confirm the new p32 clean presets no longer expose the stale `shards=96`
default on the T=128 MT remote observe shape, the p32 lane was rebuilt and
run directly.

Commands:

```bash
gmake -C /Users/tomoaki/git/hakozuna/hakozuna clean all_ldpreload_scale_p32_128
gmake -C /Users/tomoaki/git/hakozuna/hakozuna clean all_ldpreload_scale_p32_255
HZ3_SO=/Users/tomoaki/git/hakozuna/libhakozuna_hz3_scale_p32_128.so ALLOCATORS=hz3 \
  ./mac/run_mac_mt_remote_compare.sh 128 100000 400 16 2048 90 262144
HZ3_SO=/Users/tomoaki/git/hakozuna/libhakozuna_hz3_scale_p32_255.so ALLOCATORS=hz3 \
  ./mac/run_mac_mt_remote_compare.sh 128 100000 400 16 2048 90 262144
```

Observed log markers:

- `p32_128`: `[HZ3_SHARD_COLLISION_FAILFAST] shards=128 hit_shard=0 counter=129`
- `p32_255`: `[HZ3_MACOS_MALLOC_SIZE] calls=199 hz3_hit=199 system_fallback=0 arena_unknown=0 zero_return=0`
  and `[EFFECTIVE_REMOTE] target=90.0% actual=90.0% fallback_rate=0.00%`

Takeaway:

- the clean presets are doing the right thing now
- the stale `shards=96` display is gone on both runs
- if a collision still happens, the log now reflects the actual p32 preset
  shard count instead of the raw default
