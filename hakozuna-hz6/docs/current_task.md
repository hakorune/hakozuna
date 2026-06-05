# HZ6 Current Task

HZ6 is now in R1 implementation. Keep the implementation modular from the
start: API, route, frontcache, transfer, source, owner, policy, and future
fronts should stay separated.

Current Windows capacity lane names and promotion status are summarized in
`HZ6_LANE_GUIDE.md`. Keep this file as the longer investigation log.

Repo hygiene note:

```text
Use these orientation docs before reading this long ledger:
  HZ6_SELECTED_FAMILY_SUMMARY.md
  HZ6_LANE_GUIDE.md
  HZ6_ELASTIC_CAPACITY_PLAN.md
  HZ6_REPO_HYGIENE.md
  HZ6_SOURCE_MODULARIZATION.md
```

### 2026-06-06: DepotDescriptorRehomeBudget2048 repeat/guard and run-meta crash fix

Goal:

```text
Validate DepotDescriptorRehomeBudget2048-L1 as the bounded follow-up to
DepotDescriptorRehome-L1, including same-owner worker-warmup guard rows.
```

Issue found:

```text
The non-diagnostic Larson worker-warmup 8k/10k rows could terminate with
0xC0000094 during early warmup. Diagnostic builds passed, which initially
made the failure look like a parser/capture issue.

Root cause:
  elastic depot source-run metadata could observe a run-active block whose
  run_slot_count was transiently zero during concurrent worker warmup.
  The metadata mark path then used block->run_slot_count in a modulo.
```

Fix:

```text
1. Add a bench-only unhandled-exception print path so future Larson crashes
   report exception code/address and a compact HZ6 stats snapshot.

2. Make the capacity matrix capture that exception line and wait briefly for
   redirected stdout/stderr to flush after process exit.

3. Harden hz6_allocator_elastic_depot_source_run_mark_slot():
   - reject active run metadata with run_slot_count == 0,
   - use the already validated local slot_count for used-count bounds and
     next-hint modulo instead of rereading block->run_slot_count.
```

Validation:

```text
Manual non-diagnostic worker10k after fix:
  3 consecutive runs exit 0.

Matrix:
  docs/benchmarks/windows/paper/
    hz6_depotdescrehome_budget2048_repeat_guard_final/
      20260606_004135_hz6_capacity_matrix_windows.md

Repeat-3 medians:
  larson_T1              8.625M
  larson_T4             23.239M
  larson_T8             30.697M
  larson_T16            44.043M
  larson_t16_main_10k   44.034M
  larson_t16_worker_10k 45.404M
  larson_t16_main_4k    52.708M
  larson_t16_worker_4k  54.011M
  larson_t16_main_1k    57.438M
  larson_t16_worker_1k  58.247M
```

Decision:

```text
KEEP:
  DepotDescriptorRehomeBudget2048-L1 remains the current bounded
  source-depot candidate-control.

Interpretation:
  The budget cap keeps descriptor materialization bounded while preserving
  strong Larson main/worker throughput after the run-meta zero-count fix.

Not yet:
  Do not make it default/paper-selected until broader non-Larson lanes
  are checked.

Next:
  1. Commit this crash fix + runner hygiene + ledger.
  2. Run a small broad-lane check for mixed_ws/random_mixed/large_slices if
     promoting beyond Larson is considered.
  3. If broad lanes are clean, compare against the selected low-RSS HZ6 row
     and decide whether Budget2048 is a source-depot sibling or just Larson
     evidence.
```

Broad follow-up:

```text
mixed_ws balanced:
  PASS in a 1-run smoke, around 65.0M ops/s and ~112000 KB,
  alloc_fail=0, route_invalid=0, route_miss=0, route_register_fail=0.

mixed_ws wide_ws:
  NO-GO. Direct run exits with an access violation:
    code=0xc0000005
    info0=1
    info1=000001D0AE140020

random_mixed run-1 smoke:
  small  = 26.253M / 50508 KB, safety clean
  medium = 28.993M / 12468 KB, safety clean
  mixed  = 27.576M / 12988 KB, safety clean

Decision update:
  Budget2048 stays valuable Larson source-depot evidence, but it is not a
  broad mixed_ws/default promotion candidate. The next broad target should
  diagnose wide_ws ownership/source-depot safety separately rather than
  increasing the Budget2048 cap or defaulting this lane.
```

Current short read:

```text
Selected HZ6 Windows RSS/throughput sibling:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512

Status:
  good compact / Larson cross-owner throughput
  very low payload relative to static allocator tables
  not yet a general promotion lane

Current bottleneck hypothesis:
  RSS is now dominated less by payload and more by per-worker static capacity.
  SourceBlock local capacity can be trimmed from source16k to source10k for
  Larson main-warmup, but source8k/source2k are too tight. The next large-ROI
  HZ6 work remains elastic/shared descriptor-route-source capacity, not
  hot-path counters and not another packing-only lane.

Immediate action queue:
  1. source10k guard matrix is complete and safety-clean.
     Keep source10k as the current Larson combined-packed minimum-RSS sibling.
  2. ElasticProjection-L1, ElasticHighWater-L1, and MainWarmupCapacity-L1 are
     fixed as diagnostic evidence.
       Projection shows large static-table upside.
       HighWater shows final worker steady-state is tiny.
       MainWarmupCapacity shows the main allocator transiently owns the seeded
       live set: descriptor 160,048; route 170,051; source block 10,003.
  2a. ElasticOverflowProjection-L0 is fixed as the first shared-overflow
      sizing diagnostic.
        full10k with final high-water local caps projects:
          descriptor overflow 159,024
          route overflow      153,667
          source overflow       9,939
          frontcache overflow       0
          transfer overflow         0
        This reinforces that descriptor/route/source need a unified overflow
        contract; frontcache/transfer are not the first ElasticCapacity target.
  2b. ElasticRouteOverflow-L1 is fixed as a narrow RSS candidate-control.
        lane:
          ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-
          thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
          routebytes16-storageowner16-ownersourcel2-frontcachepacked-
          sourceblockpacked-source10k-route16k-run512
        result:
          smoke main1k clean
          full10k run-1: 41.723M / 335,132 KB
          safety: route_invalid=0, route_miss=0, route_register_fail=0,
                  alloc_fail=0
        read:
          shared exact/envelope route overflow is viable and reduces RSS.
          It is not speed promotion: it is slower than source10k control.
          Next behavior should cover descriptor/source overflow, not keep
          shrinking route alone.
  2c. ElasticDescriptorRouteOverflow-L1 is fixed as the current ElasticCapacity
      best RSS candidate-watch after depot storage fast path.
        lane:
          ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-
          thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
          routebytes16-storageowner16-ownersourcel2-frontcachepacked-
          sourceblockpacked-source10k-route16k-run512
        result:
          before storagefast full10k run-1: 33.184M / 246,824 KB
          after storagefast diagnostic full10k: 40.951M / 246,912 KB
          after storagefast non-diagnostic full10k: 43.843M / 246,888 KB
          non-diagnostic repeat-3 guards:
            main10k   42.770M / 246,880 KB
            worker10k 46.060M / 237,056 KB
            main1k    56.686M / 115,460 KB
            worker1k  57.326M / 115,272 KB
            main4k    55.410M / 162,072 KB
            worker4k  51.050M / 156,040 KB
          safety: route_invalid=0, route_miss=0, route_register_fail=0,
                  descriptor_exhausted=0, source_block_exhausted=0,
                  alloc_fail=0
          main-warmup descriptor depot alloc: 143,664
        read:
          descriptor depot + route overflow is viable and drops RSS sharply.
          The original speed loss was depot descriptor storage-owner lookup
          locality. Routing depot descriptors through SourceBlock side metadata
          removes descriptor_storage_miss and recovers source10k-class speed in
          non-diagnostic full10k. Repeat-3 guards are safety-clean. Treat as
          ElasticCapacity candidate-watch/evidence; do not broad-promote until
          source-block overflow / unified overflow drain is designed.
  2d. ElasticDescriptorSourceRouteOverflow-L1 is fixed as a lower-RSS
      source-depot component/control.
        lane:
          ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-
          thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
          routebytes16-storageowner16-ownersourcel2-frontcachepacked-
          sourceblockpacked-source64-route16k-run512
        implementation:
          descriptor local cap 16k + shared descriptor depot
          route local cap 16k + shared exact/envelope route overflow
          source block local cap 64 + shared SourceBlock depot
        result:
          diagnostic smoke main1k: 56.362M / 106,048 KB
          diagnostic full10k:      41.516M / 225,212 KB
          non-diagnostic full10k:  41.733M / 227,852 KB
          main-warmup local source_block_used=64 while
            source_block_active_max=10,003
          safety: source_block_exhausted=0, descriptor_exhausted=0,
                  route_register_fail=0, alloc_fail=0
        read:
          SourceBlock depot absorbs the main-warmup source spike and cuts RSS
          below ElasticDescriptorRouteOverflow-L1. Throughput is lower than the
          descriptor+route depot candidate-watch, so this is a lower-RSS
          source-depot component/control, not promotion.
  3. Continue ElasticCapacity-L1 design work:
       local small descriptor/route/source/frontcache caps
       shared overflow / depot for warmup pressure
       fail-closed INVALID/MISS preserved
       no hot-path global scan
       next target: depot accounting and unified overflow drain/localize
       contract; descriptor/route/source depot pieces are now bounded evidence.
  4. Do not promote SourceBlock-only overflow as the main behavior. The current
     source-depot lane works only as part of descriptor+route overflow. Treat
     it as a subcomponent until drain/localize and owner/destroy accounting are
     explicit.
  5. Do not add another packing lane now. ElasticCapacity-L1 has route,
     descriptor, and source-block depot component evidence; the next useful
     work is accounting, drain/localize, and promotion/no-go criteria.
```

Immediate read after ElasticHighWater-L1:

```text
source10k selected lane:
  full10k high-water is modest:
    descriptor_live_max      = 400
    source_block_active_max  = 25
    frontcache_total_max     = 401
    route occupied max       = 5425

Meaning:
  The source10k selected lane itself is not near its static caps.
  The source8k/source2k failures and local-only elastic failures are caused by
  warmup ownership/lifecycle placement in the low-cap lanes, not by steady-state
  pressure in source10k.

Next behavior target:
  ElasticCapacity-L1 should not merely shrink every worker-local table.
  It needs a shared overflow/depot or warmup-owner handoff for low-cap lanes.
```

Immediate read after MainWarmupCapacity-L1:

```text
main-warmup full10k before worker handoff:
  descriptor_used       = 160,048
  route_occupied        = 170,051
  source_block_used     = 10,003
  frontcache_used       = 48

worker/final full10k after handoff:
  descriptor max worker = 400
  route max worker      = 5,425
  source max worker     = 25
  frontcache max worker = 401

Meaning:
  Local-only static cap shrinking cannot work for main-warmup because the main
  allocator must temporarily hold the whole seeded live set. The RSS win needs
  shared elastic overflow for the warmup owner:
    main allocator can burst into shared descriptor/route/source metadata
    worker handoff/rehome drains back to small local steady-state
    route INVALID/MISS stays fail-closed
```

Immediate read after ElasticOverflowProjection-L0:

```text
Diagnostic:
  [HZ6_ELASTIC_OVERFLOW_PROJECTION]

Source:
  docs/benchmarks/windows/paper/hz6_elastic_overflow_l0_smoke/
    20260605_120559_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/hz6_elastic_overflow_l0_full10k/
    20260605_120639_hz6_capacity_matrix_windows.md

full10k selected source10k lane:
  throughput / RSS:
    43.472M / 412,868 KB

  projected local caps from final worker high-water:
    descriptor local cap  = 1,024
    route local cap       = 16,384
    source block local cap= 64
    frontcache local cap  = 1,024
    transfer local cap    = 128

  projected main-warmup overflow:
    descriptor overflow   = 159,024 / 5,591 KiB
    route overflow        = 153,667 / 3,902 KiB
    source block overflow = 9,939 / 1,242 KiB
    frontcache overflow   = 0
    transfer overflow     = 0
    total overflow        = 10,735 KiB

Meaning:
  The next behavior must support descriptor, route, and source-block overflow
  together. Frontcache and transfer are not the first ElasticCapacity pressure
  points in this selected Larson row.
```

Immediate read after ElasticRouteOverflow-L1:

```text
Lane:
  ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-thindesc-
  nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512

Implementation:
  Local route table is reduced to 16k.
  Shared route directory handles exact overflow.
  Shared invalid-range table handles SourceBlock envelope overflow.
  SourceBlock remembers whether its invalid envelope is local or shared so
  release/destroy unregister the correct table.

Results:
  smoke main1k:
    clean, non-diagnostic smoke also builds/runs

  full10k:
    41.723M ops/s
    335,132 KB peak RSS
    route_invalid=0
    route_miss=0
    route_register_fail=0
    alloc_fail=0
    elastic_route_overflow_lookup/hit = 393,519,809 / 393,519,809

Read:
  Route overflow is safe enough as a mechanism/control and gives a large RSS
  reduction versus the source10k control (`412,280 KB` repeat-3 median;
  `412,868 KB` diagnostic run-1). However throughput is below source10k
  control (`44.864M` repeat-3 median; `43.472M` diagnostic run-1).

Decision:
  KEEP as ElasticRouteOverflow-L1 RSS candidate-control.
  Do not promote as the main HZ6 Larson lane.
  Next step is descriptor/source overflow or a unified metadata depot; route
  alone is not enough.
```

Immediate read after ElasticDescriptorRouteOverflow-L1:

```text
Lane:
  ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-thindesc-
  nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512

Implementation:
  Descriptor local cap is reduced to 16k.
  A shared descriptor depot supplies overflow descriptors after local
  descriptor exhaustion.
  Depot descriptors use a separate owner16 side array so owned/free state does
  not rely on allocator-local descriptor indexes.
  ElasticRouteOverflow-L1 remains enabled for exact and invalid-envelope route
  overflow.

Results:
  smoke main1k:
    56.223M / 126,824 KB
    safety clean
    descriptor depot unused because main1k fits local 16k descriptors

  full10k:
    33.184M / 246,824 KB
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    main-warmup elastic_descriptor_overflow_alloc=143,664

  storagefast full10k:
    diagnostic:
      40.951M / 246,912 KB
      descriptor_storage_lookup=409,709,924
      descriptor_storage_hit=409,709,924
      descriptor_storage_miss=0
      safety clean
    non-diagnostic:
      43.843M / 246,888 KB

  storagefast non-diagnostic repeat-3 guards:
    main10k:
      42.770M / 246,880 KB
    worker10k:
      46.060M / 237,056 KB
    main1k:
      56.686M / 115,460 KB
    worker1k:
      57.326M / 115,272 KB
    main4k:
      55.410M / 162,072 KB
    worker4k:
      51.050M / 156,040 KB
    alloc_fail / descriptor_exhausted / route_register_fail /
    source_block_exhausted:
      0 on all runs

Read:
  Descriptor overflow is viable and gives the largest RSS drop so far.
  The first run was not promotion because throughput was much lower than
  source10k control and route-only overflow. The storagefast follow-up shows
  that depot descriptors need a direct storage-owner contract: using
  SourceBlock side metadata turns descriptor_storage_miss to zero and recovers
  the lane to source10k-class throughput while keeping roughly 247 MB RSS.
  Repeat-3 guards keep the lane safety-clean. The remaining gap versus the
  selected source10k sibling is no longer descriptor depot lookup; the next
  design question is whether source-block overflow or unified overflow
  drain/localization can recover the last throughput while preserving the low
  RSS class.

Decision:
  KEEP as ElasticDescriptorRouteOverflow-L1 strong RSS candidate-watch.
  Do not broad-promote yet. Next step is source-block overflow / unified
  ElasticCapacity drain-localize design, not another descriptor/route-only
  knob.
```

Immediate read after ElasticDescriptorSourceRouteOverflow-L1:

```text
Lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-
  thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-
  storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-
  source64-route16k-run512

Implementation:
  Descriptor and route local caps remain at 16k.
  Local SourceBlock cap is reduced to 64.
  Shared SourceBlock depot supplies overflow blocks after local source-block
  exhaustion would otherwise happen.
  ElasticDescriptorRouteOverflow-L1 remains enabled for descriptor and route
  overflow.

Results:
  diagnostic smoke main1k:
    56.362M / 106,048 KB
    source_block_exhausted=0
    descriptor_exhausted=0
    route_register_fail=0
    alloc_fail=0

  diagnostic full10k:
    41.516M / 225,212 KB
    main-warmup source_block_used=64
    main-warmup source_block_active_max=10,003
    source_block_exhausted=0
    descriptor_storage_miss=0
    descriptor_exhausted=0
    route_register_fail=0
    alloc_fail=0

  non-diagnostic full10k:
    41.733M / 227,852 KB
    safety clean

Read:
  SourceBlock depot is viable as an ElasticCapacity subcomponent. It proves the
  source-block warmup spike can move out of per-worker local static capacity
  and drops RSS below the ElasticDescriptorRouteOverflow candidate-watch.
  Throughput is lower than ElasticDescriptorRouteOverflow, so the correct label
  is lower-RSS component/control, not promotion.

Decision:
  KEEP as ElasticDescriptorSourceRouteOverflow-L1 source-depot evidence.
  Next target is not another cap shrink. Add explicit depot accounting /
  drain-localize design if this lower-RSS shape is going to compete for
  candidate-watch; otherwise keep ElasticDescriptorRouteOverflow-L1 as the
  current best RSS/throughput balance.
```

Immediate read after SourceBlock depot accounting:

```text
Implementation:
  Added slow-path SourceBlock depot counters:
    elastic_source_block_overflow_alloc
    elastic_source_block_overflow_release
    elastic_source_block_overflow_exhausted

  Added RSS attribution:
    source_block_depot_bytes

Smoke source:
  docs/benchmarks/windows/paper/
    hz6_elastic_source_block_depot_accounting_smoke/
    20260605_151806_hz6_capacity_matrix_windows.md

Read:
  The counters are visible in both [HZ6_STATS] and
  [HZ6_MAIN_WARMUP_CAPACITY]. Main-warmup pressure rows show SourceBlock depot
  allocation directly, while source_block_exhausted remains zero. RSS
  attribution now includes the shared depot static cost separately from the
  per-worker local source_block_table_bytes.

Decision:
  KEEP as observability cleanup for ElasticDescriptorSourceRouteOverflow-L1.
  This is not a behavior change and does not make the source-depot lane a
  promotion candidate by itself. Next promotion question remains
  drain/localize and owner-safe depot lifecycle.
```

Immediate read after SourceBlock localize dry-run:

```text
Lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-localizedryrun-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run512

Implementation:
  Diagnostic-only.
  On transfer reuse, if the descriptor's SourceBlock is in the shared depot,
  count whether the depot block could be localized to the current allocator.
  Conservative localizable condition:
    storage owner differs from current allocator
    ref_count <= 1
    current allocator has a free local SourceBlock slot

Smoke source:
  docs/benchmarks/windows/paper/
    hz6_elastic_source_block_localize_dryrun_smoke/
    20260605_153225_hz6_capacity_matrix_windows.md

Result:
  main-warmup rows show:
    elastic_source_block_localize_probe > 0
    elastic_source_block_localize_storage_mismatch == probe
    elastic_source_block_localize_would_move = 0
    elastic_source_block_localize_block_shared == probe

  worker-warmup rows show:
    transfer reuse is absent, so localize_probe = 0

Read:
  Whole-block localize is not the next behavior. The depot SourceBlocks that
  cross owner/storage boundaries are still shared physical runs, so moving the
  whole SourceBlock metadata to one worker would be unsafe or ineffective.

Decision:
  KEEP as no-go/control for whole-SourceBlock localize.
  Do not implement whole-block localize behavior.
  Next useful design target should be slot-level/source-run ownership,
  descriptor storage locality without moving the whole block, or a different
  drain policy that respects shared run ownership.
```

### 2026-06-05: CapacityUtil-L1 diagnostic

Goal:

```text
Follow the RSS residual audit with a capacity utilization split:
  descriptors used / descriptor capacity
  route active / route capacity
  source blocks used / source block capacity
  frontcache used / frontcache capacity
  transfer current / transfer capacity

This is diagnostic-only:
  no HZ6 allocator hot path behavior change
  no production atomics/counters added
  derived in the Larson runner from existing stats and build-time capacities
```

Implementation:

```text
bench_larson_compare:
  emits [HZ6_CAPACITY_UTIL] under HZ6_DIAGNOSTIC_PROBES.

run_win_hz6_capacity_matrix:
  captures [HZ6_CAPACITY_UTIL]
  adds "HZ6 capacity utilization audit" markdown table.

Follow-up:
  [HZ6_CAPACITY_UTIL] also reports per-worker max usage and a diagnostic
  `local_cap_2x` projection:
    local_cap_2x = next_pow2(max_worker_used * 2)
```

Smoke:

```text
Command:
  win/run_win_hz6_capacity_matrix.ps1
    -Families larson
    -BenchmarkProfiles larson_t16_main_1k
    -Hz6Profiles speed
    -CapacityLanes ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512
    -Runs 1
    -ForceBuild
    -DiagnosticHz6Probes

Source:
  docs/benchmarks/windows/paper/hz6_capacity_util_l1_smoke/
    20260605_091404_hz6_capacity_matrix_windows.md

Result:
  descriptor used/cap: 2,352 / 2,621,440 = 0.09%
  route active/cap:    10,499 / 3,145,728 = 0.33%
  source blocks/cap:   147 / 262,144 = 0.06%
  frontcache used/cap: 2,352 / 1,048,576 = 0.22%
  transfer current/cap: 0 / 65,536 = 0.00%

Per-worker max / projected local cap:
  descriptor: 160 / 512
  route:      670 / 2,048
  source:     10 / 32
  frontcache: 160 / 512
  transfer:   0 / 0
```

Full 10k check:

```text
Command:
  same lane and diagnostic flags, but:
    -BenchmarkProfiles larson_t16_main_10k
    -Runs 1
    -SkipBuild

Source:
  docs/benchmarks/windows/paper/hz6_capacity_util_l1_full10k/
    20260605_091547_hz6_capacity_matrix_windows.md

Result:
  throughput/RSS:
    45.593M / 426112 KB

  descriptor used/cap:
    6,088 / 2,621,440 = 0.23%
    max worker / local_cap_2x = 400 / 1,024

  route active/cap:
    86,471 / 3,145,728 = 2.75%
    max worker occupied / local_cap_2x = 5,425 / 16,384

  source blocks/cap:
    383 / 262,144 = 0.15%
    max worker / local_cap_2x = 25 / 64

  frontcache used/cap:
    6,088 / 1,048,576 = 0.58%
    max worker / local_cap_2x = 400 / 1,024
```

Decision:

```text
KEEP as diagnostic evidence.

Read:
  The current selected lane is not payload-heavy in the smoke row.
  The large static RSS tables are mostly unused because capacity is replicated
  per worker allocator.

Next:
  Avoid more entry packing as the main line unless it is near-free.
  Explore an HZ6 ElasticCapacity / SharedCapacity design:
    local small descriptor/route tables
      initial Larson full-10k projection:
        descriptor local cap ~= 1,024
        route local cap ~= 16,384
        source block local cap ~= 64
        frontcache local cap ~= 1,024
    shared overflow or shared backing pool
    fail-closed route INVALID/MISS contract preserved
    no hot-path global scan
```

### 2026-06-05: ElasticProjection-L1 diagnostic

Goal:

```text
Before implementing shared/elastic descriptor-route capacity, quantify the
static-table upside of local-cap sizing with the already observed
`local_cap_2x` headroom.

This is diagnostic-only:
  no HZ6 allocator hot path behavior change
  no production atomics/counters added
  no route/descriptor/source lifetime behavior change
```

Implementation:

```text
bench_larson_compare:
  emits [HZ6_ELASTIC_PROJECTION] under HZ6_DIAGNOSTIC_PROBES.
  It projects table bytes for:
    descriptor
    route
    source block
    frontcache
    transfer

run_win_hz6_capacity_matrix:
  captures [HZ6_ELASTIC_PROJECTION]
  adds "HZ6 elastic capacity projection" markdown table.
```

Read:

```text
ElasticProjection-L1 is not a promotion lane.
It is the design bridge between:
  CapacityUtil-L1:
    current per-worker fixed tables are mostly unused

and:
  ElasticCapacity-L1:
    local small tables plus shared overflow/depot
```

Acceptance:

```text
The diagnostic must build and run only when DiagnosticHz6Probes is enabled.
The projection table must not change throughput/safety behavior by itself.
Use the output to choose the first real ElasticCapacity target.
```

Smoke:

```text
Command:
  win/run_win_hz6_capacity_matrix.ps1
    -Families larson
    -BenchmarkProfiles larson_t16_main_1k
    -Hz6Profiles speed
    -CapacityLanes ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512
    -Runs 1
    -ForceBuild
    -DiagnosticHz6Probes

Source:
  docs/benchmarks/windows/paper/hz6_elastic_projection_l1_smoke2/
    20260605_104834_hz6_capacity_matrix_windows.md

Result:
  throughput/RSS:
    56.041M / 290,952 KB

  safety:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0

  current static:
    234,502 KiB

  projected static:
    15,212 KiB

  projected static savings:
    219,290 KiB
```

Full 10k check:

```text
Command:
  same lane and diagnostic flags, but:
    -BenchmarkProfiles larson_t16_main_10k
    -Runs 1
    -SkipBuild

Source:
  docs/benchmarks/windows/paper/hz6_elastic_projection_l1_full10k/
    20260605_104905_hz6_capacity_matrix_windows.md

Result:
  throughput/RSS:
    40.575M / 412,840 KB

  safety:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0

  current static:
    234,502 KiB

  projected static:
    21,616 KiB

  projected static+payload:
    46,000 KiB

  projected static savings:
    212,886 KiB

  projected caps:
    descriptor 16,384 entries / 576 KiB
    route      262,144 entries / 6,656 KiB
    source     1,024 entries / 128 KiB
    frontcache 16,384 entries / 384 KiB
    transfer   2,048 entries / 48 KiB
```

Decision:

```text
KEEP as diagnostic evidence.

Read:
  The remaining RSS opportunity is now overwhelmingly static-table capacity,
  not payload. Source10k solved the local source-block lower bound for the
  main-warmup 10k row, but descriptor/route/frontcache/source tables are still
  replicated per worker at much larger caps than observed use.

Next:
  Start ElasticCapacity-L1 as behavior design, with route/descriptor safety
  first:
    local route/descriptor/source/frontcache tables stay small
    shared overflow/depot handles warmup pressure
    route INVALID/MISS and owned-looking invalid fail-closed semantics are
      preserved
    no hot-path global scan
```

### 2026-06-05: ElasticHighWater-L1 diagnostic

Goal:

```text
Tighten ElasticProjection by recording successful-run high-water marks for:
  descriptor live count
  source block active count
  frontcache total count

This is diagnostic-only:
  fields and updates are compiled only under HZ6_DIAGNOSTIC_PROBES
  no production hot-path counter/atomic is added
```

Implementation:

```text
Hz6Allocator diagnostic fields:
  diagnostic_descriptor_live_current
  diagnostic_source_block_active_current
  diagnostic_frontcache_total_current

Hz6StatsSnapshot high-water fields:
  descriptor_live_max
  source_block_active_max
  frontcache_total_max

bench_larson_compare:
  capacity local_cap_2x projection now uses high-water where available.
  [HZ6_CAPACITY_UTIL] reports descriptor/source/frontcache peak values.
```

Smoke:

```text
Source:
  docs/benchmarks/windows/paper/hz6_elastic_highwater_l1_smoke/
    20260605_113623_hz6_capacity_matrix_windows.md

Result:
  larson_t16_main_1k:
    56.166M / 290,936 KB
    safety clean
    descriptor_live_max=160
    source_block_active_max=10
    frontcache_total_max=161
```

Full 10k check:

```text
Source:
  docs/benchmarks/windows/paper/hz6_elastic_highwater_l1_full10k/
    20260605_113648_hz6_capacity_matrix_windows.md

Result:
  larson_t16_main_10k:
    45.200M / 412,840 KB
    safety clean

  high-water:
    descriptor_live_max=400
    source_block_active_max=25
    frontcache_total_max=401
    route occupied max allocator=5425

  projected static:
    21,616 KiB

  projected static+payload:
    46,128 KiB

  projected static savings:
    212,886 KiB
```

Decision:

```text
KEEP as diagnostic evidence.

Read:
  In the selected source10k lane, high-water is close to final utilization.
  That strengthens the static-table RSS diagnosis. The selected lane has large
  unused static tables even during successful warmup/runtime.

  The low-cap no-go rows are not fixed by pure final-snapshot shrinking. They
  need an ElasticCapacity behavior that can handle warmup-owner concentration
  and cross-owner live-set placement:
    local small tables
    shared overflow/depot
    fail-closed route ownership
```

Non-diagnostic build smoke:

```text
Source:
  docs/benchmarks/windows/paper/hz6_elastic_highwater_l1_nondiag_smoke/
    20260605_113908_hz6_capacity_matrix_windows.md

Result:
  larson_t16_main_1k:
    57.642M / 290,924 KB
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0

Read:
  The high-water fields and updates are correctly isolated from normal
  non-diagnostic build behavior.
```

### 2026-06-05: MainWarmupCapacity-L1 diagnostic

Goal:

```text
Capture the main allocator snapshot after main-thread warmup and before worker
handoff. This closes the gap left by worker/final high-water snapshots.

This is diagnostic-only:
  [HZ6_MAIN_WARMUP_CAPACITY] is emitted only under HZ6_DIAGNOSTIC_PROBES.
  No allocator behavior changes.
```

Implementation:

```text
bench_larson_compare:
  after main warmup, snapshot the main thread HZ6 allocator.
  emit [HZ6_MAIN_WARMUP_CAPACITY].

run_win_hz6_capacity_matrix:
  captures the line and adds "HZ6 main-warmup capacity audit".
```

Smoke:

```text
Source:
  docs/benchmarks/windows/paper/hz6_main_warmup_capacity_l1_smoke/
    20260605_115500_hz6_capacity_matrix_windows.md

Result:
  larson_t16_main_1k:
    main warmup:
      descriptor_used=16,016
      route_occupied=17,017
      source_block_used=1,001
      frontcache_used=16
    final/worker:
      descriptor max=160
      route max=670
      source max=10
      frontcache max=161
    safety clean
```

Full 10k check:

```text
Source:
  docs/benchmarks/windows/paper/hz6_main_warmup_capacity_l1_full10k/
    20260605_115543_hz6_capacity_matrix_windows.md

Result:
  larson_t16_main_10k:
    42.898M / 412,828 KB
    safety clean

  main warmup:
    descriptor_used=160,048
    route_occupied=170,051
    source_block_used=10,003
    frontcache_used=48

  final/worker:
    descriptor max=400
    route max=5,425
    source max=25
    frontcache max=401
```

Decision:

```text
KEEP as diagnostic evidence.

Read:
  The low-cap failures are not explained by final steady-state pressure.
  They are main-warmup owner concentration. ElasticCapacity-L1 must support
  a temporary shared overflow/depot for descriptor/route/source metadata.

  SourceBlock-only overflow would be incomplete:
    main warmup source pressure is 10,003 blocks
    but descriptor and route also spike to 160k/170k

Next:
  Design a combined shared overflow contract:
    local small steady-state tables
    shared burst metadata for main-warmup
    handoff/rehome drains to worker-local or shared-retained state
    fail-closed route INVALID/MISS preserved
```

### 2026-06-05: ElasticProjection local-cap controls

Goal:

```text
Use CapacityUtil-L1 to test whether the selected Larson lane can simply lower
per-worker static capacities, before implementing shared/elastic overflow.

This is a control track:
  no production promotion
  no shared overflow yet
  tests where local-only capacity breaks
```

Lane 1:

```text
ownerlocalityfast-rsscap-2-elasticproj-local1k-route16k-source64-front1k-packed

Caps:
  descriptor 1024
  route 16384
  source block 64
  frontcache bin 1024
  selected packed/owner/source flags

Result:
  main-warmup 1k:
    warmup_alloc_fail
    descriptor_exhausted=2
    source_block_exhausted=1
    source_alloc=64

  worker-warmup 1k:
    warmup_alloc_fail
    descriptor_exhausted=2
    source_block_exhausted=1
```

Read:

```text
The final-snapshot max/cap2x projection was too optimistic for warmup live-set
pressure. The lane is useful as a no-go/control, but not a behavior candidate.
```

Lane 2:

```text
ownerlocalityfast-rsscap-2-elasticproj-live2k-route16k-source128-front1k-packed

Caps:
  descriptor 2048
  route 16384
  source block 128
  frontcache bin 1024
  selected packed/owner/source flags
```

Smoke:

```text
Source:
  docs/benchmarks/windows/paper/hz6_elasticproj_l1_smoke/
    20260605_092158_hz6_capacity_matrix_windows.md

main-warmup 1k:
  failed:parse
  read:
    still not enough for cross-owner main-thread warmup.
    The main allocator must seed multiple worker live sets before rehome.

worker-warmup 1k:
  55.585M / 60952 KB
  safety clean:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0

  residual:
    static table = 30598 KiB
    static+payload = 104326 KiB
    descriptor = 2176 KiB
    route = 6656 KiB
    source block = 256 KiB
    frontcache = 6150 KiB
    payload = 73728 KiB

  utilization:
    descriptor used/cap = 18,432 / 32,768 = 56.25%
    route active/cap = 19,584 / 262,144 = 7.47%
    source blocks/cap = 1,152 / 2,048 = 56.25%
    frontcache used/cap = 2,432 / 262,144 = 0.93%
```

Decision:

```text
KEEP as boundary evidence.

Read:
  Local-only low capacity is viable for same-owner 1k.
  It is not enough for cross-owner main-warmup because the main allocator
  carries the initial live-set seeding pressure.

Next design:
  HZ6 ElasticCapacity-L1 needs either:
    A. shared overflow descriptor/source-block allocation for warmup pressure
    B. benchmark/lifecycle-aware worker preseed path for same-owner lanes
    C. a source-backed global descriptor/source-block depot

Do not:
  promote elasticproj-local-only lanes.
  shrink selected full-10k local capacities without overflow.
```

### 2026-06-05: Larson packed source-cap trim L1

Goal:

```text
Use CapacityUtil-L1 to trim only SourceBlock local capacity in the selected
combined packed Larson lane:

  keep descriptor 160k
  keep route 192k
  keep frontcache 4k
  keep routepacked / routebytes16 / storageowner16 / ownersourcel2
  keep frontcachepacked / sourceblockpacked
  vary source block capacity
```

Results, run=1:

```text
source16k control:
  docs/benchmarks/windows/paper/hz6_source2k_packed_probe/
    20260605_101136_hz6_capacity_matrix_windows.md
  43.514M / 426088 KB
  source_block_table_bytes = 32768 KiB
  safety clean

source2k:
  same source as above
  warmup_alloc_fail
  source_block_exhausted = 257
  source_block_fail_active_max = 2048
  source_alloc = 6144

source8k:
  docs/benchmarks/windows/paper/hz6_source8k_packed_probe/
    20260605_101450_hz6_capacity_matrix_windows.md
  warmup_alloc_fail
  source_block_exhausted = 257
  source_block_fail_active_max = 8192
  source_alloc = 12288

source12k:
  docs/benchmarks/windows/paper/hz6_source12k_packed_probe/
    20260605_101638_hz6_capacity_matrix_windows.md
  43.910M / 417332 KB
  source_block_table_bytes = 24576 KiB
  safety clean

source10k:
  docs/benchmarks/windows/paper/hz6_source10k_packed_probe/
    20260605_101906_hz6_capacity_matrix_windows.md
  47.375M / 412736 KB
  source_block_table_bytes = 20480 KiB
  safety clean:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0

source10k repeat-3 confirmation:
  docs/benchmarks/windows/paper/hz6_source10k_packed_repeat/
    20260605_102400_hz6_capacity_matrix_windows.md
  44.864M / 412280 KB
  safety clean:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    descriptor_exhausted=0
    route_register_fail=0
    source_block_exhausted=0
  residual:
    static table = 234502 KiB
    static+payload = 258950 KiB
    source block table = 20480 KiB

source10k guard matrix:
  docs/benchmarks/windows/paper/hz6_source10k_guard/
    20260605_103956_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/hz6_source10k_guard_1k/
    20260605_104036_hz6_capacity_matrix_windows.md

  larson_t16_worker_10k:
    45.707M / 412128 KB
    safety clean

  larson_t16_main_4k:
    48.305M / 331152 KB
    safety clean

  larson_t16_worker_4k:
    51.418M / 331100 KB
    safety clean

  larson_t16_main_1k:
    55.927M / 290380 KB
    safety clean

  larson_t16_worker_1k:
    55.836M / 290372 KB
    safety clean
```

Decision:

```text
Promote source10k as the current Larson combined packed minimum-RSS sibling.
It is repeat-3 clean and keeps the combined packed RSS improvement while
trimming SourceBlock table capacity from source16k to source10k.
Guard rows are clean for 10k worker, 4k main/worker, and 1k main/worker.

Keep:
  source12k as the safety backup / boundary control.

No-go/control:
  source2k and source8k.

Read:
  Final active SourceBlock usage is tiny, but cross-owner main-warmup has a
  transient single-allocator SourceBlock pressure above 8k. Source10k is the
  first tested local-only capacity that survives that warmup pressure while
  cutting about 13 MB peak RSS versus source16k in the same probe family.
```

### 2026-06-05: SourceBlockPackedFlags-L1 candidate

Goal:

```text
Continue HZ6 Larson lowest-RSS lane cleanup after FrontCachePackedMeta-L1.

Observation:
  OwnerSourceSideMeta-L2 already removes the descriptor-storage owner scan in
  the selected lane. The next clean static RSS target is SourceBlock entry
  shape, not another owner lookup knob.
```

Implementation:

```text
Flag:
  HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1

Change:
  SourceBlock now has accessor helpers for:
    source_kind
    active
    route_registered
    run_active

  With the flag enabled, those four values are packed into
  `source_state_flags`.

Preserved:
  source_release stays inline
  OwnerSourceSideMeta-L2 pointer stays inline
  source-run bitmap/slot metadata stays inline
  source-run reuse / route invalid-envelope / release semantics unchanged
```

Validation:

```text
Default Windows HZ6 smoke build:
  hakozuna-hz6/win/build_win_hz6_r1_smokes.ps1 -SkipRun
  result: build OK

Packed SourceBlock smoke:
  flags:
    HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1
    HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1
    HZ6_SOURCE_RUN_REUSE_L1=1
    HZ6_SOURCE_RUN_SLOT_LOOKUP_L1=1
  result:
    hz6-r1-sourceblock-smoke ok

Packed safety smoke:
  plus:
    HZ6_ROUTE_PACKED_META_L1=1
    HZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1
    HZ6_OWNER_SOURCE_SIDE_META_L2=1
  result:
    hz6-r1-safety-smoke ok
```

Metadata check:

```text
Same diagnostic bench flags except packed on/off:

unpacked:
  source_block_entry_bytes = 144
  source_block_table_bytes = 2304  # small-cap diagnostic

packed:
  source_block_entry_bytes = 128
  source_block_table_bytes = 2048  # small-cap diagnostic

Projected selected source16k / 16-thread effect:
  16 bytes * 16384 source blocks * 16 allocators ~= 4 MiB metadata reduction
```

Smoke result:

```text
Command:
  win/run_win_hz6_capacity_matrix.ps1
    -Families larson
    -BenchmarkProfiles larson_t16_main_10k
    -Hz6Profiles speed
    -CapacityLanes ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512
    -Runs 1

Result:
  47.620M ops/s
  435768 KB

Source:
  docs/benchmarks/windows/paper/20260605_042827_hz6_capacity_matrix_windows.md
```

Decision:

```text
KEEP as lower-RSS candidate:
  SourceBlockPackedFlags-L1 is safety/build clean and produces the expected
  `144 -> 128` SourceBlock entry reduction.

Not promoted yet:
  Needs repeat-3 closeout against:
    OwnerSourceSideMeta-L2 baseline
    FrontCachePackedMeta-L1 candidate

Do not yet combine:
  FrontCachePackedMeta-L1 + SourceBlockPackedFlags-L1.
  Isolate SourceBlockPackedFlags first.

Next:
  Run repeat-3 closeout for:
    larson-cross-owner-selected
    or a smaller explicit A/B:
      ownersourcel2
      ownersourcel2-frontcachepacked
      ownersourcel2-sourceblockpacked
```

### 2026-06-05: SourceBlockPackedFlags closeout and combined packed candidate

Closeout:

```text
Command:
  win/run_win_hz6_selected_family.ps1
    -LarsonCrossOwnerSelected
    -Runs 3
    -ForceBuild

Source:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-sourceblockpacked-closeout/
      larson-cross-owner-selected/
        20260605_045708_hz6_capacity_matrix_windows.md

Rows:
  desc160k:
    46.721M / 753204 KB

  front4k:
    42.731M / 716328 KB

  OwnerSourceSideMeta-L2:
    44.355M / 439916 KB

  FrontCachePackedMeta-L1:
    41.131M / 430692 KB

  SourceBlockPackedFlags-L1:
    41.070M / 435304 KB
```

Read:

```text
SourceBlockPackedFlags-L1 is clean and reduces RSS versus OwnerSourceSideMeta-L2,
but FrontCachePackedMeta-L1 is still lower RSS by itself. Because the two
packed candidates target separate metadata tables, test the combined lane.
```

Combined lane:

```text
Lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512

Flags:
  HZ6_FRONTCACHE_PACKED_META_L1=1
  HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1
  plus the OwnerSourceSideMeta-L2 selected-lane flags
```

Validation:

```text
1-run smoke:
  40.521M / 426536 KB
  safety clean

Repeat-3:
  docs/benchmarks/windows/paper/20260605_051427_hz6_capacity_matrix_windows.md

  40.837M / 426084 KB
  safety clean
```

Decision:

```text
KEEP:
  Combined packed is the current clean Larson minimum-RSS candidate/sibling.

Do not claim:
  broad throughput promotion.

Use:
  paper/ledger as the minimum-RSS HZ6 Larson full-10k row.

Next:
  Do not add another packing lane yet.
  First run LarsonRssResidualAudit-L1 to identify the remaining RSS source.
  Only after that choose route/directory slimming, descriptor side metadata
  compaction, source retention/scavenge, or a broader ownership redesign.
```

### 2026-06-05: Pro consult after combined packed lane cleanup

Decision:

```text
OwnerSourceSideMeta-L2:
  keep as Larson cross-owner speed/RSS balance sibling

combined packed:
  keep as selected minimum-RSS sibling/candidate
  not universal default
  not broad throughput promotion

FrontCachePackedMeta-L1:
  keep as component control/evidence

SourceBlockPackedFlags-L1:
  keep as component control/evidence
```

Acceptance for combined packed as minimum-RSS paper/profile lane:

```text
safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  alloc_fail = 0
  descriptor_exhausted = 0
  source_block_exhausted = 0
  remote_free_transfer_fail = 0

performance:
  throughput >= OwnerSourceSideMeta-L2 - 3%
  or repeat-3 median stays >= 40M ops/s

RSS:
  peak <= OwnerSourceSideMeta-L2 - 10 MiB
  strong if <= OwnerSourceSideMeta-L2 - 13 MiB
  combined packed remains lower RSS than either component alone

hygiene:
  diagnostic-only counters/output stay out of non-diagnostic speed lanes
```

Next high-ROI experiment:

```text
LarsonRssResidualAudit-L1

Scope:
  behavior unchanged
  diagnostic-only
  compare:
    OwnerSourceSideMeta-L2
    FrontCachePackedMeta-L1
    SourceBlockPackedFlags-L1
    combined packed

Audit static tables:
  descriptor hot/cold table bytes
  route table bytes
  shared directory bytes
  owner-locality index bytes
  source block table bytes
  source-run metadata bytes
  frontcache table bytes
  transfer table bytes

Audit runtime retention:
  active descriptors
  local/free/cached descriptors if available
  active source blocks
  ref-nonzero/ref-zero source blocks if available
  payload reserved bytes
  payload committed estimate
  route entries active/tombstone if available
  frontcache total/largest if available
```

Read:

```text
If route/shared directory dominates:
  try RouteDirectorySlim / OwnerLocalityDirectorySlim.

If descriptor side metadata dominates:
  try OwnerSourceSideMeta compaction or descriptor owner representation
  redesign.

If source-block table dominates:
  try SourceBlockSlim-L2 only if the residual is large enough.

If payload/source retention dominates:
  try source retention / scavenge checkpoint.

If no single table dominates:
  stop adding micro-packing lanes and return to speed/RSS balance work.
```

Implementation:

```text
Added diagnostic-only residual attribution:
  [HZ6_RSS_RESIDUAL]

New snapshot fields:
  shared_route_directory_bytes
  owner_locality_index_bytes
  static_table_bytes
  static_plus_payload_bytes
  ref_zero_source_blocks

New selected-family preset:
  win/run_win_hz6_selected_family.ps1 -LarsonRssResidualAudit

The preset automatically enables DiagnosticHz6Probes so these counters do not
mix into normal speed lanes.
```

Initial 1-run audit:

```text
Command:
  win/run_win_hz6_selected_family.ps1
    -LarsonRssResidualAudit
    -Runs 1
    -TimeoutSeconds 90
    -ForceBuild
    -ContinueOnFailure

Source:
  docs/benchmarks/windows/paper/hz6_rss_residual_audit_l1/
    larson-rss-residual-audit/
      20260605_084006_hz6_capacity_matrix_windows.md
```

Residual read, run=1:

```text
OwnerSourceSideMeta-L2:
  throughput/RSS:
    47.926M / 439948 KB
  static_table:
    259078 KiB
  static_plus_payload:
    283590 KiB
  descriptor / route / sourceblock / frontcache:
    94208 / 79872 / 36864 / 32774 KiB
  payload:
    24512 KiB

FrontCachePackedMeta-L1:
  throughput/RSS:
    45.220M / 430780 KB
  static_table:
    250886 KiB
  static_plus_payload:
    275398 KiB
  descriptor / route / sourceblock / frontcache:
    94208 / 79872 / 36864 / 24582 KiB

SourceBlockPackedFlags-L1:
  throughput/RSS:
    47.347M / 435776 KB
  static_table:
    254982 KiB
  static_plus_payload:
    279494 KiB
  descriptor / route / sourceblock / frontcache:
    94208 / 79872 / 32768 / 32774 KiB

combined packed:
  throughput/RSS:
    44.431M / 426548 KB
  static_table:
    246790 KiB
  static_plus_payload:
    271238 KiB
  descriptor / route / sourceblock / frontcache:
    94208 / 79872 / 32768 / 24582 KiB
  shared_dir / owner_index / transfer:
    7680 / 6144 / 1536 KiB
  payload:
    24448 KiB
```

Read:

```text
The combined packed lane is doing exactly what expected:
  FrontCachePacked saves about 8192 KiB of static table bytes.
  SourceBlockPacked saves about 4096 KiB of static table bytes.
  Combined saves both, dropping static table bytes to about 246790 KiB.

The largest remaining explicit table components are:
  descriptor table:
    about 94208 KiB
  route table:
    about 79872 KiB
  source block table:
    about 32768 KiB
  frontcache table:
    about 24582 KiB after packing

The measured peak RSS is still about 155 MiB above static_plus_payload in the
combined packed run. Before adding another micro-packing lane, we need to
separate process/CRT/thread stack overhead from allocator-retained backing.
```

Next:

```text
1. Use residual audit rows as the new RSS map.
2. If continuing RSS:
   first inspect descriptor/route representation or non-table retained memory.
3. Do not start another FrontCache/SourceBlock flag packing lane unless the
   audit shows a new static table target larger than the expected win.
```

This file is intentionally the historical experiment ledger. Stable decisions
should be copied into the selected summary or lane guide instead of being left
only in this file.

Superseded historical attack plan:

```text
Latest candidate:
  FrontCachePackedMeta-L1 over OwnerSourceSideMeta-L2.

  Purpose:
    reduce the remaining Larson metadata gap by shrinking
    Hz6FrontCacheEntry from 32 bytes to 24 bytes.

  Lane:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
    noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
    ownersourcel2-frontcachepacked-source16k-route192k-run512

  Implementation:
    HZ6_FRONTCACHE_PACKED_META_L1=1
    packs frontcache bytes-minus-one, class id, and descgov detached flag
    into a 32-bit entry meta word.

  Validation so far:
    HZ6 R1 Windows smokes pass.
    tiny direct diagnostic:
      frontcache_entry_bytes = 24
      safety counters = 0
    Larson T16 main-warmup 1k direct diagnostic:
      56.245M ops/s
      frontcache_table_bytes = 25171968
      baseline L2 diagnostic frontcache_table_bytes was 33560576
      descriptor_table_bytes = 96468992
      route_table_bytes = 81788928
      source_block_table_bytes = 37748736
      ownerlocality_index_bytes = 14155776
      route_invalid = 0
      route_miss = 0
      remote_free_transfer_fail = 0
      alloc_fail = 0
    Larson T16 main-warmup full 10k direct non-diagnostic, run=1:
      41.660M ops/s
      safety counters = 0
    Larson T16 main-warmup full 10k direct closeout, same exe pair:
      baseline OwnerSourceSideMeta-L2 median:
        46.467M ops/s
        peak_kb = 439912
        safety counters = 0
      FrontCachePackedMeta-L1 median:
        44.831M ops/s
        peak_kb = 430708
        safety counters = 0

  Read:
    promising lowest-RSS sibling evidence.
    It cuts about 9 MiB versus OwnerSourceSideMeta-L2 in the direct closeout,
    but costs about 3.5% median throughput in that same comparison.
    Keep OwnerSourceSideMeta-L2 as the selected throughput/RSS balance lane;
    keep FrontCachePackedMeta-L1 as the current lowest-RSS candidate/sibling.
    The matrix runner can overrun because Larson runtime is fixed at 10s and
    process output has a trailing sleep; direct runs were used for this first
    check.

Selected lane to preserve:
  OwnerSourceSideMeta-L2 over routebytes16/routepacked/no-routebackptr/dir192k
    same-run repeat-3 Larson T16 main-warmup:
      routebytes16 comparison control:
        40.750M ops/s
        449128 KB peak
      storageowner16 + ownersourcel2:
        40.754M ops/s
        439912 KB peak
    safety clean

RSS reference:
  HZ5 policy remains the stronger Larson RSS reference:
    HZ5 Larson main-warmup paper row:
      47.485M ops/s
      183180 KB peak

Current HZ6 gap:
  HZ6 is now within about 14% of HZ5 throughput on the selected Larson lane,
  but uses about 2.40x HZ5 peak RSS. The remaining gap is metadata/lifecycle
  dominated, not payload dominated.

Next:
  HZ6 OwnerSourceSideMeta-L2 selected-lane closeout and commit.

Goal:
  keep the routebytes16 comparison-control hot path and test whether descriptor owner
  side metadata can be made owner-source-aware without paying the StorageOwner16
  lookup cost on every hot owner read.

OwnerSourceSideMeta-L2 implementation:
  HZ6_OWNER_SOURCE_SIDE_META_L2=1
  HZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1
  source blocks carry owner_source_storage_allocator
  descriptor owner reads use descriptor->source_block as an O(1) descriptor
  storage hint before falling back to the StorageOwner16 scan

Lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-source16k-route192k-run512

Validation lane:
  same lane plus HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1
  suffix:
    ownersourcel2dryrun-source16k-route192k-run512

Initial results:
  Larson T16 main-warmup 1k diagnostic, run=1:
    routebytes16 comparison control:
      55.881M ops/s
      327248 KB peak
    storageowner16 + ownersourcel2:
      56.348M ops/s
      318488 KB peak
      owner_source_side_meta_l2_lookup = 145018652
      owner_source_side_meta_l2_hit = 145018652
      l2_miss_no_block = 0
      l2_miss_inactive = 0
      l2_storage_mismatch = 0

  Larson T16 main-warmup full 10k non-diagnostic, run=1:
    routebytes16 comparison control:
      43.911M ops/s
      449128 KB peak
    storageowner16 + ownersourcel2:
      44.546M ops/s
      440364 KB peak

  Larson T16 main-warmup full 10k diagnostic dry-run comparison, run=1:
    44.239M ops/s
    439936 KB peak
    owner_source_side_meta_l2_lookup = 1253200849
    owner_source_side_meta_l2_hit = 1253200849
    owner_source_side_meta_l2_miss_no_block = 0
    owner_source_side_meta_l2_miss_inactive = 0
    owner_source_side_meta_l2_storage_mismatch = 0
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    remote_free_transfer_fail = 0
    lifecycle_foreign_free_invalid = 0
    alloc_fail = 0
    descriptor_exhausted = 0

Read:
  OwnerSourceSideMeta-L2 is promoted to the current Larson lowest-RSS selected
  sibling. It preserves the StorageOwner16 ownerless descriptor shape while
  avoiding the scan-based hot read on source-block descriptors.

Promotion evidence:
  Larson T16 main-warmup full 10k non-diagnostic, repeat-3:
    routebytes16 comparison control:
      40.750M ops/s
      449128 KB peak
    storageowner16 + ownersourcel2:
      40.754M ops/s
      439912 KB peak

  Larson T16 worker-warmup full 10k non-diagnostic, run=1:
    routebytes16 comparison control:
      40.126M ops/s
      448948 KB peak
    storageowner16 + ownersourcel2:
      40.787M ops/s
      439740 KB peak

  Diagnostic validation:
    owner_source_side_meta_l2_lookup = 1253200849
    owner_source_side_meta_l2_hit = 1253200849
    owner_source_side_meta_l2_miss_no_block = 0
    owner_source_side_meta_l2_miss_inactive = 0
    owner_source_side_meta_l2_storage_mismatch = 0
    safety counters = 0

Closed L2 dry-run:
  route_bytes side array:
    current = uint32_t[route_capacity]
    projected = uint16_t[route_capacity] if all registered route bytes fit
                UINT16_MAX, otherwise count overflow.

Observation:
  Larson T=16 main-warmup 1k diagnostic, selected routepacked lane:
    route_bytes16_table_bytes = 6291456
    route_bytes16_savings_bytes = 6291456
    route_bytes16_active_checked = 10499
    route_bytes16_overflow = 147
    route_bytes16_max = 65536

  Raw uint16_t route_bytes is no-go because 64KiB source-block invalid
  envelopes overflow UINT16_MAX by one.

  Biased uint16_t route_bytes_minus1 is the next behavior candidate:
    route_bytes16_minus1_overflow = 0
    route_bytes16_minus1_zero = 0
    route_bytes16_minus1_max_stored = 65535

Promoted behavior lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512

  Mechanism:
    HZ6_ROUTE_BYTES16_MINUS1_L2 stores route bytes as uint16_t(bytes - 1)
    behind the existing route accessor boundary.

  Larson T=16 main-warmup full 10k A/B, runs=1:
    routepacked:
      44.140M ops/s
      456040 KB peak
      safety clean
    routepacked-routebytes16:
      44.392M ops/s
      449132 KB peak
      safety clean

  Larson T=16 main-warmup full 10k A/B, runs=3:
    routepacked:
      45.079M ops/s
      456040 KB peak
      safety clean
    routepacked-routebytes16:
      48.367M ops/s
      449144 KB peak
      safety clean

  Read:
    RoutePackedMeta-L2 routebytes16 became the clean comparison-control
    sibling. OwnerSourceSideMeta-L2 later superseded it for selected Larson
    lowest RSS. RoutePackedMeta-L1 remains the lower-level comparison control.

Acceptance to proceed to behavior:
  achieved for L2 implementation and repeat-3 promotion:
    routebytes16 safety clean
    median RSS below RoutePackedMeta-L1
    median throughput above RoutePackedMeta-L1 in same-run repeat-3

Do not:
  continue static route-capacity trimming
  promote storageowner16 by default
  move descriptor pointer or generation out of the hot route entry yet
  reopen allocator-local side-owner16
```

