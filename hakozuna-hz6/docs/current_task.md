# HZ6 Current Task

HZ6 is now in R1 implementation. Keep the implementation modular from the
start: API, route, frontcache, transfer, source, owner, policy, and future
fronts should stay separated.

Current Windows capacity lane names and promotion status are summarized in
`HZ6_LANE_GUIDE.md`. Keep this file as the longer investigation log.

## Current Direction Freeze

```text
Decision:
  HZ6 is no longer judged as a single "fast or slow" lane.

Evaluation split:
  compact control:
    shows the raw strength of route / transfer / front-cache hot paths

  stress:
    shows how source admission, cache retention, remote backlog, scavenge,
    and OS backing fail or recover under pressure

Main next design target:
  HZ6-ControlPlane-L1-admission

Role:
  stress-aware source / transfer / scavenge governor

Do not mix:
  hot path probe/atomic counters into production benchmark lanes
```

## Shared Contract And OS Backing

```text
Shared core contract:
  RouteLayer:
    MISS / VALID / INVALID

  OwnerLayer:
    owner token
    generation
    alive/dead
    stale owner publish forbidden

  StateLayer:
    ACTIVE
    LOCAL_FREE
    CENTRAL_FREE / TRANSFER_FREE
    REMOTE_PENDING
    RELEASED
    ORPHAN

  TransferLayer:
    bounded
    consumer-visible
    checked before source refill
    no unbounded backlog

  SourceLayer:
    reserve
    commit
    decommit
    release

  ScavengeLayer:
    byte budget
    pressure checkpoint
    payload decommit

  PolicyLayer:
    no hot path counters
    slow path / epoch / checkpoint only
```

```text
Shared:
  allocator semantics
  route result contract
  state transition
  transfer/refill/scavenge ordering
  profile names
  benchmark attribution rules

Windows-specific backing:
  VirtualAlloc / VirtualFree
  CRT bridge
  sidecar route table
  no VirtualQuery in the hot path
  allocation granularity
  TLS/FLS behavior
  page-boundary handling

Linux-specific backing:
  mmap / munmap
  madvise
  rseq/per-cpu candidates
  region/slab map
  preload route order
  ELF/TLS/linkage behavior
```

## Benchmark Interpretation Freeze

```text
compact control:
  Use smaller working-set controls such as Larson chunks=400.
  Purpose is hot-path and front-cache comparison.

stress:
  Use Larson T=16 / high chunks and similar pressure rows.
  Purpose is ControlPlane evaluation, not hot-path ranking.

paper wording:
  HZ6 compact profile:
    fast route / transfer / front-cache path evidence

  HZ6 stress profile:
    source admission and scavenge policy stress evidence

  HZ6 RSS profile:
    low-RSS / bounded-backlog / fail-closed evidence

Do not conclude:
  stress regression alone means the HZ6 hot path is weak

Do conclude:
  stress regression points at ControlPlane-L1 admission/scavenge work
```

## Lane Map Freeze

```text
Active lanes:
  compact control:
    same runtime/size/seed settings with reduced working-set pressure
    use this for hot-path and front-cache comparison

  Larson main-warmup:
    main thread seeds warmup allocations; workers free/replace them
    use this as cross-owner warmup / route-lifecycle stress evidence

  Larson worker-warmup:
    each worker seeds its own live set before the timer starts
    use this as same-owner control for toy/small source placement

  descriptor/source ownership L2:
    diagnostic evidence only
    do not turn this into a production atomic/probe lane

Frozen lane intent:
  main-warmup:
    cross-owner stress lane

  worker-warmup:
    same-owner control lane

  compact control:
    hot-path control lane

  stress:
    control-plane evidence lane

Do not mix:
  diagnostics / atomic counters / probe-only paths into production benchmark
  summaries

Canonical summary rule:
  keep one representative summary per active lane
  archive dated summaries after the lane is frozen
  do not promote raw results into paper-facing sources
```

## Route Locality Checkpoint 2026-06-01

```text
Decision:
  stop deepening negative_filter knobs
  stop treating visible-first as the main answer

Current next lane:
  OwnerLocalityIndex-L1

Shape:
  truth:
    exact route / descriptor state

  hint:
    cheap local exact ownership hint

  backend:
    shared route directory exact lookup

Why:
  source-block-only locality hints were no-go
  shared directory proved foreign exact routes can be recovered
  the remaining hurt is the worker-local MISS scan around rehomed exact routes
  owner locality is the narrowest next step that keeps hint/backend separation
  clean

Implementation rule:
  keep owner locality diagnostic-backed
  keep shared directory as the backend exact path
  keep the ordinary local route path as fallback, not as a replacement for the
  truth source
```

## Owner Locality Smoke 2026-06-01

```text
Build:
  build_win_larson_suite.ps1 -DiagnosticHz6Probes
  succeeded, including ownerlocality-appcap

Smoke readback:
  worker-warmup:
    throughput = 49.3M ops/s
    route_exact_lookup_probe_total = 52.9M
    owner_locality_lookup = 0
    owner_locality_hit_foreign_allocator = 0
    shared_dir_first_hit = 0

  main-warmup:
    throughput = 43.6M ops/s
    route_exact_lookup_probe_total = 44.7M
    owner_locality_lookup = 200
    owner_locality_hit_foreign_allocator = 200
    owner_locality_probe_total = 200
    shared_dir_register = 488
    route_rehome_success = 200

Read:
  the owner-locality lane is alive and the hint/backend split is visible in the
  stats
  worker-warmup stays clean
  main-warmup now exposes the rehomed foreign exact route path instead of a
  blind local MISS scan
```

## Owner Locality Comparison 2026-06-01

```text
Same small smoke, main-warmup:
  ownerlocality-appcap:
    throughput = 43.6M ops/s
    route_lookup_probe_total = 44.7M
    route_exact_lookup_probe_total = 44.7M
    owner_locality_lookup = 200

  appcap:
    throughput = 47.7M ops/s
    route_lookup_probe_total = 153.8M
    route_visibility_lookup = 200

Read:
  owner locality is not a pure speed win yet
  but it cuts the worker-local MISS scan by a large factor
  worker-warmup remains healthy either way
```

## Owner Locality Lifecycle Fix 2026-06-02

```text
Problem:
  ownerlocality-appcap recovered main-warmup throughput and removed the local
  MISS scan, but the process could crash after printing stats.

Root cause:
  Larson captured worker stats, then let worker allocators tear down while the
  shared live-set still contained objects rehomed to those worker allocators.
  The later main-thread cleanup could then touch routes/descriptors whose owner
  allocator lifetime had ended.

Fix:
  each Larson worker now releases its live set after taking the measured stats
  snapshot and before hz_bench_allocator_thread_teardown().

Why the snapshot stays valid:
  cleanup remains outside the measured interval and outside the printed HZ6
  worker stats, so the benchmark row still describes the timed phase.

Smoke after fix:
  ownerlocality-appcap main-warmup chunks=1000/T=16:
    exit = 0
    throughput = 37.0M ops/s
    route_invalid = 0
    route_miss = 0
    owner_locality_hit_foreign_allocator = 8000
    shared_dir_first_hit = 8000
    route_rehome_success = 8000
    route_rehome_fail = 0
    route_lookup_probe_total = 2504
    Done sleeping... printed

  ownerlocality-appcap worker-warmup chunks=1000/T=16:
    exit = 0
    throughput = 49.7M ops/s
    owner_locality_lookup = 0
    route_rehome_attempt = 0
    route_invalid = 0
    route_miss = 0

  ownerlocality-appcap main-warmup chunks=4000/T=16:
    exit = 0
    throughput = 14.1M ops/s
    owner_locality_hit_foreign_allocator = 32000
    shared_dir_first_hit = 32000
    route_rehome_success = 32000
    route_rehome_fail = 0
    route_lookup_probe_total = 3688

Read:
  OwnerLocalityIndex-L1 is now a valid mechanism-evidence lane rather than a
  crashy lifecycle artifact.
  It converts main-warmup from a worker-local route MISS scan problem into a
  bounded exact-route rehome/transfer path.
  It is still not a default promotion lane; next evaluation should compare
  ownerlocality-appcap against appcap on repeat rows and RSS/cleanup behavior.
```

## Owner Locality Appcap 1-Run Comparison 2026-06-02

```text
Runner:
  bench_larson_hz6_speed_*_appcap.exe
  args:
    runtime=1
    min=8
    max=1024
    rounds=1
    seed=12345
    threads=16

chunks=1000:
  appcap main-warmup:
    exit = 0
    throughput = 455 ops/s
    route_lookup_probe_total = 1267732241

  ownerlocality main-warmup:
    exit = 0
    throughput = 51.4M ops/s
    route_lookup_probe_total = 2503
    route_exact_lookup_probe_total = 53670289
    owner_locality_lookup = 8000
    shared_dir_first_hit = 8000
    route_rehome_fail = 0
    route_invalid = 0
    route_miss = 0

  appcap worker-warmup:
    exit = 0
    throughput = 54.8M ops/s

  ownerlocality worker-warmup:
    exit = 0
    throughput = 55.6M ops/s
    owner_locality_lookup = 0
    route_rehome_fail = 0
    route_invalid = 0
    route_miss = 0

chunks=4000:
  appcap main-warmup:
    exit = 0
    throughput = 95 ops/s
    route_lookup_probe_total = 1269828992

  ownerlocality main-warmup:
    exit = 0
    throughput = 44.3M ops/s
    route_lookup_probe_total = 4066
    route_exact_lookup_probe_total = 46060427
    owner_locality_lookup = 32000
    shared_dir_first_hit = 32000
    route_rehome_fail = 0
    route_invalid = 0
    route_miss = 0

  appcap worker-warmup:
    exit = 0
    throughput = 45.7M ops/s

  ownerlocality worker-warmup:
    exit = 0
    throughput = 46.5M ops/s
    owner_locality_lookup = 0
    route_rehome_fail = 0
    route_invalid = 0
    route_miss = 0

Read:
  ownerlocality-appcap cleanly closes the appcap main-warmup collapse in this
  1-run matrix.
  The same-owner worker-warmup control is not harmed.
  The remaining production question is not "does the mechanism work"; it is
  whether the owner-locality/shared-directory exact path can be made
  non-diagnostic, lifecycle-safe, and RSS-neutral across the wider suite.
```

## Owner Locality Matrix Runner Wiring 2026-06-02

```text
Change:
  run_win_hz6_capacity_matrix.ps1 now accepts:
    ownerlocality-appcap

  Larson benchmark profile selection now includes focused short rows:
    larson_t16_main_1k
    larson_t16_worker_1k
    larson_t16_main_4k
    larson_t16_worker_4k

Why:
  the default larson_T16 row uses chunks=10000 main-warmup and can be too slow
  for the broken appcap baseline.
  The focused rows let us compare route-lifecycle mechanisms without waiting
  for the known appcap local-MISS scan collapse.

Runner fix:
  capacity matrix output capture now redirects stdout/stderr to temporary files
  while monitoring PeakWorkingSet.
  This avoids pipe back-pressure when diagnostic HZ6 stats are long.

Focused ownerlocality run:
  command:
    run_win_hz6_capacity_matrix.ps1
      -Families larson
      -Hz6Profiles speed
      -CapacityLanes ownerlocality-appcap
      -BenchmarkProfiles larson_t16_main_1k,larson_t16_worker_1k,
                         larson_t16_main_4k,larson_t16_worker_4k
      -Runs 1
      -DiagnosticHz6Probes
      -SkipBuild

  ownerlocality-appcap:
    main_1k   = 50.8M ops/s
    worker_1k = 56.8M ops/s
    main_4k   = 45.2M ops/s
    worker_4k = 50.8M ops/s

Read:
  the HZ6 capacity matrix can now run the owner-locality Larson rows directly.
  Use the short focused rows for route-lifecycle iteration.
  Keep chunks=10000 main-warmup as a stress row, not as the default iteration
  target while appcap is known to collapse.
```

## Owner Locality Fast Lane 2026-06-02

```text
Change:
  split the owner-locality mechanism from diagnostic probes.

  ownerlocality-appcap:
    diagnostic evidence lane
    forces HZ6_DIAGNOSTIC_PROBES=1

  ownerlocalityfast-appcap:
    non-diagnostic behavior lane
    enables:
      HZ6_SHARED_ROUTE_DIRECTORY_L1=1
      HZ6_OWNER_LOCALITY_INDEX_L1=1
    does not force HZ6_DIAGNOSTIC_PROBES=1

Implementation:
  shared route directory storage/register/unregister/exact lookup now compile
  under HZ6_SHARED_ROUTE_DIRECTORY_L1 without requiring diagnostic probes.
  Diagnostic counters remain behind HZ6_DIAGNOSTIC_PROBES.

Build checks:
  non-diagnostic:
    build_win_hz6_capacity_suite.ps1
      -Families larson
      -Hz6Profiles speed
      -CapacityLanes ownerlocalityfast-appcap
      -OutDirName out_win_hz6_capacity_fastprobe
    OK

  diagnostic:
    build_win_hz6_capacity_suite.ps1
      -Families larson
      -Hz6Profiles speed
      -CapacityLanes ownerlocality-appcap
      -OutDirName out_win_hz6_capacity_diag_check
      -DiagnosticHz6Probes
    OK

Smoke:
  ownerlocalityfast-appcap:
    main_1k   = 47.1M..51.5M ops/s
    worker_1k = 53.1M..57.0M ops/s
    main_4k   = 41.5M ops/s
    worker_4k = 44.8M ops/s
    route_invalid = 0
    route_miss = 0
    Done sleeping... printed

Read:
  the owner-locality/shared-directory exact-route mechanism no longer depends
  on diagnostic probes to work.
  The diagnostic lane remains the place to validate hit/rehome counters.
  The fast lane is now the better candidate for throughput/RSS comparison.
```

## Owner Locality Fast Lane Repeat-3 2026-06-02

```text
Repeat-3 median results:
  ownerlocalityfast-appcap main_1k   = 52.783M ops/s
  ownerlocalityfast-appcap worker_1k = 57.274M ops/s
  ownerlocalityfast-appcap main_4k   = 46.329M ops/s
  ownerlocalityfast-appcap worker_4k = 49.091M ops/s

Safety:
  route_invalid = 0
  route_miss = 0
  Done sleeping... printed on every row

Read:
  ownerlocalityfast-appcap is now stable enough to treat as the non-diagnostic
  candidate-control lane for Larson owner-locality checks.
  Keep ownerlocality-appcap as the diagnostic evidence lane for counter
  validation and route-rehome inspection.
  Use the fast lane for the next throughput/RSS comparisons instead of
  re-running the diagnostic lane by default.
```

