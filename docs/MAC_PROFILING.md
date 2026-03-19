# Mac Profiling

This page records the lightest profiling path that was useful for the current
Mac allocator investigation. The current Mac pass was measured on an Apple
Silicon M1 machine, full Xcode is installed, and `xctrace` is available. Keep
the lane separate from Ubuntu/Linux and Windows.

## What To Try First

1. `Time Profiler`
   - Best first xctrace template for a quick slowness check.
   - Good for answering "which code path is hot?"
   - Use it on a single allocator lane first, not on a full allocator sweep.

2. `CPU Counters`
   - Useful when you want hardware-level confirmation after Time Profiler.
   - This is the best second pass when you want to know whether the run is
     front-end bound, processing bound, or bottlenecked by delivery.

3. `sample`
   - Lightweight fallback if the xctrace export is awkward.
   - Good for a fast stack answer without building a larger trace workflow.

4. `System Trace`
   - Useful if the first pass shows mostly waiting or scheduler noise.
   - Best when you need to understand lock contention, wakeups, or thread
     parking.

5. `Allocations` / `Leaks`
   - Helpful for memory growth or leak questions.
   - Not the first choice for throughput slowness.

## Working Commands

### 1) Time Profiler on canonical Larson

```bash
xcrun xctrace record --template 'Time Profiler' \
  --time-limit 5s \
  --output /tmp/hz3_larson_timeprof.trace \
  --no-prompt \
  --env DYLD_INSERT_LIBRARIES="$PWD/libhakozuna_hz3_scale.so" \
  --launch -- ./mac/out/bench_larson_compare 5 4096 32768 1000 1000 0 4
```

The same shape works for `hz4` and `tcmalloc` by swapping the injected library.
For example:

```bash
xcrun xctrace record --template 'Time Profiler' \
  --time-limit 5s \
  --output /tmp/tcmalloc_larson_timeprof.trace \
  --no-prompt \
  --env DYLD_INSERT_LIBRARIES=/opt/homebrew/opt/gperftools/lib/libtcmalloc.dylib \
  --launch -- ./mac/out/bench_larson_compare 5 4096 32768 1000 1000 0 4
```

Useful verification:

```bash
DYLD_PRINT_LIBRARIES=1 \
DYLD_INSERT_LIBRARIES="$PWD/libhakozuna_hz3_scale.so" \
./mac/out/bench_larson_compare 1 4096 32768 1000 1000 0 1
```

### 2) sample on canonical Larson

```bash
DYLD_INSERT_LIBRARIES="$PWD/libhakozuna_hz3_scale.so" \
./mac/out/bench_larson_compare 5 4096 32768 1000 1000 0 4 \
  >/tmp/hz3_larson_run.log 2>&1 &
PID=$!
sleep 1
sample "$PID" 5 -file /tmp/hz3_larson_sample.txt
wait "$PID"
```

### 3) sample on MT remote

```bash
DYLD_INSERT_LIBRARIES="$PWD/libhakozuna_hz3_scale.so" \
./mac/out/bench_random_mixed_mt_remote_malloc \
  4 50000000 400 16 2048 50 262144 \
  >/tmp/hz3_mt_remote_run.log 2>&1 &
PID=$!
sleep 1
sample "$PID" 5 -file /tmp/hz3_mt_remote_sample.txt
wait "$PID"
```

## What The Current Pass Showed

### Canonical Larson

The Time Profiler traces on the current Mac lane were readable enough to show
the dominant leafs directly.

`hz3` leaf hot spots:

- `_xzm_xzone_malloc_small_freelist`
- `_xzm_xzone_malloc_from_freelist_chunk`
- `_xzm_xzone_malloc_freelist_outlined`
- `_xzm_xzone_free_freelist`
- `_xzm_xzone_find_and_malloc_from_freelist_chunk`
- `_xzm_free_outlined`
- `larson_thread`
- `_xzm_chunk_list_slot_push`
- `_xzm_chunk_list_pop`
- `_malloc_zone_malloc`

`hz4` leaf hot spots:

- `hz4_mid_malloc`
- `larson_thread`
- `hz4_mid_free`
- `hz4_free`
- `hz4_malloc`
- `pthread_mutex_lock`
- `pthread_mutex_unlock`
- `pthread_once`
- `_tlv_get_addr`

`tcmalloc` leaf hot spots:

- `find_zone_and_free`
- `larson_thread`
- `_malloc_zone_malloc_instrumented_or_legacy`
- `tc_malloc`
- `tc_free`
- `_malloc_zone_malloc`
- `TCMallocImplementation::GetAllocatedSize`
- `TCMallocImplementation::GetOwnership`
- `MallocExtension::instance()`

`mimalloc` leaf hot spots:

- `_mi_malloc_generic`
- `mi_page_queue_find_free_ex`
- `mi_page_fresh_alloc`
- `mi_arenas_page_regular_alloc`
- `mi_bitmap_try_find_and_claim`
- `mi_find_page`
- `_mi_os_reuse`
- `madvise`
- `_mi_arenas_collect`
- `_mi_prim_clock_now`

Takeaway:

- `Time Profiler` is good enough to show allocator-specific leafs on Mac.
- the first `hz3` Time Profiler read looked xzone-heavy, but the later fresh
  `sample` + observe split showed the current tree is allocator-owned and
  workload-shaped; treat the xzone-heavy read as an earlier clue, not the final
  Mac picture.
- `hz4` is spending its time in its own mid/free path and lock/unlock helpers.
- `mimalloc` stays mostly inside its own page-queue / arena path and does not
  show the same xzone-heavy or mutex-heavy shape as `hz3` / `hz4`.
- `tcmalloc` is dominated by zone bridge helpers and its own malloc/free path.
- This makes `Time Profiler` the right first xctrace tool for allocator slowness
  and `CPU Counters` the right second pass when you want to talk about delivery
  vs processing.

### Fresh `hz3` sample after rebuild

After the fresh `hz3` rebuild, a direct `sample` pass on canonical Larson was
taken from the live release library:

```bash
DYLD_INSERT_LIBRARIES="$PWD/libhakozuna_hz3_scale.so" \
./mac/out/bench_larson_compare 10 4096 32768 1000 1000 0 4 \
  >/tmp/hz3_larson_run_current.log 2>&1 &
PID=$!
sleep 1
sample "$PID" 5 -file /tmp/hz3_larson_sample_current.txt
wait "$PID"
```

This newer read looked very different from the earlier stale-looking xzone-heavy
picture. The dominant stacks were now centered on:

- `hz3_free`
- `hz3_malloc`
- `hz3_macos_malloc`
- `hz3_macos_free`
- `hz3_alloc_slow`

The same run also reported:

- `[HZ3_MACOS_MALLOC_SIZE] ... system_fallback=0`
- no `[HZ3_NEXT_FALLBACK]` line
- runtime throughput `156,931,707 ops/s`

Takeaway:

- The older xzone-heavy `hz3` profile should not be treated as the final Mac
  picture for the current tree.
- On the fresh rebuilt lane, `hz3` is spending its time primarily inside its
  own allocator entrypoints, not visibly escaping into a system fallback path.
- The next `hz3` profiling box should therefore focus on allocator-owned hot
  work such as `hz3_malloc` / `hz3_free` / `hz3_alloc_slow`, plus any remaining
  medium-path and remote-dispatch cost.

### Fresh `hz3` two-box observe split

To separate the current `hz3` Mac story into allocator-owned boxes, the observe
lane was rebuilt with `HZ3_S74_STATS=1` and rerun on canonical Larson and
canonical MT remote:

```bash
./mac/build_mac_observe_lane.sh --skip-hz4 --hz3-s74-stats \
  --hz3-lib /Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so
```

Canonical Larson:

```bash
HZ3_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so \
ALLOCATORS=hz3 ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4
```

Key counters:

- `[HZ3_S196_REMOTE_DISPATCH] medium_calls=2992 medium_objs=2992`
- `[HZ3_S74] refill_calls=1961 refill_burst_total=7328 refill_burst_max=8 lease_calls=1961`
- `[HZ3_MEDIUM_PATH] slow=1957 inbox_hit=0 central_hit=0 central_miss=1957 central_objs=0 segment_hit=1957 segment_objs=7328 segment_fail=0`
- throughput `172,624,394 ops/s`

Canonical MT remote:

```bash
HZ3_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz3_obs_s74.so \
ALLOCATORS=hz3 ./mac/run_mac_mt_remote_compare.sh 4 2000000 400 16 2048 50 262144
```

Key counters:

- `[HZ3_S196_REMOTE_DISPATCH] small_calls=1998167 small_objs=1998167 direct_n1_small_calls=1998167`
- `[HZ3_S97_REMOTE] ... groups=1998167 ... saved_calls=0 nmax=1`
- `[HZ3_S74] refill_calls=9 refill_burst_total=52 refill_burst_max=8 lease_calls=9`
- `[HZ3_MEDIUM_PATH] slow=5 ... segment_hit=5 segment_objs=52`
- throughput `100,498,754.65 ops/s`

Takeaway:

- The current Mac `hz3` path is not one box.
- Canonical Larson is a **medium segment-burst refill** box: central is missing,
  inbox is unused, and the allocator is living in `hz3_alloc_slow -> segment hit`.
- Canonical MT remote is a **small direct remote-dispatch** box: remote groups
  are effectively all `n=1`, and alloc-side segment refill barely matters.
- That means future `hz3` tuning should treat Larson and MT remote separately
  instead of assuming one shared Mac bottleneck.

### Shared-core `hz3` fast-path confirmation

The next shared-core patch was applied to
`hz3_slow_alloc_from_segment_locked()`:

- add an early current-segment run-allocation fast path
- centralize the repeated medium-run metadata commit work into one helper

Serial release reruns after that patch came back as:

- canonical Larson median: `190,414,408 ops/s`
- canonical MT remote median: `119,130,812.56 ops/s`

Compared with the fresh pre-patch baselines:

- Larson: `170,619,781 -> 190,414,408` (`+11.6%`)
- MT remote: `102,102,613.40 -> 119,130,812.56` (`+16.7%`)

Takeaway:

- the current Mac `hz3` hot path still benefits from shared-core fixed-cost
  reduction
- the patch fits the two-box story instead of fighting it: Larson benefits most,
  but MT remote also moves in the right direction
- the next `hz3` profiling question is no longer whether this path is live, but
  whether the remaining cost sits in `hz3_alloc_slow` itself or in the
  `n=1` remote-dispatch side

### Current-source `hz4` segment-registry observe check

The current-source high-remote specialist lane was also compared directly on
Mac observe builds.

Default route box:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_obs_default_route.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Segment-registry route box:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_obs_segreg_route.so \
ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144
```

Key deltas:

- default route:
  - `[HZ4_FREE_ROUTE_STATS_B26] seg_checks=0 seg_hits=0 large_validate_calls=16001732`
  - `[HZ4_ST_STATS_B1] free_fallback_path=496793 refill_to_slow=8658`
  - `ops/s=4,360,481.85`, `actual=90.0%`, `fallback_rate=0.05%`
- segment-registry route:
  - `[HZ4_FREE_ROUTE_STATS_B26] seg_checks=16001732 seg_hits=15724114 seg_misses=277618 large_validate_calls=277618`
  - `[HZ4_ST_STATS_B1] free_fallback_path=496793 refill_to_slow=8593`
  - `ops/s=4,527,136.81`, `actual=89.5%`, `fallback_rate=0.52%`

Takeaway:

- `segment_registry` is still clearly short-circuiting the expensive free-route
  validation path.
- This particular observe run also showed some ring-pressure noise, so release
  medians remain the ranking source.
- The box still reads as a specialist free-route win, not as a blanket default
  for every MT shape.

### Why This Matters

- The direct `mimalloc` Mac pass supports the current design split:
  keep allocator improvements in the shared core, and keep dyld / zone /
  profiling glue in the Mac overlay.
- The current evidence does not point to a Mac-only compiler-flag explanation.
- The current evidence does point to `hz3/hz4` carrying more ownership,
  synchronization, or Mac-glue work in their hot paths.

### CPU Counters

The `CPU Counters` template is available and records the useful bottleneck
tables for the current M1 Mac lane.

The exported tables include:

- `MetricAggregationForThread`
- `MetricTableForThread`

That makes it a good follow-up when `Time Profiler` says "the allocator path is
hot" but you want front-end versus processing confirmation.

## Recommendation

Use this order:

1. `Time Profiler` on canonical Larson
2. `CPU Counters` if you need hardware-level confirmation
3. `sample` if the trace export is awkward
4. `sample` on MT remote if Larson is too noisy
5. `System Trace` only if the question is lock contention or wakeups

## Related Docs

- [Mac build notes](/Users/tomoaki/git/hakozuna/docs/MAC_BUILD.md)
- [Mac benchmark prep order](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md)
- [GO / NO-GO ledger](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)