OwnerSourceSideMeta-L1 dry-run:

```text
Implementation:
  HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1
  diagnostic-only
  no behavior change
  counts projected side-owner storage/source ownership lookups on the selected
  routebytes16 lane

Lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-
  source16k-route192k-run512

Larson T=16 main-warmup 1k smoke:
  56.379M ops/s
  327696 KB peak
  owner_source_side_meta_lookup = 115067026
  owner_source_side_meta_foreign = 97335556
  owner_source_side_meta_miss = 0
  owner_source_side_meta_probe_max = 1
  safety clean

Larson T=16 main-warmup full 10k run=1:
  46.202M ops/s
  449164 KB peak
  owner_source_side_meta_lookup = 925750890
  owner_source_side_meta_local = 53771176
  owner_source_side_meta_foreign = 871979714
  owner_source_side_meta_miss = 0
  owner_source_side_meta_probe_total = 871979714
  owner_source_side_meta_probe_max = 1
  descriptor_storage_lookup = 462912401
  descriptor_storage_probe_total = 691140834
  descriptor_storage_probe_max = 17
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  remote_free_transfer_fail = 0
  lifecycle_foreign_free_invalid = 0
  alloc_fail = 0
  descriptor_exhausted = 0

Read:
  Owner side metadata is still plausible, but the design must eliminate hot
  scan-based storage owner reads. The dry-run says the owner-source lookup is
  resolvable and miss-free, but extremely frequent. Next design should make
  descriptor storage/source owner O(1), not repeat StorageOwner16 scan behavior.
```

Latest Larson StorageOwner16-L1 evidence:

```text
Goal:
  test whether side-owner metadata can be made safe by looking up the
  descriptor storage allocator instead of keying the side table by the current
  or route allocator.

Implementation:
  HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
    removes owner from the hot descriptor like side-owner16
    stores packed owner slot/generation in descriptor_side_owner16[]
    resolves the side table through descriptor storage ownership when needed

Lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
  storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512

Results:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-storageowner16-l1-smoke/
      20260604_183152_hz6_capacity_matrix_windows.md
    larson-storageowner16-l1-full10k/
      20260604_183219_hz6_capacity_matrix_windows.md
    larson-storageowner16-l1-repeat/
      20260604_183328_hz6_capacity_matrix_windows.md

  repeat-3 full 10k:
    42.024M ops/s
    444520 KB peak
    route_invalid = 0
    route_miss = 0
    remote_free_transfer_fail = 0
    lifecycle_foreign_free_invalid = 0

Decision:
  KEEP as RSS-first descriptor side metadata evidence/control.
  Do not replace RoutePackedMeta-L1 as selected Larson lowest-RSS sibling:
  storageowner16 saves about 11.5 MB RSS versus routepacked repeat-3 but loses
  about 12% throughput.

Next:
  no more allocator-local side-owner table work.
  if descriptor RSS is reopened, reduce storage-owner lookup cost or design a
  first-class owner-source side metadata map; keep routepacked as selected.
```

Latest Larson RoutePackedMeta-L1 decision:

```text
Goal:
  reduce full-10k Larson cross-owner static route metadata without touching
  remote handoff behavior or descriptor ownership semantics.

Implementation:
  HZ6_ROUTE_PACKED_META_L1
    Hz6RouteEntry hot payload:
      base
      descriptor
      generation
      packed meta(front_id, class_id, active, exact_valid, tombstone)

    route bytes:
      moved to allocator-owned side array route_bytes[].

  Keep descriptor in the hot entry.
  Keep generation in the hot entry.
  Do not move route ownership or descriptor ownership in this lane.

Safety / build:
  default Windows HZ6 R1 smokes: PASS
  packed lane 1k diagnostic smoke:
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    route_entry_bytes = 24

Results:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-routepacked-l1-smoke/
      20260604_174821_hz6_capacity_matrix_windows.md
    larson-routepacked-l1-full10k/
      20260604_174858_hz6_capacity_matrix_windows.md
    larson-routepacked-l1-repeat/
      20260604_175016_hz6_capacity_matrix_windows.md

  repeat-3 full 10k:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
    noroutebackptr-dir192k-routepacked-source16k-route192k-run512:
      47.616M ops/s
      456048 KB peak

Decision:
  PROMOTE as current Larson cross-owner lowest-RSS sibling candidate.
  Supersedes plain SourceBlockNoRouteBackptr-L1 for both speed and RSS.
  Keep plain no-routebackptr/dir192k as an isolation control.

Next:
  do not continue static route-capacity trimming.
  if Larson RSS is reopened, attack owner-source-aware descriptor side metadata
  or a broader route/descriptor ownership representation.
  selected-family / cross-allocator docs have been refreshed with the
  routepacked row in commit 1c3a802.
```

Latest mixed clean-lane decision:

```text
Goal:
  replace the old balanced / wide_ws descavail pressure row with a
  safety-clean selected lane.

New capacity lanes:
  mixedclean-front16k-sourcerun-desc17k-source2k-route17k
  mixedclean-front16k-sourcerun-desc18k-source2k-route18k
  mixedclean-front16k-sourcerun-desc20k-source2k-route20k
  mixedclean-front16k-sourcerun-desc22k-source2k-route22k
  mixedclean-front16k-sourcerun-desc24k-source2k-route24k
  mixedclean-front16k-sourcerun-desc32k-source2k-route32k
  mixedclean-front16k-sourcerun-desc32k-source3k-route32k
  mixedclean-front16k-sourcerun-desc32k-source4k-route32k

Initial repeat-3:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-l1-repeat/
      20260603_195317_hz6_capacity_matrix_windows.md

Result:
  source2k:
    balanced:
      56.937M ops/s
      122108 KB peak
    wide_ws:
      20.636M ops/s
      151820 KB peak
    safety:
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0

  source4k:
    balanced:
      64.129M ops/s
      131952 KB peak
    wide_ws:
      21.312M ops/s
      161248 KB peak
    safety:
      clean

Decision:
  superseded by the desc24 repeat below.
  Keep desc32/source2k and source4k as extra-capacity / speed controls.

Profile scan:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-profile-scan/
      20260603_200404_hz6_capacity_matrix_windows.md

  read:
    strict is no-go for mixed clean rows:
      wide_ws RSS jumps to about 575-584 MB.
    speed is effectively the same as rss for wide_ws.
    next improvement should come from capacity/source-retention shape, not
    from switching HZ6 profile flags.

Source capacity scan:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-source3k-scan/
      20260603_200610_hz6_capacity_matrix_windows.md

  source3k:
    balanced 60.420M / 127648 KB
    wide_ws  20.113M / 156504 KB
    clean
    read:
      no promotion. It is between source2k/source4k but does not improve
      wide_ws enough to justify the extra RSS.

Desc/route lower-bound scan:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-desc24-repeat/
      20260603_200812_hz6_capacity_matrix_windows.md

  desc24/source2k/route24:
    balanced:
      64.796M ops/s
      116188 KB peak
    wide_ws:
      21.161M ops/s
      145564 KB peak
    safety:
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0

  desc32/source2k/route32:
    balanced:
      63.037M ops/s
      122304 KB peak
    wide_ws:
      20.399M ops/s
      151636 KB peak
    safety:
      clean

Decision:
  superseded by desc20/18/17 scans below.

Desc20/22 lower-bound scan:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-desc20-22-repeat/
      20260603_201344_hz6_capacity_matrix_windows.md

  desc20/source2k/route20:
    balanced 66.395M / 112976 KB
    wide_ws  22.410M / 142596 KB
    safety clean

  desc22/source2k/route22:
    balanced 66.078M / 114692 KB
    wide_ws  21.183M / 144124 KB
    safety clean

  desc24/source2k/route24:
    balanced 64.493M / 116300 KB
    wide_ws  22.262M / 145644 KB
    safety clean

Desc18/17 lower-bound scan:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-desc17-repeat/
      20260603_201715_hz6_capacity_matrix_windows.md

  desc17/source2k/route17:
    balanced 64.117M / 110976 KB
    wide_ws  21.119M / 140256 KB
    safety clean

  desc18/source2k/route18:
    balanced 64.979M / 111524 KB
    wide_ws  20.398M / 140860 KB
    safety clean

  desc20/source2k/route20:
    balanced 59.888M / 113076 KB
    wide_ws  21.498M / 142676 KB
    safety clean

Desc16 transfer isolation:
  docs/benchmarks/windows/paper/hz6_selected_family/
    mixed-clean-desc16-transfer-scan/
      20260603_201856_hz6_capacity_matrix_windows.md

  result:
    desc16 base:
      wide_ws alloc_fail = 6943
    desc16 transfer2304:
      wide_ws alloc_fail = 6943
    desc16 transfer2560:
      wide_ws alloc_fail = 6943

  read:
    desc16 failure is not fixed by transfer-cache widening.
    desc17 is the current clean lower bound.

Decision:
  promote rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k as the
  selected balanced / wide_ws clean low-RSS lane.
  Keep desc20 as the wide-speed sibling/control.
  Keep desc18/22/24/32/source3/source4 as controls.
  Keep descavail-noboost-route4k as pressure evidence only: it is much faster
  and lower RSS, but not safety-clean because it finishes with large
  alloc_fail / source-block-exhaustion counts.

Script update:
  win/run_win_hz6_selected_family.ps1:
    selected-mixed-lowrss now uses the clean mixedclean desc17/source2k lane.
    selected-mixed-pressure preserves descavail pressure evidence.

Next:
  refresh the selected-family matrix with the desc17 mixed lane.
  then update the cross-allocator table so paper-facing rows separate:
    clean selected rows
    pressure evidence rows
    control/no-go rows
  after that, choose one focused optimization target:
    A. wide_ws throughput while preserving desc17 safety/RSS
    B. Larson RSS metadata/static-table reduction
```

ThinDescriptor-L1 is now implemented behind a profile flag. Plain
`ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc` stays compact/moderate
evidence because it fails full-10k warmup, while
`ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k` is promoted to
the selected low-RSS Larson sibling after the source-block recovery repeat-3.
It is still not a universal default; use it only for the Larson/owner-locality
profile family where peak RSS is the priority.

Latest design freeze:

```text
Selected-family state:
  mixed balanced/wide:
    desc17/source2k/route17 is selected clean low-RSS.
    descavail is pressure evidence only.

  random_mixed:
    sameownerfast-descavail remains selected.

  larger_sizes:
    largerlowrss-front8k remains selected.

  Larson:
    desc160k remains throughput/RSS selected.
    front4k-thindesc-source16k remains selected low-RSS sibling.

Next order:
  A. cross-allocator selected/control table cleanup
  B. pick one next optimization lane:
     wide_ws throughput or Larson RSS

Do not yet:
  make thindesc broad default
  promote descavail pressure rows as clean rows
  add runtime adaptive profile selection
```

Latest selected-family desc17 refresh:

```text
Run:
  win/run_win_hz6_selected_family.ps1 -SelectedFamily -Runs 3
  output root:
    docs/benchmarks/windows/paper/hz6_selected_family/
      selected-family-desc17-refresh/

Selected-family repeat-3 read:
  mixed_ws / rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k:
    balanced = 55.504M ops/s, 110780 KB peak, safety clean
    wide_ws  = 19.978M ops/s, 140236 KB peak, safety clean

  random_mixed / strict + sameownerfast-descavail-noboost-route4k:
    small  = 45.755M ops/s, 4968 KB peak, safety clean
    medium = 42.408M ops/s, 4964 KB peak, safety clean
    mixed  = 41.306M ops/s, 4964 KB peak, safety clean

  larger_sizes / largerlowrss-front8k-sourcerun-desc8k-route8k:
    speed = 26.404M ops/s, 71040 KB peak, safety clean
    rss   = 27.178M ops/s, 71012 KB peak, safety clean

  larson_t16_main_10k / speed:
    desc160k       = 44.754M ops/s, 808488 KB peak, safety clean
    front4k        = 45.092M ops/s, 716324 KB peak, safety clean
    thindesc-16k   = 44.609M ops/s, 665704 KB peak, safety clean
    route192k      = 44.610M ops/s, 628844 KB peak, safety clean control
    route192k-run512 = 48.512M ops/s, 499820 KB peak, safety clean selected

Safety:
  checked selected rows keep:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0

Decision:
  freeze this as the current paper-facing HZ6 selected-family snapshot.
  The earlier isolated mixed boundary and larger_sizes snapshots remain useful
  evidence, but paper-facing selected tables should use this refreshed matrix.
  Larson route192k is a later MetadataSlim-L1 improvement over the refresh
  snapshot. SourceBlockMetaSlim-L1 route192k-run512 is now the selected
  lowest-RSS Larson sibling.

Next:
  update cross-allocator selected/control tables, then choose one focused
  optimization target:
    A. wide_ws throughput while preserving desc17 safety/RSS
    B. Larson metadata layout slimming only after checking the next table
       bottleneck
```

Larson MetadataSlim-L1:

```text
Goal:
  reduce full-10k Larson RSS without losing completion or adding hot-path
  behavior.

Implementation:
  added route-capacity siblings for the selected thindesc/source16k shape:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k
    route96k / route64k are available as no-go controls but not in the default
    MetadataSlim preset.

Diagnostic run:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-metadata-slim-l1b/

  baseline thindesc-source16k:
    39.908M / 665708 KB, safety clean
  route224k:
    38.916M / 647728 KB, safety clean
  route192k:
    39.919M / 629272 KB, safety clean
  route160k:
    failed warmup; route table saturated
  route128k:
    failed warmup; route table saturated

Repeat-3:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-metadata-slim-route192-repeat/

  thindesc-source16k baseline:
    40.267M / 665700 KB
    safety clean

  route224k:
    40.168M / 647264 KB
    safety clean

  route192k:
    40.260M / 628828 KB
    safety clean

Decision:
  superseded by SourceBlockMetaSlim-L1 run512 below.
  Keep route192k as a clean route-capacity control.
  Keep route224k as a clean control.
  Keep route160k/route128k as no-go boundary evidence: full-10k warmup needs
  more than 160K route capacity under this static-table design.

Next:
  no more static route trimming for Larson.
  Further RSS reduction should come from metadata layout slimming, not another
  capacity cut.
```

SourceBlockMetaSlim-L1:

```text
Goal:
  reduce full-10k Larson RSS by shrinking source-run bitmap metadata rather
  than trimming route capacity further.

Implementation:
  added `HZ6_SOURCE_RUN_MAX_SLOTS` ladder on top of route192k:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512

Diagnostic run:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-sourcerun-metaslim-l1/

  route192k:
    47.649M / 628852 KB
    source_block_table_bytes = 155189248

  run2048:
    44.677M / 555568 KB
    source_block_table_bytes = 88080384

  run1024:
    45.686M / 518708 KB
    source_block_table_bytes = 54525952

  run512:
    43.742M / 500264 KB
    source_block_table_bytes = 37748736

Repeat-3:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-sourcerun-metaslim-repeat/

  route192k:
    44.610M / 628844 KB
    safety clean

  route192k-run1024:
    44.396M / 518256 KB
    safety clean

  route192k-run512:
    48.512M / 499820 KB
    safety clean

Decision:
  promote route192k-run512 as the selected Larson lowest-RSS sibling.
  Keep route192k as route-capacity control.
  Keep run1024 as source-run metadata control.

Default check:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-lowest-rss-default-check/

  larson-cross-owner-lowest-rss now includes:
    front4k
    route192k
    route192k-run512

  one-run confirmation:
    front4k:
      40.157M / 716312 KB
    route192k:
      40.755M / 628836 KB
    route192k-run512:
      40.688M / 499812 KB
      safety clean

Next:
  SourceBlock table is no longer the same dominant gap. Re-run metadata
  attribution before choosing the next table/layout target.
```

Run512 attribution check:

```text
Run:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-run512-attribution-check/

Result:
  route192k-run512:
    40.978M / 499820 KB
    safety clean

Memory attribution:
  descriptor_table_bytes       = 127926272
  route_table_bytes            = 100663296
  source_block_table_bytes     = 37748736
  frontcache_table_bytes       = 33560576
  ownerlocality_index_bytes    = 18874368
  transfer_table_bytes         = 1572864
  source_block_payload_bytes   = 24969216

Read:
  SourceBlock metadata is no longer the main static-table gap after run512.
  The next candidates are descriptor table and route table.

Next:
  do not continue blind source-run slot cuts.
  First check whether route capacity can be retested under the run512 shape,
  then consider descriptor table/lifecycle work.
```

Historical selected-family measurement before desc17 refresh:

```text
Run:
  win/run_win_hz6_selected_family.ps1 -SelectedFamily -Runs 3
  outputs:
    docs/benchmarks/windows/paper/hz6_selected_family/
      selected-mixed-lowrss/
        20260603_192651_hz6_capacity_matrix_windows.md
      selected-random-sameowner/
        20260603_192652_hz6_capacity_matrix_windows.md
      selected-larger-lowrss/
        20260603_192657_hz6_capacity_matrix_windows.md
      larson-cross-owner-selected/
        20260603_192657_hz6_capacity_matrix_windows.md

Selected-family repeat-3 read:
  mixed_ws balanced / rss + descavail-noboost-route4k:
    75.467M ops/s
    17376 KB peak
    not safety-clean:
      alloc_fail = 1509729
      descriptor_exhausted = 3019459
      source_block_exhausted = 1509730

  mixed_ws wide_ws / rss + descavail-noboost-route4k:
    57.144M ops/s
    12524 KB peak
    not safety-clean:
      alloc_fail = 1504489
      descriptor_exhausted = 3009031
      source_block_exhausted = 1504644

  random_mixed / strict + sameownerfast-descavail-noboost-route4k:
    small  = 45.788M ops/s, 4964 KB peak, safety clean
    medium = 42.895M ops/s, 4964 KB peak, safety clean
    mixed  = 41.541M ops/s, 4968 KB peak, safety clean

  larger_sizes / largerlowrss-front8k-sourcerun-desc8k-route8k:
    speed = 31.899M ops/s, 70928 KB peak, safety clean
    rss   = 32.260M ops/s, 70952 KB peak, safety clean

  larson_t16_main_10k / speed:
    desc160k       = 45.122M ops/s, 808484 KB peak, safety clean
    front4k        = 45.138M ops/s, 716324 KB peak, safety clean
    thindesc-16k   = 44.549M ops/s, 665712 KB peak, safety clean

Decision:
  random_mixed, larger_sizes, and Larson selected families are clean enough to
  keep as current profile-family rows.
  mixed balanced / wide_ws descavail remains valuable pressure evidence but is
  not a clean selected/default row because the high speed comes with massive
  allocation failure and source-block exhaustion.

Next:
  freeze this selected-family snapshot.
  attack a clean balanced / wide_ws low-RSS lane next, or relabel those rows
  explicitly as capacity-failure pressure rows in paper-facing tables.

Historical Larson source-block recovery:

Run:
  win/run_win_hz6_selected_family.ps1 -LarsonCrossOwnerSelected -Runs 3
  profile:
    larson_t16_main_10k
  output:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-cross-owner-selected/
        20260603_174948_hz6_capacity_matrix_windows.md

Results:
  ownerlocalityfast-rsscap-2-desc160k:
    43.506M ops/s
    808484 KB peak
    safety clean

  ownerlocalityfast-rsscap-2-desc160k-front4k:
    42.925M ops/s
    716332 KB peak
    safety clean
    read:
      selected full-10k lower-RSS sibling

  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc:
    failed 3/3 before timed run
    label:
      larson_warmup_alloc_fail
    source_alloc:
      12288
    alloc_fail:
      1
    descriptor_exhausted:
      0
    route_register_fail:
      0
    source_block_exhausted:
      257

Decision:
  thindesc does not become the full-10k Larson paper lane.
  front4k is the current lower-RSS full-10k sibling.
  Next design pressure is not descriptor thinness; it is source-block
  capacity / retention when thin descriptors are combined with owner-locality.

Next source-block recovery experiment:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k

Purpose:
  check whether thindesc full-10k failure is only source-block capacity, while
  keeping frontcache at 4k and descriptor capacity at 160k.

Acceptance:
  warmup passes
  alloc_fail = 0
  source_block_exhausted = 0
  throughput >= front4k - 5%
  peak RSS is below or near front4k

Source-block recovery result:
  run:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-thindesc-sourcecap/
        20260603_180307_hz6_capacity_matrix_windows.md
  repeat:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-thindesc-source16k-repeat/
        20260603_180630_hz6_capacity_matrix_windows.md

  front4k:
    repeat-3 median:
      44.887M ops/s
      716328 KB peak
    safety:
      clean

  thindesc-source16k:
    repeat-3 median:
      46.819M ops/s
      665704 KB peak
    safety:
      clean
    read:
      GO as selected low-RSS Larson sibling.
      It fixes the thindesc full-10k source-block failure and improves both
      throughput and RSS versus front4k in this repeat.

  thindesc-source32k:
    run-1:
      42.029M ops/s
      836644 KB peak
    read:
      no-go / over-retention control.

Decision:
  promote source16k to selected-family low-RSS Larson sibling.
  keep plain thindesc as compact/moderate evidence only.
  keep source32k as over-retention control.

Source-cap tightening:
  run:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-thindesc-source-tighten/
        20260603_190043_hz6_capacity_matrix_windows.md
  repeat:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-thindesc-source12k-repeat/
        20260603_190401_hz6_capacity_matrix_windows.md

  source12k:
    repeat-3 median:
      44.308M ops/s
      623084 KB peak
    safety:
      clean
    read:
      lower RSS than source16k, but lower throughput.
      Keep as lowest-RSS control, not selected default.

  source14k:
    run-1:
      44.471M ops/s
      644836 KB peak
    safety:
      clean
    read:
      interpolation control; no repeat needed unless we want a mid-point table.

  source16k:
    repeat-3 in same comparison:
      47.040M ops/s
      665712 KB peak
    read:
      remains selected throughput/RSS sibling.
```

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

## Next Windows Focus

```text
Frozen evidence:
  Redis lanes are now evidence-only.
  Do not widen Redis further before the Windows mixed profiles are checked.

Current lane organization:
  HZ6 Windows is a profile family.

  balanced / wide_ws clean low-RSS:
    HZ6 profile:
      rss
    capacity lane:
      mixedclean-front16k-sourcerun-desc17k-source2k-route17k
    superseded pressure evidence:
      descavail-noboost-route4k

  random_mixed same-owner speed:
    HZ6 profile:
      strict
    capacity lane:
      sameownerfast-descavail-noboost-route4k

  larger_sizes RSS/speed:
    HZ6 profiles:
      speed or rss
    capacity lane:
      largerlowrss-front8k-sourcerun-desc8k-route8k

  Larson cross-owner full 10k:
    HZ6 profile:
      speed
    capacity lane:
      ownerlocalityfast-rsscap-2-desc160k
    lowest-RSS sibling:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
    lower-RSS sibling:
      ownerlocalityfast-rsscap-2-desc160k-front4k
    stable near-capacity sibling:
      ownerlocalityfast-rsscap-2-desc192k

  performance upper-bound / completion control:
    ownerlocalityfast-appcap

  generic route/capacity controls:
    route4k
    appcap

Next attack surface:
  HZ6 MetadataSlim-L1 continues, but the first descriptor hot/cold split is
  now done. The remaining work is to decide whether the next gain should come
  from SlimRouteEntry, deeper source-block slimming, or a broader paper-grade
  repeat before any promotion.

  The Larson desc160/front4k work closed the simple static-capacity trim:
    descriptor lower than 160k crosses the live-object floor.
    route128k fails warmup.
    source2k worsens RSS and speed.
    front4k is the only clean static trim and is now a lower-RSS sibling.

  Remaining Larson RSS is dominated by bytes per backing-table slot rather
  than by payload bytes. The next high-ROI direction is therefore to keep
  selected capacities stable and reduce metadata bytes per slot.

Goal:
  keep selected profile-family lanes fixed, then attack metadata layout:
    descriptor table:
      200 MiB across workers in desc160k.
      Candidate: ThinDescriptor / hot-cold descriptor split.

    route table:
      192 MiB across workers in desc160k.
      Candidate: SlimRouteEntry or index/handle route payload split.

    source-block table:
      74 MiB across workers in desc160k.
      Candidate: SourceBlockSlim metadata split for run bitmap / cold fields.

  Do not:
    lower descriptor capacity below the live-object floor.
    halve route capacity again without changing route ownership/layout.
    add diagnostic atomics/counters to production speed lanes.

Cross-allocator summary:
  HZ6_CROSS_ALLOCATOR_COMPARISON.md
  keeps the paper-aligned HZ3/HZ4/HZ5/HZ6/mimalloc/tcmalloc rows together
  and keeps the current HZ6 selected-family snapshot in one place.

Do not:
  collapse widecap-4 and rsscap-4 into a single broad default yet
  re-open Redis control-plane branch as the next broad HZ6 track
  add diagnostic atomics/counters to production speed lanes
```

## Next Attack: MetadataSlim-L1

```text
Why now:
  Static capacity trimming reached its useful boundary.
  front4k is the only clean capacity trim. The remaining peak is not payload:
    desc160k diagnostic final snapshot:
      descriptor_table_bytes    = 209715200
      route_table_bytes         = 201326592
      source_block_table_bytes  = 77594624
      frontcache_table_bytes    = 117446656  # front8k baseline
      source_block_payload      = ~25 MiB

  front4k lowers the frontcache table, but descriptor/route/source-block
  metadata still dominate.

Primary target order:
  1. SlimRouteEntry-L1:
     reduce route entry bytes without changing VALID / INVALID / MISS
     semantics. This is the cleanest first implementation because route table
     capacity reduction already failed, while route payload packing is local.

  2. ThinDescriptor-L1 dry-run/design:
     split hot fields required for route/free/activate from cold fields needed
     only for source release, owner teardown, diagnostics, and uncommon fronts.

  3. SourceBlockSlim-L1:
     source-block table is large because each slot stores run bitmap / source
     release metadata inline. Split active run metadata from cold source backing
     fields if descriptor/route slimming is insufficient.

Acceptance:
  Larson full 10k:
    selected desc160/front4k speed stays within -3%.
    RSS drops materially without new alloc_fail, descriptor_exhausted,
    route_register_fail, or source_block_exhausted.

  mixed profiles:
    balanced / wide_ws / larger_sizes selected-family rows do not regress
    beyond existing profile variance.

No-go:
  changing the route VALID / INVALID / MISS contract.
  making owned-looking invalid fall through as MISS.
  adding production hot-path counters or atomics.
  turning selected HZ6 into a single universal lane again.
```

MetadataSlim-L1 diagnostic implementation:

```text
Added:
  diagnostic-only [HZ6_METADATA_SLIM] output.

Scope:
  Reports current entry bytes and proposed hot/slim entry bytes for:
    descriptor
    route
    source-block
    frontcache

  Reports estimated table bytes and savings for the current build capacity.

Non-diagnostic:
  No [HZ6_METADATA_SLIM] line.
  No production hot-path counters/atomics added.
```

MetadataSlim-L1 first read:

```text
Run:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k
  Larson T=16 main-warmup chunks=1000
  diagnostic probes only

Result:
  safety clean:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0

  current table bytes:
    descriptor_table_bytes   = 209715200
    route_table_bytes        = 201326592
    source_block_table_bytes = 77594624
    frontcache_table_bytes   = 58726400

  candidate entry sizes:
    descriptor:
      current = 80
      thin hot candidate = 48
      estimated savings = 83886080 bytes

    route:
      current = 48
      slim candidate = 32
      estimated savings = 67108864 bytes

    source-block:
      current = 592
      slim candidate = 560
      estimated savings = 4194304 bytes

    frontcache:
      current = 56
      slim candidate = 32
      estimated savings = 25171968 bytes

Interpretation:
  SourceBlockSlim is low ROI for the current selected Larson row.
  FrontcacheSlim is useful but already partly addressed by front4k.
  The next real layout work should prioritize:
    1. ThinDescriptor-L1
    2. SlimRouteEntry-L1

  ThinDescriptor has the largest single savings estimate, but it touches
  descriptor lifetime APIs. SlimRouteEntry is slightly smaller but may be
  cleaner if route payload can be packed without changing VALID / INVALID /
  MISS semantics.
```

SlimRouteEntry-L1 implementation read:

```text
Change:
  Hz6RouteEntry packed route payload from 48 bytes to 32 bytes:
    base
    descriptor
    bytes as uint32_t
    generation as uint32_t
    front_id / class_id
    exact_valid / active / tombstone bit flags

  Exact and invalid-range registration now reject ranges larger than UINT32_MAX
  before storing the packed byte count.

Diagnostic smoke:
  Run:
    speed + ownerlocalityfast-rsscap-2-desc160k-front4k
    Larson T=16 main-warmup chunks=1000
    diagnostic probes only

  Result:
    throughput = 56.430M ops/s
    safety clean:
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0
      route_invalid = 0
      route_miss = 0

    route_table_bytes = 134217728
    route_entry_bytes = 32
    route_slim_entry_bytes = 32
    route_slim_savings_bytes = 0

  Read:
    SlimRouteEntry-L1 captured the earlier estimated route saving:
      pre-slim route table bytes  = 201326592
      post-slim route table bytes = 134217728
      structural saving           = 67108864 bytes

Non-diagnostic smoke:
  Run:
    same lane, no DiagnosticHz6Probes

  Result:
    throughput = 56.844M ops/s
    no [HZ6_MEMORY_ATTR] line
    no [HZ6_METADATA_SLIM] line
    safety clean

Status:
  SlimRouteEntry-L1 is implemented and smoke-clean.
  Do not promote full Larson RSS/performance yet without repeat/full-row data.

Next:
  ThinDescriptor-L1 is now the largest remaining metadata target:
    descriptor_entry_bytes = 80
    descriptor_thin_hot_entry_bytes = 48
    estimated table saving = 83886080 bytes
```

DescriptorPack-L1 implementation read:

```text
Why:
  Full ThinDescriptor hot/cold split touches source lifetime APIs. Before that,
  pack the safe descriptor fields that already have bounded HZ6 contracts.

Change:
  Hz6ObjectDescriptor packed from 80 bytes to 64 bytes:
    bytes as uint32_t
    source_bytes as uint32_t
    source_kind as uint8_t
    state as uint8_t

  prepare_descriptor now rejects:
    bytes > UINT32_MAX
    effective source_bytes > UINT32_MAX
    source_kind / state values outside uint8_t range

Diagnostic smoke:
  Run:
    speed + ownerlocalityfast-rsscap-2-desc160k-front4k
    Larson T=16 main-warmup chunks=1000
    diagnostic probes only

  Result:
    throughput = 56.832M ops/s
    safety clean:
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0
      route_invalid = 0
      route_miss = 0

    descriptor_table_bytes = 167772160
    descriptor_entry_bytes = 64
    descriptor_thin_hot_entry_bytes = 48
    descriptor_thin_hot_savings_bytes = 41943040

  Read:
    DescriptorPack-L1 captured the first safe descriptor saving:
      pre-pack descriptor table bytes  = 209715200
      post-pack descriptor table bytes = 167772160
      structural saving                = 41943040 bytes

Non-diagnostic smoke:
  Run:
    same lane, no DiagnosticHz6Probes

  Result:
    throughput = 56.953M ops/s
    no [HZ6_MEMORY_ATTR] line
    no [HZ6_METADATA_SLIM] line
    safety clean

Status:
  DescriptorPack-L1 is implemented and smoke-clean.
  It is a low-risk metadata reduction, not the full ThinDescriptor hot/cold
  split.

Next:
  First freeze SlimRouteEntry-L1 + DescriptorPack-L1 with repeat/full-row data.
  Do not start ThinDescriptor-L1 until the post-slim selected rows show:
    throughput within selected-lane tolerance
    material RSS reduction
    safety clean
    no diagnostic output in non-diagnostic runs

  After that, for more descriptor reduction, do not blindly pack pointers.
  The remaining 64 -> 48 target requires a true cold-source side table or a
  source-block-only descriptor profile:
    allocator / ptr / source_block / source_release / owner are pointer-sized
    or lifetime-sensitive.
    direct-source descriptors still need source_ptr/source_bytes/release
    semantics.
```

ThinDescriptor decision checkpoint:

```text
External design read:
  Do not jump directly from smoke-clean SlimRoute/DescriptorPack into
  ThinDescriptor. First run selected Larson repeat/full-row so the RSS and
  throughput deltas are attributable to the low-risk structural changes.

Immediate next:
  post-slim repeat for:
    ownerlocalityfast-rsscap-2-desc160k
    ownerlocalityfast-rsscap-2-desc160k-front4k

  Suggested row:
    Larson T=16 main-warmup
    start with the capacity runner full connected row:
      larson_t16_main_4k
    use repeat-3

Keep criteria:
  throughput within -3% of pre-slim selected baseline
  peak RSS materially lower
  safety clean:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
    route_invalid = 0
    route_miss = 0
    remote_free_transfer_fail = 0

If post-slim repeat passes:
  MetadataSlim remains open.
  ThinDescriptor-L1 can start behind a profile flag.

ThinDescriptor-L1 shape if pursued:
  scope:
    SourceBlock-backed descriptors first.

  implementation:
    hot descriptor table + sparse cold source side table.

  direct-source:
    cold record required.
    cold generation must match hot generation.
    cold source metadata missing or mismatched is fail-closed / no-go, never
    external fallback.

  L2 only:
    source_release handle / source registry replacement.

No-go:
  route_invalid > 0
  route_miss > 0
  route_register_fail > 0
  descriptor_exhausted increases
  direct-source or source-block smoke fails
  cold source missing/mismatch is accepted
  owned-looking invalid falls through to MISS/fallback
  diagnostic atomics/output leaks into real bench
```

Post-slim selected-row repeat:

```text
Run:
  windows-hz6-postslim-repeat3
  family = larson
  profile = larson_t16_main_4k
  hz6 profile = speed
  runs = 3
  diagnostic probes = off

Lanes:
  ownerlocalityfast-rsscap-2-desc160k
  ownerlocalityfast-rsscap-2-desc160k-front4k

Result:
  desc160k:
    median ops/s   = 51.299M
    median peak KB = 635272

  desc160k-front4k:
    median ops/s   = 52.382M
    median peak KB = 570772

Safety:
  all runs clean:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
    route_invalid = 0
    route_miss = 0
    remote_free_transfer_fail = 0

Read:
  SlimRouteEntry-L1 + DescriptorPack-L1 pass the connected 4k repeat-3 row.
  Compared with the earlier front4k bounded 4k read:
    old front4k bounded 4k:
      49.425M ops/s, peak 690560 KB
    post-slim front4k 4k:
      52.382M ops/s, peak 570772 KB

  This supports keeping the low-risk structural metadata reductions.
  It is enough to continue design toward ThinDescriptor-L1, but not enough to
  claim every full Larson 10k/paper row without a dedicated final run.

Next:
  Begin ThinDescriptor-L1 design behind a profile flag:
    hot descriptor table + sparse cold source side table.
  Keep direct-source semantics via cold record generation checks.
```

## LargerSizes Front Retention Ladder 2026-06-03

```text
Purpose:
  front8k fixed the larger_sizes source/route churn problem, but it also sets
  a fairly high front-retention budget. Test whether a smaller front budget
  keeps the larger_sizes speed/RSS shape without returning to allocation
  failure or route/source churn.

New production-capacity lanes:
  largerlowrss-front6k-sourcerun-desc8k-route8k:
    descriptor 8K
    route 8K
    source-block 512
    frontcache 6144
    SourceRunReuse-L1

  largerlowrss-front4k-sourcerun-desc8k-route8k:
    same as front6k, but frontcache 4096

No diagnostic counters:
  These are capacity-shape lanes only. They do not enable probe atomics or
  diagnostic-only instrumentation.
```

Run1:

```text
larger_sizes / speed:
  front4k:
    21.015M ops/s, 72,744 KB
  front6k:
    31.813M ops/s, 72,832 KB
  front8k:
    33.836M ops/s, 72,356 KB

larger_sizes / rss:
  front4k:
    30.020M ops/s, 72,776 KB
  front6k:
    33.299M ops/s, 72,824 KB
  front8k:
    31.768M ops/s, 72,404 KB

Safety:
  route_invalid=0
  route_miss=0
  route_register_fail=0
  alloc_fail=0
  descriptor_exhausted=0
  source_block_exhausted=0
```

Repeat-3 front6k vs front8k:

```text
larger_sizes / speed:
  front6k:
    34.744M ops/s, 72,348 KB
  front8k:
    33.855M ops/s, 72,412 KB

larger_sizes / rss:
  front6k:
    34.249M ops/s, 72,364 KB
  front8k:
    34.727M ops/s, 72,424 KB

Safety:
  both lanes keep route_invalid/route_miss/route_register_fail/alloc_fail at 0.
  source_alloc remains profile-stable:
    speed: 1329
    rss:   1652
```

Read:

```text
front4k:
  no-go as a larger_sizes replacement. It is too small for the speed profile.

front6k:
  keep as a tighter larger_sizes candidate-control. It matches front8k within
  noise, is slightly better in the speed repeat-3, and keeps peak RSS in the
  same 72MB band.

front8k:
  keep as the selected larger_sizes lane for now because the earlier repeat-3
  remains the documented selection baseline and the new repeat-3 does not show
  a decisive RSS reduction from front6k.

Next:
  If more larger_sizes tuning is needed, compare front6k/front8k on large_slice
  rows before changing the selected lane table. Do not re-open frontcache
  spill/borrow/cap knobs.
```

Promotion check:

```text
Run:
  mixed_ws larger_sizes + large_slices
  speed/rss
  front6k vs front8k
  repeat-3

larger_sizes:
  speed:
    front6k 33.361M ops/s, 72,328 KB
    front8k 31.945M ops/s, 72,432 KB

  rss:
    front6k 32.481M ops/s, 72,424 KB
    front8k 32.550M ops/s, 72,460 KB

large_slice read:
  front6k wins or ties some rows:
    speed 2k / 16k / 64k / 128k
    rss 4k / 16k / 64k / 128k

  front8k remains stronger on important slice rows:
    speed 4k / 8k / 32k
    rss 256 / 512 / 1k / 8k / 32k

Safety:
  route_invalid=0
  route_miss=0
  route_register_fail=0
  alloc_fail=0
  descriptor_exhausted=0
  source_block_exhausted=0
```

Final decision:

```text
Do not promote front6k yet.

front6k:
  keep as close candidate-control and evidence that front retention can be
  tightened without breaking larger_sizes itself.

front8k:
  remains selected larger_sizes RSS/speed lane because it covers the wider
  large_slice envelope more consistently.

Next:
  larger_sizes front-retention capacity tuning is closed for now. Move to a
  different weak row or to selected-lane documentation / comparison cleanup.
```

## Larson CrossOwner Current Check 2026-06-03

```text
Runner fix:
  run_win_hz6_capacity_matrix.ps1 now captures Larson's
  "Throughput = ... operations per second" and [OPS] lines as important output.
  Without this, large [HZ6_STATS] lines could push throughput out of the
  captured output and create false parse failures.

Scope:
  HZ6 capacity runner only.
  No allocator behavior change.
```

Compact / moderate live-set check:

```text
Run:
  larson_t16_main_1k / worker_1k
  larson_t16_main_4k / worker_4k
  speed profile
  appcap / ownerlocalityfast-appcap / ownerlocalityfast-rsscap-4
  run1

main_1k:
  appcap:
    0.001M ops/s, 2,681,612 KB
  ownerlocalityfast-appcap:
    53.025M ops/s, 2,700,844 KB
  ownerlocalityfast-rsscap-4:
    56.446M ops/s, 557,968 KB

worker_1k:
  appcap:
    57.536M ops/s, 2,682,416 KB
  ownerlocalityfast-appcap:
    57.015M ops/s, 2,700,840 KB
  ownerlocalityfast-rsscap-4:
    57.852M ops/s, 557,548 KB

main_4k:
  appcap:
    near-zero / parseable but effectively collapsed
  ownerlocalityfast-appcap:
    46.258M ops/s, 2,741,636 KB
  ownerlocalityfast-rsscap-4:
    50.481M ops/s, 598,336 KB

worker_4k:
  appcap:
    50.237M ops/s, 2,723,132 KB
  ownerlocalityfast-appcap:
    51.066M ops/s, 2,741,580 KB
  ownerlocalityfast-rsscap-4:
    53.432M ops/s, 598,280 KB

Read:
  ownerlocalityfast closes the main-vs-worker cross-owner gap for compact and
  moderate live sets.
  rsscap-4 is especially strong for 1k/4k live-set checks.
```

Full 10k live-set check:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-1/2/3/4 and ownerlocalityfast-appcap
  run1

ownerlocalityfast-rsscap-1:
  43.570M ops/s, 1,230,552 KB
  source_alloc=381
  safety clean

ownerlocalityfast-rsscap-2:
  41.700M ops/s, 1,066,896 KB
  source_alloc=381
  safety clean

ownerlocalityfast-rsscap-3:
  failed warmup allocation
  source_alloc=8192
  alloc_fail=1
  descriptor_exhausted=2
  source_block_exhausted=1

ownerlocalityfast-rsscap-4:
  failed warmup allocation
  source_alloc=7711
  alloc_fail=1

ownerlocalityfast-appcap:
  43.511M ops/s, 2,822,756 KB
  source_alloc=381
  safety clean
```

Read:

```text
ownerlocalityfast-rsscap-1:
  best full-10k speed/RSS balance in this check.
  Matches appcap throughput while cutting peak from ~2.82GB to ~1.23GB.

ownerlocalityfast-rsscap-2:
  lower RSS than rsscap-1, still 41.7M ops/s and safety clean.
  Candidate if RSS priority is stronger than the last few percent of speed.

rsscap-3/4:
  too tight for full 10k main-warmup.
  Keep as compact/moderate live-set evidence only.

Next:
  Do not add more visible/negative-filter knobs.
  Treat ownerlocalityfast-rsscap-1 as the current full Larson cross-owner
  candidate-control, and rsscap-2 as the lower-RSS sibling.
  If optimizing further, reduce rsscap-1 peak without crossing the rsscap-3/4
  warmup-failure boundary.
```

Descriptor-boundary probes:

```text
Purpose:
  rsscap-2 succeeds at full 10k Larson with descriptor capacity 262144.
  rsscap-3 fails after lowering descriptor capacity to 131072 while keeping
  route/source/frontcache shape mostly aligned with rsscap-2.

New non-diagnostic capacity lanes:
  ownerlocalityfast-rsscap-2-desc192k:
    descriptor 196608
    route 262144
    source-block 8192
    transfer/frontcache 4096/8192

  ownerlocalityfast-rsscap-2-desc160k:
    descriptor 163840
    route 262144
    source-block 8192
    transfer/frontcache 4096/8192

Read:
  These are capacity-boundary probes only. They do not enable diagnostic
  counters or change allocator behavior. Use them to find whether full 10k
  Larson can reduce peak below rsscap-2 without falling into the rsscap-3/4
  warmup-allocation failure.
```

Boundary run1:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-1/2/2-desc192k/2-desc160k
  run1

ownerlocalityfast-rsscap-1:
  42.082M ops/s, 1,230,116 KB

ownerlocalityfast-rsscap-2:
  42.365M ops/s, 1,066,456 KB

ownerlocalityfast-rsscap-2-desc192k:
  42.593M ops/s, 974,740 KB

ownerlocalityfast-rsscap-2-desc160k:
  41.257M ops/s, 928,652 KB

Safety:
  all rows keep route_invalid/route_miss/route_register_fail/alloc_fail at 0.
  source_alloc stays 381.
```

Repeat-3:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-2 vs ownerlocalityfast-rsscap-2-desc192k
  repeat-3

ownerlocalityfast-rsscap-2:
  42.808M ops/s, 1,066,448 KB

ownerlocalityfast-rsscap-2-desc192k:
  43.679M ops/s, 974,296 KB

Safety:
  both lanes keep route_invalid/route_miss/route_register_fail/alloc_fail at 0.
  remote_free_transfer_fail=0.
  transfer_current=0.
  source_alloc=381..382.
```

Desc160 repeat-3:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-2-desc160k
  repeat-3

ownerlocalityfast-rsscap-2-desc160k:
  corrected median:
    43.721M ops/s, 928,228 KB
  runner summary before numeric median fix printed the max-like 48.108M value;
  the run lines are 42.305M / 43.721M / 48.108M, so the median is 43.721M.

Safety:
  route_invalid=0
  route_miss=0
  route_register_fail=0
  alloc_fail=0
  descriptor_exhausted=0
  remote_free_transfer_fail=0
  transfer_current=0
```

Decision:

```text
ownerlocalityfast-rsscap-2-desc160k:
  promote to current full Larson cross-owner candidate-control.
  It keeps desc192k-class throughput while cutting peak to ~928 MiB.

ownerlocalityfast-rsscap-2-desc144k:
  next narrow descriptor-boundary probe.
  Same rsscap-2 shape, but descriptor capacity 147456.
  Use it to check whether desc160k can be tightened one more step before the
  known rsscap-3/131k warmup-failure boundary.

ownerlocalityfast-rsscap-2-desc192k:
  keep as stable near-capacity sibling / control.

ownerlocalityfast-rsscap-1/2:
  keep as stable controls.

rsscap-3/4:
  still too tight for full 10k Larson.
```

Desc144 run1:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-2-desc144k
  run1

Result:
  warmup allocation failed

Stats:
  source_alloc=24576
  alloc_fail=1
  descriptor_exhausted=2
  source_block_exhausted=1025
  route_register_fail=0

Decision:
  desc144k is no-go / boundary evidence.
  desc160k remains the selected full Larson cross-owner candidate-control.
  The next step is not to continue static descriptor trimming unless a new
  source-block/admission design changes this boundary.
```

Desc144 diagnostic read:

```text
Run:
  larson_T16
  speed profile
  ownerlocalityfast-rsscap-2-desc144k
  diagnostic probes
  run1

Key stats:
  owner_locality_register=147456
  source_owned_prepare=147456
  descriptor_exhausted=2
  descriptor_fail_active_max=147422
  descriptor_fail_local_free_max=34
  source_block_exhausted=1025
  source_block_fail_active_max=8192
  source_block_fail_registered_max=8192
  source_block_fail_ref_nonzero_max=8192
  source_block_fail_ref_zero_max=0
  source_run_reuse_dryrun_candidate_calls=9216
  source_run_reuse_dryrun_largest_free_slots_max=4080

Read:
  The source-run dry-run sees many free physical slots, but the failure is not
  primarily missing physical slot reuse. The descriptor table is effectively
  full of active objects.

  Larson T=16 chunks=10000 has about 160k live objects. A one-descriptor-per
  live-object contract therefore has a hard descriptor floor around 160k.

  desc144k fails because it is below that live-set floor. desc160k succeeds
  because 163840 descriptors is barely above the workload floor.

Decision:
  Static descriptor trimming is closed for full 10k Larson.
  desc160k is close to the one-descriptor-per-live-object lower bound.
  Further RSS reduction would require a different lifecycle contract, not just
  a smaller descriptor capacity flag.
```

Design review decision:

```text
Accept the current interpretation:
  static descriptor capacity trim is closed.

Selected:
  speed + ownerlocalityfast-rsscap-2-desc160k

Role:
  HZ6 current cross-owner full-10k candidate-control.
  HZ6-internal low-RSS / high-throughput selected lane for Larson
  cross-owner stress, not a universal production default.

Boundary:
  ownerlocalityfast-rsscap-2-desc144k

Role:
  no-go / lower-bound evidence.
  It demonstrates the one-live-object-one-descriptor floor for this workload.

Do not:
  keep slicing static descriptor flags such as desc152k / desc148k.
  The ROI is low because T=16 * chunks=10000 is already around the 160k live
  object floor.
```

Completed design order:

```text
Done:
  D:
    desc160k is frozen as the current selected Larson cross-owner lane.

  E-lite:
    LarsonMemoryAttribution-L1 was added as diagnostic-only.

Result:
  ThinDescriptor is not the only next lever. The desc160k RSS is dominated by
  per-worker static backing tables across descriptor, route, frontcache, and
  source-block metadata.

Next:
  static backing-table trim controls before changing descriptor layout.

  Candidate axes:
    route table trim with desc160k held fixed.
    source-block table trim with desc160k held fixed.
    frontcache table trim with desc160k held fixed.
    owner-locality/shared-directory trim is lower priority after corrected
    attribution because it is global, not per-worker.

  Then C if static-table trims are insufficient:
    ThinDescriptor / hot-cold descriptor split.

  Then B only if C is insufficient:
    compact handle table / live descriptor indirection.

Lower priority:
  A descriptor lifecycle compression for cold/free/cached objects.
  It is useful for other rows, but this Larson full-10k row is dominated by
  ACTIVE live descriptors, not cached/free descriptors.
```

LarsonMemoryAttribution-L1 scope:

```text
Diagnostic-only. Do not mix into production benchmark lanes.

Purpose:
  Explain the 928MB desc160k peak before changing descriptor layout.

Report static capacity bytes:
  descriptor_table_bytes
  route_table_bytes
  source_block_table_bytes
  frontcache_table_bytes
  transfer_table_bytes
  ownerlocality_index_bytes

Report dynamic state:
  active_descriptors
  local_free_descriptors
  transfer_free_descriptors
  remote_pending_descriptors
  dead_with_ptr_descriptors

Report source memory:
  active_source_blocks
  registered_source_blocks
  ref_nonzero_source_blocks
  source_block_payload_bytes
  source_block_committed_estimate

Report route/frontcache:
  route_active_current
  route_active_max
  route_tombstone_current if available
  frontcache_total
  frontcache_largest_bin

Decision after attribution:
  descriptor table is dominant alone:
    proceed to ThinDescriptor-L1.

  source payload / committed source blocks are dominant:
    work on SourceBlock/RSS policy instead of descriptor compression.

  route / ownerlocality / frontcache static tables dominate:
    trim those backing tables before touching descriptor layout.
```

LarsonMemoryAttribution-L1 read:

```text
Run:
  speed + ownerlocalityfast-rsscap-2-desc160k
  Larson T=16 / chunks=10000
  diagnostic probes only

Result:
  throughput:
    44.134M ops/s

  safety:
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    remote_free_transfer_fail = 0
    route_rehome_fail = 0
    lifecycle_foreign_free_invalid = 0

  static table bytes across worker allocators:
    descriptor_table_bytes    = 209715200
    route_table_bytes         = 201326592
    frontcache_table_bytes    = 117446656
    source_block_table_bytes  = 77594624
    transfer_table_bytes      = 1572864

  global static bytes:
    ownerlocality_index_bytes = 18874368   # shared directory + owner-locality

  dynamic/final state:
    source_block_payload_bytes = 24969216
    active_source_blocks       = 381
    registered_source_blocks   = 381
    ref_nonzero_source_blocks  = 381
    active_descriptors         = 4806
    local_free_descriptors     = 1250
    route_active_current       = 86437
    route_tombstone_current    = 0
    frontcache_total           = 6056
    frontcache_largest_bin     = 187

Interpretation:
  The successful desc160k row is not payload-heavy at final snapshot. RSS is
  dominated by per-worker static metadata capacity. Descriptor compression can
  help, but route, frontcache, and source-block table sizing have equal or
  better immediate ROI. The owner-locality/shared directory is still measurable,
  but it is global and much smaller than the per-worker tables after corrected
  aggregation.

Next attack:
  add one-axis static-table trim controls around the selected desc160k lane.
  Keep descriptor capacity at 160k first so the live-object floor is not mixed
  with route/source/frontcache table trimming.
```

StaticBackingTrim-R1:

```text
Purpose:
  Keep the desc160k live-object floor fixed and trim one static backing table
  at a time.

New lanes:
  ownerlocalityfast-rsscap-2-desc160k-route128k:
    descriptor = 163840
    route = 131072
    source-block = 8192
    frontcache = 8192

  ownerlocalityfast-rsscap-2-desc160k-source2k:
    descriptor = 163840
    route = 262144
    source-block = 2048
    frontcache = 8192

  ownerlocalityfast-rsscap-2-desc160k-front4k:
    descriptor = 163840
    route = 262144
    source-block = 8192
    frontcache = 4096

R1 one-run read:
  route128k:
    no-go / boundary.
    Warmup alloc_fail after source_alloc=7711.
    Route capacity cannot be halved under the current main-warmup route
    lifecycle without a different route ownership contract.

  source2k:
    no-go / control.
    39.951M ops/s, peak 1177676 KB.
    Lower source-block table capacity did not reduce RSS and worsened speed.

  front4k:
    low-RSS selected sibling.
    Initial 1-run: 42.361M ops/s, peak 864216 KB.
    Bounded 4k repeat-3:
      main-warmup:   49.425M ops/s, peak 690560 KB
      worker-warmup: 51.631M ops/s, peak 690496 KB
    Full 10k repeat-3:
      42.191M ops/s, peak 863772 KB

    Compared with desc160k repeat-3:
      desc160k:
        43.721M ops/s, peak 928228 KB
      front4k:
        42.191M ops/s, peak 863772 KB

    Interpretation:
      front4k trades about 3.5% throughput for about 64MB lower peak RSS.
      Safety counters stay clean:
        alloc_fail = 0
        descriptor_exhausted = 0
        route_register_fail = 0
        source_block_exhausted = 0

Next read:
  frontcache trim is the only promising R1 axis.
  The earlier repeat timeout was caused by the outer command timeout over a
  two-lane repeat-3 run, not by front4k itself. Full front4k-only repeat-3
  completes when the outer timeout is long enough.

Promotion status:
  ownerlocalityfast-rsscap-2-desc160k:
    selected throughput/RSS balance for full Larson cross-owner.

  ownerlocalityfast-rsscap-2-desc160k-front4k:
    selected lower-RSS sibling for full Larson cross-owner.
    Use when -3.5% throughput is acceptable for about 64MB lower peak RSS.
```

ThinDescriptor-L1 sketch:

```text
Goal:
  reduce bytes per live object without changing route safety semantics.

Keep:
  one live object = one stable descriptor core.
  route VALID / INVALID / MISS contract.
  generation check.
  state transition.
  owner token.
  source block lifetime/ref safety.

Move to SourceBlock/cold metadata where possible:
  source_ptr
  source_bytes
  source_kind
  source_release
  run geometry
  block bytes
  slot bytes

No-go:
  INVALID becomes MISS.
  owned-looking invalid falls back.
  source block releases while a live core still references it.
  route result becomes ambiguous between descriptor pointer and handle.
```

## RandomMixed A/B Fast-Path Plan 2026-06-03

```text
Flag lane cleanup:
  sameownerfast-descavail-noboost-route4k is now the selected readable alias
  for the strong DirectLocalFree + DirectLocalAlloc + DirectLocalReuse +
  DescriptorAvailCount composition.

  Implementation cleanup:
    sameownerfast now uses a single contract flag:
      HZ6_SAME_OWNER_FAST_L1

    The A-ladder flags remain available only as evidence/control switches:
      HZ6_LOCAL_CACHE_DIRECT_FREE_L1
      HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
      HZ6_LOCAL_CACHE_DIRECT_REUSE_L1

  The longer directlocal* names remain A-ladder evidence/control lanes.

  Do not make the flags global compile defaults:
    rss balanced / wide_ws still prefer descavail-noboost-route4k
    larger_sizes still uses largerlowrss-front8k-sourcerun-desc8k-route8k