## Next Implementation Order 2026-06-01

```text
Primary next target:
  OwnerLocalityIndex-L1 in the Larson main-warmup lane

Why:
  source-block-only hints were no-go
  shared directory recovered foreign exact routes
  the remaining pain is the worker-local MISS scan around rehomed exact routes
  owner locality is the narrowest next step that keeps hint/backend separation
  clean

Implementation rule:
  keep the owner-locality lane diagnostic-backed
  keep shared directory as the backend exact path
  keep the ordinary local route path as fallback, not as a replacement for the
  truth source

Evaluation:
  compact control should stay within the existing noise band
  worker-warmup should stay unchanged
  main-warmup should recover if local MISS scans were the dominant blocker
  do not treat the lane as a promotion claim yet

Immediate follow-up:
  run the new ownerlocality-appcap lane with the existing Larson matrix
  repeat a few times and watch route_exact / owner_locality / route_lookup
  totals before deciding whether to deepen the directory or stop at evidence
```

## Diagnostic Checkpoint 2026-05-31

```text
Larson T=16 diagnostic read:
  stress stays near zero throughput
  compact control stays around 60M ops/s

Probe readout:
  route_register_probe_total tracks source_alloc closely
  descriptor_probe_total tracks source_alloc closely
  source_block_probe_total stays 0

Current read:
  route lookup / source_block search are not the main hot spot
  the next attack should be source reuse / admission, not more search knobs
```

## Diagnostic Checkpoint 2026-05-31b

```text
Larson T=16 with reuse probes:
  stress still collapses
  compact control stays healthy

Reuse readout:
  frontcache_reuse_hit is high in compact rows
  frontcache_reuse_invalid stays 0
  transfer_reuse_hit stays 0
  transfer_reuse_invalid stays 0
  route_miss and source_alloc still move together under stress

Current read:
  frontcache reuse is not the missing piece
  transfer reuse is not the missing piece
  the next attack should be stress admission / source refill pressure
```

## Diagnostic Checkpoint 2026-05-31c

```text
Larson T=16 with source refill probes:
  stress still collapses
  compact control stays healthy

Source refill readout:
  source_refill_starvation = 0
  source_refill_saturation = 0
  source_refill_boost = 0
  source_refill_clamp = 0

Current read:
  source refill control is not the limiting factor
  the next attack should be source admission before refill,
  not more batch scaling inside refill itself
```

## Diagnostic Checkpoint 2026-05-31d

```text
Larson T=16 front-prefill read:
  front-prefill is active only on toy
  local2p / midpage / large stay at zero

Prefill readout:
  toy front_source_prefill_attempt/fill is non-zero
  toy front_source_prefill_fallback stays 0
  front_source_prefill_alloc is now visible in the aggregated snapshot

Current read:
  the missing piece is not a general prefill helper
  the next attack should focus on toy/front-specific source placement
  and on why the toy path is the only one reaching source-prefill
```

## Design Checkpoint 2026-05-31e

```text
Pro review:
  Larson min=8 max=1024 is currently a toy-only workload.
  local2p / midpage / large prefill at zero is expected because their fronts
  do not accept this size range.

Important distinction:
  current Larson stress:
    main thread owns the warmup allocations
    workers free and replace them from worker-local allocators
    this is also a cross-owner warmup / route-lifecycle stress

  worker-warmup stress:
    each worker creates its own initial live set before the timer starts
    this isolates same-owner toy/small source placement

Next implementation order:
  Step 0:
    add worker-warmup control to the Larson runner

  Step 1:
    if worker-warmup still collapses, add toy/small source-block prefill
    instead of per-object source-prefill

  Step 2:
    if worker-warmup recovers but current stress still collapses, move next to
    owner-aware remote free / shared route visibility

Do not:
  treat appcap capacity as the sole cause yet
  add more admission clamp before the worker-warmup split
  turn toy into a special route outside the common FrontSource contract
```

## Diagnostic Checkpoint 2026-05-31f

```text
Worker-warmup control smoke:
  command:
    bench_larson_hz6_speed_appcap.exe 2 8 1024 10000 1 12345 16 <mode>

main-warmup mode:
  throughput:
    0.002M ops/s
  route_miss:
    4751
  source_alloc:
    5312

worker-warmup mode:
  throughput:
    11.653M ops/s
  route_miss:
    0
  source_alloc:
    165008

Current read:
  worker-warmup recovers the catastrophic stress collapse by orders of
  magnitude
  current Larson stress is primarily a cross-owner warmup / route-lifecycle
  stress, not just a toy source-prefill placement failure

Next:
  keep current stress as cross-owner stress evidence
  use worker-warmup stress to evaluate toy/small source-block placement
  move owner-aware free / shared route visibility up in priority for the
  current stress lane
```

## Diagnostic Checkpoint 2026-05-31g

```text
Shared route visibility + safety smoke:
  smoke:
    hz6_r1_* all ok

Targeted Larson speed appcap T=16:
  main-warmup 3s:
    throughput:
      4.293K ops/s
    route_miss:
      11431
    source_alloc:
      11968

  worker-warmup 10s:
    throughput:
      13.717M ops/s
    route_miss:
      0
    source_alloc:
      165728

Current read:
  shared route visibility is now proven safe at the smoke level, but it does
  not yet close the main-warmup cross-owner route miss collapse
  worker-warmup remains the strong control lane

Next:
  keep shared route visibility
  keep worker-warmup as control
  continue toward owner-aware free / shared route lifecycle, because the
  main-warmup route miss is still the real stress bottleneck
```

## Next Attack: HZ6-ControlPlane-L1

```text
Goal:
  Reduce stress collapse without hurting compact control.

Core rule:
  before source refill, check reusable layers in this order:
    local cache
    transfer cache
    released pool
    central free
    source refill

Signals are slow-path only:
  source refill:
    refill_count
    batch_size
    mapped_bytes
    committed_bytes

  transfer overflow:
    transfer_push_fail
    transfer_spill
    transfer_bytes

  scavenge checkpoint:
    free_bytes
    released_bytes
    retained_bytes

  epoch:
    worker aggregate allocation pressure
    remote backlog proxy
```

```text
Initial pressure modes:
  NORMAL:
    regular transfer/source caps

  PRESSURE:
    source refill batch down
    transfer cap down
    local cache cap down
    released pool preferred

  SCAVENGE:
    decommit payload aggressively
    source refill only after transfer/released miss

Mode transitions:
  NORMAL -> PRESSURE:
    committed_bytes > class_soft_limit
    or transfer overflow observed
    or source refill too frequent

  PRESSURE -> SCAVENGE:
    committed_bytes > class_hard_limit

  PRESSURE -> NORMAL:
    two epochs below low watermark
```

Acceptance:

```text
Compact control:
  accept if compact local/remote is within -3%
  no-go if compact local/remote drops more than 5%

Larson stress:
  accept if T=16 improves by at least 15%
  strong if T=16 improves by at least 25%
  no-go if alloc failure / fallback increases

RSS:
  no-go if rows that previously matched or beat tcmalloc now exceed tcmalloc RSS
  no-go if retained bytes grow across epochs

Safety:
  no owned-looking invalid pointer may escape to CRT/libc fallback
  INVALID must not become MISS
  double-free must not become reusable
  remote backlog must stay bounded
  transfer overflow must not drop objects

Design:
  no production hot-path probe/atomic counters
  no VirtualQuery or OS query in the hot path
  do not force Windows and Linux into the same backing implementation
```

## Frozen Windows Ledger

```text
Reference summaries:
  results/debug-larson-hz6-appcap-default/20260528_162426_paper_larson_windows.md
  results/debug-redis-hz6-appcap-default/20260528_162416_paper_redis_workload_windows.md

Fixed comparison policy:
  appcap rows are the main HZ6 comparison lane
  default and broad rows are control lanes only
  do not treat control-row failures as production defaults
```

```text
Larson appcap:
  T=1:
    hz6-strict-appcap  11.251M ops/s
    hz6-speed-appcap   10.625M ops/s
    hz6-rss-appcap     11.484M ops/s
  T=16:
    hz6-strict-appcap   0.003M ops/s
    hz6-speed-appcap    0.003M ops/s
    hz6-rss-appcap      0.003M ops/s

Redis appcap:
  SET:
    hz6-strict-appcap  29.34 M ops/s
    hz6-speed-appcap   33.44 M ops/s
    hz6-rss-appcap     33.75 M ops/s
  GET:
    hz6-strict-appcap  84.73 M ops/s
    hz6-speed-appcap  116.36 M ops/s
    hz6-rss-appcap    104.01 M ops/s
  LPUSH:
    hz6-strict-appcap  21.49 M ops/s
    hz6-speed-appcap   26.04 M ops/s
    hz6-rss-appcap     28.72 M ops/s
  LPOP:
    hz6-strict-appcap 117.24 M ops/s
    hz6-speed-appcap  167.28 M ops/s
    hz6-rss-appcap    146.60 M ops/s
  RANDOM:
    hz6-strict-appcap  47.82 M ops/s
    hz6-speed-appcap   65.06 M ops/s
    hz6-rss-appcap     72.09 M ops/s
```

## Next Attack

```text
Primary next target:
  Larson T=16 appcap speed

Why:
  the lane is measurable now, but still far behind the rest of the matrix

Secondary follow-up:
  keep Redis appcap as the fixed comparison lane
  do not widen broad/default rows until the appcap lane is steadier
```

## Current Windows Engineering Position

```text
Status:
  HZ6 Windows comparison is split into fixed appcap rows and control rows.

Control rows:
  hz6-strict
  hz6-speed
  hz6-rss
  hz6-strict-broad
  hz6-speed-broad
  hz6-rss-broad

Main rows:
  hz6-strict-appcap
  hz6-speed-appcap
  hz6-rss-appcap
```

Latest HZ6 standalone Windows benchmark:

```text
results:
  private/allocators/hakozuna/hakozuna-hz6/private/raw-results/windows/
    hz6_benchmark_20260528_124050

local:
  strict 8192   ~60.16M ops/s
  strict 65536  ~68.05M ops/s
  speed  8192   ~47.57M ops/s
  rss    8192   ~52.44M ops/s

remote 131072:
  strict/speed/rss ~57.49M..60.44M ops/s

reuse 131072:
  strict/speed/rss ~58.30M..63.88M ops/s
```

## Diagnostic Checkpoint 2026-05-31d

```text
Larson front-specific source_alloc read:
  front-specific source_alloc counters are now wired into the paper runner
  compact control keeps frontcache reuse healthy
  transfer reuse remains 0 in the observed rows
  source_refill_* remains 0
  source_prefill_* remains 0 in Larson

Current read:
  Larson stress is not asking for more refill scaling
  the new counter split points at source admission / front-specific source alloc
  local2p is the visible source consumer in the current Larson rows
  in the observed stress rows, local2p_source_alloc matches source_alloc and the
  other front buckets stay 0
  next attack should stay on source admission before refill and front routing
```

## Diagnostic Checkpoint 2026-05-31e

```text
Larson T=16 diagnostic rerun after toy-front prefill split:
  source_prefill_attempt = 21735
  source_prefill_filled = 29610
  source_prefill_fallback = 0
  front_source_prefill_alloc = 29610
  frontcache_reuse_hit = 37064
  source_refill_starvation = 525
  source_refill_boost = 525

Interpretation:
  the source prefill path is active in the diagnostic build
  the next bottleneck is not more prefill but how the source/front path is
  registered and attributed under toy stress
  toy_source_prefill_call stayed 0 in the current readout, so the toy wrapper
  is not yet the right witness for the active prefill path
  route_register_probe_total still tracks source_alloc closely
```

## Design Decision 2026-05-31f

```text
Decision:
  do not grow toy as a special route
  keep toy as the minimal reference front for the shared HZ6 front contract

Next design:
  reuse -> transfer/prefill reuse -> source prefill -> direct source
  should be represented as a common front-source flow

Attribution:
  replace toy-specific witness counters with common front_id x alloc_path
  attribution in diagnostic lanes only

Rationale:
  front_source_prefill_alloc and source_prefill_attempt/filled are active
  toy_source_prefill_call is a weak witness because it is tied to one wrapper
  the useful question is which front took which allocation path, not whether a
  toy-specific hook fired

Minimum next implementation:
  add Hz6AllocPath enum
  add common diagnostic path counters keyed by front_id and path
  make common source acquire/prefill helpers own path attribution
  keep speed lanes free of diagnostic atomics/counters
```

## Implementation Checkpoint 2026-05-31g

```text
Status:
  front_id x alloc_path diagnostic counters are now in the tree
  attribution is owned by the common front/source helper path
  source-prefill, direct-source, and reuse notes are being recorded in
  diagnostic lanes only

Next:
  expose the new path counters in the Windows Larson diagnostic output
  then use the common attribution to decide whether toy/front routing or
  source admission is the next pressure point
```

## Diagnostic Checkpoint 2026-05-31h

```text
Larson diagnostic runner with front/path columns:
  compact control rows now surface [HZ6_PATH] lines

Observed compact profile:
  front=toy is the active attributed front in the appcap rows
  local_reuse dominates
  prefill_reuse and source_prefill are present
  direct_source stays 0

Observed stress profile:
  longer timeout captured stress rows
  throughput still collapses under stress
  front=toy remains the active attributed front
  local_reuse, prefill_reuse, and source_prefill are all visible
  direct_source still stays 0
  next step is to decide whether source admission or front routing should be
  tightened first under stress
```

## Diagnostic Checkpoint 2026-05-31i

```text
Larson admission mode columns:
  source_admission_open / boosted / clamped are now surfaced

Observed stress profile:
  source_admission_open dominates
  source_admission_boosted is small
  source_admission_clamped stays 0
  source_admission is therefore not obviously the main blocker

Observed front/path profile:
  front=toy remains the active attributed front
  local_reuse still dominates
  prefill_reuse and source_prefill stay visible
  direct_source stays 0

Next read:
  stress collapse may now be more about front-source reuse placement
  or another downstream pressure point than admission gating alone
```

