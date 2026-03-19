# Mac Design Boxes

This page defines the Mac-specific design overlay for allocator bring-up and tuning.
Keep the allocator core shared across platforms. Keep the Mac-specific bits in
thin wrappers, explicit tuning boxes, and Mac-only profiling lanes.

## Why This Exists

The current Mac pass was measured on an Apple Silicon M1 machine, and the hot
paths do not line up 1:1 with the Linux or Windows observations:

- `hz3` currently splits into two allocator-owned boxes:
  - canonical Larson: `hz3_alloc_slow -> medium segment-burst refill`
  - canonical MT remote: `small direct remote-dispatch`
- `hz4` leans into `mid/free` and mutex / once overhead.
- the available profiling stack is Mac-specific (`xctrace`, `sample`,
  `System Trace`, `CPU Counters`).

That means we need a Mac-specific design layer, but not a Mac-specific allocator
fork.

## Shared Versus Mac-Specific

Keep these shared:

- workload shapes
- allocator comparison lists
- result schema
- go / no-go decisions

Keep these Mac-specific:

- launcher and injection path (`DYLD_INSERT_LIBRARIES`)
- profiling and trace export commands
- platform tuning values
- Mac-only build wrappers

## Proposed Split

Design the next passes in two layers:

### Shared Core

Keep these in the allocator core because Linux, Windows, and macOS all benefit:

- ownership classification must be cheap and explicit
- hot-path helpers must not do one-time init
- fast paths should avoid OS / zone lookups
- lock and refill costs should be visible through shared counters
- thread-local and batch behavior should stay allocator-level, not platform glue

### Mac Overlay

Keep these in the Mac-only layer:

- dyld interpose entrypoints and `malloc_size` glue
- system-zone fallback for foreign pointers only
- `xctrace` / `sample` / `System Trace` recipes
- M1-specific tuning values such as `ring_slots=262144`
- lane-specific candidate wrappers and build scripts

## First Design Moves

The current design direction for Mac should be:

1. keep the allocator core shared and make ownership checks cheaper there
2. keep the Mac shim thin and push zone fallback into the cold branch
3. remove hot-path init from repeated Mac helpers before adding new knobs
4. use Mac-only boxes for lane-specific tuning, not for allocator forks

The first concrete implementation step from this split is already aligned with
that rule: `hz3` Mac `malloc_size` stats registration should use constructor
setup instead of `pthread_once` in the repeated helper path.

The next shared-core step is also in place now:

- `hz4_mid` lock initialization keeps `pthread_once`, but the steady-state hot
  path first checks an atomic ready flag and only falls back to `pthread_once`
  before first init

That keeps the correctness model shared while making the repeated lock-acquire
path thinner on Mac and on the other OSes too.

## Direct Allocator Read

The current Mac profiling picture now has direct stack evidence for all of the
allocators we care about:

- `hz3`: current fresh runs are allocator-owned and split by workload shape
- `hz4`: `mid/free` plus mutex / once heavy
- `mimalloc`: page-queue / arena / reuse heavy
- `tcmalloc`: zone-bridge plus allocator-owned malloc/free heavy

That means the Mac design question should be framed like this:

- shared core work: cheaper ownership checks, less lock traffic, no repeated
  one-time init, better thread-local / batch behavior
- Mac overlay work: dyld interpose glue, foreign-pointer fallback, `malloc_size`
  bridge, profiling recipes

The direct `mimalloc` pass is useful because it confirms that a strong Mac lane
does not need a large Mac-specific fork. It needs a narrow hot path.

The fresh `hz3` rerun adds an important correction to that picture:

- canonical Larson on the current tree came back at `170.6M ops/s` median
- canonical MT remote came back at `102.1M ops/s` median
- the observe lane showed `HZ3_MACOS_MALLOC_SIZE system_fallback=0` and no
  `HZ3_NEXT_FALLBACK` dump on canonical Larson

So the next `hz3` question is no longer "why is Mac always weak?" but "what
artifact or old lane state produced the earlier weak Mac read, and what hot
path still remains after the fresh rebuild?"

The latest observe split sharpens that further:

- canonical Larson is a **shared-core medium segment-burst refill box**
- canonical MT remote is a **shared-core small direct remote-dispatch box**

So the right `hz3` tuning question is now "which box are we touching?" rather
than "is Mac globally hostile to `hz3`?"

The first shared-core `hz3` win after that split is now in place too:

- `hz3_slow_alloc_from_segment_locked()` now has an early current-segment
  run-allocation fast path
- the repeated medium-run metadata commit work is centralized in one helper
- serial Mac reruns improved canonical Larson from `170.6M` to `190.4M ops/s`
  and canonical MT remote from `102.1M` to `119.1M ops/s`

That makes this a **shared-core GO** on the current tree, not a Mac-only fork.

## Current Mac Boxes

### Build Box

- [`mac/build_mac_release_lane.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_release_lane.sh)
- [`mac/build_mac_observe_lane.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_observe_lane.sh)

### Profiling Box

- [`docs/MAC_PROFILING.md`](/Users/tomoaki/git/hakozuna/docs/MAC_PROFILING.md)
- [`mac/run_mac_malloc_size_probe.sh`](/Users/tomoaki/git/hakozuna/mac/run_mac_malloc_size_probe.sh)

### Current Mid Tuning Box