Decision from design review:
  Start with A, then move to B only if the ablation signal is real.

A:
  Per-operation contract cost ablation ladder.
  This is not promotion. It tests which part of the same-owner cost is still
  visible after directlocalfree + descavail.

Base:
  strict + directlocalfree-descavail-noboost-route4k

New ablation lanes:
  directlocalalloc-descavail-noboost-route4k:
    Dispatch-bypass alloc probe.
    TOY/MIDPAGE/LOCAL2P malloc tries the existing cached/transfer reuse helper
    before calling the front function pointer.

  directlocalreuse-descavail-noboost-route4k:
    Narrow local-cache reuse probe.
    TOY/MIDPAGE/LOCAL2P malloc tries only materialized LOCAL_FREE frontcache
    descriptors before the normal front path.

  directlocalfreealloc-descavail-noboost-route4k:
    Composition probe.
    DirectLocalFree-L1 plus DirectLocalAlloc-L1 plus DescriptorAvailCount-L1.

  directlocalfreereuse-descavail-noboost-route4k:
    Closing composition probe.
    DirectLocalFree-L1 plus DirectLocalAlloc-L1 plus DirectLocalReuse-L1 plus
    DescriptorAvailCount-L1.

B:
  Small/medium same-owner fast path as a shared front contract.
  Do not jump here until A shows at least one +5% ablation signal.

Safety contract:
  eligible fronts:
    HZ6_FRONT_TOY
    HZ6_FRONT_MIDPAGE
    HZ6_FRONT_LOCAL2P

  excluded:
    HZ6_FRONT_LARGE
    foreign / visible / remote handoff
    shared route directory path
    descriptorless / descgov materialization as the narrow reuse probe

Acceptance:
  weak A signal:
    any ablation >= base + 5%

  strong A signal:
    any ablation >= base + 10%

  B candidate:
    random_mixed medium/mixed >= base + 10%
    RSS flat
    route_miss / route_invalid / route_register_fail stay zero

No-go:
  speed lane gets diagnostic atomics/counters
  large front is pulled into the direct local fast path
  MISS/INVALID route semantics are weakened
  random_mixed gains only by hurting balanced / wide_ws / larger_sizes guards
```

Initial repeat-1 read:

```text
Command:
  powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
    -OutputDir Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-randommixed-ablation-r1 `
    -Runs 1 `
    -Families random_mixed `
    -BenchmarkProfiles small,medium,mixed `
    -Hz6Profiles strict `
    -CapacityLanes directlocalfree-descavail-noboost-route4k,`
      directlocalalloc-descavail-noboost-route4k,`
      directlocalreuse-descavail-noboost-route4k,`
      directlocalfreealloc-descavail-noboost-route4k

Result:
  random_mixed small:
    base directlocalfree-descavail  34.867M, 5,020 KB
    directlocalalloc                41.336M, 5,004 KB
    directlocalreuse                43.642M, 5,052 KB
    directlocalfreealloc            43.580M, 5,024 KB

  random_mixed medium:
    base directlocalfree-descavail  37.919M, 4,972 KB
    directlocalalloc                36.662M, 5,016 KB
    directlocalreuse                38.632M, 5,080 KB
    directlocalfreealloc            38.405M, 4,984 KB

  random_mixed mixed:
    base directlocalfree-descavail  34.439M, 5,032 KB
    directlocalalloc                34.189M, 4,980 KB
    directlocalreuse                36.676M, 4,972 KB
    directlocalfreealloc            38.451M, 5,072 KB

Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  remote / visibility / transfer counters stay 0 in same-owner random_mixed.

Read:
  A has a real signal, especially small and mixed.
  DirectLocalReuse is the cleanest narrow local-cache signal.
  DirectLocalFreeAlloc composes best on mixed.
  Medium is not yet decisive; repeat-3 plus balanced/wide_ws/larger_sizes guards
  are needed before B becomes a design target.
```

Guard repeat-1 read:

```text
Command:
  powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
    -OutputDir Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-randommixed-ablation-guards-r1 `
    -Runs 1 `
    -Families mixed_ws `
    -BenchmarkProfiles balanced,wide_ws,larger_sizes `
    -Hz6Profiles strict,rss `
    -CapacityLanes descavail-noboost-route4k,`
      directlocalfree-descavail-noboost-route4k,`
      directlocalreuse-descavail-noboost-route4k,`
      directlocalfreealloc-descavail-noboost-route4k

Result:
  mixed_ws balanced:
    strict descavail         55.228M, 24,400 KB
    strict directlocalreuse  63.329M, 24,804 KB
    strict freealloc         57.659M, 24,832 KB
    rss descavail            74.094M, 18,160 KB
    rss freealloc            71.935M, 18,544 KB

  mixed_ws wide_ws:
    strict descavail         26.759M, 25,108 KB
    strict directlocalreuse  27.413M, 25,124 KB
    strict freealloc         27.511M, 24,988 KB
    rss descavail            52.769M, 13,220 KB
    rss directlocalreuse     45.830M, 13,292 KB
    rss freealloc            46.204M, 13,264 KB

  mixed_ws larger_sizes:
    strict descavail          1.369M, 15,744 KB
    strict directlocalreuse   1.166M, 15,788 KB
    strict freealloc          1.258M, 15,760 KB
    rss descavail             0.711M, 15,024 KB
    rss directlocalreuse      0.730M, 15,008 KB
    rss freealloc             0.711M, 15,000 KB

Read:
  A-ladder remains a random_mixed/same-owner investigation, not a selected
  default. Strict balanced/wide_ws tolerate it, but strict larger_sizes and rss
  wide_ws guard against broad promotion.
  Keep rss + descavail-noboost-route4k as selected balanced/wide_ws low-RSS
  lane until repeat evidence says otherwise.
```

Closing A-ladder repeat-3:

```text
Command:
  powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
    -OutputDir Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-randommixed-freereuse-r3 `
    -Runs 3 `
    -Families random_mixed `
    -BenchmarkProfiles small,medium,mixed `
    -Hz6Profiles strict `
    -CapacityLanes directlocalfree-descavail-noboost-route4k,`
      directlocalreuse-descavail-noboost-route4k,`
      directlocalfreealloc-descavail-noboost-route4k,`
      directlocalfreereuse-descavail-noboost-route4k

Result:
  random_mixed small:
    base directlocalfree-descavail  35.526M, 5,080 KB
    directlocalreuse                43.951M, 5,036 KB
    directlocalfreealloc            43.562M, 5,040 KB
    directlocalfreereuse            46.128M, 5,432 KB

  random_mixed medium:
    base directlocalfree-descavail  37.642M, 5,112 KB
    directlocalreuse                39.564M, 4,984 KB
    directlocalfreealloc            40.994M, 5,056 KB
    directlocalfreereuse            42.299M, 5,040 KB

  random_mixed mixed:
    base directlocalfree-descavail  36.455M, 5,052 KB
    directlocalreuse                37.581M, 5,016 KB
    directlocalfreealloc            39.571M, 5,048 KB
    directlocalfreereuse            41.352M, 5,072 KB

Safety:
  route_invalid nonzero search: none
  route_miss nonzero search: none
  route_register_fail nonzero search: none

Read:
  directlocalfreereuse clears the B-candidate speed threshold on all three
  random_mixed rows:
    small  +29.2% vs base
    medium +12.4% vs base
    mixed  +16.4% vs base

  This is the strongest evidence yet that same-owner small/medium allocation
  needs a shared fast contract. However, the slightly higher small RSS and the
  mixed_ws guard mean this should feed B design rather than become the broad
  selected lane.
```

Closing A-ladder guard:

```text
Command:
  powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
    -OutputDir Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-freereuse-guards-r1 `
    -Runs 1 `
    -Families mixed_ws `
    -BenchmarkProfiles balanced,wide_ws,larger_sizes `
    -Hz6Profiles strict,rss `
    -CapacityLanes descavail-noboost-route4k,`
      directlocalfree-descavail-noboost-route4k,`
      directlocalfreereuse-descavail-noboost-route4k

Result:
  strict balanced:
    descavail        58.160M, 24,412 KB
    directlocalfree  70.671M, 24,428 KB
    freereuse        70.386M, 24,800 KB

  strict wide_ws:
    descavail        25.379M, 25,084 KB
    directlocalfree  32.168M, 25,092 KB
    freereuse        28.222M, 25,148 KB

  strict larger_sizes:
    descavail         1.273M, 15,772 KB
    directlocalfree   1.250M, 15,788 KB
    freereuse         1.428M, 15,756 KB

  rss balanced:
    descavail        78.340M, 18,184 KB
    freereuse        71.885M, 18,528 KB

  rss wide_ws:
    descavail        57.580M, 13,280 KB
    freereuse        47.020M, 13,280 KB

Read:
  strict guards are acceptable and larger_sizes even improves in this run.
  rss balanced/wide_ws still prefer descavail. Keep the selected low-RSS
  mixed_ws lane unchanged.

Next:
  Move to B design:
    a shared same-owner fast contract for TOY/MIDPAGE/LOCAL2P based on the
    directlocalfreereuse signal.
  Do not broaden to LARGE or remote/cross-owner paths.
```

## RandomMixed Selected-Lane Read 2026-06-02

```text
Context:
  HZ5 already has paper coverage.
  HZ6 now needs weakness-driven engineering.
  App-like random_mixed is the clearest current same-owner weakness row.

Runner fix:
  win/run_win_hz6_capacity_matrix.ps1 now captures generic bench_* ops/s
  lines plus [RSS] and [HZ6_STATS] lines.
  This lets the HZ6 capacity runner parse random_mixed rows instead of
  failing after a successful benchmark run.
```

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -OutputDir Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-selected-random-mixed `
  -Runs 1 `
  -Families random_mixed `
  -BenchmarkProfiles small,medium,mixed `
  -CapacityLanes noboost-route4k,ownerlocalityfast-widecap-4,ownerlocalityfast-rsscap-4,ownerlocalityfast-appcap `
  -SkipBuild
```

Result file:

```text
Z:\TextureVoice_local\git\allocator-bench-lab\results\windows-hz6-selected-random-mixed\20260602_195941_hz6_capacity_matrix_windows.md
```

Run1 read:

```text
random_mixed / small:
  strict noboost-route4k              33.798M ops/s,   5,540 KB
  strict ownerlocalityfast-widecap-4  33.268M ops/s,  92,804 KB
  strict ownerlocalityfast-rsscap-4   34.325M ops/s,  62,544 KB
  strict ownerlocalityfast-appcap     33.424M ops/s, 298,620 KB

random_mixed / medium:
  strict noboost-route4k              34.261M ops/s,   5,540 KB
  strict ownerlocalityfast-widecap-4  34.156M ops/s,  92,456 KB
  strict ownerlocalityfast-rsscap-4   34.621M ops/s,  62,180 KB
  strict ownerlocalityfast-appcap     33.673M ops/s, 298,316 KB

random_mixed / mixed:
  strict noboost-route4k              32.352M ops/s,   5,540 KB
  strict ownerlocalityfast-widecap-4  31.732M ops/s,  92,756 KB
  strict ownerlocalityfast-rsscap-4   32.392M ops/s,  62,492 KB
  strict ownerlocalityfast-appcap     31.512M ops/s, 298,556 KB
```

Interpretation:

```text
Safety:
  clean.
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  descriptor_exhausted = 0
  source_block_exhausted = 0

Capacity:
  not the blocker.
  appcap uses much more RSS but is not faster.
  widecap/rsscap add memory without a meaningful random_mixed speed win.

Current random_mixed lane:
  noboost-route4k is the useful low-RSS control:
    roughly the same speed as larger capacity lanes
    far lower RSS

Weakness:
  HZ6 selected lanes are around 28M-34M ops/s here.
  Existing-data app-like baselines put HZ3/tcmalloc/mimalloc much higher on
  random_mixed, so HZ6 is not yet a speed candidate for this family.

Next attack:
  do not add another capacity lane first.
  attack same-owner hot-path overhead:
    route lookup / route registration churn
    descriptor activation/state transitions
    wrapper/front dispatch cost
    source-run/frontcache local reuse overhead
  keep diagnostic counters out of speed lanes.
```

## LocalExactFirstFree-L1 2026-06-02

```text
Hypothesis:
  random_mixed same-owner frees are exact-valid almost every time.
  A full route lookup on every free preserves safety, but may be too heavy for
  the app-like hot path.

Experiment:
  localexactfree-noboost-route4k

Change:
  free/free_remote first try exact route lookup.
  exact HIT:
    proceed with the normal tagged free path.
  exact MISS:
    fall back to the existing full route lookup, so invalid range detection
    and foreign/visible fallback semantics remain intact.

Do not:
  promote by default.
  skip full lookup after exact MISS.
  convert owned-looking invalid into MISS.
  combine this with capacity expansion or remote handoff changes.
```

Acceptance:

```text
Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0

Performance:
  random_mixed selected rows improve vs noboost-route4k.
  strong signal is +5% or better without RSS increase.

Probe read:
  route_exact_lookup_probe_total should replace most of route_lookup_probe_total.
  route_lookup_probe_total should drop on exact-valid rows.

No-go:
  speed does not improve.
  route_lookup_probe_total remains dominant.
  any INVALID/MISS safety counter appears.
```

First run:

```text
normal build:
  random_mixed small:
    noboost-route4k                 32.879M, 5,540 KB
    localexactfree-noboost-route4k  33.673M, 5,548 KB

  random_mixed medium:
    noboost-route4k                 33.905M, 5,540 KB
    localexactfree-noboost-route4k  33.785M, 5,540 KB

  random_mixed mixed:
    noboost-route4k                 32.612M, 5,544 KB
    localexactfree-noboost-route4k  32.431M, 5,540 KB

diagnostic build:
  exact-first works mechanically:
    small route_lookup_probe_total:
      10,305,315 -> 0
    medium route_lookup_probe_total:
      15,710,042 -> 433
    mixed route_lookup_probe_total:
      14,958,421 -> 401

  safety remains clean:
    route_invalid = 0
    route_miss = 0
```

Read:

```text
LocalExactFirstFree-L1 is good mechanism evidence but not a promotion lane.
It proves exact-first can remove full route lookup probes on same-owner
random_mixed frees without breaking safety, but normal-build throughput does
not improve enough. The remaining random_mixed weakness is not only full route
lookup; it is likely the broader HZ6 per-operation contract cost:
  front dispatch
  descriptor activation/cache transitions
  frontcache push/pop
  route/descriptor generation checks

Next:
  keep this lane buildable as a control.
  do not add more route lookup ordering knobs first.
  attack local reuse/free object-state overhead or compare a stripped
  same-owner toy fast path against the full contract path.
```

## DirectLocalFree-L1 2026-06-02

```text
Small free-path cleanup:
  When both diagnostic probes and FRONTCACHE_CAP_ON_FREE are off, skip the
  frontcache cap dry-run / cap-release function calls entirely.
  This is semantics-preserving for speed lanes because the cap code is
  behavior-off and diagnostics-off in those builds.

Experiment:
  directlocalfree-noboost-route4k

Change:
  For local-owner HZ6_FRONT_TOY / HZ6_FRONT_MIDPAGE / HZ6_FRONT_LOCAL2P frees,
  bypass:
    hz6_front_for_id
    front->free_tagged
    hz6_front_free_local_to_cache

  and call:
    hz6_allocator_cache_active_descriptor()

  directly from hz6_free().

Safety boundary:
  HZ6_FRONT_LARGE is excluded because it can route through CentralSpanPool.
  foreign-owner, MISS, INVALID, and visible/remote paths stay unchanged.
```

Acceptance:

```text
Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0

Performance:
  random_mixed improves vs noboost-route4k after cap-call skip.
  strong signal is +5% without RSS increase.

No-go:
  no speed gain.
  larger/front guard rows break.
  any safety counter appears.
```

First result:

```text
random_mixed repeat-3:
  small:
    noboost-route4k             34.522M, 5,548 KB
    directlocalfree-noboost     35.805M, 5,548 KB

  medium:
    noboost-route4k             34.730M, 5,544 KB
    directlocalfree-noboost     38.037M, 5,540 KB

  mixed:
    noboost-route4k             32.947M, 5,544 KB
    directlocalfree-noboost     36.433M, 5,544 KB

mixed_ws guard run1:
  balanced:
    noboost-route4k             17.609M, 24,836 KB
    directlocalfree-noboost     23.196M, 24,848 KB

  wide_ws:
    noboost-route4k             14.861M, 25,536 KB
    directlocalfree-noboost     14.290M, 25,532 KB

  larger_sizes:
    noboost-route4k              1.407M, 16,200 KB
    directlocalfree-noboost      1.250M, 16,184 KB

Safety:
  clean in checked rows.
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
```

Repeat-3 guard:

```text
mixed_ws balanced:
  noboost-route4k               23.409M, 24,844 KB
  directlocalfree-noboost       24.194M, 24,852 KB

mixed_ws wide_ws:
  noboost-route4k               16.017M, 25,536 KB
  directlocalfree-noboost       16.697M, 25,536 KB

mixed_ws larger_sizes:
  noboost-route4k                1.419M, 16,204 KB
  directlocalfree-noboost        1.448M, 16,200 KB
```

Read:

```text
DirectLocalFree-L1 is strong random_mixed mechanism evidence:
  same-owner front dispatch / wrapper validation cost is real.

Repeat-3 guard:
  removes the initial run1 concern.
  balanced / wide_ws / larger_sizes all improve modestly while RSS stays flat.

Current status:
  KEEP as same-owner hot-path candidate-control.
  Promotion is still not automatic because it bypasses the generic front
  contract for TOY/MIDPAGE/LOCAL2P, but it is now a real candidate direction
  rather than only a mechanism witness.

Next:
  repeat against the selected HZ6 lane set or make the direct path a named
  profile component.
  keep LARGE excluded.
  keep exact MISS / foreign / remote paths on the normal contract.
```

## DirectLocalFree Selected-Family Lanes 2026-06-02

```text
Lane organization:
  directlocalfree-noboost-route4k:
    low-RSS route4k evidence/control.

  directlocalfree-ownerlocalityfast-rsscap-4:
    larger_sizes selected-family candidate-control.
    Same capacity shape as ownerlocalityfast-rsscap-4, plus
    DirectLocalFree-L1.

  directlocalfree-ownerlocalityfast-widecap-4:
    wide_ws selected-family candidate-control.
    Same capacity shape as ownerlocalityfast-widecap-4, plus
    DirectLocalFree-L1.

Reason:
  DirectLocalFree-L1 improved same-owner random_mixed and repeat-3 mixed_ws
  guard rows while preserving safety. The next question is whether it also
  lifts the existing selected-family lanes instead of only the low-RSS
  noboost route4k control.

Acceptance:
  rsscap-4 direct variant improves larger_sizes or keeps it within noise while
  preserving RSS and clean safety counters.
  widecap-4 direct variant improves wide_ws or keeps it within noise while
  preserving RSS and clean safety counters.

No-go:
  safety counter appears.
  selected-family RSS shape worsens materially.
  the direct variant only helps noboost but not selected-family lanes.
```

First selected-family read:

```text
run1:
  directlocalfree-ownerlocalityfast-rsscap-4:
    strict balanced     +25.0%
    strict wide_ws      +13.0%
    strict larger_sizes  +6.0%
    speed/rss mixed results, some rows negative

  directlocalfree-ownerlocalityfast-widecap-4:
    helps some balanced/larger_sizes rows
    regresses wide_ws in strict/speed/rss
```

Repeat-3 for rsscap-4 direct:

```text
balanced:
  strict  14.124M -> 12.363M  (-12.5%)
  speed   51.598M -> 53.438M  (+3.6%)
  rss     37.397M -> 36.958M  (-1.2%)

wide_ws:
  strict   1.243M ->  1.282M  (+3.1%)
  speed   14.939M -> 13.601M  (-9.0%)
  rss      4.685M ->  4.924M  (+5.1%)

larger_sizes:
  strict  21.043M -> 21.542M  (+2.4%)
  speed   27.745M -> 27.295M  (-1.6%)
  rss     24.005M -> 24.797M  (+3.3%)
```

Read:

```text
Selected-family directlocalfree is not clean enough for promotion.
The noboost-route4k direct lane remains strong same-owner hot-path evidence,
but adding the same behavior to ownerlocalityfast rsscap/widecap does not
produce a stable broad-selected improvement.

Status:
  directlocalfree-noboost-route4k:
    KEEP candidate-control / hot-path evidence

  directlocalfree-ownerlocalityfast-rsscap-4:
    KEEP as control evidence, not promotion

  directlocalfree-ownerlocalityfast-widecap-4:
    no-go/control for selected-family wide_ws

Next:
  do not blindly mix directlocalfree into all selected-family lanes.
  If continuing this path, make the direct local free behavior profile-aware
  or class/front gated.
```

## DescriptorAvailCount-L1 2026-06-02

```text
Context:
  mixed_ws noboost-route4k spends a large amount of time failing descriptor
  acquisition. Diagnostic runs showed descriptor_exhausted in the millions
  and descriptor_probe_total in the billions:
    balanced     1.546B probes
    wide_ws      1.541B probes
    larger_sizes 0.472B probes

Change:
  HZ6_DESCRIPTOR_AVAIL_COUNT_L1 adds descriptor_available_count and a
  descriptor->allocator back-pointer.

  find_free_descriptor:
    if descriptor_available_count == 0:
      return exhausted immediately
    else:
      scan normally and decrement the count when a descriptor is claimed

  release / detach / orphan adopt:
    reset the descriptor through hz6_allocator_reset_descriptor_available()
    so the count is restored exactly when a descriptor returns to DEAD/free.

Lane:
  descavail-noboost-route4k

Contract:
  capacity unchanged
  RSS intent unchanged
  no frontcache behavior change
  no source/admission behavior change
  no production diagnostic atomics/counters
```

Repeat-3 mixed_ws strict:

```text
balanced:
  noboost-route4k             21.631M, 24,312 KB
  descavail-noboost-route4k   70.746M, 24,320 KB

wide_ws:
  noboost-route4k             15.376M, 25,004 KB
  descavail-noboost-route4k   37.250M, 25,000 KB

larger_sizes:
  noboost-route4k              1.441M, 15,660 KB
  descavail-noboost-route4k    1.468M, 15,660 KB
```

Diagnostic mechanism check, run1:

```text
balanced:
  descriptor_probe_total 1,546,434,546 -> 7,154

wide_ws:
  descriptor_probe_total 1,540,957,689 -> 44,537

larger_sizes:
  descriptor_probe_total   471,693,810 -> 54,258

Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
```

Random_mixed guard repeat-3:

```text
small:
  noboost-route4k             34.124M, 4,964 KB
  descavail-noboost-route4k   34.118M, 5,416 KB

medium:
  noboost-route4k             34.190M, 4,960 KB
  descavail-noboost-route4k   34.836M, 4,968 KB

mixed:
  noboost-route4k             32.836M, 4,960 KB
  descavail-noboost-route4k   33.160M, 4,968 KB
```

Read:

```text
DescriptorAvailCount-L1 is a strong mechanism fix for descriptor-failure
heavy mixed_ws rows. It does not change the number of failures; it changes
failure cost from "scan the entire descriptor table every time" to "known full,
fail immediately".

Status:
  descavail-noboost-route4k:
    KEEP as mixed_ws candidate-control / descriptor-failure cost fix

  random_mixed:
    neutral guard; do not claim this as the primary random_mixed speed lane

Next:
  test whether descavail composes with directlocalfree-noboost-route4k.
  If it composes, the next lane should be explicitly named rather than silently
  replacing either mechanism.
```

## DirectLocalFree + DescriptorAvail Composition 2026-06-02

```text
Lane:
  directlocalfree-descavail-noboost-route4k

Purpose:
  Check whether the local-owner free fast path and descriptor failure-cost fix
  compose cleanly under the same low-RSS noboost-route4k capacity shape.

Important:
  This is not a silent replacement for either parent lane. DirectLocalFree-L1
  bypasses the generic front contract for TOY/MIDPAGE/LOCAL2P local-owner frees,
  while DescriptorAvailCount-L1 changes descriptor exhaustion cost. Keep the
  composition explicitly named.
```

Repeat-3 mixed_ws strict:

```text
balanced:
  noboost-route4k                         22.483M, 24,324 KB
  directlocalfree-noboost-route4k         21.863M, 24,316 KB
  descavail-noboost-route4k               74.332M, 24,312 KB
  directlocalfree-descavail-noboost-route4k
                                             62.531M, 24,760 KB

wide_ws:
  noboost-route4k                         15.413M, 25,004 KB
  directlocalfree-noboost-route4k         14.660M, 25,008 KB
  descavail-noboost-route4k               31.559M, 25,000 KB
  directlocalfree-descavail-noboost-route4k
                                             33.910M, 24,996 KB

larger_sizes:
  noboost-route4k                          1.428M, 15,656 KB
  directlocalfree-noboost-route4k          1.446M, 15,656 KB
  descavail-noboost-route4k                1.496M, 15,652 KB
  directlocalfree-descavail-noboost-route4k
                                              1.511M, 15,652 KB
```

Repeat-3 random_mixed strict:

```text
small:
  noboost-route4k                         34.687M, 4,972 KB
  directlocalfree-noboost-route4k         35.089M, 4,968 KB
  descavail-noboost-route4k               34.523M, 4,968 KB
  directlocalfree-descavail-noboost-route4k
                                             36.120M, 5,408 KB

medium:
  noboost-route4k                         34.505M, 4,968 KB
  directlocalfree-noboost-route4k         37.531M, 4,964 KB
  descavail-noboost-route4k               34.986M, 4,968 KB
  directlocalfree-descavail-noboost-route4k
                                             38.598M, 4,972 KB

mixed:
  noboost-route4k                         32.960M, 4,968 KB
  directlocalfree-noboost-route4k         36.126M, 4,964 KB
  descavail-noboost-route4k               32.924M, 4,968 KB
  directlocalfree-descavail-noboost-route4k
                                             36.924M, 4,968 KB
```

Read:

```text
Composition is workload-shaped:
  mixed_ws balanced:
    descavail alone wins. DirectLocalFree adds overhead/noise here.

  mixed_ws wide_ws / larger_sizes:
    composition edges out descavail.

  random_mixed:
    composition is the strongest strict low-RSS speed lane, especially medium
    and mixed. Small improves speed but has a small RSS jump.

Status:
  directlocalfree-descavail-noboost-route4k:
    KEEP as explicit composition candidate-control
    not universal promotion

Next:
  if continuing low-RSS speed recovery, consider profile selection:
    mixed_ws balanced -> descavail-noboost-route4k
    random_mixed -> directlocalfree-descavail-noboost-route4k
    wide_ws/larger_sizes -> compare descavail vs composition in closeout
```

## MixedWs Profile Family Closeout 2026-06-02

```text
Command:
  powershell -NoProfile -ExecutionPolicy Bypass `
    -File win/run_win_hz6_capacity_matrix.ps1 `
    -Families mixed_ws `
    -BenchmarkProfiles balanced,wide_ws,larger_sizes `
    -Hz6Profiles strict,speed,rss `
    -CapacityLanes `
      noboost-route4k,`
      descavail-noboost-route4k,`
      directlocalfree-descavail-noboost-route4k,`
      ownerlocalityfast-rsscap-4,`
      ownerlocalityfast-widecap-4,`
      ownerlocalityfast-appcap `
    -Runs 3
```

Best reads from repeat-3:

```text
balanced:
  rss + directlocalfree-descavail-noboost-route4k:
    78.527M ops/s, 18,464 KB

  rss + descavail-noboost-route4k:
    78.412M ops/s, 18,472 KB

  strict + descavail-noboost-route4k:
    75.792M ops/s, 24,316 KB

wide_ws:
  rss + descavail-noboost-route4k:
    57.183M ops/s, 13,164 KB

  rss + directlocalfree-descavail-noboost-route4k:
    55.279M ops/s, 13,160 KB

  speed + ownerlocalityfast-widecap-4:
    24.624M ops/s, 358,904 KB

larger_sizes:
  speed + ownerlocalityfast-rsscap-4:
    27.063M ops/s, 169,976 KB

  rss + ownerlocalityfast-rsscap-4:
    24.155M ops/s, 169,996 KB

  rss + descavail-noboost-route4k:
     0.793M ops/s, 14,896 KB
```

Read:

```text
descavail changes the profile family:
  balanced / wide_ws no longer need ownerlocalityfast-widecap-4 as the primary
  RSS/speed answer. rss profile + descavail-noboost-route4k is faster and far
  lower RSS.

larger_sizes is still different:
  descavail fixes descriptor failure cost, but it does not solve larger object
  supply/backend pressure. ownerlocalityfast-rsscap-4 remains the larger_sizes
  performance candidate-control, with much higher RSS.

Status:
  rss + descavail-noboost-route4k:
    historical balanced / wide_ws pressure evidence.
    Superseded as the clean selected row by:
      rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k

  strict + directlocalfree-descavail-noboost-route4k:
    selected random_mixed low-RSS speed candidate-control

  speed/rss + ownerlocalityfast-rsscap-4:
    superseded as larger_sizes selected lane by largerlowrss-front8k

  ownerlocalityfast-widecap-4:
    demoted from current wide_ws selected lane to historical control/evidence

Next:
  test a larger_sizes low-RSS supply/backend lane:
    front retention should be raised without appcap-sized source/route caps.
```

## LargerLowRss Front8k SourceRun 2026-06-02

```text
Problem:
  descavail fixes descriptor failure cost, but larger_sizes remained slow.

Diagnostic read:
  descavail larger_sizes:
    allocation failures remain high
    descriptor exhaustion remains high
    source_block probes remain high

  desc8k/source512/route8k:
    allocation failures reach 0
    but route register probes remain around 2B
    source_alloc remains about 22K..27K

  ownerlocalityfast-rsscap-4:
    source_alloc falls to about 1.4K..2.8K
    route register probes fall to about 20K
    but RSS is about 170MB

Hypothesis:
  larger_sizes needs more front retention for larger object slots. Without it,
  HZ6 keeps recreating source blocks and exact routes. The issue is not
  descriptor failure cost anymore.
```

Target lane:

```text
largerlowrss-front8k-sourcerun-desc8k-route8k:
  descriptor capacity: 8192
  route capacity: 8192
  source block capacity: 512
  frontcache bin capacity: 8192
  SourceRunReuse-L1: on
```

Diagnostic run1:

```text
rss:
  largerlowrss-sourcerun-desc8k-route8k:
     0.706M ops/s, 70,800 KB
     source_alloc=27869
     route_register_probe_total=2.162B

  largerlowrss-front8k-sourcerun-desc8k-route8k:
    32.560M ops/s, 72,892 KB
    source_alloc=1652
    route_register_probe_total=33K

  ownerlocalityfast-rsscap-4:
    21.552M ops/s, 170,028 KB
    source_alloc=2795
    route_register_probe_total=22K

speed:
  largerlowrss-sourcerun-desc8k-route8k:
     0.737M ops/s, 70,036 KB
     source_alloc=22475
     route_register_probe_total=2.125B

  largerlowrss-front8k-sourcerun-desc8k-route8k:
    34.107M ops/s, 72,768 KB
    source_alloc=1329
    route_register_probe_total=32K

  ownerlocalityfast-rsscap-4:
    24.043M ops/s, 169,968 KB
    source_alloc=1409
    route_register_probe_total=20K
```

Repeat-3 larger_sizes:

```text
rss:
  largerlowrss-front8k-sourcerun-desc8k-route8k:
    34.286M ops/s, 72,836 KB

  ownerlocalityfast-rsscap-4:
    23.842M ops/s, 169,972 KB

speed:
  largerlowrss-front8k-sourcerun-desc8k-route8k:
    35.353M ops/s, 72,792 KB

  ownerlocalityfast-rsscap-4:
    27.041M ops/s, 170,012 KB
```

Guard:

```text
balanced / run1:
  rss + descavail-noboost-route4k:
    77.222M ops/s, 18,040 KB

  rss + largerlowrss-front8k-sourcerun-desc8k-route8k:
    66.387M ops/s, 100,036 KB

  speed + largerlowrss-front8k-sourcerun-desc8k-route8k:
    81.704M ops/s, 100,084 KB

wide_ws / run1:
  rss + descavail-noboost-route4k:
    60.546M ops/s, 13,164 KB

  rss + largerlowrss-front8k-sourcerun-desc8k-route8k:
     0.337M ops/s, 72,984 KB

  speed + largerlowrss-front8k-sourcerun-desc8k-route8k:
     0.367M ops/s, 72,912 KB
```

Read:

```text
largerlowrss-front8k is a strong larger_sizes-specific candidate-control.
It is not a universal mixed_ws lane.

Status:
  larger_sizes:
    SELECT speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

  balanced:
    keep rss + descavail-noboost-route4k as lower-RSS default
    largerlowrss-front8k can be an upper-speed control only

  wide_ws:
    largerlowrss-front8k is no-go

Next:
  larger_sizes is no longer the obvious weak row in this matrix. The remaining
  cleanup is lane naming / selection table and then broader benchmark
  comparison against HZ3/HZ4/HZ5/mimalloc/tcmalloc using the selected HZ6 rows.
```

## Windows Profile Family Freeze 2026-06-02

```text
Decision:
  HZ6 Windows is now treated as a profile family, not as one universal best
  lane.

Family:
  balanced-widews-lowrss-speed:
    lane:
      rss + descavail-noboost-route4k
    role:
      low-RSS speed candidate-control for balanced / wide_ws
    strong:
      balanced
      wide_ws
    weak:
      larger_sizes performance

  randommixed-lowrss-speed:
    lane:
      strict + directlocalfree-descavail-noboost-route4k
    role:
      random_mixed same-owner hot-path + descriptor-failure cost control
    strong:
      random_mixed medium / mixed
    weak:
      small row has a small RSS jump

  larger-sizes-perf:
    lane:
      speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k
    role:
      larger_sizes performance candidate-control
    strong:
      larger_sizes
    weak:
      not suitable for wide_ws; balanced RSS is higher than descavail

  perf-recovery-upper-bound:
    lane:
      ownerlocalityfast-appcap
    role:
      owner-locality / high-capacity completion upper-bound
    strong:
      completion / capacity stress
    weak:
      peak working set is appcap-sized

  redis-evidence:
    lanes:
      redislowrss-sourcerun-desc8k-route8k
      redislowrss-sourcerun-desc8k-route8k-tombcompact
      redislowrss-sourcerun-desc8k-route8k-slotlookup
    role:
      Redis paper-row evidence
    status:
      frozen / evidence-only

Next design step:
  larger_sizes low-RSS supply/backend track

  D-lite:
    explain why largerlowrss-front8k wins larger_sizes using existing
    route / descriptor / source / frontcache counters first

  A:
    reduce largerlowrss-front8k RSS further without returning to route/source
    churn

Do not:
  blur descavail-noboost-route4k by adding broad large-size capacity.
  Balanced / wide_ws already have a low-RSS speed answer; larger_sizes needs a
  separate supply/backend design.
```

## Windows Wide-WS First Pass 2026-06-02

```text
Fast lane run:
  wide_ws / mixed_ws / run1

  strict:
    route4k              0.330M ops/s, 25,376 KB
    noboost-route4k     15.618M ops/s, 25,512 KB
    ownerlocalityfast    1.801M ops/s, 958,204 KB

  speed:
    route4k              0.311M ops/s, 12,952 KB
    noboost-route4k      0.337M ops/s, 12,648 KB
    ownerlocalityfast   21.475M ops/s, 538,548 KB

  rss:
    route4k              0.378M ops/s, 13,356 KB
    noboost-route4k     18.877M ops/s, 13,692 KB
    ownerlocalityfast    9.002M ops/s, 562,680 KB

Diagnostic lane run:
  strict:
    route4k              0.286M ops/s, 25,392 KB
    noboost-route4k      5.875M ops/s, 25,536 KB
    ownerlocality-appcap 2.512M ops/s, 960,516 KB

  speed:
    route4k              0.289M ops/s, 12,964 KB
    noboost-route4k      0.348M ops/s, 12,664 KB
    ownerlocality-appcap 16.877M ops/s, 538,488 KB

  rss:
    route4k              0.362M ops/s, 13,372 KB
    noboost-route4k      5.253M ops/s, 13,704 KB
    ownerlocality-appcap 6.864M ops/s, 562,480 KB

Balanced lane run:
  strict:
    route4k              0.523M ops/s, 24,808 KB
    noboost-route4k     23.790M ops/s, 24,848 KB
    ownerlocalityfast   14.625M ops/s, 560,900 KB

  speed:
    route4k              0.510M ops/s, 18,156 KB
    noboost-route4k      0.553M ops/s, 18,148 KB
    ownerlocalityfast   44.525M ops/s, 506,168 KB

  rss:
    route4k              3.273M ops/s, 18,568 KB
    noboost-route4k     16.397M ops/s, 18,552 KB
    ownerlocalityfast   32.424M ops/s, 509,404 KB

Interpretation:
  noboost-route4k still owns strict wide_ws as the current candidate-control.
  ownerlocalityfast-appcap is the speed / rss recovery lane.
  ownerlocality-appcap does not beat noboost on strict, but remains useful
  as diagnostic evidence for route-locality and cross-owner lifecycle fixes.
  balanced confirms the same split: noboost wins strict, while
  ownerlocalityfast dominates speed/rss.

Next attack:
  separate strict wide_ws from speed/rss recovery instead of assuming one lane
  should win everywhere
```

## Windows Larger-Sizes First Pass 2026-06-02

```text
Fast lane run:
  larger_sizes / mixed_ws / run1

  strict:
    route4k              0.745M ops/s, 16,148 KB
    noboost-route4k      1.316M ops/s, 16,276 KB
    ownerlocalityfast   14.943M ops/s, 289,864 KB

  speed:
    route4k              0.523M ops/s, 15,168 KB
    noboost-route4k      0.544M ops/s, 15,240 KB
    ownerlocalityfast   19.014M ops/s, 284,680 KB

  rss:
    route4k              0.620M ops/s, 15,472 KB
    noboost-route4k      0.615M ops/s, 15,444 KB
    ownerlocalityfast   18.659M ops/s, 284,652 KB

Diagnostic lane run:
  strict:
    route4k              0.704M ops/s, 16,184 KB
    noboost-route4k      1.038M ops/s, 16,244 KB
    ownerlocality-appcap 11.984M ops/s, 289,944 KB

  speed:
    route4k              0.753M ops/s, 15,184 KB
    noboost-route4k      0.715M ops/s, 15,256 KB
    ownerlocality-appcap 16.170M ops/s, 284,788 KB

  rss:
    route4k              0.731M ops/s, 15,476 KB
    noboost-route4k      0.728M ops/s, 15,504 KB
    ownerlocality-appcap 13.218M ops/s, 284,724 KB

Interpretation:
  larger_sizes does not follow the balanced/wide_ws split.
  ownerlocalityfast-appcap wins strict/speed/rss, while noboost-route4k only
  preserves low peak working set.

Next attack:
  treat larger_sizes as a capacity / owner-locality recovery profile.
  Do not use strict noboost as the large-size performance lane.
```

## Larger-Sizes D-lite Read 2026-06-02

```text
Important correction:
  mixed_ws larger_sizes is size=256..8192.
  It is larger than balanced/wide_ws, but it is not the LargeSpan front.

Observed:
  ownerlocalityfast-appcap wins larger_sizes, but:
    large_span_central_push = 0
    large_span_central_pop = 0
    large_span_source_alloc = 0

  noboost-route4k fails performance with:
    descriptor_exhausted > 0
    alloc_fail > 0
    route_register_fail = 0
    source_block_exhausted = 0

  ownerlocalityfast-appcap removes:
    descriptor_exhausted
    alloc_fail
    source_block_exhausted
    route_register_fail

Interpretation:
  larger_sizes is a mid/source-block capacity and owner-locality profile, not a
  LargeSpan / large central pool profile.

Next:
  ownerlocalityfast-rsscap should trim appcap capacity in stages while keeping
  owner-locality behavior:
    1. frontcache / transfer
    2. source-block
    3. descriptor
    4. route

  LargeSpan-specific counters are not the next bottleneck for this row.
```

## OwnerLocalityFast RSSCap Read 2026-06-02

```text
Lane definitions:
  ownerlocalityfast-rsscap-1:
    ownerlocalityfast-appcap behavior
    transfer/frontcache trimmed:
      transfer = 4096
      frontcache bin = 8192
    descriptor / route / source-block stay appcap-sized

  ownerlocalityfast-rsscap-2:
    rsscap-1 plus source-block trimmed:
      source-block = 8192

  ownerlocalityfast-rsscap-3:
    rsscap-2 plus descriptor trimmed:
      descriptor = 131072

  ownerlocalityfast-rsscap-4:
    rsscap-3 plus route trimmed:
      route = 131072

larger_sizes / mixed_ws / run1:
  rsscap-1:
    performance preserved, but peak working set is essentially unchanged.
    This suggests transfer/frontcache capacity is not the main peak driver for
    this row.

  rsscap-2:
    strict:
      appcap 15.509M ops/s, 289,944 KB
      rsscap-2 18.058M ops/s, 232,980 KB
    speed:
      appcap 19.962M ops/s, 284,740 KB
      rsscap-2 20.420M ops/s, 227,880 KB
    rss:
      appcap 17.933M ops/s, 284,824 KB
      rsscap-2 20.669M ops/s, 227,928 KB

  rsscap-3:
    strict:
      appcap 16.600M ops/s, 289,916 KB
      rsscap-2 17.530M ops/s, 233,012 KB
      rsscap-3 19.053M ops/s, 196,152 KB
    speed:
      appcap 19.132M ops/s, 284,776 KB
      rsscap-2 22.983M ops/s, 227,920 KB
      rsscap-3 20.889M ops/s, 191,096 KB
    rss:
      appcap 17.730M ops/s, 284,756 KB
      rsscap-2 21.117M ops/s, 227,888 KB
      rsscap-3 21.846M ops/s, 191,088 KB

  rsscap-4:
    strict:
      rsscap-3 18.171M ops/s, 196,196 KB
      rsscap-4 20.340M ops/s, 171,636 KB
    speed:
      rsscap-3 23.315M ops/s, 191,040 KB
      rsscap-4 26.260M ops/s, 166,472 KB
    rss:
      rsscap-3 23.554M ops/s, 191,040 KB
      rsscap-4 23.693M ops/s, 166,464 KB
    repeat-3:
      appcap median peak 289,856 KB
      rsscap-4 median peak 166,480 KB
      strict 20.573M ops/s
      speed 23.221M ops/s
      rss 23.693M ops/s

Safety counters checked in the rsscap-2/rsscap-3 runs:
  descriptor_exhausted = 0
  route_register_fail = 0
  source_block_exhausted = 0
  route_invalid = 0
  route_miss = 0
  large_span_* = 0

Interpretation:
  source-block capacity / retention is a major peak contributor for
  larger_sizes. Trimming source-block capacity from 32768 to 8192 lowers peak
  by roughly 57 MiB while preserving or improving throughput in this run.

  Descriptor capacity is also a major peak contributor. Trimming descriptor
  capacity from 262144 to 131072 on top of rsscap-2 lowers peak again to about
  191..196 MiB with clean safety counters in the current larger_sizes run.

  Route capacity can also be trimmed for larger_sizes. Trimming route capacity
  from 262144 to 131072 on top of rsscap-3 lowers peak again to about
  166..172 MiB with clean safety counters in the current larger_sizes run.
  The repeat-3 run confirms the same shape: peak stays around 166 MiB and
  throughput remains strong.

Guard read:
  balanced:
    rsscap-4 cuts appcap peak roughly in half and improves speed/rss profiles,
    but strict noboost-route4k remains the low-RSS strict lane.

  wide_ws:
    rsscap-4 cuts appcap peak, but throughput regresses versus
    ownerlocalityfast-appcap in strict/speed/rss.
    Keep rsscap-4 as larger_sizes-specific candidate-control.

Status:
  ownerlocalityfast-rsscap-1:
    no-op evidence for transfer/frontcache trimming.

  ownerlocalityfast-rsscap-2:
    promising perf-recovery RSSCap candidate-control.
    Superseded by rsscap-3 for peak reduction if guard rows stay clean.

  ownerlocalityfast-rsscap-3:
    strong larger_sizes RSSCap evidence, superseded by rsscap-4 for this row
    if guard rows stay acceptable.
    Not promotion yet; needs repeat and guard rows.

  ownerlocalityfast-rsscap-4:
    current strongest larger_sizes RSSCap candidate-control.
    Not a universal mixed_ws promotion because wide_ws guard regresses versus
    appcap throughput.

Next:
  repeat larger_sizes for rsscap-4 and keep wide_ws guarded by
  ownerlocalityfast-appcap / noboost-route4k instead of promoting rsscap-4
  broadly.
```

## OwnerLocalityFast WideCap Read 2026-06-02

```text
Why:
  rsscap-4 is strong for larger_sizes, but wide_ws regresses versus
  ownerlocalityfast-appcap. The wide_ws sweep separates hot-cache capacity
  from source-block / descriptor retention.

First sweep:
  ownerlocalityfast-rsscap-1:
    transfer/frontcache = 4096 / 8192

  ownerlocalityfast-widecap-1:
    transfer/frontcache = 32768 / 32768
    descriptor / route / source-block stay appcap-sized

  ownerlocalityfast-widecap-2:
    transfer/frontcache = 16384 / 16384
    descriptor / route / source-block stay appcap-sized

wide_ws / mixed_ws / run1:
  strict:
    appcap     2.891M ops/s, 959,264 KB
    rsscap-1   1.087M ops/s, 951,220 KB
    widecap-1  2.583M ops/s, 959,888 KB
    widecap-2  2.694M ops/s, 960,116 KB

  speed:
    appcap    20.277M ops/s, 538,484 KB
    rsscap-1  12.621M ops/s, 531,720 KB
    widecap-1 20.756M ops/s, 538,672 KB
    widecap-2 18.951M ops/s, 538,424 KB

  rss:
    appcap     9.172M ops/s, 562,816 KB
    rsscap-1   4.166M ops/s, 555,984 KB
    widecap-1  9.414M ops/s, 562,756 KB
    widecap-2  9.410M ops/s, 562,644 KB

Interpretation:
  wide_ws is hot-cache sensitive. rsscap-1 trims transfer/frontcache too far:
  source_alloc rises from appcap-class 132,617 / 8,302 / 33,165 to
  302,863 / 75,737 / 75,737 in strict/speed/rss. widecap-1/2 restore the
  source_alloc shape and appcap-class throughput, but peak remains appcap-sized.

Second sweep:
  ownerlocalityfast-widecap-3:
    widecap-2 plus source-block = 8192

  ownerlocalityfast-widecap-4:
    widecap-3 plus descriptor = 131072

wide_ws / mixed_ws / run1:
  strict:
    appcap     2.591M ops/s, 959,912 KB
    widecap-2  2.394M ops/s, 960,012 KB
    widecap-3  2.518M ops/s, 846,688 KB
    widecap-4  2.727M ops/s, 772,980 KB

  speed:
    appcap    18.375M ops/s, 538,388 KB
    widecap-2 20.601M ops/s, 538,440 KB
    widecap-3 20.603M ops/s, 424,876 KB
    widecap-4 18.620M ops/s, 351,076 KB

  rss:
    appcap     7.218M ops/s, 562,824 KB
    widecap-2  8.907M ops/s, 562,528 KB
    widecap-3  8.235M ops/s, 449,116 KB
    widecap-4  9.540M ops/s, 375,304 KB

wide_ws / mixed_ws / repeat-3:
  strict:
    appcap     2.646M ops/s, 960,448 KB
    widecap-2  2.807M ops/s, 960,032 KB
    widecap-4  2.789M ops/s, 772,956 KB

  speed:
    appcap    20.622M ops/s, 538,500 KB
    widecap-2 21.089M ops/s, 538,592 KB
    widecap-4 22.115M ops/s, 351,104 KB

  rss:
    appcap     9.019M ops/s, 562,752 KB
    widecap-2  9.663M ops/s, 562,748 KB
    widecap-4 10.742M ops/s, 375,384 KB

Safety counters checked in the widecap-3/widecap-4 run:
  descriptor_exhausted = 0
  route_register_fail = 0
  source_block_exhausted = 0
  route_invalid = 0
  route_miss = 0

Interpretation:
  wide_ws can keep a larger hot cache while trimming source-block and
  descriptor capacity. This gives a distinct profile from rsscap-4:

    rsscap-4:
      larger_sizes RSSCap candidate-control

    widecap-4:
      wide_ws RSS/speed candidate-control

  Do not collapse them into one lane yet. widecap-4 needs repeat/guard rows,
  while rsscap-4 remains the stronger larger_sizes profile-scoped lane.

Guard read:
  balanced:
    widecap-4 stays competitive and lowers peak versus appcap:
      strict 14.081M ops/s, 373,240 KB
      speed  50.230M ops/s, 318,800 KB
      rss    32.246M ops/s, 321,880 KB
    rsscap-4 still has the lowest peak, but widecap-4 is not a balanced no-go.

  larger_sizes:
    rsscap-4 remains clearly better than widecap-4:
      rsscap-4 speed/rss 25.504M / 23.536M ops/s, about 166 MiB
      widecap-4 speed/rss 20.190M / 17.049M ops/s, about 191 MiB

Status:
  ownerlocalityfast-widecap-4:
    current strongest wide_ws RSS/speed candidate-control.
    Also acceptable as balanced guard evidence.
    Not a larger_sizes replacement; keep rsscap-4 for larger_sizes.

Next:
  freeze the split:
    wide_ws:
      ownerlocalityfast-widecap-4
    larger_sizes:
      ownerlocalityfast-rsscap-4
    strict-lowrss:
      noboost-route4k

  If optimizing further, attack policy selection / profile naming, not another
  static capacity sweep.
```

## Redis ControlPlane Checkpoint 2026-06-02

```text
Observation:
  redis_long diagnostics now capture completed-run HZ6_REDIS_STATS.

  noboost-route4k:
    remains a timeout / low-capacity control on Redis long.

  redislowrss-slim-route4k:
    completes, but LPUSH collapses under descriptor/source-block exhaustion:
      descriptor_exhausted > 0
      source_block_exhausted > 0
      alloc_fail > 0

  redislowrss-route4k:
    completes Redis long with LPUSH materially recovered and alloc_fail = 0.
    It is the current Redis-like candidate-control, not a general mixed_ws
    promotion lane.

Next narrow lane:
  redislowrss-sourcerun-route4k

Purpose:
  Keep the redislowrss descriptor/source-block capacity shape, add
  SourceRunReuse-L1, and check whether RANDOM source_block_exhausted /
  source_prefill_fallback can fall without widening capacity again.

Observed:
  redis_long / rss / run1:
    redislowrss-route4k:
      SET 30.33M, GET 249.81M, LPUSH 29.75M, LPOP 368.55M, RANDOM 43.19M
      peak 23,260 KB
      RANDOM source_block_exhausted = 488

    redislowrss-sourcerun-route4k:
      SET 40.24M, GET 547.00M, LPUSH 35.47M, LPOP 727.36M, RANDOM 92.20M
      peak 13,116 KB
      source_block_exhausted = 0 in checked Redis patterns
      source_alloc falls sharply, e.g. RANDOM 4744 -> 36

  mixed_ws guard / rss / run1:
    noboost-route4k remains the general low-capacity mixed_ws lane.
    redislowrss-sourcerun-route4k is not a general promotion lane.
    It improves some redislowrss source pressure, but still over-retains for
    balanced/wide_ws relative to noboost.

Decision:
  Keep redislowrss-sourcerun-route4k as Redis-long SourceRun evidence.
  Keep redislowrss-sourcerun-desc8k-route8k as paper-aligned Redis
  candidate-control.
  Keep noboost-route4k as general mixed_ws low-capacity candidate-control.

Follow-up:
  paper-aligned Redis row uses a larger live set:
    threads=4 cycles=500 ops=2000 size=16..256

  redislowrss-sourcerun-route4k:
    repeat-3:
      SET 38.04M, GET 276.90M, LPUSH 8.12M, LPOP 827.92M, RANDOM 41.41M
      peak 14,444 KB
    Diagnostic:
      LPUSH route_register_fail > 0
      route_register_probe_max = 4096
    Meaning:
      source-run fixed source pressure, but route4k is too small for paper
      LPUSH.

  redislowrss-sourcerun-route8k:
    Diagnostic:
      route_register_fail = 0
      LPUSH descriptor_exhausted > 0
    Meaning:
      route pressure is fixed, descriptor capacity becomes the next bottleneck.

  redislowrss-sourcerun-desc8k-route8k:
    repeat-3:
      SET 36.62M, GET 297.81M, LPUSH 32.48M, LPOP 1101.11M, RANDOM 39.97M
      peak 17,704 KB
    Diagnostic:
      route_register_fail = 0
      descriptor_exhausted = 0
      source_block_exhausted = 0
      alloc_fail = 0
    Meaning:
      current paper-aligned Redis candidate-control. It does not beat
      mimalloc/tcmalloc on SET/LPUSH/RANDOM, but it beats the older HZ6 appcap
      shape on GET/LPOP and keeps much lower peak working set.

Do not:
  treat slim as the current Redis candidate
  widen beyond redislowrss before trying source-run reuse
  mix diagnostic stats into non-diagnostic benchmark lanes
```

## RedisRouteChurnDiag-L1 2026-06-02

```text
Decision:
  Next step is route/register churn attribution, not immediate behavior.

Why:
  redislowrss-sourcerun-desc8k-route8k removes allocation capacity failures:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0

  Remaining paper-row gap is LPUSH/RANDOM throughput. RANDOM still shows large
  route lookup/register probe totals, so classify route churn before changing
  retention policy.

Implemented diagnostic shape:
  register reasons:
    source_run_slot
    direct_source
    materialize
    rehome
    unknown

  unregister reasons:
    frontcache_overflow
    cap_release
    descriptorless_detach
    source_slot_release
    rehome
    unknown

  tombstone:
    route_tombstone_current
    route_tombstone_max
    route_register_used_tombstone
    route_register_full_probe_with_tombstone

Next read:
  If unregister is dominated by overflow/source_slot_release, try
  RouteRetainedOverflow-L1.

  If tombstone/full-probe dominates, try RouteTombstoneCompact-L1.

  Do not jump to SourceRunSlotLookup until exact-route retention and tombstone
  evidence are exhausted.

Observed run:
  results/hz6-routechurn-diag-l1-paper
  redis_workload paper row:
    threads=4 cycles=500 ops=2000 size=16..256

  redislowrss-sourcerun-desc8k-route8k / rss / diagnostic / run1:
    SET 38.23M, GET 400.24M, LPUSH 34.19M, LPOP 1682.30M, RANDOM 41.25M
    peak 17,324 KB

  SET:
    source_run_slot registers = 8032
    frontcache_overflow unregisters = 5936
    route_register_full_probe_with_tombstone = 0
    route_tombstone_current = 5956

  LPUSH:
    source_run_slot registers = 16608
    frontcache_overflow unregisters = 14444
    route_register_full_probe_with_tombstone = 0
    route_tombstone_current = 14444
    route_register_probe_max = 23

  RANDOM:
    source_run_slot registers = 42960
    frontcache_overflow unregisters = 40824
    route_register_probe_total = 86406606
    route_register_probe_max = 8192
    route_lookup_probe_total = 58634916
    route_lookup_probe_max = 8128
    route_register_used_tombstone = 10252
    route_register_full_probe_with_tombstone = 10252
    route_tombstone_current = 30572
    route_tombstone_max = 7645

Interpretation:
  Route unregister churn is almost entirely frontcache overflow. LPUSH has many
  overflow unregisters but no tombstone full-probe. RANDOM is the route-table
  collapse row: tombstones accumulate and exact register/lookup hit full-table
  probes. The next safest behavior experiment is RouteTombstoneCompact-L1,
  because it targets RANDOM's measured full-probe/tombstone failure without
  changing descriptor/source retention policy yet. RouteRetainedOverflow-L1
  remains the follow-up if compacting helps RANDOM but LPUSH/SET stay behind.
```