## Diagnostic Checkpoint 2026-05-31j

```text
Larson shared visibility and transfer backlog columns:
  route_visibility_lookup / hit / miss / probe_total / probe_max
  transfer_current / transfer_current_max

Observed main-warmup profile:
  route_miss = 0
  route_visibility_lookup = 3934
  route_visibility_hit = 3934
  route_visibility_probe_max = 1
  transfer_push = 3803
  transfer_pop = 3610
  transfer_current = 193
  transfer_current_max = 25
  remote_free_attempt = 3803
  remote_free_transfer_fail = 0
  source_admission_open = 3943
  source_admission_clamped = 0
  source_refill_saturation = 0
  source_refill_clamp = 0
  source_prefill_attempt = 864
  source_prefill_filled = 864
  front=toy remains the active attributed front

Observed worker-warmup profile:
  route_miss = 0
  route_visibility_lookup = 0
  route_visibility_probe_total = 0
  transfer_push = 0
  transfer_pop = 0
  transfer_current = 0
  remote_free_attempt = 0
  source_admission_open = 337416007
  source_admission_clamped = 0
  source_refill_saturation = 0
  source_refill_clamp = 0
  source_prefill_attempt = 166016
  source_prefill_filled = 166016

Read:
  shared route visibility is working and is shallow (one probe max)
  the main-warmup collapse is not explained by visibility depth alone
  transfer backlog stays bounded but nonzero in main-warmup
  remote free attempts match transfer pushes in main-warmup
  transfer failures stay zero
  source admission clamp is not the blocker in the current build
  next pressure point is cross-owner transfer / handoff policy, not
  visibility scanning
```

## Design Checkpoint 2026-05-31k

```text
Selected next lane:
  RemoteHandoff-L1
  owner-aware transfer / remote handoff lifecycle

Lane split:
  Larson worker-warmup:
    same-owner lifecycle control
    use this to verify toy/small hot path, frontcache, and source prefill
    without cross-owner route visibility or remote_free

  Larson main-warmup:
    cross-owner handoff stress
    use this to evaluate shared route visibility, remote_free, transfer
    handoff, and ownership lifecycle

Decision:
  do not deepen shared route visibility as the next optimization
  do not add source admission clamps as the next optimization
  keep toy/small source-block placement for a later isolated lane

Why:
  route_visibility_probe_max is already 1
  route_visibility_lookup hits cleanly
  remote_free_attempt tracks transfer_push in main-warmup
  remote_free_transfer_fail stays 0
  worker-warmup keeps high throughput with visibility/transfer/remote_free all
  at 0
```

```text
Minimum next counters:
  route_visibility_hit_local_owner
  route_visibility_hit_foreign_owner
  route_rehome_attempt
  route_rehome_success
  route_rehome_fail

RemoteHandoff-L1 experiment:
  visible foreign route hit records the origin allocator
  remote free or transfer pop attempts route locality rehome into the consumer
  allocator
  successful rehome should reduce future visibility lookups for the same
  worker-owned objects

Acceptance:
  weak accept:
    main-warmup throughput improves at least 10x
    worker-warmup stays within -3%
    route_rehome_fail = 0
    remote_free_transfer_fail = 0

  strong accept:
    main-warmup reaches at least 1M ops/s
    route visibility hits fall after rehome
    transfer_current remains bounded
    worker-warmup remains around the current 30M ops/s class

No-go:
  route_miss or route_visibility_miss appears
  route_invalid increases
  route_rehome_fail is nonzero
  transfer_current grows with runtime/chunks
  worker-warmup or compact control regresses by more than 5%
  rehome failure escapes as MISS/fallback
```

## Diagnostic Checkpoint 2026-05-31l

```text
RemoteHandoff-L1 owner split:
  main-warmup:
    throughput = 1630 ops/s
    route_visibility_lookup = 4968
    route_visibility_hit = 4968
    route_visibility_hit_local_owner = 207
    route_visibility_hit_foreign_owner = 4761
    route_visibility_probe_max = 1
    remote_free_attempt = 4761
    route_rehome_attempt = 4761
    route_rehome_success = 4761
    route_rehome_fail = 0
    transfer_push = 4761
    transfer_pop = 4546
    transfer_current = 215

  worker-warmup:
    throughput = 11.049M ops/s
    route_visibility_lookup = 0
    route_visibility_hit = 0
    route_visibility_hit_local_owner = 0
    route_visibility_hit_foreign_owner = 0
    route_rehome_attempt = 0
    route_rehome_success = 0
    route_rehome_fail = 0
    transfer_push = 0
    transfer_pop = 0
    transfer_current = 0

Current read:
  the new owner split is visible and safe
  foreign visibility dominates main-warmup
  same-owner worker-warmup remains the healthy control lane
  next follow-up should be route/transfer rehome locality, not deeper scan depth
```

## Diagnostic Checkpoint 2026-06-01

```text
RemoteHandoff-L1 route rehome enabled:
  main-warmup:
    throughput = 2073 ops/s
    route_visibility_lookup = 5899
    route_visibility_hit = 5899
    route_visibility_hit_local_owner = 0
    route_visibility_hit_foreign_owner = 5899
    route_visibility_probe_max = 1
    remote_free_attempt = 5899
    route_rehome_attempt = 5899
    route_rehome_success = 5899
    route_rehome_fail = 0
    transfer_push = 5899
    transfer_pop = 5718
    transfer_current = 181

  worker-warmup:
    throughput = 12.426M ops/s
    route_visibility_lookup = 0
    route_visibility_hit = 0
    route_visibility_hit_local_owner = 0
    route_visibility_hit_foreign_owner = 0
    route_rehome_attempt = 0
    route_rehome_success = 0
    route_rehome_fail = 0
    transfer_push = 0
    transfer_pop = 0
    transfer_current = 0

Current read:
  rehome is now an actual route locality move, not only a counter
  foreign visibility is still the dominant main-warmup shape
  worker-warmup remains the healthy same-owner control lane
  next question is whether route locality rehome is enough, or whether
  descriptor/source ownership L2 is still needed
```

## Diagnostic Checkpoint 2026-06-01b

```text
RemoteHandoff-L1 repeat-3:
  main-warmup:
    throughput = 1323 / 1570 / 1600 ops/s
    route_visibility_lookup = 3911 / 4596 / 4694
    route_visibility_hit_local_owner = 0 / 0 / 0
    route_visibility_hit_foreign_owner = 3911 / 4596 / 4694
    route_rehome_attempt = 3911 / 4596 / 4694
    route_rehome_success = 3911 / 4596 / 4694
    route_rehome_fail = 0 / 0 / 0
    transfer_current = 210 / 233 / 237

  worker-warmup:
    throughput = 10.975M / 29.478M / 29.283M ops/s
    route_visibility_lookup = 0 / 0 / 0
    route_visibility_hit = 0 / 0 / 0
    route_visibility_hit_local_owner = 0 / 0 / 0
    route_visibility_hit_foreign_owner = 0 / 0 / 0
    route_rehome_attempt = 0 / 0 / 0
    route_rehome_success = 0 / 0 / 0
    route_rehome_fail = 0 / 0 / 0
    transfer_current = 0 / 0 / 0

Current read:
  route rehome is stable and safe, but it does not close the main-warmup
  collapse
  foreign visibility still dominates main-warmup across repeats
  worker-warmup remains the healthy same-owner control lane
  next step should be descriptor/source ownership L2, not deeper visibility
  scan depth
```

## Diagnostic Checkpoint 2026-06-01c

```text
Descriptor/source ownership L2 diagnostics:
  main-warmup:
    throughput = 997 ops/s
    source_owned_prepare = 848
    source_owned_route_hit_local_owner = 0
    source_owned_visibility_hit_local_owner = 0
    source_owned_visibility_hit_foreign_owner = 2973
    source_owned_remote_free_attempt = 2973
    source_owned_release = 0
    route_rehome_success = 2973
    transfer_current = 196

  worker-warmup:
    throughput = 14.987M ops/s
    source_owned_prepare = 165392
    source_owned_route_hit_local_owner = 0
    source_owned_visibility_hit_local_owner = 0
    source_owned_visibility_hit_foreign_owner = 0
    source_owned_remote_free_attempt = 0
    source_owned_release = 0
    route_rehome_success = 0
    transfer_current = 0

Current read:
  the new source-owned counters are diagnostic-only and stay out of the
  production benchmark lane
  main-warmup is still foreign-visibility dominated, and the source-owned
  descriptors are the ones taking the remote handoff path
  worker-warmup stays healthy with source-owned allocations but no visibility
  or remote handoff pressure
  next question is whether descriptor/source ownership needs a behavior L2,
  or whether this should now be frozen as evidence
```

## Diagnostic Checkpoint 2026-06-01d

```text
Descriptor/source ownership L2 repeat:
  main-warmup:
    throughput = 1187 ops/s
    source_owned_prepare = 848
    source_owned_route_hit_local_owner = 109
    source_owned_visibility_hit_local_owner = 0
    source_owned_visibility_hit_foreign_owner = 3495
    source_owned_remote_free_attempt = 3495
    route_rehome_success = 3495
    transfer_current = 206

  worker-warmup:
    throughput = 13.455M ops/s
    source_owned_prepare = 165296
    source_owned_route_hit_local_owner = 48098278
    source_owned_visibility_hit_local_owner = 0
    source_owned_visibility_hit_foreign_owner = 0
    source_owned_remote_free_attempt = 0
    route_rehome_success = 0
    transfer_current = 0

Current read:
  the direct local source-owned route hit is now visible in the worker lane
  the main-warmup shape is still foreign-visibility dominated
  this is enough evidence for descriptor/source ownership L2, without any
  production atomic counter path
  next decision is whether to freeze this as evidence or turn it into a
  behavior experiment
```

## Diagnostic Checkpoint 2026-06-01e

```text
Larson lifecycle/frontcache class diagnostic smoke:
  build:
    build_win_larson_suite.ps1 -DiagnosticHz6Probes

  command:
    bench_larson_hz6_speed_appcap.exe 2 8 1024 10000 1 12345 16 <mode>

  added diagnostic-only counters:
    lifecycle_owner_mismatch
    lifecycle_foreign_free_attempt
    lifecycle_foreign_free_handled
    lifecycle_foreign_free_invalid
    frontcache_push_by_class[16]
    frontcache_pop_empty_by_class[16]

main-warmup:
  throughput = 1249 ops/s
  route_visibility_lookup = 2499
  route_visibility_hit_foreign_owner = 2499
  route_visibility_probe_max = 1
  remote_free_attempt = 2499
  route_rehome_attempt = 2499
  route_rehome_success = 2499
  route_rehome_fail = 0
  lifecycle_owner_mismatch = 2499
  lifecycle_foreign_free_attempt = 2499
  lifecycle_foreign_free_handled = 2499
  lifecycle_foreign_free_invalid = 0
  transfer_push = 2499
  transfer_pop = 2302
  transfer_current = 197
  route_lookup_probe_total = 2620397339
  route_lookup_probe_max = 524290

worker-warmup:
  throughput = 44.044M ops/s
  route_visibility_lookup = 0
  remote_free_attempt = 0
  route_rehome_attempt = 0
  lifecycle_owner_mismatch = 0
  lifecycle_foreign_free_attempt = 0
  lifecycle_foreign_free_handled = 0
  lifecycle_foreign_free_invalid = 0
  transfer_push = 0
  transfer_pop = 0
  transfer_current = 0
  route_lookup_probe_total = 99234056
  route_lookup_probe_max = 5

Current read:
  foreign handoff itself is not failing:
    handled == attempt
    route_rehome_success == route_rehome_attempt
    route_rehome_fail == 0
    lifecycle_foreign_free_invalid == 0

  route visibility remains shallow:
    route_visibility_probe_max == 1

  the new main-warmup bottleneck signal is the worker-local route MISS before
  visibility:
    local route lookup probes explode to 2.62B with max 524290

  worker-warmup stays healthy and keeps lifecycle/visibility/transfer at zero.

Next:
  do not add more remote handoff capacity yet
  next high-ROI experiment should avoid the expensive local-route miss before
  shared visibility on cross-owner warmup pointers
  candidates:
    route negative filter / owned-range hint
    visible-first diagnostic lane for non-strict cross-owner stress
    shared route directory keyed by source block or route envelope
```

## Design Checkpoint 2026-06-01f

```text
Pro review:
  do L0 before VisibleFirstFree-L1.

L0:
  VisibleAfterLocalMiss refactor
  Existing hz6_allocator_route_lookup_visible() includes a local lookup first.
  hz6_free() already performed a local lookup before calling it, so
  main-warmup was doing:
    worker-local route MISS
    visible helper worker-local route MISS again
    shared visibility hit

Implementation:
  add hz6_allocator_route_lookup_visible_only()
  add hz6_allocator_route_lookup_visible_after_local_miss()
  keep hz6_allocator_route_lookup_visible() as the compatibility helper that
  still does local lookup first
  change hz6_free() and hz6_free_remote() to call visible_after_local_miss()
  after a local MISS

Expected:
  semantics unchanged
  local INVALID remains local INVALID
  visible-only MISS is not treated as final MISS without local fallback
  route_lookup_probe_total should drop because the duplicate local MISS is gone

Next after L0:
  A. VisibleFirstFree-L1 diagnostic:
     in non-strict diagnostic lane, try visible-only before local route lookup
     to measure the upper bound of avoiding local MISS scans

  B. NegativeFilter-L1:
     production-near conservative filter with DEFINITELY_NOT_LOCAL /
     MAYBE_LOCAL and shadow verification

  C. SharedRouteDirectory-L1:
     long-term design if B cannot recover enough of A
```

## Diagnostic Checkpoint 2026-06-01g

```text
L0 VisibleAfterLocalMiss refactor:
  build:
    build_win_larson_suite.ps1 -DiagnosticHz6Probes

  main-warmup smoke:
    bench_larson_hz6_speed_appcap.exe 2 8 1024 10000 1 12345 16 0
    throughput = 4651 ops/s
    route_invalid = 0
    route_miss = 0
    route_visibility_probe_max = 1
    lifecycle_foreign_free_invalid = 0
    route_rehome_fail = 0
    route_lookup_probe_total = 4656736489
    route_lookup_probe_max = 524290

  worker-warmup smoke:
    bench_larson_hz6_speed_appcap.exe 2 8 1024 10000 1 12345 16 1
    throughput = 42.430M ops/s
    lifecycle/visibility/transfer counters stay 0
    route_lookup_probe_total = 94911023
    route_lookup_probe_max = 7

Read:
  L0 is safe and removes the known duplicate local lookup path, but main-warmup
  remains dominated by expensive local route MISS scans.
  Per operation, main-warmup route lookup probes are roughly halved versus the
  prior smoke, but the absolute local MISS scan remains the bottleneck.
```

