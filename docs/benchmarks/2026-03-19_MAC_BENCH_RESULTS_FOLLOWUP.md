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

### 2026-03-21 `malloc-large` Research Capture

The new `mac/run_mac_malloc_large_research.sh` wrapper builds a stats-enabled
`hz4` observe lib and captures allocator stderr so the first-read counters stay
visible in the logs. The `RUNS=5` isolated pass kept `hz4` as the clear
outlier:

| Allocator | median time (ns) | median time (s) |
|---|---:|---:|
| system | 796,408,000 | 0.796 |
| hz3 | 805,145,000 | 0.805 |
| hz4 | 2,698,075,000 | 2.698 |
| mimalloc | 875,469,000 | 0.875 |
| tcmalloc | 913,846,000 | 0.914 |

Captured `hz4` counters on the same run:

| Counter | Median |
|---|---:|
| `HZ4_OS_STATS_B16.ext_acq_hit` | 172 |
| `HZ4_OS_STATS_B16.ext_acq_miss` | 939 |
| `HZ4_OS_STATS_B16.ext_rel_hit` | 203 |
| `HZ4_OS_STATS_B16.ext_rel_miss` | 908 |
| `HZ4_OS_STATS_B16.ext_rel_drop_cap` | 908 |
| `HZ4_OS_STATS_B16.ext_bytes_peak` | 268,435,456 |
| `HZ4_FREE_ROUTE_STATS_B26.free_calls` | 2,014 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 2,014 |
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_hits` | 2,000 |

Interpretation:

- the new box is observable now, but `hz4` is still the broad slow outlier on
  `malloc-large`
- `ext_bytes_peak` plateaued at 256 MiB, so the next hypothesis should be
  narrower than the first band/cap treatment
- the free route still pays one `large_validate` per large candidate, so the
  remaining cost is not just the OS cache band

### 2026-03-21 Segment-Registry High-Remote A/B

The same capture-enabled workflow was then used on the high-remote segment-
registry lane set with `ring_slots=262144` fixed.

Commands:

```bash
HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_slots32768.so \
  ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh --out-dir /Users/tomoaki/git/hakozuna/mac/out/research_mt_segreg_32768 \
  16 2000000 400 16 2048 90 262144

HZ4_SO=/Users/tomoaki/git/hakozuna/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_slots65536.so \
  ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh --out-dir /Users/tomoaki/git/hakozuna/mac/out/research_mt_segreg_65536 \
  16 2000000 400 16 2048 90 262144
```

Throughput summary:

| Slots | ops/s | actual remote | fallback rate | ring_full_fallback |
|---|---:|---:|---:|---:|
| 32768 | 12,923,089.54 | 57.8% | 32.19% | 5,150,023 |
| 65536 | 11,761,185.37 | 54.8% | 35.21% | 5,632,292 |

Counter read:

| Counter | 32768 | 65536 |
|---|---:|---:|
| `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` | 16 | 16 |
| `HZ4_MID_STATS_B1.malloc_lock_path` | 97 | 97 |
| `HZ4_ST_STATS_B1.free_fallback_path` | 16,001,636 | 16,001,636 |
| `HZ4_ST_STATS_B1.refill_to_slow` | 187,334 | 165,096 |

Interpretation:

- `32768` remains the better reference on the pressure-bound 90% lane
- `65536` worsens actual remote utilization and increases fallback pressure
- the remaining gap looks like bench pressure rather than allocator cost on
  this shape