## RouteTombstoneCompact-L1 2026-06-02

```text
Lane:
  redislowrss-sourcerun-desc8k-route8k-tombcompact

Scope:
  Redis paper-row route-churn behavior probe.
  Not a promotion lane.
  Not a descriptor/source retention change.

Behavior:
  After exact-route unregister, if route tombstones exceed:
    max(HZ6_ROUTE_TOMBSTONE_COMPACT_MIN, route_capacity / 2)
  rebuild the route table from active entries and clear tombstones.

Why first:
  RANDOM has route_register_probe_max = 8192 and
  route_register_full_probe_with_tombstone = 10252.
  LPUSH has overflow unregisters but no tombstone full-probe, so retained
  overflow may still be needed later, but tombstone compact is the smallest
  direct fix for the measured RANDOM collapse.

Counters:
  route_tombstone_compact_attempt
  route_tombstone_compact_success
  route_tombstone_compact_fail_alloc
  route_tombstone_compact_moved

Acceptance:
  RANDOM route_register_full_probe_with_tombstone drops near zero.
  RANDOM route_register_probe_total / route_lookup_probe_total fall materially.
  RANDOM throughput improves without route_register_fail, route_miss, or
  descriptor/source exhaustion.
  LPUSH/SET do not regress materially.

No-go:
  compact_fail_alloc > 0
  route_register_fail > 0
  route_miss / route_invalid safety counters worsen
  Redis peak working set rises beyond the current candidate-control envelope
  compact overhead erases RANDOM/LPUSH gains

Observed:
  results/hz6-tombcompact-l1-paper-repeat3b
  non-diagnostic repeat-3, redis_workload paper row:

  redislowrss-sourcerun-desc8k-route8k:
    SET 38.40M, GET 281.19M, LPUSH 29.13M, LPOP 767.67M, RANDOM 41.70M
    peak 17,288 KB

  redislowrss-sourcerun-desc8k-route8k-tombcompact:
    SET 36.72M, GET 275.75M, LPUSH 31.81M, LPOP 769.75M, RANDOM 81.58M
    peak 17,428 KB

Read:
  KEEP as Redis RANDOM route-churn behavior evidence.
  RANDOM improves about 96% and LPUSH improves about 9%, with only a small
  peak increase. SET/GET are modestly lower, so this is still Redis-specific
  evidence rather than general promotion. Next candidate after this is
  RouteRetainedOverflow-L1 if we want to recover SET/LPUSH without relying on
  tombstone cleanup alone.
```

## RouteRetainedOverflow-L1 2026-06-02

```text
Lane:
  redislowrss-sourcerun-desc8k-route8k-retainedoverflow

Scope:
  Redis frontcache-overflow behavior probe.
  Not combined with tombcompact at first.
  Not a new unbounded retained list.

Behavior:
  On free:
    ACTIVE -> LOCAL_FREE
    frontcache push fails
    instead of exact unregister + descriptor/source release:
      set descriptor state to TRANSFER_FREE
      push the object into the existing bounded transfer cache
      keep exact route registered

  If transfer push fails:
    fall back to existing exact unregister + release path.

Why:
  RedisRouteChurnDiag-L1 showed unregister reason is dominated by
  frontcache_overflow. Tombcompact fixes RANDOM's tombstone full-probe, but it
  does not reduce the overflow churn itself. RetainedOverflow-L1 tests whether
  a small bounded cold-retain layer can reduce SET/LPUSH/RANDOM churn without
  introducing a new cache structure.

Counters:
  route_retained_overflow_attempt
  route_retained_overflow_success
  route_retained_overflow_fail
  transfer_push / transfer_pop / transfer_current_max

Acceptance:
  frontcache_overflow unregisters fall materially.
  route_tombstone_current and register/lookup probes fall.
  SET/LPUSH/RANDOM improve versus the candidate-control.
  transfer_current remains bounded and transfer fail stays zero.

No-go:
  transfer_current grows toward capacity and stops draining.
  stale double-free becomes reusable.
  route_invalid / route_miss safety counters worsen.
  Redis peak working set rises materially.
  GET/LPOP or compact control regress more than the RANDOM/LPUSH gain.

Observed:
  results/hz6-retainedoverflow-l1-paper-diag
  diagnostic run1, redis_workload paper row:
    SET 39.10M, GET 400.35M, LPUSH 34.35M, LPOP 1313.50M, RANDOM 40.72M
    peak 17,760 KB

  RANDOM:
    route_retained_overflow_attempt = 414340
    route_retained_overflow_success = 373728
    route_retained_overflow_fail = 40612
    route_unregister_reason_frontcache_overflow = 40612
    route_register_full_probe_with_tombstone = 10106
    route_tombstone_current = 30507
    route_lookup_probe_total = 65624334
    route_register_probe_total = 85531512

Read:
  NO-GO as the next Redis fix. Bounded transfer retention does work
  mechanically, but it does not reduce the final overflow-unregister count
  enough to prevent tombstone/full-probe collapse. It also leaves RANDOM near
  the candidate-control speed. Keep as evidence that a simple transfer-backed
  retained overflow layer is not sufficient; tombstone compact remains the
  cleaner Redis RANDOM behavior lane.
```

## SourceRunSlotLookup-L1 2026-06-02

```text
Lane:
  redislowrss-sourcerun-desc8k-route8k-slotlookup

Scope:
  Redis source-run lookup policy probe.
  Same SourceRunReuse-L1 contract.
  No new retention or route-table behavior.

Behavior:
  When SourceRunReuse-L1 scans reusable run blocks, prefer the block with the
  most free slots instead of returning the first reusable block.

Why:
  TombstoneCompact fixed the route-table collapse row, and RetainedOverflow was
  no-go. The remaining way to improve SET/LPUSH without widening capacities is
  to see whether source-run slot selection can pick a better reusable block.

Counters:
  existing source_run_reuse_* counters only

Acceptance:
  SET and LPUSH recover versus the current source-run evidence lane.
  RANDOM does not regress materially.
  source_run_reuse_candidate remains bounded and safe counters stay zero.

No-go:
  source_run_reuse_used_count_mismatch appears.
  source_run_reuse_route_fail / prepare_fail / rollback rise.
  RANDOM or peak working set regresses materially.

Observed:
  results/hz6-slotlookup-l1-paper-repeat3
  non-diagnostic repeat-3, redis_workload paper row:

  redislowrss-sourcerun-desc8k-route8k:
    SET 37.22M, GET 282.89M, LPUSH 32.89M, LPOP 929.65M, RANDOM 40.69M
    peak 17,312 KB

  redislowrss-sourcerun-desc8k-route8k-slotlookup:
    SET 37.62M, GET 267.12M, LPUSH 32.29M, LPOP 1052.55M, RANDOM 39.16M
    peak 17,776 KB

Read:
  KEEP as source-run lookup evidence, not promotion.
  Slot lookup is mildly better on SET and roughly neutral on RANDOM, but it
  lowers GET, leaves LPUSH basically flat, and raises peak modestly. This is a
  useful lookup-policy witness, but not the new Redis candidate-control.
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

## Wide Working Set Source Admission Probe 2026-06-02

```text
Problem:
  route4k is safe and low-RSS, but wide_ws remains weak.

Baseline repeat-3:
  hz6-rss-route4k balanced     = 3.439M ops/s, 18,016 KB
  hz6-rss-route4k wide_ws      = 0.400M ops/s, 12,800 KB
  hz6-rss-route4k larger_sizes = 0.744M ops/s, 14,888 KB

Capacity probes:
  desc4k-route4k:
    wide_ws = 0.268M ops/s, 125,936 KB
    descriptor exhaustion disappears, but source allocation and RSS explode.

  source512-route4k:
    wide_ws = 0.362M ops/s, 14,588 KB
    source-block exhaustion disappears, but descriptor pressure remains.

  desc4k-source512-route4k:
    wide_ws = 0.223M ops/s, 77,760 KB

  front1k-desc4k-source512-route4k:
    wide_ws = 0.217M ops/s, 77,820 KB

Read:
  wide_ws is not fixed by raw capacity widening.
  More descriptors increase success count but drive source allocation and RSS
  too high.
  The problem is source admission / refill policy under alloc-fail pressure.
```

```text
No-go probes:
  descriptorcoldgov-route4k:
    balanced = 0.403M
    wide_ws = 0.344M
    larger_sizes = 0.733M

  descriptorcoldgov-widews-route4k:
    balanced = 0.394M
    wide_ws = 0.334M
    larger_sizes = 0.750M

  sourcerun-sameclass-route4k:
    balanced = 0.408M
    wide_ws = 0.383M
    larger_sizes = 0.784M

Read:
  descriptorless/cold-governor and source-run reclaim variants do not solve
  wide_ws. Keep them as evidence/control lanes, not next promotion candidates.
```

```text
New candidate-control:
  noboost-route4k

Change:
  route4k + HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1

Repeat-3:
  hz6-rss-noboost-route4k balanced     = 23.602M ops/s, 18,440 KB
  hz6-rss-noboost-route4k wide_ws      = 17.188M ops/s, 13,124 KB
  hz6-rss-noboost-route4k larger_sizes = 0.738M ops/s, 14,876 KB

Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0

Read:
  starvation boost is harmful for mixed_ws route4k on Windows.
  noboost-route4k is now the best Windows mixed_ws candidate-control lane.
  Next checks should compare it against ownerlocalityfast-appcap and broader
  app-like rows, then decide whether to make no-boost profile policy rather
  than a capacity lane flag.
```

## NoBoost Cross-Family Check 2026-06-02

```text
random_mixed repeat-3:
  small:
    route4k  = 26.607M ops/s
    noboost  = 29.238M ops/s

  medium:
    route4k  = 32.243M ops/s
    noboost  = 32.127M ops/s

  mixed:
    route4k  = 30.040M ops/s
    noboost  = 30.700M ops/s

Read:
  noboost-route4k is not only a mixed_ws/wide_ws rescue.
  It improves random_mixed small, is neutral on medium, and is slightly better
  on mixed while keeping peak working set in the same range.
  Keep it as the current Windows low-capacity candidate-control lane.
```

```text
redis check:
  default redis_workload row timed out on route4k before a matrix summary could
  be written.

  short direct smoke:
    args = 2 100 200 16 256
    route4k and noboost both complete.

  observation:
    Redis-like SET/LPUSH still emits many redis_alloc_string_fail stats lines
    as descriptor/source-block pressure accumulates.

Next:
  split Redis-like investigation from noboost validation.
  First make the Redis row shorter or add a focused profile in the matrix
  runner, then compare route4k vs noboost without letting timeout/output volume
  dominate the result.
```

## Redis Short Triage 2026-06-02

```text
Runner change:
  win/run_win_hz6_capacity_matrix.ps1 now supports focused Redis-like profiles:
    redis_workload = 4 500 2000 16 256
    redis_short    = 2 100 200 16 256
    redis_tiny     = 1 50 100 16 256

  Default Redis behavior is unchanged:
    no BenchmarkProfiles => redis_workload

  Redis summaries now keep stdout/stderr in the raw log but write only
  runN:ok / failed markers into the markdown table, so repeated HZ6 stats lines
  do not make the summary unreadable.
```

```text
redis_short run-1:
  route4k:
    SET    0.02 M ops/s
    GET   22.25 M ops/s
    LPUSH  0.01 M ops/s
    LPOP  30.83 M ops/s
    RANDOM 0.29 M ops/s
    peak  7.7 MiB

  noboost-route4k:
    SET    0.01 M ops/s
    GET   27.61 M ops/s
    LPUSH  0.01 M ops/s
    LPOP  21.19 M ops/s
    RANDOM 0.31 M ops/s
    peak  7.8 MiB

  appcap:
    SET    0.91 M ops/s
    GET    0.90 M ops/s
    LPUSH  0.59 M ops/s
    LPOP   0.96 M ops/s
    RANDOM 0.74 M ops/s
    peak  381.3 MiB
```

```text
Failure read:
  route4k and noboost-route4k have the same Redis failure shape:
    redis_alloc_string_fail lines = 77560
    source_alloc                 = 128
    alloc_fail                   = 1638
    descriptor_exhausted         = 3276
    source_block_exhausted       = 1638
    route_invalid                = 0
    route_miss                   = 0

  appcap completes with zero redis_alloc_string_fail lines but high peak working
  set.

Interpretation:
  The default Redis row timeout is not primarily a noboost policy question.
  Redis SET/LPUSH stresses low-capacity descriptor/source-block supply in
  route4k/noboost-route4k. noboost remains good for mixed_ws, but Redis needs a
  separate low-RSS capacity/admission design rather than more starvation-boost
  tuning.
```

## Redis LowRSS Design 2026-06-02

```text
Observation sufficiency:
  Enough:
    noboost is not the Redis fix.
    route4k/noboost low-capacity Redis failures are descriptor/source-block
    supply failures, not route-safety failures.
    appcap proves the workload can complete when capacity is abundant, but its
    peak working set is far too high for the low-RSS lane.

  Not enough yet:
    whether Redis wants more descriptors, more source blocks, or a small
    coordinated increase in both.
    whether a behavior governor is needed, or whether a narrow capacity shape
    is enough.
```

```text
Design target:
  HZ6 RedisLowRSS-L1

Goal:
  Keep route4k/noboost-style low RSS, but avoid SET/LPUSH collapse by giving
  Redis-like string/list churn enough descriptor/source-block supply.

Non-goals:
  Do not use appcap as the answer.
  Do not tune starvation source-refill boost further for Redis.
  Do not add diagnostic atomics to speed rows.
  Do not reopen broad descriptorless/frontcache knobs unless a new diagnostic
  points there.
```

```text
L0 first:
  Use existing capacity-axis lanes on redis_short before adding new behavior:
    route4k
    noboost-route4k
    desc4k-route4k
    source512-route4k
    desc4k-source512-route4k
    front1k-desc4k-source512-route4k
    appcap

Read:
  desc4k helps, source512 does not:
    descriptor supply is the primary Redis bottleneck.

  source512 helps, desc4k does not:
    source-block supply / run lifetime is primary.

  combined helps but each alone does not:
    Redis needs a coordinated descriptor+source-block budget.

  only appcap helps:
    low-RSS Redis needs behavior, not just fixed capacities.
```

```text
L1 candidate if L0 shows a clear axis:
  redislowrss-route4k:
    start from noboost-route4k
    keep route table at 4096
    raise only the winning supply axis, not every capacity
    keep frontcache modest

  possible shapes:
    desc2k-source128-route4k
    desc1k-source256-route4k
    desc2k-source256-route4k

Acceptance:
  redis_short:
    zero or sharply reduced redis_alloc_string_fail
    SET/LPUSH materially above route4k/noboost
    peak working set much closer to route4k than appcap

  mixed_ws/random_mixed guard:
    noboost gains must not regress materially
    larger_sizes must remain within route4k noise band

No-go:
  peak working set moves toward appcap
  route_invalid/route_miss/route_register_fail become nonzero
  Redis improves only by turning the lane into broad/appcap in disguise
```

```text
Implementation order:
  1. Run redis_short L0 capacity-axis matrix with existing lanes.
  2. If a single axis is clear, add one narrow redislowrss-route4k lane.
  3. If no axis is clear, ask for design review before adding behavior.
```

## Redis L0 Capacity-Axis Result 2026-06-02

```text
redis_short run-1:
  route4k:
    SET 0.01M, LPUSH 0.01M, RANDOM 0.31M, peak 7.5 MiB
    redis_alloc_string_fail lines = 77560

  noboost-route4k:
    SET 0.02M, LPUSH 0.01M, RANDOM 0.29M, peak 7.5 MiB
    redis_alloc_string_fail lines = 77560

  desc4k-route4k:
    SET 11.31M, LPUSH 6.08M, RANDOM 14.66M, peak 13.3 MiB
    redis_alloc_string_fail lines = 0

  source512-route4k:
    SET 0.01M, LPUSH 0.01M, RANDOM 0.29M, peak 8.4 MiB
    redis_alloc_string_fail lines = 77560

  desc4k-source512-route4k:
    SET 11.93M, LPUSH 9.82M, RANDOM 18.26M, peak 10.9 MiB
    redis_alloc_string_fail lines = 0

  front1k-desc4k-source512-route4k:
    SET 11.52M, LPUSH 9.74M, RANDOM 19.29M, peak 13.1 MiB
    redis_alloc_string_fail lines = 0

  appcap:
    SET 1.64M, LPUSH 1.54M, RANDOM 1.67M, peak 576.3 MiB
    redis_alloc_string_fail lines = 0
```

```text
Read:
  descriptor supply is the primary Redis collapse point.
  source512 alone does not help, but descriptor+source-block capacity improves
  LPUSH/RANDOM and peak working set versus desc4k-only in this run.
  frontcache widening is unnecessary for the Redis fix.
  appcap completes but is too high-RSS and slower on this short row.

Implementation:
  Add redislowrss-route4k:
    base = desc4k-source512-route4k
    plus HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1

  This is a named capacity-shape lane, not a new behavior governor.
  Next guard it against mixed_ws/random_mixed before using it beyond Redis-like
  rows.
```

## RedisLowRSS Lane Check 2026-06-02

```text
redislowrss-route4k implementation:
  descriptor capacity 4096
  route table capacity 4096
  transfer capacity 512
  source-block capacity 512
  frontcache capacity 256
  HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1
```

```text
Next narrow candidate:
  redislowrss-slim-route4k
    descriptor capacity 2048
    route table capacity 4096
    transfer capacity 512
    source-block capacity 256
    frontcache capacity 256
    HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1

Purpose:
  Try to keep the Redis completion gain of redislowrss-route4k while lowering
  retention / peak working set and checking whether mixed_ws guard becomes less
  hostile.
```

```text
redis_short run-1:
  noboost-route4k:
    SET    0.02M
    GET   27.52M
    LPUSH  0.01M
    LPOP  27.41M
    RANDOM 0.35M
    peak  7.5 MiB
    redis_alloc_string_fail lines = 77560

  redislowrss-route4k:
    SET   11.17M
    GET   30.53M
    LPUSH 10.11M
    LPOP  32.76M
    RANDOM 20.10M
    peak  10.9 MiB
    redis_alloc_string_fail lines = 0

Read:
  redislowrss-route4k is a strong Redis-like completion/throughput fix versus
  noboost-route4k while staying far below appcap peak working set.
```

```text
slim follow-up run:
  redislowrss-slim-route4k:
    Redis short:
      SET    9.55M
      GET   19.50M
      LPUSH  7.45M
      LPOP  22.62M
      RANDOM 14.96M
      peak  10.0 MiB

    mixed guard:
      balanced     2.047M, peak 61.3 MiB
      wide_ws      2.779M, peak 51.1 MiB
      larger_sizes 1.276M, peak 40.4 MiB

Read:
  slim is the better Redis-like balance than redislowrss-route4k.
  It keeps Redis completion strong, lowers mixed guard damage a lot, and keeps
  peak working set far below appcap.
  It is still not a general mixed_ws lane, but it is the best Redis-like
  candidate-control lane so far.
```

```text
mixed_ws guard run-1:
  noboost-route4k:
    balanced     21.768M, peak 17.6 MiB
      wide_ws      17.078M, peak 12.8 MiB
      larger_sizes  0.708M, peak 14.5 MiB

  redislowrss-route4k:
    balanced      0.533M, peak 104.4 MiB
    wide_ws       0.243M, peak 80.6 MiB
    larger_sizes  0.847M, peak 62.1 MiB

Read:
  redislowrss-route4k is not a general mixed_ws lane.
  It fixes Redis-like descriptor/source-block collapse but over-retains and
  slows broad mixed profiles.
  redislowrss-slim-route4k is materially better than redislowrss-route4k on
  mixed guard, but noboost-route4k still owns the general mixed_ws low-capacity
  lane.

Decision:
  KEEP as Redis-like candidate-control.
  DO NOT promote to primary HZ6 Windows lane.
  Use noboost-route4k for mixed_ws/random_mixed low-capacity checks.
```

## ControlPlane Direction 2026-06-02

```text
Design read:
  HZ6 is not failing because the hot path / route safety design is bad.
  The current evidence shows the limit of static capacity lanes:
    noboost-route4k fits mixed_ws / random_mixed.
    redislowrss-slim-route4k fits Redis-like string/list churn.
    one static lane cannot yet cover both without hurting one side.

Interpretation:
  redislowrss-slim-route4k is the BURST_SUPPLY upper-shape evidence.
  noboost-route4k is the NORMAL low-RSS shape.
  The next design target is a runtime control plane with hard capacity above
  NORMAL and soft budgets that open only under Redis-like pressure.
```

```text
Recommended order:
  1. Freeze redislowrss-slim-route4k as Redis-like candidate-control.
  2. Add staged Redis profiles before default redis_workload:
       redis_tiny
       redis_short
       redis_medium
       redis_long
       redis_workload
  3. Use staged rows to find the first collapse phase.
  4. Add ControlPlane-R1 dry-run:
       NORMAL
       BURST_SUPPLY_WOULD_OPEN
       CLOSE_WOULD_START
  5. Only after dry-run, add behavior:
       NORMAL soft budget roughly noboost-route4k
       BURST_SUPPLY soft budget roughly redislowrss-slim-route4k
       CLOSE returns retention toward NORMAL
```

```text
Runner update:
  The HZ6 capacity matrix now supports:
    redis_medium = 2 200 500 16 256
    redis_long   = 4 300 1000 16 256

  Timeout capture now preserves BENCH_ARGS / Pattern / Throughput / Ops lines
  and the last 200 redis_alloc_string_fail stats lines. This keeps default-row
  timeouts useful without dumping the entire stats stream into the summary.
```

```text
redis_medium smoke:
  noboost-route4k:
    SET    0.01M
    GET   63.82M
    LPUSH  0.01M
    LPOP 106.45M
    RANDOM 0.16M
    peak  8.1 MiB

  redislowrss-slim-route4k:
    SET    16.32M
    GET   112.32M
    LPUSH  12.96M
    LPOP  161.20M
    RANDOM 34.36M
    peak   9.8 MiB
    redis_alloc_string_fail lines = 0

Read:
  redis_medium confirms the short-row read: slim remains the Redis-like
  candidate-control lane, while noboost still collapses on writer/list phases.
  Next staged check is redis_long before returning to redis_workload.
```

```text
redis_long check:
  args = 4 300 1000 16 256
  timeout = 180s

  noboost-route4k:
    SET/GET/LPUSH/LPOP/RANDOM all timed out as failed:rc124.
    Timeout tail shows the same descriptor/source-block exhaustion shape:
      source_alloc           = 128
      alloc_fail             grows past 235k
      descriptor_exhausted   grows past 470k
      source_block_exhausted grows past 235k
      route_register_fail    = 0
      route_invalid/miss     = 0

  redislowrss-slim-route4k:
    SET    35.97M
    GET   253.08M
    LPUSH   3.09M
    LPOP  355.10M
    RANDOM 37.77M
    peak  29.0 MiB
    result = all patterns completed

Read:
  redis_long confirms the staged direction strongly:
    noboost-route4k is not a Redis-like pressure answer.
    redislowrss-slim-route4k is the current BURST_SUPPLY upper-shape evidence.
    LPUSH remains the next Redis pressure point even after slim completes.

Decision:
  Freeze redislowrss-slim-route4k as Redis-like candidate-control.
  Do not promote it to mixed_ws/general HZ6.
  Next useful implementation is ControlPlane-R1 dry-run, not another static
  Redis capacity lane, unless redis_workload reveals a new collapse phase.
```

```text
ControlPlane-R1 dry-run smoke:
  Diagnostic redis_short now emits:
    control_plane_normal
    control_plane_burst_supply_would_open
    control_plane_close_would_start

  short-row read:
    noboost-route4k stays NORMAL-dominant.
    redislowrss-slim-route4k begins surfacing BURST_SUPPLY projections under
    Redis-like pressure.
    CLOSE stays 0 in this short smoke, so the next useful check is the same
    diagnostic build on redis_medium and redis_long.

  note:
    the current matrix runner can OOM if we ask it to buffer the heavy
    diagnostic redis_medium/redis_long output as-is. For the next staged
    inspection, use a capped-tail parser or run the heavy rows directly so the
    control-plane counters can be read without buffering the full stream.

  follow-up:
    win/run_win_hz6_capacity_matrix.ps1 now streams raw logs to disk and keeps
    only capped capture lines in memory. A redis_medium diagnostic verify row
    now completes again, so the next step is to read the heavy-row counters
    rather than to rework the runner further.
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

## HZ6 ThinDescriptor-L1 Closeout

ThinDescriptor-L1 implementation:

```text
Status:
  implemented behind profile/capacity flag only

Lane:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc

Compile flags:
  HZ6_THIN_DESCRIPTOR_L1=1
  HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=4096

Shape:
  hot Hz6ObjectDescriptor removes direct source_ptr/source_bytes/source_release
  direct-source metadata moves to a sparse cold source table
  SourceBlock-backed descriptors read source metadata from SourceBlock
  direct-source descriptors carry cold_index with generation check
```

Safety contract:

```text
SourceBlock-backed descriptor:
  descriptor.source_block is the source lifetime owner
  no cold source record is allocated
  source metadata comes from SourceBlock

Direct-source descriptor:
  cold source record owns source_ptr/source_bytes/source_release metadata
  cold record is generation-checked before use
  descriptor reset/release clears the cold record first
  cold record allocation failure restores descriptor availability

Scavenge/release users:
  use hz6_allocator_descriptor_source_meta()
  do not read source_ptr/source_bytes/source_release directly

Real benchmark lanes:
  ThinDescriptor is opt-in
  diagnostic metadata output remains gated by DiagnosticHz6Probes
```

Validation:

```text
Existing non-thin smoke:
  lane:
    ownerlocalityfast-rsscap-2-desc160k-front4k
  profile:
    larson_t16_main_1k
  result:
    build/run OK

Thin diagnostic smoke:
  lane:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
  profile:
    larson_t16_main_1k
  result:
    throughput = 56.520M ops/s
    descriptor_entry_bytes = 48
    descriptor_table_bytes = 127926272
    descriptor_thin_hot_table_bytes = 127926272
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
    route_invalid = 0
    route_miss = 0
    remote_free_transfer_fail = 0

Thin non-diagnostic smoke:
  profile:
    larson_t16_main_1k
  result:
    throughput = 57.505M ops/s
    safety clean
    no diagnostic metadata output

Thin larger_sizes smoke:
  profile:
    larger_sizes
  result:
    throughput = 22.585M ops/s
    peak RSS = 168900 KB
    safety clean
```

Selected repeat-3 read:

```text
Profile:
  larson_t16_main_4k

ownerlocalityfast-rsscap-2-desc160k-front4k:
  throughput = 49.883M ops/s
  peak RSS = 570756 KB

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc:
  throughput = 49.288M ops/s
  peak RSS = 526988 KB

Read:
  ThinDescriptor-L1 is about -1.2% throughput vs same-run front4k
  ThinDescriptor-L1 saves about 43.8 MB peak RSS vs same-run front4k
  safety counters stay clean
```

Decision:

```text
KEEP:
  ThinDescriptor-L1 as an experimental lower-RSS sibling candidate.

Do not default yet:
  It needs broader repeat/full-row validation before promotion.

Do not close MetadataSlim:
  ThinDescriptor confirms that descriptor hot/cold split is viable,
  but MetadataSlim still has route/source/frontcache/table-level levers.

Next natural checks:
  1. run a selected larger_sizes repeat if large/direct-source pressure becomes
     a promotion concern
  2. run full 10k Larson only when preparing paper-grade tables
  3. consider cold-source-capacity attribution only if cold record exhaustion
     appears in later stress lanes
```

## HZ6 FrontCacheEntry Reserve Field Pack

Small source cleanup:

```text
Change:
  Hz6FrontCacheEntry.reserved_descriptor now exists only when
  HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1 is enabled.

Why:
  reserved_descriptor is used only by the descriptorreserve evidence lane.
  Selected production-like lanes do not need the field in every frontcache
  table entry.

Validation:
  ownerlocalityfast-rsscap-2-desc160k-front4k smoke:
    larson_t16_main_1k
    throughput = 56.817M ops/s
    peak RSS = 520752 KB
    safety clean

  descriptorreserve-route4k smoke:
    mixed_ws balanced
    build/run OK

Decision:
  KEEP as source cleanup / metadata hygiene.
  This is not a new promotion lane.
```

## HZ6 FrontCacheEntry SourceBlock Pack

FrontCacheEntry packing follow-up:

```text
Change:
  Hz6FrontCacheEntry.source_block now exists only when
  HZ6_DESCRIPTORLESS_FRONTCACHE_L1 is enabled.

  Hz6FrontCacheEntry.descgov_detached now exists only when
  HZ6_DESCRIPTOR_COLD_GOV_L1 is enabled.

  Field order was tightened so the selected non-descriptorless lanes reach the
  slim candidate size.

Why:
  Normal materialized frontcache entries already carry descriptor.
  SourceBlock can be reached from the descriptor when needed.
  source_block is only required for descriptorless materialization.

Validation:
  ownerlocalityfast-rsscap-2-desc160k-front4k diagnostic smoke:
    larson_t16_main_1k
    throughput = 55.552M ops/s
    peak RSS = 502340 KB
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
    route_invalid = 0
    route_miss = 0

  Metadata read:
    frontcache_entry_bytes = 32
    frontcache_table_bytes = 33560576
    frontcache_slim_entry_bytes = 32
    frontcache_slim_savings_bytes = 6144

  Descriptorless/reserve/coldgov build smoke:
    descriptorless-route4k
    descriptorreserve-route4k
    descriptorcoldgov-route4k
    build/run OK

Decision:
  KEEP as MetadataSlim frontcache packing.
  This captures the earlier FrontCacheSlim estimate for selected lanes.
```

Post-pack ThinDescriptor repeat-3:

```text
Run:
  larson_t16_main_4k
  speed profile
  capacity lanes:
    ownerlocalityfast-rsscap-2-desc160k-front4k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
  runs = 3

ownerlocalityfast-rsscap-2-desc160k-front4k:
  throughput = 49.537M ops/s
  peak RSS = 543108 KB

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc:
  throughput = 48.695M ops/s
  peak RSS = 499328 KB

Safety:
  alloc_fail = 0
  descriptor_exhausted = 0
  route_register_fail = 0
  source_block_exhausted = 0
  route_invalid = 0
  route_miss = 0

Read:
  thindesc is about -1.7% throughput versus front4k
  thindesc saves about 43780 KB peak RSS versus front4k

Decision:
  KEEP front4k as the selected lower-RSS sibling.
  KEEP front4k-thindesc as the selected lowest-RSS sibling candidate.
  It is still not a universal default; use it where peak RSS is the priority.
```

ThinDescriptor broad smoke:

```text
Run:
  mixed_ws
  profiles:
    speed, rss
  benchmark profiles:
    balanced, wide_ws, larger_sizes
  capacity lanes:
    ownerlocalityfast-rsscap-2-desc160k-front4k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
    descavail-noboost-route4k
    largerlowrss-front8k-sourcerun-desc8k-route8k
  runs = 1

Balanced:
  speed front4k:
    44.434M ops/s, 292748 KB
  speed front4k-thindesc:
    44.554M ops/s, 273012 KB
  rss descavail-noboost-route4k:
    75.640M ops/s, 17360 KB
  rss largerlowrss-front8k-sourcerun-desc8k-route8k:
    79.115M ops/s, 97036 KB

Wide_ws:
  speed front4k:
    9.113M ops/s, 315604 KB
  speed front4k-thindesc:
    9.115M ops/s, 294784 KB
  rss descavail-noboost-route4k:
    55.840M ops/s, 12520 KB

Larger_sizes:
  speed front4k:
    25.398M ops/s, 177820 KB
  speed front4k-thindesc:
    27.221M ops/s, 168088 KB
  speed largerlowrss-front8k-sourcerun-desc8k-route8k:
    26.932M ops/s, 70884 KB
  rss largerlowrss-front8k-sourcerun-desc8k-route8k:
    33.798M ops/s, 71008 KB

Read:
  thindesc stays safety-clean and generally reduces RSS versus front4k.
  thindesc is not the mixed_ws broad default because existing selected lanes
  remain stronger for balanced / wide_ws / larger_sizes.

Decision:
  KEEP thindesc as a Larson/owner-locality lowest-RSS sibling candidate.
  Do not promote thindesc to the mixed_ws default lane.
  Keep the existing mixed_ws recommendations in HZ6_LANE_GUIDE.md.
```

## HZ6 Descriptor SideOwner16 L1

Implementation:

```text
Compile flag:
  HZ6_DESCRIPTOR_SIDE_OWNER16_L1

Lane:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512

Change:
  remove owner from the hot descriptor entry when enabled
  store packed owner slot/generation in allocator->descriptor_side_owner16[]
  route descriptor owner reads/writes through descriptor helper APIs
```

Observation:

```text
Run:
  larson-run512-descriptorlayout
  runs = 1
  diagnostic probes on

baseline run512:
  45.407M ops/s
  499824 KB
  safety clean

no-backptr run512:
  47.836M ops/s
  476788 KB
  safety clean

sideowner16 run512:
  46.490M ops/s
  475672 KB
  descriptor_entry_bytes = 32
  descriptor_table_bytes = 96468992

sideowner16 no-go counters:
  route_invalid = 11739
  remote_free_transfer_fail = 11739
  lifecycle_foreign_free_invalid = 11739
  transfer_push = 403260491
  transfer_pop = 403260284
```

Read:

```text
SideOwner16 proves that a 32-byte hot descriptor entry is mechanically
reachable, but the first allocator-local side table is not a valid cross-owner
representation. Foreign descriptors can be observed from another allocator, so
owner lookup cannot blindly read the current allocator's side-owner table.

Decision:
  no-go / evidence only
  keep no-backptr as the selected lowest-RSS sibling
  do not promote sideowner16

Next if reopened:
  make descriptor owner metadata owner-source-aware
  or redesign route / descriptor ownership so the helper always has the
  descriptor's owning allocator
```

## HZ6 Descriptor Source Owner Diagnostic

Implementation:

```text
Diagnostic-only counters:
  descriptor_source_route_allocator_match
  descriptor_source_route_allocator_mismatch
  descriptor_source_current_allocator_match
  descriptor_source_current_allocator_mismatch

Meaning:
  route_allocator:
    allocator that currently owns the route entry / backend

  descriptor source:
    allocator whose descriptors[] array physically contains the descriptor
```

Observation:

```text
Run:
  larson-run512-descriptorlayout
  runs = 1
  diagnostic probes on

baseline run512:
  47.764M ops/s
  499836 KB
  descriptor_source_route_allocator_match = 27915843
  descriptor_source_route_allocator_mismatch = 450639936
  route_invalid = 0

no-backptr run512:
  47.416M ops/s
  476784 KB
  descriptor_source_route_allocator_match = 27714890
  descriptor_source_route_allocator_mismatch = 447517319
  route_invalid = 0

sideowner16 run512:
  45.352M ops/s
  475196 KB
  descriptor_source_route_allocator_match = 61033450
  descriptor_source_route_allocator_mismatch = 393294292
  route_invalid = 11731
  remote_free_transfer_fail = 11731
```

Read:

```text
Route rehome makes route ownership and descriptor-storage ownership diverge
for most frees in the Larson cross-owner run512 lane. This is safe while the
owner token lives in the descriptor itself, but unsafe when owner metadata is
stored in an allocator-local side table indexed by descriptor pointer.

Decision:
  descriptor-source diagnostic KEEP
  sideowner16 remains no-go
  no-backptr remains the safe descriptor-layout baseline before directory-cap
  trimming

Next:
  do not try another allocator-local owner side table
  either add first-class descriptor_storage_allocator metadata
  or switch to another RSS target outside descriptor owner side metadata
```

## HZ6 Larson Directory Capacity L1

Implementation:

```text
Lanes:
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512

Change:
  keep route capacity at route192k
  keep no-backptr descriptor layout
  reduce HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY only
  owner-locality index follows shared directory capacity
```

Observation:

```text
Run 1:
  larson_t16_main_10k
  runs = 1
  diagnostic probes on

no-backptr:
  45.694M ops/s
  476792 KB
  ownerlocality_index_bytes = 18874368
  owner_locality_miss = 0

dir192k:
  46.599M ops/s
  472628 KB
  ownerlocality_index_bytes = 14155776
  owner_locality_miss = 0
  shared_dir_first_fallback = 3
  shared_dir_probe_max = 29
  owner_locality_probe_max = 222
  safety clean

dir128k:
  36.220M ops/s
  467568 KB
  owner_locality_miss = 14464
  shared_dir_probe_max = 131073
  owner_locality_probe_max = 131073

dir96k:
  27.505M ops/s
  465240 KB
  owner_locality_miss = 30844
  shared_dir_probe_max = 98305
  owner_locality_probe_max = 98305

Repeat-3:
  larson_t16_main_10k
  runs = 3
  diagnostic probes on

no-backptr:
  45.310M ops/s
  476788 KB
  ownerlocality_index_bytes = 18874368
  owner_locality_miss = 0
  safety clean

dir192k:
  44.580M ops/s
  472176 KB
  ownerlocality_index_bytes = 14155776
  owner_locality_miss = 0
  safety clean
```

Read:

```text
Directory-capacity trimming has a useful middle point.
dir192k saves about 4.6 MB versus no-backptr and stays safety-clean in
repeat-3, with about -1.6% throughput versus the same-run no-backptr control.
dir128k/dir96k are too tight: they reduce RSS further, but the
owner-locality/shared-directory indexes start missing and probing full tables,
so throughput collapses.

Decision:
  dir192k PROMOTE as selected lowest-RSS sibling candidate
  no-backptr KEEP as descriptor-layout comparison control
  dir128k / dir96k KEEP as no-go boundary controls
  do not trim shared directory below dir192k without a representation change
```

## HZ6 mixed_ws wide_ws RouteOnly-L1

Question:

```text
Can wide_ws regain throughput without moving from the desc17 low-RSS descriptor
shape to the broader desc20 capacity shape?
```

Observation:

```text
Diagnostic frontcache-class run:
  lanes:
    desc17/route17
    desc18/route18
    desc20/route20

  frontcache class push/pop_empty is identical across the lanes:
    class0 push=1047   pop_empty=49
    class1 push=45232  pop_empty=1645
    class2 push=180659 pop_empty=6386
    class3 push=719290 pop_empty=25085

  source_alloc = 1692 for all lanes
  alloc_fail = 0
  descriptor_exhausted = 0
  route_register_fail = 0
  source_block_exhausted = 0

Read:
  frontcache/source shape is not the differentiator.
  route pressure is the visible differentiator.
```

Route-only repeat-3:

```text
Run:
  mixed_ws balanced, wide_ws
  profile = rss
  runs = 3

desc17/route17:
  balanced = 66.308M / 110940 KB
  wide_ws  = 20.155M / 140172 KB
  safety clean

desc17/route18:
  balanced = 64.923M / 111248 KB
  wide_ws  = 22.184M / 140456 KB
  safety clean

desc17/route20:
  balanced = 65.484M / 111552 KB
  wide_ws  = 21.907M / 141084 KB
  safety clean

desc20/route20:
  balanced = 68.561M / 113252 KB
  wide_ws  = 21.253M / 142488 KB
  safety clean
```

Decision:

```text
Promote desc17/route18 as the selected wide_ws low-RSS sibling.
Keep desc17/route17 as the selected balanced low-RSS row.
Keep desc20/route20 as speed/control evidence, not selected: it costs more RSS
and does not beat desc17/route18 on wide_ws in the repeat-3 route-only run.

Next:
  do not keep increasing descriptor capacity blindly for wide_ws
  if more wide_ws speed is needed, attack route representation / route load
  rather than frontcache or source placement
```

## HZ6 mixed_ws RouteHashShape-L1

Question:

```text
Can wide_ws route pressure be improved by changing probe shape or the initial
route hash without raising route capacity or weakening the selected low-RSS
family?
```

RouteProbeShape evidence:

```text
Diagnostic repeat-3 showed:
  balanced:
    desc17/route17 and desc17/route18 have small probe tails.

  wide_ws:
    desc17/route17 has large route lookup/register tails.
    desc17/route18 reduces the tail and remains the selected wide_ws sibling.
    desc17/route20 reduces probe totals further, but does not beat route18 in
    non-diagnostic speed and costs more route RSS.

Read:
  route pressure is real in wide_ws, but simply making the table larger is not
  the next clean answer.
```

DoubleHash-L1:

```text
Change:
  Secondary hash step for open-addressing probes.

Result:
  Diagnostic smoke:
    wide_ws route18 lookup_probe_total drops from ~5.6M to ~2.5M.
    b65+ tails almost disappear.

  Non-diagnostic repeat-3:
    route18 wide_ws     = 21.401M / 140496 KB
    route18 doublehash  = 20.886M / 140564 KB

Decision:
  KEEP as control/no-go evidence.
  DoubleHash removes probe tails, but the per-probe step / non-linear memory
  walk does not return as throughput.
```

HashXorFold-L1:

```text
Change:
  Keep linear probing, but xor-fold page/higher address bits into the initial
  hash:
    x = (base >> 4) ^ (base >> 12) ^ (base >> 20)

Diagnostic 1-run:
  route18 balanced:
    route18      = 57.099M / 111012 KB
    hashxor      = 47.529M / 111340 KB
    doublehash   = 47.602M / 111280 KB

  route18 wide_ws:
    route18      = 19.252M / 140576 KB
    hashxor      = 20.699M / 140536 KB
    doublehash   = 17.284M / 140560 KB

Non-diagnostic repeat-3:
  route18 compare:
    route18      balanced = 65.223M / 111208 KB
    hashxor      balanced = 73.433M / 111232 KB
    route18      wide_ws  = 21.545M / 140464 KB
    hashxor      wide_ws  = 21.792M / 140480 KB

  selected compare:
    route17      balanced = 67.767M / 110820 KB
    hashxor      balanced = 67.107M / 111140 KB
    route17      wide_ws  = 21.148M / 140296 KB
    hashxor      wide_ws  = 20.701M / 140332 KB

Safety:
  HZ6 Windows R1 smokes all pass.
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0

Decision:
  KEEP as route-hash evidence/control.
  Do not promote HashXorFold-L1 over the selected family yet.

HashXorFold can help a route18-only comparison, but it does not beat the
  selected desc17/route17 balanced row in same-run repeat-3, and it does not
  clearly beat selected wide_ws either.
```

LinearWrap-L1:

```text
Change:
  Preserve linear probing and hash semantics, but avoid the per-probe modulo
  in the default probe macro:
    old: (start + i) % capacity
    new: start + i, subtract capacity once when needed

  This is guarded by HZ6_ROUTE_LINEAR_WRAP_L1 and does not change
  VALID / INVALID / MISS semantics.

Repeat-3:
  mixed_ws balanced / wide_ws:
    route17             balanced = 67.229M / 110764 KB
    route17-linearwrap  balanced = 68.203M / 110740 KB
    route18             balanced = 64.148M / 111228 KB
    route18-linearwrap  balanced = 65.475M / 111096 KB

    route17             wide_ws  = 21.403M / 140348 KB
    route17-linearwrap  wide_ws  = 22.174M / 140144 KB
    route18             wide_ws  = 21.029M / 140492 KB
    route18-linearwrap  wide_ws  = 22.562M / 140548 KB

Guard repeat-3:
  mixed_ws balanced / wide_ws / larger_sizes:
    route17             balanced     = 67.840M / 110744 KB
    route17-linearwrap  balanced     = 69.821M / 110836 KB
    route18             balanced     = 69.707M / 111104 KB
    route18-linearwrap  balanced     = 78.106M / 111076 KB

    route17             wide_ws      = 20.961M / 140260 KB
    route17-linearwrap  wide_ws      = 22.964M / 140280 KB
    route18             wide_ws      = 22.660M / 140536 KB
    route18-linearwrap  wide_ws      = 21.728M / 140632 KB

    route17             larger_sizes = 32.152M / 78028 KB
    route17-linearwrap  larger_sizes = 33.720M / 77992 KB
    route18             larger_sizes = 33.388M / 78104 KB
    route18-linearwrap  larger_sizes = 34.202M / 78124 KB

Safety:
  HZ6 Windows R1 smokes all pass.

Decision:
  PROMOTE route17-linearwrap as the selected mixed_ws clean low-RSS row.
  It beats route17 on balanced / wide_ws / larger_sizes guard rows while
  keeping route17 RSS shape.

  Do not promote route18-linearwrap globally yet:
    it wins balanced and larger_sizes, but loses wide_ws to route18 in the
    guard run.

  Do not turn HZ6_ROUTE_LINEAR_WRAP_L1 into a global default yet:
    first keep it as an explicit selected lane, then verify broader families.
```

LoopCarry-L1:

```text
Change:
  Build on LinearWrap-L1, but carry the hot exact-route probe index in the loop
  instead of recomputing HZ6_ROUTE_PROBE_INDEX each iteration.

  Scope:
    exact lookup first pass
    exact register
    exact unregister

  Contract:
    VALID / INVALID / MISS unchanged
    linear probing semantics unchanged
    no production counters/atomics added

Repeat-3:
  mixed_ws balanced / wide_ws / larger_sizes:
    route17-linearwrap            balanced     = 62.124M / 110756 KB
    route17-linearwrap-loopcarry  balanced     = 67.462M / 110888 KB

    route17-linearwrap            wide_ws      = 21.882M / 140264 KB
    route17-linearwrap-loopcarry  wide_ws      = 22.674M / 140320 KB

    route17-linearwrap            larger_sizes = 33.594M / 77976 KB
    route17-linearwrap-loopcarry  larger_sizes = 34.032M / 78008 KB

Safety:
  HZ6 Windows R1 smokes all pass.

Decision:
  PROMOTE route17-linearwrap-loopcarry as the selected mixed_ws clean low-RSS
  row.

  Keep route17-linearwrap as the previous selected control.
  Keep route18 / route18-linearwrap as route-capacity siblings, not selected.
```

Current selected mixed_ws lanes:

```text
balanced low-RSS:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry

wide_ws low-RSS:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry

controls:
  route20
  doublehash
  hashxor
  route18
  route18-linearwrap
  route17-linearwrap
```

Next:

```text
Do not promote another hash/probe knob yet.
If wide_ws still needs speed, the likely next clean targets are:
  1. reduce route lookup/register cost while keeping linear locality
     (e.g. wrap-increment loop or payload split),
  2. route payload/layout split,
  3. profile-level table policy only if it beats selected rows repeatably.
```

### 2026-06-04: DescriptorStorageOwnerDryRun-L1

Goal:

```text
Before trying another descriptor side-metadata behavior, measure where a
descriptor pointer is physically stored:

  current allocator?
  route allocator?
  another visible allocator's descriptor array?

This is diagnostic-only under HZ6_DIAGNOSTIC_PROBES. It must not affect speed
lanes.
```

Change:

```text
Add descriptor_storage_* diagnostic counters:
  descriptor_storage_lookup
  descriptor_storage_hit / miss
  descriptor_storage_probe_total / max
  descriptor_storage_route_allocator_match / mismatch
  descriptor_storage_current_allocator_match / mismatch

Add hz6_allocator_descriptor_storage_owner_diagnostic():
  scan visible allocators
  return the allocator whose descriptor array owns the descriptor pointer
  update counters only from diagnostic free / remote_free paths
```

Result:

```text
Source:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-descriptor-storage-recheck/larson-run512-descriptorlayout/
    20260604_161726_hz6_capacity_matrix_windows.md

baseline thindesc:
  ops/s = 44.113M
  RSS   = 499828 KB
  route_invalid = 0
  remote_free_transfer_fail = 0
  descriptor_storage_hit = 441624204
  descriptor_storage_miss = 0
  descriptor_storage_route_allocator_mismatch = 415952846
  descriptor_storage_current_allocator_mismatch = 416032846

no-backptr:
  ops/s = 44.523M
  RSS   = 476776 KB
  route_invalid = 0
  remote_free_transfer_fail = 0
  descriptor_storage_hit = 446150919
  descriptor_storage_miss = 0
  descriptor_storage_route_allocator_mismatch = 420203005
  descriptor_storage_current_allocator_mismatch = 420283005

sideowner16:
  ops/s = 43.900M
  RSS   = 475152 KB
  route_invalid = 11626
  remote_free_transfer_fail = 11626
  lifecycle_foreign_free_invalid = 11626
  descriptor_storage_hit = 439632243
  descriptor_storage_miss = 0
  descriptor_storage_route_allocator_mismatch = 380830077
  descriptor_storage_current_allocator_mismatch = 380910077
```

Decision:

```text
KEEP:
  no-backptr remains the selected Larson descriptor layout.

NO-GO:
  sideowner16 remains unsafe even though it saves RSS.

Finding:
  Descriptor storage owner is always discoverable in the visible allocator set,
  but it is frequently not the route allocator and not the current allocator.

Implication:
  A safe side-owner redesign cannot be keyed by current allocator or route
  allocator alone. If revisited, side metadata must be descriptor-storage-owned
  or avoided by keeping owner inline/no-backptr.

Next:
  Do not try another sideowner knob immediately.
  Prefer:
    1. keep no-backptr selected for Larson,
    2. move to a source/route payload layout split, or
    3. design descriptor-storage-owned side metadata as a larger clean track.
```

### 2026-06-04: SourceBlockNoRouteBackptr-L1

Goal:

```text
Reduce Larson full-10k static RSS by removing the SourceBlock route-backend
back-pointer from the selected no-backptr/run512 shape.

This is a narrow metadata layout experiment:
  keep SourceBlock source_release inline
  remove only route_backend from SourceBlock under a build flag
  pass allocator explicitly to SourceBlock release/unregister helpers
```

Implementation:

```text
Flag:
  HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1

API change:
  hz6_allocator_release_source_block(allocator, block)

Why allocator is now explicit:
  invalid-range unregister belongs to the owning allocator route backend.
  Storing that route backend pointer in every SourceBlock is avoidable for
  same-allocator SourceBlock lifetime.

Why source_release stays in SourceBlock:
  source-block smoke uses custom source ops. Inferring release from allocator
  source registry was unsafe for custom source backing, so only route_backend
  was removed in L1.
```

Validation:

```text
Default Windows HZ6 smokes:
  build_win_hz6_r1_smokes.ps1
  result: all pass

Repeat-3:
  docs/benchmarks/windows/paper/hz6_selected_family/
    larson-sourceblock-noroutebackptr-dir192k-repeat/
      larson-cross-owner-lowest-rss/
      20260604_164022_hz6_capacity_matrix_windows.md

Rows:
  front4k:
    45.680M ops/s, 716344 KB, safety clean

  thindesc-source16k-route192k:
    48.575M ops/s, 628868 KB, safety clean

  no-backptr-route192k-run512:
    40.355M ops/s, 476792 KB, safety clean

  no-backptr-dir192k-route192k-run512:
    40.790M ops/s, 472184 KB, safety clean

  no-backptr-noroutebackptr-dir192k-route192k-run512:
    41.107M ops/s, 469868 KB, safety clean
    source_block_entry_bytes = 136
    source_block_table_bytes = 35651584
    ownerlocality_index_bytes = 14155776
```

Decision:

```text
KEEP:
  SourceBlockNoRouteBackptr-L1 is clean source-block metadata slimming.

Use:
  lowest-RSS Larson sibling/control.

Do not replace:
  the existing dir192k/no-backptr selected sibling when throughput/RSS balance
  matters. The repeat-3 no-route-backptr combo is the lowest RSS row in this
  run, but it is not a broad speed promotion.

No-go boundary:
  do not remove SourceBlock source_release in this track. Custom source ops
  need the block-owned release pointer.

Next:
  static SourceBlock route-backptr slimming is done.
  Further Larson RSS work should move to:
    1. route/source payload layout split,
    2. descriptor-storage-owned side metadata as a larger redesign, or
    3. non-static policy only if it preserves selected safety.
```
# HZ6 current task

## 2026-06-06: Elastic source-depot wide_ws fail-closed hardening

Context:

```text
DepotDescriptorRehomeBudget2048-L1 was clean on Larson repeat/guard, but
mixed_ws wide_ws hit 0xc0000005 during broad smoke. Direct checks showed the
fault was not Budget2048-specific:

  elasticdescsource-route
  depotownerdirect
  depotdescrehome
  depotdescrehome-budget2048

all reproduced the wide_ws access violation before the hardening.
```

Diagnostic:

```text
Added bench-only HZ_BENCH_TRACE_LAST_OP, enabled only with
-DiagnosticHz6Probes. The first global last-op probe was racy, so it was moved
to TLS.

TLS result:
  last_op = alloc-touch
  last_ptr = MEM_FREE
  fault   = MEM_FREE write

Meaning:
  HZ6 returned a stale source-backed slot as ACTIVE, and the benchmark crashed
  on the immediate memset. This is a stale cached/route/source-block lifecycle
  problem, not a user payload commit issue and not specific to Budget2048.
```

Implementation:

```text
1. Shared exact-route lookup now fail-closes stale entries:
   descriptor ptr/generation/state must still match the exact route.
   descriptors whose source_block is inactive or has no backing ptr are not
   returned as VALID.

2. hz6_allocator_activate_descriptor() now validates source_block backing:
   source_block must be active, have a non-null ptr, and the descriptor ptr /
   bytes must be inside the block.

3. hz6_allocator_release_source_block() hides block->ptr before OS release:
   stale cached descriptors see ptr == NULL and fail activation instead of
   reactivating memory that is already being released.
```

Validation:

```text
Diagnostic elasticdescsource-route wide_ws:
  before: 0xc0000005, alloc returned MEM_FREE ptr
  after:  ExitCode 0

Normal elasticdescsource-route wide_ws:
  ExitCode 0
  ops/s ~= 0.671M
  route_invalid ~= 23K
  route_miss ~= 21

Normal DepotDescriptorRehomeBudget2048 wide_ws:
  ExitCode 0
  ops/s ~= 0.597M
  route_invalid ~= 20K
  route_miss ~= 27

Selected mixed_ws route17-linearwrap-loopcarry short guard:
  balanced short: route_invalid = 0, route_miss = 0
  wide_ws short:  route_invalid = 0, route_miss = 0
```

Decision:

```text
KEEP:
  source-block activation/release guards.
  bench last-op tracing as diagnostic-only plumbing.

Do not promote:
  elasticdescsource-route / depotdescrehome-budget2048 to broad mixed_ws.

Finding:
  The source-depot family is now fail-closed instead of crashing, but wide_ws
  still exposes stale lifecycle pressure via route_invalid/route_miss.

Next:
  Treat this as an ElasticCapacity source-depot lifecycle track. The next
  useful design is not another Budget2048 knob; it is exact-route/source-block
  lifecycle ownership cleanup, likely around stale shared exact entries and
  cached descriptor invalidation.
```