## Diagnostic Checkpoint 2026-06-01h

```text
VisibleFirstFree-L1 diagnostic:
  lane:
    visiblefirst-appcap
  build:
    build_win_hz6_capacity_suite.ps1 -Families larson -Hz6Profiles speed
      -CapacityLanes appcap,visiblefirst-appcap -DiagnosticHz6Probes
      -OutDirName out_win_hz6_visiblefirst_diag

Safety adjustment:
  visible-first accepts only visible VALID as an early hit.
  visible INVALID falls back to local lookup first; if local also MISSes,
  the visible INVALID can still be returned.

Small live-set smoke:
  bench_larson_hz6_speed_visiblefirst_appcap.exe 1 8 1024 1000 1 12345 16 0
    throughput = 6862 ops/s
    route_invalid = 0
    route_miss = 0
    visible_first_attempt = 7102
    visible_first_hit = 4677
    visible_first_visible_invalid = 2250
    visible_first_local_fallback = 2425
    visible_first_local_lookup_skipped = 4677
    route_lookup_probe_total = 3481
    route_lookup_probe_max = 2

Full appcap live-set smoke:
  bench_larson_hz6_speed_visiblefirst_appcap.exe 1 8 1024 10000 1 12345 16 0
    exit = -1073741819

Read:
  visible-first proves the upper-bound hypothesis on a smaller live set:
    local route lookup probes collapse when visible VALID is accepted first.

  However, full appcap main-warmup is not safe as a behavior lane.
  The diagnostic also reveals frequent visible INVALID before local fallback,
  so visible-first can observe foreign invalid envelopes ahead of the local
  exact route.

Decision:
  keep visiblefirst-appcap as no-go / upper-bound diagnostic evidence.
  Do not promote visible-first behavior.

Next:
  move to B NegativeFilter-L1:
    skip local lookup only when a conservative local owned-range hint says
    DEFINITELY_NOT_LOCAL
    use diagnostic shadow verification before treating it as production-near
```

## Diagnostic Checkpoint 2026-06-01i

```text
NegativeFilter-L1:
  lane:
    negativefilter-appcap
  build:
    appcap with /DHZ6_NEGATIVE_FILTER_L1=1 and diagnostic probes enabled

Shape:
  use a conservative local owned-range hint
  only skip local lookup when the pointer is definitely not inside any active
  local source block
  shadow verify skipped cases in diagnostic builds

Expected counters:
  negative_filter_attempt
  negative_filter_skip_local
  negative_filter_maybe_local
  negative_filter_shadow_false_skip
  negative_filter_shadow_local_valid
  negative_filter_shadow_local_invalid

Goal:
  reduce the worker-local route MISS scan without reintroducing the visible-
  first crash behavior
  keep route_invalid / route_miss at zero
  keep worker-warmup unchanged
```

## Diagnostic Checkpoint 2026-06-01j

```text
NegativeFilter-L1 smoke:
  bench_larson_hz6_speed_negativefilter_appcap.exe 2 8 1024 10000 1 12345 16 0

Read:
  throughput = 32403 ops/s
  route_invalid = 0
  route_miss = 0
  negative_filter_attempt = 66054
  negative_filter_skip_local = 60954
  negative_filter_maybe_local = 5100
  negative_filter_shadow_false_skip = 0
  route_lookup_probe_total = 23303
  route_lookup_probe_max = 3

Interpretation:
  the conservative filter is now visible in diagnostics.
  it skips local lookup on the majority of free attempts and has no shadow
  false-skip so far.
  this is still diagnostic evidence, not promotion.
```

## Diagnostic Checkpoint 2026-06-01k

```text
Pro design read:
  do not jump to SharedRouteDirectory-L1 yet.
  move NegativeFilter-L1 into a validation phase first.

Current decision:
  negativefilter-appcap is promoted from smoke-only evidence to validation
  candidate, not to production/default.

Validation order:
  1. main-warmup repeat-5
  2. worker-warmup repeat-5
  3. compact control repeat-3
  4. full appcap T=16 check

New diagnostic counters:
  negative_filter_range_probe_total
  negative_filter_range_probe_max

Why:
  route_lookup_probe_total collapsed, but the source-block range scan inside
  the negative filter could become the next cost.

Production-near rule:
  HZ6_NEGATIVE_FILTER_L1 should eventually control behavior.
  HZ6_DIAGNOSTIC_PROBES should control counters and shadow verification only.
  Before that split, false-skip must stay zero across guards.

Coverage rule:
  source-block-only hints are valid for this Larson small/source-block
  diagnostic lane.
  do not generalize to every HZ6 free path until unindexed local exact,
  local2p, large, and direct source ranges are covered or force MAYBE_LOCAL.
```

## Diagnostic Checkpoint 2026-06-01l

```text
NegativeFilter-L1 validation:
  added negative_filter_range_probe_total / max to see the hidden cost of the
  source-block range scan.
  added an armed gate so same-owner worker-warmup does not scan source blocks
  before any foreign visibility hit.

Smoke before rehome safety block:
  main-warmup negativefilter:
    throughput = 28735 ops/s
    route_invalid = 13504
    negative_filter_skip_local = 55141
    negative_filter_range_probe_max = 32768
  worker-warmup negativefilter:
    throughput = 37.20M ops/s after arming gate
    negative_filter_not_armed = all attempts
    negative_filter_range_probe_total = 0

Safety finding:
  source-block-only negative filtering is not compatible with route rehome.
  After rehome, exact routes can live in the consumer allocator while the
  physical SourceBlock envelope remains in the origin allocator. A filter that
  only checks local source block ranges can incorrectly skip local exact routes
  and hit the origin invalid envelope instead.

Rehome safety block:
  negative_filter_rehome_blocked disables local-skip once route_rehome_success
  is nonzero for that allocator.

Smoke after rehome safety block:
  main-warmup negativefilter:
    throughput = 4613 ops/s
    route_invalid = 0
    route_miss = 0
    negative_filter_not_armed = 16
    negative_filter_rehome_blocked = 9346
    negative_filter_skip_local = 0
  worker-warmup negativefilter:
    throughput = 38.55M ops/s
    route_invalid = 0
    route_miss = 0
    negative_filter_not_armed = all attempts
    negative_filter_range_probe_total = 0

Baseline appcap smoke:
  main-warmup appcap = 5426 ops/s
  worker-warmup appcap = 39.24M ops/s

Decision:
  NegativeFilter-L1 is no-go as an optimization lane.
  Keep it as control evidence:
    local MISS scan is real,
    same-owner arming is necessary,
    source-block-only hints are insufficient after rehome.

Next:
  do not deepen NegativeFilter knobs.
  move to a route-locality design that accounts for rehomed exact routes:
    owner-range hint with rehome coverage, or SharedRouteDirectory-L1.
```

## Diagnostic Checkpoint 2026-06-01m

```text
SharedRouteDirectory-L1:
  added sharedir-appcap as dry-run:
    exact route register publishes ptr -> allocator/descriptor
    exact route unregister removes it
    free probes the directory only after local MISS
    behavior unchanged

Dry-run stress main-warmup:
  throughput = 2296 ops/s
  route_invalid = 0
  route_miss = 0
  shared_dir_lookup = 4534
  shared_dir_hit = 4534
  shared_dir_hit_foreign_allocator = 4534
  shared_dir_would_skip_local = 4534
  shared_dir_probe_total = 8235
  shared_dir_probe_max = 30

Dry-run worker-warmup:
  throughput = 37.91M ops/s
  shared_dir_lookup = 0
  route_invalid = 0
  route_miss = 0

Read:
  shared directory direction is confirmed:
    every post-local-MISS main-warmup lookup is a foreign exact-route hit.
  worker-warmup is not polluted because dry-run happens only after local MISS.

SharedRouteDirectoryFirst-L1:
  added sharedirfirst-appcap as behavior evidence:
    after first foreign visibility hit, try shared directory before local route
    lookup
    only foreign allocator hits skip local lookup

Compact / moderate main-warmup control:
  command:
    bench_larson_hz6_speed_sharedirfirst_appcap.exe 1 8 1024 1000 1 12345 16 0
  throughput = 49.98M ops/s
  route_invalid = 0
  route_miss = 0
  route_visibility_lookup = 16
  shared_dir_first_attempt = 51462711
  shared_dir_first_hit = 51462711
  shared_dir_first_fallback = 0

Stress main-warmup:
  command:
    bench_larson_hz6_speed_sharedirfirst_appcap.exe 2 8 1024 10000 1 12345 16 0
  result:
    timeout > 120s

Decision:
  sharedir-appcap is KEEP as dry-run direction evidence.
  sharedirfirst-appcap is promising on compact/moderate main-warmup but no-go
  as a broad stress behavior until large live-set scaling is explained.
  The missing piece is not shared directory existence; it is a scalable exact
  route directory / owner-locality design that keeps the 10k-chunk live-set
  from collapsing.

Next:
  avoid more visible/negative-filter knobs.
  design OwnerLocalityIndex-L1:
    cheap local exact ownership hint for rehomed routes
    or a shared directory lookup mode that can distinguish foreign-only before
    expensive fallback
```

## Diagnostic Checkpoint 2026-06-01n

```text
Route invalid-range register fix:
  hz6_route_register_invalid_range previously did a full-table duplicate scan
  before insertion.
  appcap route capacity is 262144, so every source-block invalid envelope could
  cost ~262k probes during main-warmup.

Change:
  use the same hash/probe path as exact registration.
  track tombstones in the same pass.

Smoke:
  bench_larson_hz6_speed_appcap.exe 1 8 1024 10000 1 12345 16 0

Result:
  command completes quickly again.
  route_invalid = 0
  route_miss = 0
  route_register_probe_total = 3593
  route_register_probe_max = 2

Remaining blocker:
  route_lookup_probe_total is still huge:
    1401949714
  This is the worker-local MISS scan on foreign/rehome objects, not route
  register anymore.

Decision:
  keep the invalid-range register fix as a safe route-table improvement.
  continue route-locality work separately:
    shared directory / owner-locality index must eliminate local MISS scans
    without making cleanup or same-owner free unsafe.
```

Read:

```text
The standalone runner is a stable contract baseline for local/remote/reuse.
Remote/reuse are usable, but still profile-sensitive.

Treat the legacy mt_remote runner as contract-mismatched until the HZ6 owner-
aware remote runner exists.
The mt_remote paper runner is now timeout-bounded and skips HZ6 rows by
default. Use `-IncludeHz6Legacy` only to debug the mismatch; HZ6 remote verdicts
should come from the HZ6 standalone local/remote/reuse runner or the owner-
aware HZ6 mt-remote runner.
```

## Repo Hygiene

```text
Keep:
  docs/ZENN_HZ5_SIDECAR_ALLOCATOR_DRAFT.md
  one canonical benchmark summary per active lane

Archive:
  dated docs/benchmarks/windows/**/*.md files that are no longer the current
  representative summary

Ignore for now:
  results/ raw outputs
  regenerated logs / TSVs / scratch artifacts

Modified-but-noisy files:
  hakozuna-hz6/api/hz6_allocator_init_state_source_blocks.c
  hakozuna-hz6/api/hz6_allocator_types.h
  hakozuna-hz6/policy/hz6_profiles_config.c
  these are currently line-ending noise only and should stay untouched
```

Interpretation:

```text
Default rows keep the small R1 capacities and are useful as controls.
If broad matrix raw output shows high alloc_fail, do not interpret that row as
a fair throughput result.

Broad rows keep the same HZ6 policy profiles but raise descriptor, route,
transfer, source-block, and front-cache capacities for working-set matrix
profiles. They are fixed-capacity benchmark lanes, not yet production defaults.
```

Latest Windows large-slice read after exact-route hash probing:

```text
8K:
  hz6-strict-broad ~54.7M ops/s
  tcmalloc         ~57.1M ops/s

16K:
  hz6-strict-broad ~51.4M ops/s
  tcmalloc         ~46.7M ops/s

32K:
  hz6-strict-broad ~58.7M ops/s
  tcmalloc         ~48.6M ops/s

64K:
  hz6-strict-broad ~58.1M ops/s
  tcmalloc         ~50.2M ops/s

128K:
  hz6-speed-broad  ~69.6M ops/s
  tcmalloc         ~46.7M ops/s
```

Current decision:

```text
Keep:
  fixed appcap rows as the frozen HZ6 comparison lane
  small-cap and broad-cap rows as controls only
  exact-route hash probing with fail-closed scan fallback

Do not yet:
  promote broad/default rows to production defaults
  remove control rows
  widen the lane set before Larson T=16 appcap is cleaned up

Next possible research:
  keep appcap rows as Larson capacity-separation evidence
  compare the HZ6 standalone runner against HZ3/HZ4/HZ5 on the same machine
  then build an HZ6 owner/remote-aware remote runner
  then inspect Larson lifecycle/front-source behavior
  then consider hz6-adaptive-cap-dryrun
```

Capacity-breakdown checkpoint:

```text
Windows matrix:
  results/windows-hz6-capacity-breakdown/20260528_105215_allocator_matrix.md

8K default HZ6:
  route_register_fail dominates allocation failure.

64K default HZ6:
  descriptor_exhausted dominates allocation failure.

Broad-cap HZ6:
  alloc_fail, descriptor_exhausted, route_register_fail, and
  source_block_exhausted are all zero in the checked 8K/64K slices.
```

## Current Claim

```text
HZ6 should be a route-safe, transfer-first, RSS-aware allocator family.

It should promote:
  HZ3 thin local cache
  HZ4 remote grouping
  HZ5 fail-closed descriptor ownership and low-RSS profile discipline

It should not directly port HZ3/HZ4/HZ5 internals.
```

## Windows Build Seed