- live baseline: `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=16`
- current Mac candidate: `HZ4_MID_FREE_BATCH_CONSUME_MIN=2`
- keep the candidate lane-specific until the lower-remote spot-check is reconciled
- use [`mac/build_mac_mid_candidate_lane.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane.sh)
  to build the candidate quickly
- use [`mac/build_mac_mid_candidate_lane_segreg.sh`](/Users/tomoaki/git/hakozuna/mac/build_mac_mid_candidate_lane_segreg.sh)
  when you want the same box under the segment-registry free-route lane
- verified on the Mac observe lane with canonical Larson, canonical MT remote,
  and segment-registry `cross64_r90`
- release confirmation: good on canonical Larson and canonical MT remote, but
  the segment-registry `cross64_r90` release lane regresses, so do not promote
  it as a blanket default
- a shared-core guard box now exists too:
  `HZ4_MID_FREE_BATCH_CONSUME_SC_MAX`
- that guard is generic and safe to reuse on other OSes
- the first Mac specialist tuple (`MIN=2`, `SC_MAX=127`) regressed `main_r50`,
  so the **mechanism stays GO, the tested value stays NO-GO**

### Prefetched Bin Head Specialist Box

- current-tree `PREV_SCAN_MAX=2`
  - cross/high-remote specialist on the live `chunk16` baseline
  - better on `cross64_r90`, below default on `main_r50`
- current-tree `PREV_SCAN_MAX=1`
  - no-go on both `main_r50` and `cross64_r90`
- keep it separate from `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` and from the
  segment-registry free-route box

### Free-Route Box

- `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` is the free-route box for
  `guard_r0`, `main_r50`, and `cross64_r90`
- keep it separate from the canonical small-path candidate used by Larson and
  MT remote
- on the current-source Mac pass, `cross64_r90` also responds to a larger
  registry table (`HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS=65536`)
- treat that as a **cross/high-remote specialist overlay**, not a blanket
  promotion for `main_r50`

### Current `hz3` Two-Box Read

- canonical Larson:
  - `hz3_malloc / hz3_alloc_slow / segment refill`
  - `HZ3_MEDIUM_PATH slow=1957 segment_hit=1957 segment_objs=7328`
  - `HZ3_S74 refill_calls=1961 refill_burst_total=7328 refill_burst_max=8`
  - current serial release median after the shared-core fast path:
    `190,414,408 ops/s`
- canonical MT remote:
  - `small direct remote-dispatch`
  - `HZ3_S196_REMOTE_DISPATCH small_calls=1998167 direct_n1_small_calls=1998167`
  - `HZ3_S97_REMOTE ... saved_calls=0 nmax=1`
  - current serial release median after the shared-core fast path:
    `119,130,812.56 ops/s`

Treat these as separate shared-core boxes. Do not assume one Mac `hz3` fix
will move both lanes.

## Current Implementation Rule

Before promoting any new Mac box:

1. write the workload shape
2. write the allocator list
3. write the repeat rule
4. write the counters to read first
5. write the success criterion

Use the shared cross-platform ledger for the conditions:

- [`docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md`](/Users/tomoaki/git/hakozuna/docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [`docs/benchmarks/GO_NO_GO_LEDGER.md`](/Users/tomoaki/git/hakozuna/docs/benchmarks/GO_NO_GO_LEDGER.md)

## Current Mac Recommendation

The only live Mac mid candidate that still deserves an implementation box is:

- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2`

Treat it as a Mac lane-specific tuning box, not a shared default.
Keep the canonical Larson / MT remote build in one wrapper and the
segment-registry follow-up in a separate wrapper so the lane boundary stays
obvious.

## Open Research Boxes

The current Mac paper-suite follow-up has narrowed to two boxes:

### 1) `hz4` `malloc-large` large-path box

- Symptom: the latest paper suite shows `hz4` as the clear outlier on
  `malloc-large`.
- Boundary: keep this separate from canonical Larson, canonical MT remote, and
  the segment-registry free-route lane.
- Live knob: the current evidence points at the large extent cache band / cap,
  not a new runner.
- Read first: `HZ4_OS_STATS_B16.ext_acq_hit`, `ext_acq_miss`,
  `ext_rel_hit`, `ext_rel_miss`, `ext_rel_drop_cap`, `ext_bytes_peak`,
  plus `HZ4_FREE_ROUTE_STATS_B26.large_validate_calls` and
  `large_validate_hits`.
- Run shape: use the existing `run_mac_mimalloc_bench_subset.sh` with
  `DO_CACHE_THRASH=0 DO_CACHE_SCRATCH=0 DO_MALLOC_LARGE=1 RUNS=5`.
- Treatment candidate: compare the current build against
  `HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400` and
  `HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB`.
- Success criterion: close the gap materially without regressing the canonical
  Mac compare lanes.

### 2) Segment-registry high-remote fallback box

- Symptom: the 90% remote diagnostic lane still shows
  `fallback_rate=12.32%` and actual remote below target.
- Boundary: keep this separate from the canonical 50% remote MT pass and from
  the B37 small-path work.
- Read first: `actual`, `fallback_rate`, `ring_full_fallback`,
  `overflow_sent`, `overflow_received`, `large_validate_calls`,
  `malloc_lock_path`, `free_fallback_path`, and `refill_to_slow`.
- A/B shape: use `build_mac_mid_candidate_lane_segreg.sh --slots 32768` and
  `--slots 65536`, both with `ring_slots=262144`.
- Success criterion: decide whether the lane is pressure-bound only. If it is,
  keep `ring_slots` tuning as bench conditioning instead of promoting it as an
  allocator win.