```text
Windows HZ6 build entrypoint:
  win/build_win_hz6_r1_smokes.ps1
  win/build_win_hz6_benchmark.ps1
  win/run_win_hz6_benchmark.ps1

Current role:
  build-environment seed for the R1 smoke suite
  Windows HZ6-only benchmark wiring seed
  Windows VirtualAlloc source backing via source/win_source_virtualalloc.*
  verified locally with clang-cl by building and running the R1 smoke suite
  benchmark runner mirrors the Linux local / remote / reuse HZ6-only matrix
  no cross-family benchmark claim yet
```

## Benchmark Status

```text
HZ6 has had its first benchmark run.
Current evidence is HZ6-only and provisional.
Do not treat R1 modularization as a performance result.
Benchmark harnesses are now in place, but no cross-family benchmark table has
been published yet.
Benchmark entrypoints:
  linux/build_hz6_benchmark.sh
  linux/run_hz6_benchmark.sh
  win/build_win_hz6_benchmark.ps1
  win/run_win_hz6_benchmark.ps1
  local / remote / reuse single-process lanes
Windows note:
  win/run_win_hz6_benchmark.ps1 writes a Linux-compatible results.tsv shape
  but fills ru_maxrss_kb from Windows PeakWorkingSet64 / 1024
Current snapshot:
  docs/HZ6_R1_BENCHMARK_20260528.md
Broad trend snapshot:
  docs/HZ6_R1_BROAD_TRENDS_20260528.md
The next benchmark pass should compare HZ6 against HZ3 / HZ4 / HZ5 on the same
machine with the same runner.
```

Current broad R1 read:

```text
strict:
  strongest local-only profile in the HZ6-only runner

rss:
  strongest remote/reuse profile across 1024..131072 in the HZ6-only runner

speed / remote:
  still profile intents, not validated winners

128K:
  CentralSpanPool is the cleanest current signal, with local/remote/reuse
  around 39M ops/s in the best profile and full reuse hits in the reuse lane
```

## Windows Comparison Read

```text
recent Windows matrix:
  smoke:
    hz6 is behind hz3 / mimalloc / tcmalloc, but still in the viable range

  balanced:
    hz6-strict and hz6-rss are stronger than hz4 and tcmalloc

  wide_ws:
    hz6-strict is stronger than hz4 / mimalloc / tcmalloc
    but still below hz3

  larger_sizes:
    hz6 is the clear gap
    this is the next concrete hole to attack
```

Working read:

```text
HZ6 already has a credible mid-range shape on Windows.
The current weakness is large sizes, not the balanced or wide working-set rows.
```

## Large128 Counter Read

```text
latest Linux large128 counter run at 131072:
  local:
    strict/rss around 40M ops/s
    source_alloc=1
    large_span_source_alloc=1
    no central push/pop
    no transfer push/pop

  remote:
    about 34M-35M ops/s
    source_alloc=1
    large_span_source_alloc=1
    central_push=200000
    central_pop=199999
    no transfer push/pop

  reuse:
    about 34M-38M ops/s
    reuse_hits=100000
    source_alloc=1
    large_span_source_alloc=1
    central_push=100000
    central_pop=100000
    no transfer push/pop
```

Working read:

```text
The current 128K path is not spending time in transfer dispatch.
It is using the central large-span pool as intended, and the remaining work
for "larger_sizes" is more likely outside the 128K central reuse seed.
```

## Current Implementation Step

```text
HZ6 LargeSpan L1 is now being implemented as a modular CentralSpanPool.

Current scope:
  128K ownerless CENTRAL_FREE reuse
  local frontcache reuse
  single-source fallback when the central pool is empty

Not yet in scope:
  256K / 512K / 1M LargeSpan family
  policy-layer adaptation
  cross-family benchmark tables
```

## Active Documents

```text
hakozuna-hz6/README.md
hakozuna-hz6/READMEjp.md
hakozuna-hz6/docs/HZ6_WINDOWS_BUILD.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_BENCHMARK_20260528.md
hakozuna-hz6/docs/HZ6_R1_BROAD_TRENDS_20260528.md
hakozuna-hz6/docs/HZ6_R1_STATUS.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
```

## Proposed First Engineering Steps

```text
1. Review and tighten HZ6 folder layout. DONE.
2. Review HZ6_BLUEPRINT.md and select the first prototype. DONE.
3. Add headers/contracts first. DONE:
   include/hz6_contract.h
   include/hz6.h
   api/hz6_allocator.h
   route/hz6_route.h
   transfer/hz6_transfer.h
   source/hz6_source.h
4. Add route smoke before allocator behavior. DONE.
5. Add transfer smoke before full malloc/free integration. DONE.
6. Add first toy allocation path. DONE:
   api/hz6_allocator.c
   source/hz6_source_system_ops.c
   source/hz6_source_system_memory.c
   frontcache/hz6_frontcache.c
   route/hz6_route.c
7. Add first transfer-first toy path. DONE:
   hz6_free_remote()
   ACTIVE -> TRANSFER_FREE
   transfer pop before source allocation
8. Choose first real target. DONE:
   Linux LargeFront 128K transfer seed was selected first.
9. Keep modular cleanup moving before real target:
   size-class mapping moved to frontcache/hz6_size_class.*
   toy allocation/free transitions moved to fronts/toy/
   allocator API calls toy front through Hz6FrontOps
   front selection moved to fronts/hz6_front.*
10. First real-front seed. DONE:
   fronts/large/hz6_large128_front.*
   handles >4KiB..128KiB through the front registry
   uses transfer-first reuse in HZ6_PROFILE_REMOTE
11. Front utility common path. DONE:
   fronts/hz6_front_util.*
   removes duplicated descriptor/cache/transfer transitions from toy/large
12. Front range correctness. DONE:
   toy front limited to <=4KiB
   Large128 front handles >4KiB..128KiB
   unsupported >128KiB allocation returns NULL in R1 smoke
13. Build script hygiene. DONE:
   linux/build_hz6_r1_smokes.sh uses source/include arrays
   new modules should be added to HZ6_SOURCES/HZ6_INCLUDES explicitly
14. Smoke split. DONE:
   tests/hz6_r1_core_contract_smoke.c now covers owner/profile/frontcache
   after later route/transfer/source smoke splits
   tests/hz6_r1_allocator_smoke.c covers allocator/front integration
15. Linux SourceLayer seed. DONE:
   source/linux_source_mmap.*
   reserve/commit/decommit/release covered by contract smoke
16. Large128 SourceLayer backing. DONE:
   descriptors carry source kind / release metadata
   Large128 uses Linux mmap source ops in the Linux R1 smoke
   destroy and cache overflow release through descriptor SourceLayer metadata
17. Allocator-level SourceRegistry. DONE:
   source/hz6_source_registry.*
   allocator owns system/Linux source selection
   fronts name source kind instead of including OS-specific source backends
18. Local2P exact seed front. DONE:
   fronts/local2p/hz6_local2p_front_alloc.c /
   fronts/local2p/hz6_local2p_front_free.c /
   fronts/local2p/hz6_local2p_front_prefill.c /
   fronts/local2p/hz6_local2p_front_ops.c
   exact 64KiB requests route to HZ6_FRONT_LOCAL2P before Large128
   remote transfer reuse is covered by allocator smoke
19. Route backend seam. DONE:
   route/hz6_route_backend.*
   allocator and front util use backend wrapper instead of direct table calls
   exact table remains the only backend, covered by contract smoke
20. Transfer backend seam. DONE:
   transfer/hz6_transfer_backend.*
   allocator and front util use backend wrapper instead of direct cache calls
   single-cache remains the only backend, covered by contract smoke
21. Sharded transfer backend seed. DONE:
   transfer backend can split one object buffer into fixed shards
   speed/remote profiles select sharded transfer
   contract smoke covers bounded push, class pop, and aggregate count
22. Descriptor owner token lifecycle seed. DONE:
   allocator has an owner record
   descriptors carry owner tokens while ACTIVE / LOCAL_FREE
   remote transfer clears owner and transfer pop restores it
   allocator smoke covers owner assign / clear / restore
23. Owner-dead orphan seed. DONE:
   hz6_allocator_mark_owner_dead()
   owned ACTIVE / LOCAL_FREE descriptors become ORPHAN
   dead owner malloc is rejected
   orphan free is fail-closed in allocator smoke
24. Orphan release path. DONE:
   hz6_allocator_release_orphan()
   unregisters route and releases descriptor SourceLayer backing
   allocator smoke verifies route miss and descriptor DEAD after release
25. OS-paged SourceLayer abstraction. DONE:
   HZ6_SOURCE_OS_PAGED
   Linux maps OS-paged to mmap
   Windows maps OS-paged to VirtualAlloc behind _WIN32
   Local2P/Large128 fronts no longer name Linux-specific source kind
26. MidPage seed front. DONE:
   fronts/midpage/hz6_midpage_front.*
   >4KiB..32KiB requests route to HZ6_FRONT_MIDPAGE
   remote transfer reuse is covered by allocator smoke
27. Dedicated safety smoke. DONE:
   tests/hz6_r1_safety_smoke.c
   covers interior invalid, foreign miss, local/remote double-free, owner-dead
   orphan rejection, and orphan release
28. R1 smoke runner naming. DONE:
   linux/build_hz6_r1_smokes.sh is the canonical R1 runner
   linux/build_hz6_r1_contract_smoke.sh remains as a compatibility wrapper
29. ScavengeLayer seed. DONE:
   scavenge/hz6_scavenge.*
   bounded release accounting is covered by contract smoke
   hz6_allocator_scavenge_orphans() releases ORPHAN descriptors only
   safety smoke covers over-budget retention and route/source release
30. Frontcache invalidation for scavenging. DONE:
   hz6_frontcache_remove()
   hz6_allocator_scavenge_local_free() removes stale cache entries before
   releasing LOCAL_FREE descriptors
   safety smoke covers over-budget local retention and route/source release
31. Slow-path scavenge policy hook. DONE:
   Hz6ProfileConfig carries scavenge_local_free_bytes / scavenge_orphan_bytes
   hz6_allocator_scavenge_profile() applies profile budgets explicitly
   RSS profile scavenges cached objects, strict profile keeps auto scavenge off
32. Cross-owner orphan adoption. DONE:
   hz6_allocator_adopt_orphan()
   dead-owner ORPHAN descriptors can move to a live allocator as LOCAL_FREE
   ACTIVE descriptors cannot be adopted
   allocator smoke covers adoption reuse and source-route removal
33. Transfer observability seed. DONE:
   hz6_transfer_count_class()
   hz6_transfer_backend_count_class()
   hz6_transfer_backend_shard_count_at()
   contract smoke verifies class visibility and shard distribution
34. Profile transfer capacity wiring. DONE:
   Hz6ProfileConfig.transfer_capacity now selects transfer backend capacity
   capacity is capped by HZ6_TRANSFER_CACHE_CAPACITY
   allocator smoke covers RSS profile capacity and capped remote profile capacity
35. Explicit source prefill seed. DONE:
   hz6_front_prefill_source_kind()
   uses profile.source_batch from slow-path tests without changing malloc hit path
   allocator smoke covers RSS profile source_batch prefill and no hidden refill
36. Route backend variant seed. DONE:
   HZ6_ROUTE_BACKEND_PAGE_TABLE
   shares MISS / VALID / INVALID result contract with exact-table backend
   contract smoke covers register, valid lookup, invalid interior, unregister
37. Strict remote pending drain. DONE:
   strict_owner_remote uses ACTIVE -> REMOTE_PENDING
   hz6_allocator_drain_remote_pending() moves REMOTE_PENDING -> LOCAL_FREE
   allocator and safety smoke cover strict remote reuse after explicit drain
38. Remote-pending owner-death safety. DONE:
   hz6_allocator_mark_owner_dead() now converts REMOTE_PENDING to ORPHAN
   hz6_allocator_drain_remote_pending() rejects dead owners
   safety smoke covers pending remote orphan release after owner death
39. Page route granularity contract. DONE:
   HZ6_ROUTE_BACKEND_PAGE_TABLE now carries an explicit page granularity
   lookup filters entries by page envelope before preserving VALID / INVALID / MISS
   contract smoke covers custom page granularity and object-end MISS
40. MidPage page-run policy seed. DONE:
   fronts/midpage/hz6_midpage_front.* now owns an 8K / 32K page-run geometry
   hz6_midpage_policy_for_size() exposes class, slot bytes, run bytes, slots/run
   allocator smoke covers 8K class routing, descriptor bytes, and local reuse
41. Descriptor source pointer split. DONE:
   Hz6ObjectDescriptor now stores user ptr and source_ptr separately
   release uses source_ptr/source_bytes while route/cache still use user ptr
   allocator smoke covers user ptr != source_ptr release behavior
42. Source-block slot helper. DONE:
   hz6_front_source_slot_kind()/ops() allocate a source block and expose one user slot
   descriptor records user bytes separately from source bytes
   allocator smoke covers MidPage 8K slot inside a 64KiB source block
43. Shared SourceBlock ownership seed. DONE:
   Hz6SourceBlock tracks source block release metadata and descriptor references
   hz6_front_source_block_slot() registers multiple user slots against one block
   allocator smoke verifies first slot release does not release the shared source block
44. MidPage run prefill seed. DONE:
   hz6_midpage_prefill_run() creates one 64KiB SourceBlock for 8K / 32K classes
   prefilled slots enter the MidPage local cache as LOCAL_FREE descriptors
   allocator smoke verifies 8 slots share one SourceBlock and avoid source refill
45. MidPage alloc miss run refill. DONE:
   MidPage malloc now tries transfer cache and local cache before run prefill
   only if both are empty, alloc miss pre-fills a 64KiB run into local cache
   allocator smoke verifies normal 8K malloc now comes from a SourceBlock run
46. Shared SourceBlock scavenge budget. DONE:
   scavenge charges shared SourceBlock descriptors by user slot bytes
   non-shared descriptors still charge source bytes
   safety smoke verifies one 8K slot can be scavenged without releasing the 64KiB run
47. SourceBlock route envelope. DONE:
   route table supports invalid-range entries alongside exact-valid slot entries
   SourceBlock slot registration installs an invalid envelope for the whole run
   allocator smoke verifies unregistered run slots are INVALID and become MISS after final release
48. Route invalid-range contract smoke. DONE:
   contract smoke verifies invalid-range registration and unregister
   exact-valid entries take priority over invalid-range envelopes
   backend wrapper exposes the same invalid-range contract
49. SourceBlock route teardown order. DONE:
   shared SourceBlock release now unregisters the invalid route envelope before
   SourceLayer release
   allocator destroy already follows the same route-before-source teardown order
   allocator smoke covers final SourceBlock release turning the envelope back to MISS
50. Page-table invalid-range smoke. DONE:
   PAGE_TABLE backend now has direct invalid-range smoke coverage
   exact-valid entries inside a page-range envelope still take priority
   unregistering the exact entry falls back to INVALID until the envelope is removed
51. MidPage 32K run prefill smoke. DONE:
   allocator smoke now verifies 32K class prefill creates two slots from one
   64KiB SourceBlock
   both 32K slots route as HZ6_FRONT_MIDPAGE / HZ6_MIDPAGE_32K_CLASS_ID
   the shared SourceBlock avoids a second source refill while both slots are consumed
52. SourceBlock duplicate-slot failure smoke. DONE:
   allocator smoke verifies duplicate exact slot registration is rejected
   SourceBlock refcount is restored after the failed registration path
   descriptor reuse is verified by registering the next SourceBlock slot after
   duplicate rejection without releasing the backing SourceBlock
53. Profile-selected route backend seed. DONE:
   Hz6ProfileConfig now carries route_page_granularity
   allocator init selects PAGE_TABLE when profile route granularity is nonzero
   REMOTE profile smoke verifies PAGE_TABLE route backend selection
54. Profile route contract smoke. DONE:
   contract smoke verifies SPEED / REMOTE use page route granularity
   contract smoke verifies STRICT / RSS keep exact-table route selection
   status docs now describe profile-selected route backend rather than exact-only routing
55. Allocator profile route init smoke. DONE:
   allocator smoke verifies SPEED initializes PAGE_TABLE
   allocator smoke verifies REMOTE initializes PAGE_TABLE
   allocator smoke verifies STRICT / RSS initialize EXACT_TABLE
56. Transfer shard capacity observability. DONE:
   transfer backend exposes per-shard capacity for diagnostics
   contract smoke verifies uneven capacity is preserved across shards
   inactive shard capacity reports zero
57. Transfer uneven shard fill smoke. DONE:
   contract smoke fills an uneven sharded backend to full capacity
   shard counts match the 3/2 capacity split
   one extra push is rejected after all shard capacity is consumed
58. Transfer shard steal smoke. DONE:
   contract smoke verifies pop checks the class home shard first
   if the home shard is empty, pop steals from a later non-empty shard
   this keeps consumer-visible transfer reuse from depending on producer shard locality
59. Transfer sharded class-filter smoke. DONE:
   contract smoke verifies sharded class pop retains other classes
   retained class entries can be popped afterward
   this preserves class-specific transfer cache semantics in mixed-class shards
60. Large128 profile prefill seed. DONE:
   Large128 front exposes hz6_large128_prefill()
   allocator smoke verifies RSS profile source_batch pre-fills Large128 cache
   consuming the prefilled Large128 objects avoids additional source refill
61. Local2P profile prefill seed. DONE:
   Local2P front exposes hz6_local2p_prefill()
   allocator smoke verifies RSS profile source_batch pre-fills Local2P cache
   consuming the prefilled Local2P objects avoids additional source refill
62. FrontOps prefill hook seed. DONE:
   Hz6FrontOps now carries an optional prefill hook
   Local2P and Large128 register front-specific prefill hooks
   allocator smoke calls Local2P / Large128 prefill through hz6_front_for_id()
63. Front registry prefill helper. DONE:
   hz6_front_prefill_by_id() invokes optional front prefill hooks by front id
   allocator smoke uses the registry helper for Local2P / Large128 prefill
   allocator smoke verifies fronts without a prefill hook return zero
64. Size-based prefill helper. DONE:
   hz6_front_prefill_for_allocation() selects a front through the allocation
   registry and invokes its optional prefill hook
   hz6_allocator_prefill_size() exposes the helper at the allocator layer
   allocator smoke verifies size-based prefill for Large128 / Local2P and
   originally kept MidPage out until prefill became class-aware
65. Large128 profile batch refill. DONE:
   Large128 alloc miss now uses profile.source_batch through the shared
   prefill helper before falling back to one-off source allocation
   transfer-first profiles still consume TRANSFER_FREE spans before cached
   LOCAL_FREE spans after a batch refill
   allocator smoke verifies Large128 source allocation is batched up to the
   frontcache bin capacity
66. Local2P profile batch refill. DONE:
   Local2P exact 64K alloc miss now uses the same profile.source_batch prefill
   helper as Large128
   transfer-first profiles keep TRANSFER_FREE reuse ahead of cached LOCAL_FREE
   slots after the batch refill
   allocator smoke verifies Local2P source allocation is batched up to the
   frontcache bin capacity
67. Class-aware FrontOps prefill. DONE:
   Hz6FrontOps prefill hooks now receive the selected class id
   hz6_front_prefill_by_id_class() exposes class-specific front prefill through
   the registry while hz6_front_prefill_by_id() remains compatible for
   single-class fronts
   MidPage registers a prefill hook that delegates to run prefill, so
   hz6_allocator_prefill_size() can now prefill MidPage by allocation size
68. Allocator-level front prefill wrappers. DONE:
   hz6_allocator_prefill_front() exposes front-id prefill without requiring
   callers to include the FrontLayer registry header
   hz6_allocator_prefill_front_class() exposes class-specific front prefill at
   the allocator layer
   allocator smoke verifies Large128 front prefill and MidPage 8K class prefill
   through these wrappers
69. Transfer explicit home shard pop. DONE:
   hz6_transfer_backend_pop_from_shard() lets a caller provide the consumer
   home shard instead of deriving it only from class id
   hz6_transfer_backend_pop() remains a compatibility wrapper using class id as
   the home seed
   contract smoke verifies explicit home shard pop prefers the requested shard
   and then steals from another non-empty shard for the same class
70. Front reuse consumer shard connection. DONE:
   FrontLayer transfer reuse now passes allocator owner slot as the consumer
   home shard to hz6_transfer_backend_pop_from_shard()
   cached-first and transfer-first reuse share one transfer activation helper
   allocator smoke verifies a consumer with owner slot 1 pops the same-class
   object from shard 1 before shard 0
71. Transfer explicit producer shard push. DONE:
   hz6_transfer_backend_push_to_shard() lets a caller provide the producer
   shard instead of relying only on backend round-robin state
   hz6_transfer_backend_push() remains a compatibility wrapper using
   next_push_shard
   contract smoke verifies explicit push prefers the requested shard and falls
   back to another shard when the requested shard is full
72. Remote free producer shard connection. DONE:
   FrontLayer remote free now passes allocator owner slot as the producer shard
   to hz6_transfer_backend_push_to_shard()
   allocator smoke verifies owner slot 1 remote frees land in shard 1 and are
   reused first by the same consumer home shard
73. Transfer shard policy helpers. DONE:
   hz6_profile_transfer_producer_shard() and
   hz6_profile_transfer_consumer_shard() centralize producer/consumer shard
   seed selection in PolicyLayer
   FrontLayer transfer push/pop now asks PolicyLayer for shard seeds instead of
   reading owner slot directly
   contract smoke verifies remote profile shard seeds and single-shard profiles
74. Explicit transfer shard policy id. DONE:
   Hz6ProfileConfig now carries transfer_shard_policy
   HZ6_TRANSFER_SHARD_OWNER_SLOT is the default profile policy
   HZ6_TRANSFER_SHARD_CLASS_ID is covered as a contract variant for future
   class-oriented transfer experiments
75. Explicit route backend policy id. DONE:
   Hz6ProfileConfig now carries route_backend_policy
   HZ6_ROUTE_POLICY_EXACT_TABLE and HZ6_ROUTE_POLICY_PAGE_TABLE make route
   backend selection explicit instead of inferring it from page granularity
   allocator init selects the route backend from route_backend_policy
   contract smoke verifies strict/rss exact policy and speed/remote page policy
76. Profile source kind policy. DONE:
   Hz6ProfileConfig now carries source_kind
   Large128, Local2P, and MidPage source/refill paths use profile.source_kind
   instead of hard-coding HZ6_SOURCE_OS_PAGED in each front
   contract smoke verifies speed/remote source_kind is HZ6_SOURCE_OS_PAGED
77. Source refill batch policy helper. DONE:
   hz6_profile_source_refill_batch() centralizes source refill batch selection
   in PolicyLayer
   Large128 and Local2P alloc miss paths call the helper instead of reading
   profile.source_batch directly
   contract smoke verifies speed/rss refill batch policy values
78. Scavenge budget policy helpers. DONE:
   hz6_profile_scavenge_orphan_budget() and
   hz6_profile_scavenge_local_free_budget() centralize explicit profile
   scavenge budgets in PolicyLayer
   hz6_allocator_scavenge_profile() now asks PolicyLayer for budgets instead
   of reading profile scavenge fields directly
   contract smoke verifies rss budgets and strict zero-budget policy
79. Allocator transfer observability wrappers. DONE:
   hz6_allocator_transfer_backend_kind(), transfer_capacity(), transfer_count(),
   transfer_count_class(), transfer_shard_count_at(), and
   transfer_shard_capacity_at() expose transfer diagnostics through the
   allocator API boundary
   allocator smoke now checks profile transfer shape and producer shard routing
   without reaching into allocator.transfer_backend directly
80. Allocator route observability wrappers. DONE:
   hz6_allocator_route_lookup(), route_backend_kind(), and
   route_page_granularity() expose route diagnostics through the allocator API
   boundary
   allocator and safety smoke now use allocator route lookup for descriptor
   validation instead of directly calling hz6_route_backend_lookup()
81. Allocator profile observability wrappers. DONE:
   hz6_allocator_profile_id(), profile_transfer_first(),
   profile_strict_owner_remote(), profile_source_kind(),
   profile_source_refill_batch(), and profile_transfer_capacity() expose
   profile decisions through the allocator API boundary
   allocator smoke now checks transfer-first and source refill batch behavior
   without reading allocator.profile fields directly
82. Front profile access boundary cleanup. DONE:
   Large128, Local2P, MidPage, and shared FrontUtil now obtain source kind,
   source refill batch, transfer-first, and strict-remote policy through
   allocator/profile helper APIs instead of reading allocator.profile fields
   directly
   rg confirms tests/fronts/api no longer contain allocator.profile field
   accesses outside allocator internals
83. Allocator stats snapshot boundary cleanup. DONE:
   allocator smoke now reads source_alloc through hz6_stats_snapshot() instead
   of direct allocator.stats field access
   rg confirms tests/fronts/api no longer contain allocator.stats field access
   outside allocator/front internals
84. Allocator source registry boundary helper. DONE:
   hz6_allocator_source_ops() exposes SourceLayer ops lookup through the
   allocator API boundary
   shared FrontUtil and MidPage now obtain source ops through the allocator
   helper instead of directly reading allocator.source_registry
85. Allocator route unregister boundary helper. DONE:
   hz6_allocator_route_unregister_exact() exposes exact route removal through
   the allocator API boundary
   FrontUtil, MidPage, and allocator smoke no longer call exact route
   unregister through allocator.route_backend directly
86. Allocator frontcache boundary helpers. DONE:
   hz6_allocator_frontcache_push(), pop(), remove(), count(), and capacity()
   expose front-cache operations through the allocator boundary
   allocator internals, shared FrontUtil, and MidPage now avoid direct
   frontcache_bins access outside allocator helper implementations and init
87. Allocator transfer operation boundary helpers. DONE:
   hz6_allocator_transfer_push() and hz6_allocator_transfer_pop() centralize
   profile shard selection plus TransferBackend push/pop
   shared FrontUtil no longer calls TransferBackend push/pop or transfer shard
   policy helpers directly
88. Allocator route registration boundary helpers. DONE:
   hz6_allocator_route_register_exact() and
   hz6_allocator_source_block_register_invalid_range() expose route
   registration through the allocator boundary
   shared FrontUtil and MidPage no longer call RouteBackend register/lookup
   helpers through allocator.route_backend directly
89. Allocator stats note helpers. DONE:
   hz6_allocator_note_source_alloc(), note_transfer_push(), and
   note_transfer_pop() centralize front-originated stats updates in the
   allocator API
   shared FrontUtil and MidPage no longer mutate allocator.stats directly
90. Allocator owner token boundary helper. DONE:
   hz6_allocator_owner_token() exposes owner token reads through the allocator
   API boundary
   hz6_allocator_debug_set_owner_slot() keeps smoke-only shard setup out of
   allocator internals
   shared FrontUtil and allocator smoke no longer read allocator.owner.token
   directly
91. Allocator descriptor preparation boundary helper. DONE:
   hz6_allocator_prepare_descriptor() centralizes descriptor user/source
   pointer, SourceBlock, owner token, generation, class, and initial state
   setup
   shared FrontUtil no longer hand-initializes descriptor source fields for
   one-off, slot-offset, SourceBlock, or prefill allocations
92. Allocator active-descriptor transition helpers. DONE:
   hz6_allocator_cache_active_descriptor() centralizes ACTIVE -> LOCAL_FREE
   plus frontcache push / overflow release
   hz6_allocator_remote_free_active_descriptor() centralizes ACTIVE ->
   TRANSFER_FREE / REMOTE_PENDING transitions and transfer push rollback
   FrontUtil and MidPage no longer hand-write free-path descriptor state
   transitions
93. Allocator descriptor implementation split. DONE:
   api/hz6_allocator_descriptor.c now owns descriptor pool lookup,
   descriptor preparation, activation, source release, and active-descriptor
   cache/remote transitions
   api/hz6_allocator.c is smaller and keeps allocator lifecycle, route,
   transfer, source block, scavenge, and public API orchestration
94. Allocator SourceBlock implementation split. DONE:
   api/hz6_allocator_source_block.c now owns SourceBlock allocation, retain,
   release, and invalid route envelope registration
   api/hz6_allocator.c no longer carries the MidPage run SourceBlock helper
   implementation
95. Allocator reclaim implementation split. DONE:
   api/hz6_allocator_orphan.c now owns owner-dead orphan conversion and orphan
   release/adoption
   api/hz6_allocator_remote_pending.c now owns strict remote-pending drain
   api/hz6_allocator.c now keeps lifecycle plus front/policy/route/transfer API
   orchestration separate from reclaim logic
96. Allocator facade implementation split. DONE:
   api/hz6_allocator_facade.c now owns frontcache, front prefill, profile,
   source ops, route diagnostics/registration, transfer diagnostics/ops, and
   stats note wrappers
   api/hz6_allocator.c is now focused on allocator lifecycle and public
   malloc/free/owns/stats entrypoints
97. Reclaim smoke split. DONE:
   tests/hz6_r1_reclaim_smoke.c now owns owner-dead orphan release, profile
   scavenge, and cross-owner orphan adoption coverage
   allocator smoke keeps allocator/front integration coverage and no longer
   carries reclaim-specific scenarios
98. Prefill smoke split. DONE:
   tests/hz6_r1_prefill_smoke.c now owns source-kind, front-registry,
   size-based, and allocator-front prefill coverage
   allocator smoke keeps hot allocation/front integration coverage and no
   longer carries slow-path prefill scenarios
99. SourceBlock smoke split. DONE:
   tests/hz6_r1_sourceblock_smoke.c now owns descriptor source release,
   single-slot source backing, shared SourceBlock refcount/route envelope,
   and MidPage 8K/32K run prefill coverage
   allocator smoke keeps front integration and no longer carries SourceBlock
   lifecycle-specific scenarios
100. Transfer smoke split. DONE:
   tests/hz6_r1_transfer_smoke.c now owns Local2P transfer reuse, profile
   transfer capacity/backend checks, generic remote transfer reuse,
   producer/consumer shard behavior, and strict REMOTE_PENDING drain coverage
   allocator smoke keeps basic allocator/front integration and no longer
   carries transfer-backend-specific scenarios
101. Route smoke split. DONE:
   tests/hz6_r1_route_smoke.c now owns exact route, invalid range envelope,
   exact/table backend, and page-table backend contract coverage
   contract smoke keeps transfer, owner, profile, frontcache, source, and
   scavenge low-level module coverage without RouteLayer bulk
102. Transfer contract smoke split. DONE:
   tests/hz6_r1_transfer_contract_smoke.c now owns bounded transfer cache,
   transfer backend, sharded backend class filtering, shard preference,
   explicit push/pop shard, and uneven shard capacity coverage
   contract smoke keeps owner, profile, frontcache, source, and scavenge
   low-level module coverage without TransferLayer bulk
103. Source contract smoke split. DONE:
   tests/hz6_r1_source_contract_smoke.c now owns source ops validation,
   Linux mmap source ops, source registry lookup, and ScavengeLayer budget
   accounting coverage
   core contract smoke keeps owner, profile, and frontcache low-level module
   coverage without SourceLayer/ScavengeLayer bulk
104. Core contract smoke rename. DONE:
   tests/hz6_r1_core_contract_smoke.c replaces the generic
   hz6_r1_contract_smoke.c name now that route, transfer, source, prefill,
   reclaim, safety, and SourceBlock coverage each have dedicated smoke files
   the remaining core contract smoke covers owner token, profile policy,
   size-class, and FrontCache primitives
105. Smoke runner registry cleanup. DONE:
   linux/build_hz6_r1_smokes.sh now has explicit HZ6_SMOKES entries in
   addition to HZ6_INCLUDES and HZ6_LIB_SOURCES
   adding a new R1 smoke now means adding one registry entry instead of
   appending another build_smoke call at the bottom of the script
106. OwnerLayer implementation split. DONE:
   owner/hz6_owner.c now owns hz6_owner_is_alive()
   owner/hz6_owner.h exposes the OwnerLayer contract without keeping liveness
   logic header-only
   linux/build_hz6_r1_smokes.sh registers owner/hz6_owner.c as an explicit
   HZ6_LIB_SOURCE
107. Owner equality implementation split. DONE:
   owner/hz6_owner.c now owns hz6_owner_equal()
   owner/hz6_owner.h exposes equality and liveness as the OwnerLayer API
   include/hz6_contract.h keeps shared token/result types and route result
   constructors only
108. Front source utility split. DONE:
   fronts/hz6_front_source.c now owns source-backed front allocation,
   source-block slot creation, and prefill helpers
   fronts/hz6_front_util.c now keeps reuse and free-route helpers only
   linux/build_hz6_r1_smokes.sh registers hz6_front_source.c as an explicit
   HZ6_LIB_SOURCE
109. Front source header split. DONE:
   fronts/hz6_front_source.h now exposes SourceLayer-backed front helpers
   fronts/hz6_front_util.h now exposes reusable object and free-route helpers
   front implementations and source/prefill smokes include the narrower
   header that matches the helper family they use
110. Front SourceBlock slot split. DONE:
   fronts/hz6_front_source_block.c now owns shared SourceBlock-backed slot
   creation and descriptor preparation
   fronts/hz6_front_source.c now keeps direct SourceLayer reserve and prefill
   helpers only
   linux/build_hz6_r1_smokes.sh registers hz6_front_source_block.c as an
   explicit HZ6_LIB_SOURCE
111. Allocator transfer facade split. DONE:
   api/hz6_allocator_transfer.c now owns allocator-facing TransferLayer
   diagnostics, push/pop wrappers, shard selection, and transfer stats notes
   api/hz6_allocator_facade.c no longer mixes transfer wrappers with
   frontcache, prefill, profile, source, and route wrappers
   linux/build_hz6_r1_smokes.sh registers hz6_allocator_transfer.c as an
   explicit HZ6_LIB_SOURCE
112. Allocator route facade split. DONE:
   api/hz6_allocator_route.c now owns allocator-facing RouteLayer lookup,
   register/unregister, backend diagnostics, and page granularity accessors
   api/hz6_allocator_facade.c no longer mixes route wrappers with frontcache,
   prefill, profile, source, and stats wrappers
   linux/build_hz6_r1_smokes.sh registers hz6_allocator_route.c as an explicit
   HZ6_LIB_SOURCE
113. Allocator facade removal. DONE:
   api/hz6_allocator_frontcache_mutation.c and
   api/hz6_allocator_frontcache_query.c now own allocator-facing FrontCache
   wrappers
   api/hz6_allocator_profile_query.c and api/hz6_allocator_profile_source.c
   now own profile/source policy queries, source ops lookup, and source
   allocation stats notes
   api/hz6_allocator_prefill.c now owns allocator-facing prefill wrappers
   api/hz6_allocator_facade.c was removed after all responsibilities moved to
   role-named API modules
114. Allocator public ops split. DONE:
   api/hz6_allocator_malloc.c now owns hz6_malloc()
   api/hz6_allocator_free.c now owns hz6_free()
   api/hz6_allocator_free_remote.c now owns hz6_free_remote()
   api/hz6_allocator.c now keeps allocator lifecycle, owner token helpers,
   initialization, and destruction separate from public operation dispatch
   linux/build_hz6_r1_smokes.sh registers the split public operation modules
   explicitly
115. Allocator scavenge split. DONE:
   api/hz6_allocator_scavenge.c now owns orphan/local-free/profile scavenge
   execution and descriptor scavenge cost accounting
   api/hz6_allocator_orphan.c now keeps owner-dead orphan conversion and
   orphan release/adoption separate from scavenging
   api/hz6_allocator_remote_pending.c now keeps strict remote-pending drain
   separate from scavenging
   linux/build_hz6_r1_smokes.sh registers hz6_allocator_scavenge.c as an
   explicit HZ6_LIB_SOURCE
116. Front source prefill split. DONE:
   fronts/hz6_front_source_prefill.c now owns slow-path prefill loops and
   source-backed fill helpers
   fronts/hz6_front_source.c now keeps direct source reserve and source-slot
   creation separate from prefill loops
   fronts/hz6_front_source_prefill.h provides the narrower prefill API for
   large/local2p front implementations and prefill smoke
   linux/build_hz6_r1_smokes.sh registers hz6_front_source_prefill.c as an
   explicit HZ6_LIB_SOURCE
117. Allocator orphan/remote-pending split. DONE:
   api/hz6_allocator_orphan.c now owns owner-dead orphan conversion and
   orphan release/adoption
   api/hz6_allocator_remote_pending.c now owns strict remote-pending drain
   api/hz6_allocator_reclaim.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new API modules explicitly
118. Allocator lifecycle split. DONE:
   api/hz6_allocator_init.c now owns allocator initialization
   api/hz6_allocator_destroy.c now owns allocator destruction
   api/hz6_allocator.c now keeps owner token helpers and debug owner slot
   hooks separate from lifecycle
   linux/build_hz6_r1_smokes.sh registers the init/destroy lifecycle modules
   explicitly
119. Front source slot split. DONE:
   fronts/hz6_front_source_slot_kind.c now owns source-kind based user-slot
   creation wrapper
   fronts/hz6_front_source_slot_ops.c now owns direct source-backed user-slot
   creation helpers
   fronts/hz6_front_source.c now keeps reuse and prefill helpers separate
   from direct source-slot creation
   linux/build_hz6_r1_smokes.sh registers the split front source slot modules
   explicitly
120. Allocator descriptor source split. DONE:
   api/hz6_allocator_descriptor_prepare.c now owns descriptor source setup
   api/hz6_allocator_descriptor_release.c now owns descriptor source release
   api/hz6_allocator_descriptor_source.c was replaced by the split helpers
   linux/build_hz6_r1_smokes.sh registers the split descriptor source modules
   explicitly
121. Allocator descriptor state split. DONE:
   api/hz6_allocator_descriptor_state.c now owns descriptor cache and
   remote-free state transitions
   api/hz6_allocator_descriptor.c now keeps descriptor lookup, prepare, and
   source release helpers separate from state transitions
   linux/build_hz6_r1_smokes.sh registers the descriptor state module
   explicitly
121. MidPage prefill split. DONE:
   fronts/midpage/hz6_midpage_prefill.c now owns explicit run-fill seeding
   and the shared 64KiB source-block refill loop
   fronts/midpage/hz6_midpage_front.c now keeps size policy and free/alloc
   dispatch separate from explicit run prefill
   linux/build_hz6_r1_smokes.sh registers hz6_midpage_prefill.c as an
   explicit HZ6_LIB_SOURCE
122. Route backend page-table split. DONE:
   route/hz6_route_backend_page_table.c now owns PAGE_TABLE lookup and shares
   granularity helpers via route/hz6_route_backend_util.h
   route/hz6_route_backend.c now keeps exact-table init/register/unregister
   and dispatch separate from PAGE_TABLE lookup
   linux/build_hz6_r1_smokes.sh registers the page-table backend module
   explicitly
123. Transfer backend sharded split. DONE:
   transfer/hz6_transfer_backend_sharded.c now owns sharded cache init and
   shard-aware push/pop/count helpers
   transfer/hz6_transfer_backend.c now keeps single-cache init and public
   dispatch separate from sharded cache internals
   linux/build_hz6_r1_smokes.sh registers the sharded transfer backend module
   explicitly
124. Front reuse/free split. DONE:
   fronts/hz6_front_reuse.c now owns transfer/cache reuse helpers
   fronts/hz6_front_free.c now owns free-path descriptor transition helpers
   fronts/hz6_front_util.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the reuse/free helper modules
   explicitly
125. Route table split. DONE:
   route/hz6_route_table.c now owns table init/register/unregister helpers
   route/hz6_route.c now keeps lookup separate from table management
   linux/build_hz6_r1_smokes.sh registers hz6_route_table.c as an explicit
   HZ6_LIB_SOURCE
125. MidPage policy split. DONE:
   fronts/midpage/hz6_midpage_policy.c now owns class-size mapping and size
   policy helpers
   fronts/midpage/hz6_midpage_front.c now keeps run prefill and alloc/free
   dispatch separate from class-size policy
   linux/build_hz6_r1_smokes.sh registers hz6_midpage_policy.c as an explicit
   HZ6_LIB_SOURCE
126. Transfer backend observability split. DONE:
   transfer/hz6_transfer_backend_stats.c now owns aggregate count/capacity
   helpers for single-cache and sharded transfer backends
   transfer/hz6_transfer_backend_sharded.c now keeps shard init and push/pop
   separate from observability helpers
   linux/build_hz6_r1_smokes.sh registers hz6_transfer_backend_stats.c as an
   explicit HZ6_LIB_SOURCE
127. Source block route split. DONE:
   api/hz6_allocator_source_block_route.c now owns invalid-range route
   registration for source blocks
   api/hz6_allocator_source_block.c now keeps source block create/retain/release
   separate from route-envelope registration
   linux/build_hz6_r1_smokes.sh registers hz6_allocator_source_block_route.c
   as an explicit HZ6_LIB_SOURCE
128. Descriptor source split. DONE:
   api/hz6_allocator_descriptor_prepare.c and
   api/hz6_allocator_descriptor_release.c now own descriptor source setup and
   release helpers
   api/hz6_allocator_descriptor.c now keeps descriptor lookup and activation
   separate from source-backed initialization/release
   linux/build_hz6_r1_smokes.sh registers the split descriptor source helpers
   explicitly
129. Owner-dead split. DONE:
   api/hz6_allocator_owner_dead.c now owns owner-dead transitions
   api/hz6_allocator_orphan.c now keeps orphan release/adoption separate from
   owner shutdown state changes
   linux/build_hz6_r1_smokes.sh registers hz6_allocator_owner_dead.c as an
   explicit HZ6_LIB_SOURCE
130. Front source kind/ops split. DONE:
   fronts/hz6_front_source_kind.c now owns source-kind wrappers
   fronts/hz6_front_source_ops.c now owns direct source-ops allocation
   fronts/hz6_front_source.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new front source modules
   explicitly
131. Allocator scavenge split. DONE:
   api/hz6_allocator_scavenge_orphans.c now owns ORPHAN scavenging
   api/hz6_allocator_scavenge_local_free.c now owns LOCAL_FREE scavenging
   api/hz6_allocator_scavenge_profile.c now owns profile-level scavenge
   composition
   api/hz6_allocator_scavenge.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new scavenge modules explicitly
132. Allocator query split. DONE:
   api/hz6_allocator_owns.c now owns allocator owns() observation
   api/hz6_allocator_stats_snapshot.c now owns stats_snapshot() observation
   api/hz6_allocator_malloc.c / api/hz6_allocator_free.c /
   api/hz6_allocator_free_remote.c keep malloc/free/remote-free separate from
   observation
   linux/build_hz6_r1_smokes.sh registers the split query helpers explicitly
133. Front registry split. DONE:
   fronts/hz6_front_registry.c now owns front selection lookup
   fronts/hz6_front_prefill_dispatch.c now owns prefill dispatch wrappers
   fronts/hz6_front.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new front registry modules
   explicitly
134. Orphan release/adoption split. DONE:
   api/hz6_allocator_orphan_release.c now owns orphan release
   api/hz6_allocator_orphan_adopt_prepare.c and
   api/hz6_allocator_orphan_adopt_commit.c now own cross-owner orphan
   adoption
   api/hz6_allocator_orphan.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new orphan helper modules
   explicitly
135. Profile config/policy split. DONE:
   policy/hz6_profiles_config.c now owns profile construction
   policy/hz6_profiles_policy.c now owns shard, batch, and scavenge policy
   helpers
   policy/hz6_profiles.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new policy modules explicitly
136. Route table core/exact/invalid split. DONE:
   route/hz6_route_table_core.c now owns table init
   route/hz6_route_table_exact.c now owns exact register/unregister
   route/hz6_route_table_invalid.c now owns invalid-envelope register/
   unregister
   route/hz6_route_table.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new route table modules
   explicitly
137. MidPage front alloc/free/ops split. DONE:
   fronts/midpage/hz6_midpage_front_alloc.c now owns page-run selection and
   alloc/prefill reuse
   fronts/midpage/hz6_midpage_front_free.c now owns local/remote free
   dispatch
   fronts/midpage/hz6_midpage_front_ops.c now owns the front ops table
   fronts/midpage/hz6_midpage_front.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new MidPage front modules
   explicitly
138. Large128 front alloc/free/ops split. DONE:
   fronts/large/hz6_large128_front_alloc.c now owns source refill and
   allocation selection
   fronts/large/hz6_large128_front_free.c now owns local/remote free
   dispatch
   fronts/large/hz6_large128_front_ops.c now owns the front ops table
   fronts/large/hz6_large128_front.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new Large128 front modules
   explicitly
139. Transfer backend sharded init/ops split. DONE:
   transfer/hz6_transfer_backend_sharded_init.c now owns sharded backend init
   transfer/hz6_transfer_backend_sharded_push.c now owns shard push dispatch
   transfer/hz6_transfer_backend_sharded_pop.c now owns shard pop dispatch
   the monolithic sharded backend file was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new sharded transfer modules
   explicitly
140. Allocator transfer query/dispatch split. DONE:
   api/hz6_allocator_transfer_query.c now owns transfer observation helpers
   api/hz6_allocator_transfer_dispatch.c now owns transfer push/pop and
   stats note helpers
   api/hz6_allocator_transfer.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new transfer API modules
   explicitly
141. Route backend init/dispatch split. DONE:
   route/hz6_route_backend_init.c now owns exact/page-table initialization and
   shares route granularity helpers with the lookup module
   route backend dispatch responsibilities now live in role-named register /
   unregister / lookup modules
   route/hz6_route_backend.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new route backend modules
   explicitly
142. Allocator header types/api split. DONE:
   api/hz6_allocator_types.h now owns allocator type definitions
   api/hz6_allocator_api.h now owns public allocator declarations
   api/hz6_allocator.h was reduced to a thin wrapper
   allocator sources still include the wrapper for compatibility
143. SourceBlock create/lifetime split. DONE:
   api/hz6_allocator_source_block_create.c now owns source block creation
   api/hz6_allocator_source_block_lifetime.c now owns retain/release lifetime
   api/hz6_allocator_source_block.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new source block modules
   explicitly
144. Allocator init state/backends split. DONE:
   api/hz6_allocator_init_state.c now owns allocator state initialization
   api/hz6_allocator_init_backends.c now owns backend initialization
   api/hz6_allocator_init.c now stays as a thin wrapper
   linux/build_hz6_r1_smokes.sh registers the new init modules explicitly
145. Allocator public API declaration split. DONE:
   api/hz6_allocator_api_front.h / profile.h / route_transfer.h /
   scavenge.h / state.h split the public declarations by concern
   api/hz6_allocator_api.h was reduced to a thin include wrapper
146. Front source prefill one-shot split. DONE:
   fronts/hz6_front_source_prefill_one.c now owns the one-shot source-backed
   prefill helper
   fronts/hz6_front_source_prefill.c now keeps the repeated batching loop
   front source prefill helper header exposes the helper boundary explicitly
147. Front reuse transfer helper split. DONE:
   fronts/hz6_front_reuse_transfer.c now owns transfer-first reuse probing
   fronts/hz6_front_reuse.c now keeps the cached/transfer wrappers
   fronts/hz6_front_reuse.c no longer owns the transfer helper directly
148. Transfer backend stats aggregate/shards split. DONE:
   transfer/hz6_transfer_backend_stats_aggregate.c now owns aggregate
   count/capacity helpers
   transfer/hz6_transfer_backend_stats_shards.c now owns shard access helpers
   transfer/hz6_transfer_backend_stats.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new stats modules explicitly
149. Route backend utility split. DONE:
   route/hz6_route_backend_util.h now owns shared granularity and page-alignment
   helpers
   route/hz6_route_backend_init.c and route/hz6_route_backend_page_table.c now
   include the shared utility header instead of duplicating the math helpers
   route/backend lookup and init responsibilities stay separated, but the
   common math no longer lives in the lookup source
150. Public contract route/owner split. DONE:
   include/hz6_contract_route.h now owns route kinds/result constructors
   include/hz6_contract_owner.h now owns owner token and object state enums
   include/hz6_contract.h is reduced to a thin umbrella over the public
   contract pieces
151. Allocator API state split. DONE:
   api/hz6_allocator_api_init.h now owns init profile declaration
   api/hz6_allocator_api_descriptor.h now owns descriptor lifecycle helpers
   api/hz6_allocator_api_owner.h now owns owner token / debug / owner-dead
   api/hz6_allocator_api_source.h now owns source block creation and lifetime
   api/hz6_allocator_api_orphan.h now owns orphan release/adopt helpers
   api/hz6_allocator_api_state.h is reduced to a thin umbrella over the
   specialized API headers
152. MidPage prefill policy split. DONE:
   fronts/midpage/hz6_midpage_prefill_policy.c now owns the per-class run
   geometry helper
   fronts/midpage/hz6_midpage_prefill.c now keeps the actual source-backed
   run fill loop separate from policy calculation
   fronts/midpage/hz6_midpage_front.h exposes the helper boundary explicitly
153. Allocator public ops split. DONE:
   api/hz6_allocator_malloc.c / api/hz6_allocator_free.c /
   api/hz6_allocator_free_remote.c now own the public malloc/free/remote-free
   entrypoints
   api/hz6_allocator_ops.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split public operation modules
   explicitly
154. Local2P front split. DONE:
   fronts/local2p/hz6_local2p_front_alloc.c now owns exact 64KiB can-allocate
   and alloc selection
   fronts/local2p/hz6_local2p_front_free.c now owns local/remote free routing
   fronts/local2p/hz6_local2p_front_prefill.c now owns exact 64KiB prefill
   fronts/local2p/hz6_local2p_front_ops.c now owns the FrontOps table
   fronts/local2p/hz6_local2p_front.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split Local2P modules explicitly
155. Source registry split. DONE:
   source/hz6_source_registry_init.c now owns platform source registry seeding
   source/hz6_source_registry_lookup.c now owns source kind lookup
   source/hz6_source_registry.c was replaced by the split helpers
   linux/build_hz6_r1_smokes.sh registers the split source registry modules
   explicitly
156. Route backend dispatch split. DONE:
   route/hz6_route_backend_register.c now owns backend register helpers
   route/hz6_route_backend_unregister.c now owns backend unregister helpers
   route/hz6_route_backend_lookup.c now owns backend lookup dispatch
   the monolithic route backend dispatch file was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split route backend dispatch
   modules explicitly
157. Source system split. DONE:
   source/hz6_source_system_ops.c now owns source ops validation and system
   ops assembly
   source/hz6_source_system_memory.c now owns system alloc/free/release
   helpers
   source/hz6_source.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split source system modules
   explicitly
158. Linux mmap source split. DONE:
   source/linux_source_mmap_ops.c now owns Linux mmap source ops assembly
   source/linux_source_mmap_memory.c now owns Linux mmap reserve/commit/
   decommit/release helpers
   source/linux_source_mmap.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split Linux mmap modules
   explicitly
159. Windows source split. DONE:
   source/win_source_virtualalloc_ops.c now owns Windows VirtualAlloc source
   ops assembly
   source/win_source_virtualalloc_memory.c now owns Windows VirtualAlloc
   reserve/commit/decommit/release helpers
   source/win_source_virtualalloc.c was removed after the split
   Windows source split is documented in the HZ6 folder layout
160. Orphan adoption helper split. DONE:
   api/hz6_allocator_orphan_adopt_prepare.c now owns orphan adoption
   validation
   api/hz6_allocator_orphan_adopt_commit.c now owns orphan adoption commit and
   cleanup
   api/hz6_allocator_orphan_adopt.c now stays as a thin wrapper
   linux/build_hz6_r1_smokes.sh registers the split orphan adoption modules
   explicitly
161. Route backend page-table split. DONE:
   route/hz6_route_backend_page_table.c now owns PAGE_TABLE wrapper dispatch
   route/hz6_route_backend_page_table_exact.c now owns exact-entry scans
   route/hz6_route_backend_page_table_invalid.c now owns invalid-entry scans
   the monolithic page-table lookup body was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split route backend page-table
   modules explicitly
162. Transfer backend sharded ops split. DONE:
   transfer/hz6_transfer_backend_sharded_push.c now owns sharded push dispatch
   transfer/hz6_transfer_backend_sharded_pop.c now owns sharded pop dispatch
   the monolithic sharded ops file was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split sharded transfer backend
   ops explicitly
163. Allocator init state helper split. DONE:
   api/hz6_allocator_init_state_owner.c now owns owner/profile/stats/source
   registry seeding
   api/hz6_allocator_init_state_source_blocks.c now owns source block
   initialization
   api/hz6_allocator_init_state_descriptors.c now owns descriptor
   initialization
   api/hz6_allocator_init_state_frontcache.c now owns frontcache
   initialization
   api/hz6_allocator_init_state.c now stays as a thin wrapper
   linux/build_hz6_r1_smokes.sh registers the split init-state helpers
   explicitly
164. Allocator destroy helper split. DONE:
   api/hz6_allocator_destroy_descriptors.c now owns descriptor cleanup
   api/hz6_allocator_destroy_source_blocks.c now owns source-block cleanup
   api/hz6_allocator_destroy.c now stays as a thin wrapper
   linux/build_hz6_r1_smokes.sh registers the split destroy helpers
   explicitly
165. Allocator profile helper split. DONE:
   api/hz6_allocator_profile_query.c now owns profile/query helpers
   api/hz6_allocator_profile_source.c now owns source ops lookup and source
   allocation stats note helpers
   api/hz6_allocator_profile.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split profile helpers
   explicitly
166. Allocator frontcache helper split. DONE:
   api/hz6_allocator_frontcache_mutation.c now owns frontcache mutation
   helpers
   api/hz6_allocator_frontcache_query.c now owns frontcache observation
   helpers
   the monolithic frontcache helper file was removed after the split
   linux/build_hz6_r1_smokes.sh registers the split frontcache helpers
   explicitly
144. Descriptor state local-cache/remote-transfer split. DONE:
   api/hz6_allocator_descriptor_local_cache.c now owns cache-active
   transitions
   api/hz6_allocator_descriptor_remote_transfer.c now owns remote-free active
   transitions
   api/hz6_allocator_descriptor_state.c was removed after the split
   linux/build_hz6_r1_smokes.sh registers the new descriptor state modules
   explicitly
```

Current R1 smoke:

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected:

```text
hz6-r1-core-contract-smoke ok
hz6-r1-route-smoke ok
hz6-r1-transfer-contract-smoke ok
hz6-r1-source-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-prefill-smoke ok
hz6-r1-sourceblock-smoke ok
hz6-r1-transfer-smoke ok
hz6-r1-reclaim-smoke ok
hz6-r1-safety-smoke ok
```

## Next Benchmark Plan

```text
Keep the current smoke-only status while module boundaries settle.
Once the first HZ6 prototype path is frozen, add a benchmark lane that uses:
  - the same machine as HZ3 / HZ4 / HZ5
  - the same runner settings for all families
  - explicit HZ6 profile names instead of one universal default
  - benchmark tables only after the prototype path stops moving
```

## Open Decisions

```text
Route table shape:
  PAGE_TABLE backend is now seeded as a contract variant.
  Real Windows sidecar and Linux region/page maps still need implementation.

LargeSpan 128K direction:
  choose ownerless CentralSpanPool as the primary HZ6 LargeSpan path.
  LargeSpan remote free should become ACTIVE -> CENTRAL_FREE(ownerless), not
  owner-inbox-first and not a temporary TRANSFER_FREE add-on.
  Owner inbox remains strict/debug/fallback only.

LargeSpan rollout:
  L1: stabilize 128K CentralSpanPool state transitions and RSS budget.
  L2: add 256K / 512K / 1M classes on the same backend.
  L3: run full preload comparison after broad large-size coverage exists.

Transfer shard policy:
  generic transfer shard remains useful for small/mid/exact transfer paths.
  For LargeSpan, shard design belongs inside CentralSpanPool, not as a late
  transfer128 patch.

Fail-closed timing:
  strict profile uses immediate validation.
  speed remote profile may use batch-boundary validation only if invalid
  pointers cannot be promoted to reusable bins.

Profile names:
  use role names such as hz6-speed, hz6-rss, hz6-remote, hz6-strict.
  keep numeric knobs diagnostic-only.
```

## Not Yet

```text
No HZ5 source file migration.
No HZ5 code copy.
No new performance claim.
No benchmark table until an HZ6 prototype exists.
Toy allocation and transfer paths are for contract validation only.
Large128 is still a seed front, not a performance claim.
Large128 has Linux mmap backing, but no full span/source refill policy yet.
Local2P is an exact 64KiB seed front, not the final Windows Local2P profile.
MidPage is a seed front, not the final page-run allocator.
Sharded transfer has single-thread smoke coverage and observability only;
synchronization and real benchmark validation are still pending.
Cross-owner orphan adoption is a seed contract only; no shared route table or
thread-safe owner registry exists yet.
```
