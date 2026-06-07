# Current Task

HZ6 orientation note:

```text
Before adding another HZ6 lane or benchmark row, read:
  hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
  hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
  hakozuna-hz6/docs/HZ6_REPO_HYGIENE.md
  hakozuna-hz6/docs/HZ6_SOURCE_MODULARIZATION.md
```

Keep this root file as a top-level project ledger. HZ6-specific stable lane
decisions should be mirrored into the HZ6 docs above.

HZ6 is now in active Windows/Linux implementation and benchmarking. HZ5 Linux
remains profile-stabilized; new HZ5 work should not blur the HZ6 contract.

Latest HZ6 Redis route-churn attack:

```text
2026-06-07:
  Redis long refresh showed regular tombcompact can be threshold-gated too
  conservatively on 8K route tables: summed tombstone_current may be non-zero
  while per-allocator tombstone_max stays below the half-cap floor, so
  route_tombstone_compact_attempt remains 0.

  New narrow controls:
    redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr1024
    redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr2048

  Meaning:
    Redis-only threshold boundary probes. They remove the route_capacity/2
    floor under HZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1 and use absolute
    tombstone thresholds. Existing tombcompact semantics remain unchanged.

  Next:
    run Redis long with diagnostic probes and require compact_attempt > 0,
    safety counters zero, SET not materially worse, and RSS within the Redis
    candidate-control envelope before keeping either lane.

  Observed:
    diagnostic run proves the aggressive thresholds fire:
      aggr1024: LPUSH compact_attempt=4, RANDOM compact_attempt=8
      aggr2048: RANDOM compact_attempt=4

    non-diagnostic run=1:
      base keeps best GET/LPOP
      regular tombcompact keeps best RANDOM
      aggr2048 has the best SET and near-regular LPUSH
      aggr1024 lowers RANDOM probe work but does not win the row

  Read:
    Keep aggr1024/aggr2048 as Redis threshold-boundary controls, not selected
    defaults. The next Redis fix likely needs a conditional pattern/pressure
    policy rather than a single lower tombstone threshold.

 整理:
    Redis route-churn track is not blocked by missing counters anymore.
    We now know:
      conservative tombcompact can be too cold to fire,
      aggr1024/aggr2048 can force compaction safely,
      fixed lower thresholds do not cleanly win the row.

    Therefore:
      stop adding plain tombstone threshold values.
      If Redis is reopened, attack conditional compaction or pressure-aware
      retained route/source policy, not another fixed threshold.

  Implemented follow-up:
    redislowrss-sourcerun-desc8k-route8k-condtombdry

  Meaning:
    Redis-only diagnostic projection for conditional tombstone compaction.
    It does not compact. It uses table-local state:
      tombstone_count >= 1024
      ratio25 or occupancy75
      cooldown 1024

  Close rule:
    if condtombdry does not clearly separate RANDOM/LPUSH pressure from
    GET/LPOP low-pressure rows, close the Redis fixed-threshold tombcompact
    track and return to selected-family / Larson / mixed_ws.

  Observed:
    diagnostic run=1:
      SET/GET/LPOP would_compact = 0
      LPUSH would_compact = 4
      RANDOM would_compact = 8
      safety counters remain zero

  Read:
    conditional compact dry-run is a strong witness. If Redis stays active,
    the next and only sensible behavior test is a narrow conditional compact
    lane using the same table-local condition and cooldown.

  Implemented behavior:
    redislowrss-sourcerun-desc8k-route8k-condtombcompact

  Observed:
    diagnostic:
      LPUSH compact_attempt=success=4
      RANDOM compact_attempt=success=4
      SET/GET/LPOP compact_attempt=0
      safety counters clean

    non-diagnostic repeat-3:
      condtombcompact:
        SET 34.63M, GET 292.10M, LPUSH 30.92M, LPOP 523.31M,
        RANDOM 88.40M, peak 13,828 KB, geomean 107.67

      aggr1024:
        geomean 112.23

      regular tombcompact:
        geomean 110.03

  Read:
    KEEP condtombcompact as Redis conditional behavior evidence, but do not
    select it. It proves table-local conditional compaction can target
    LPUSH/RANDOM safely, yet repeat-3 still trails aggr1024/regular on row
    geomean and loses GET/LPOP.
```

Latest HZ6 Elastic / Larson follow-up:

```text
2026-06-07:
  Redis is closed for now as evidence/control. The active target moves back to
  Larson / ElasticCapacity because it is still the cleanest path toward HZ6's
  low-RSS cross-owner strength.

  Current selected Elastic low-RSS sibling:
    DepotOwnerDirect + DirectFreeTrustedLocalCache
    main10k repeat-3 = about 43.75M / 224724 KB
    safety clean

  Prior composition guard:
    DirectFreeTrustedLocalCache + DepotDescriptorRehomeBudget2048
      main10k = 39.525M / 234696 KB
      safety clean but loses versus selected DFTLC while RSS rises

  New diagnostic-only lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
    depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-
    intersectiondryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-
    dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-
    frontcachepacked-sourceblockpacked-source64-route16k-run4096

  Purpose:
    Count the boundary between the free/local-cache speed path and the
    transfer/rehome path in the same diagnostic run:
      elastic_dftlc_rehome_intersection_directfree_hit
      elastic_dftlc_rehome_intersection_directfree_fail
      elastic_dftlc_rehome_intersection_transfer_probe
      elastic_dftlc_rehome_intersection_transfer_depot
      elastic_dftlc_rehome_intersection_transfer_already_local
      elastic_dftlc_rehome_intersection_transfer_would_rehome
      elastic_dftlc_rehome_intersection_rehome_success
      elastic_dftlc_rehome_intersection_rehome_ineligible
      elastic_dftlc_rehome_intersection_rehome_budget_denied

  Read target:
    If transfer probes are mostly already-local or ineligible under DFTLC, then
    rehome budget is not composing with the selected DFTLC mechanism and should
    remain a separate ElasticCapacity control. If would_rehome/success is high
    but selected DFTLC still loses, the next design should gate rehome by
    workload pressure or transfer class rather than applying a static per-owner
    budget.

  Observed after fixing Larson worker stats aggregation:
    main1k smoke:
      ops = 52.653M, peak = 98744 KB
      directfree_hit = 53158057
      transfer_probe = 8000
      transfer_depot = 0
      rehome_success = 0
      rehome_ineligible = 8000
      rehome_budget_denied = 0
      safety counters clean

    main10k:
      ops = 36.004M, peak = 235292 KB
      directfree_hit = 362995404
      transfer_probe = 80000
      transfer_depot = 71811
      transfer_would_rehome = 71811
      rehome_success = 30483
      rehome_ineligible = 8189
      rehome_budget_denied = 41328
      safety counters clean

  Read:
    DirectFreeTrustedLocalCache remains the dominant speed path. The rehome
    budget path is absent on main1k and only partially active on main10k, where
    it spends most eligible transfers at the static budget boundary. Keep the
    intersection lane as diagnostic evidence. Do not promote DFTLC+budget2048;
    the next Elastic work should either leave selected DFTLC alone or design a
    conditional transfer/rehome policy rather than another fixed budget value.
```

Latest HZ6 SmallRunRoute attack:

```text
2026-06-07:
  SourceBlockRoute / active-map / notoy small-knob track is closed as evidence.
  ToyFront remains the route-safe reference front; do not replace it with a
  broad source-block route.

  New direction:
    SmallRunRoute-L1

  Meaning:
    a narrow TinyRun route proof for 256B..4K toy/small source-run slots.
    It is not a generic SourceBlockRoute promotion.

  Dry-run lane:
    smallrunroute-dryrun-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k

  Dry-run check:
    256B/512B/1K/2K/4K all had:
      smallrun_would_valid == free attempts
      smallrun_exact_fallback_needed = 0
      smallrun_false_positive = 0
      route_invalid = 0
      route_miss = 0
      route_register_fail = 0

  Important detail:
    HZ6's current small class geometry rounds the 2K row into a 4K slot class,
    so HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES defaults to 4096.

  Next:
    add SmallRunRoute behavior as a separate lane.
    Keep exact route fallback.
    Keep diagnostic counters out of speed builds.
    Do not merge it with generic SourceBlockRoute yet.

  Implemented:
    smallrunroute-behavior-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
    smallrunroute-behavior-min512-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k

  Diagnostic behavior smoke:
    behavior_valid == behavior_attempt on 256B/512B/1K/2K/4K
    behavior_fallback = 0
    behavior_invalid_slot = 0
    behavior_invalid_descriptor = 0
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    route_lookup_probe_total drops materially versus exact route.

  Non-diagnostic repeat-3:
    all-size SmallRunRoute:
      256B -0.9%
      512B -1.0%
      1K   +4.4%
      2K   -13.6%
      4K   -1.8%

    min512 SmallRunRoute:
      256B -0.8%
      512B -2.5%
      1K   +7.2%
      2K   -1.3%
      4K   -0.1%

  Read:
    SmallRunRoute is safety-clean and useful mechanism evidence.
    It is not a broad default-promotion lane yet.
    The current useful signal is mostly the 1K row; avoid overfitting a
    one-size production lane until the next hotspot explains why 1K benefits.

  Worker-assisted read:
    SmallRunRoute replaces exact route lookup with range-index lookup +
    slot arithmetic + bitmap + descriptor-map validation. It wins only when
    the avoided exact-route probing is more expensive than that replacement.
    The 1K row appears to be the current sweet spot. 256B/512B are too small
    for the replacement path to amortize, and 2K is distorted by the current
    4K slot geometry / source_alloc pressure.

  Selected-small refresh repeat-5:
    Compared:
      largerlowrss
      sameownerfast
      directlocalfreereuse
      sourceblockroute-behavior-dynmap-directlocalfreereuse
      smallrunroute-behavior-min512-directlocalfreereuse

    winners:
      256B: dynmap 78.089M / 14,320 KB
      512B: dynmap 55.517M / 26,580 KB
      1K:   dynmap 57.399M / 26,572 KB
      2K:   dynmap 37.236M / 75,768 KB
      4K:   smallrun-min512 50.157M / 42,516 KB
      8K:   sameownerfast 64.009M / 25,388 KB
      16K:  dynmap 54.087M / 17,668 KB

    Read:
      The current selected-small dynmap lane remains the best broad
      candidate-watch shape in this repeat-5.
      SmallRunRoute is not a selected-small replacement; keep it as
      TinyRunRoute/SmallRunFront mechanism evidence and a 4K/1K clue.
      SameOwnerFast remains the 8K control/winner in this run.

  Diagnostic attribution:
    dynmap and smallrun both reduce route_lookup_probe_total materially, but
    SmallRun-min512 pays a range-index lookup before fallback on 256B:
      256B smallrun-min512:
        smallrun_behavior_attempt=305088
        smallrun_behavior_valid=0
        smallrun_behavior_fallback=305088
        range_index_lookup=305088
    This confirms the lower-gate overhead issue.

  Boundary repeat-10 follow-up:
    Compared directlocalfreereuse, sameownerfast, dynmap, and smallrun-min512
    on 4K/8K/16K without diagnostic counters:
      4K:
        dynmap 53.281M / 42,516 KB
        direct 52.728M / 41,962 KB
        sameowner 52.286M / 41,934 KB
        smallrun-min512 50.274M / 42,518 KB
      8K:
        dynmap 65.056M / 25,946 KB
        direct 63.219M / 25,386 KB
        smallrun-min512 59.511M / 25,948 KB
        sameowner 59.507M / 25,388 KB
      16K:
        direct 56.492M / 17,116 KB
        sameowner 55.452M / 17,120 KB
        dynmap 54.252M / 17,660 KB
        smallrun-min512 50.216M / 17,664 KB

    Existing dynmap-small8k gate refresh:
      4K:  dynmap 52.948M, dynmap-small8k 45.871M, direct 50.124M
      8K:  dynmap 59.024M, dynmap-small8k 55.729M, direct 56.435M
      16K: direct 50.583M, dynmap 46.086M, dynmap-small8k 45.075M

    Read:
      The selected dynmap row remains the cleanest 4K/8K candidate-watch.
      directlocalfreereuse remains the 16K speed/RSS control.
      dynmap-small8k is not a rescue gate; it regresses the checked boundary
      rows and should remain control/no-go evidence.
      Do not add another selected-small size gate without a new mechanism.

  Toy/small hotpath refresh:
    Existing diagnostic/control lanes were rechecked on 256B/512B/1K/2K/4K:
      toysmallhotpathdiag-directlocalfreereuse
      toysmallactivemap-directlocalfreereuse
      toysmallhotpathdiag-sourceblockroute-dynmap
      toysmallactivemap-sourceblockroute-dynmap

    Read:
      Hotpath diag confirms the Toy/small path is the active witness:
        malloc_fast_attempt is about allocation count
        malloc_fast_hit is allocation count minus initial front dispatch
        free_route_lookup / owner_equal / free_fast_hit / cache_push all align
      Active-map bypass is also real:
        route_bypass is about 296K..298K for 256B..2K and about 241K for 4K
        free_route_lookup drops to roughly 0.3%..3% of frees
      But the behavior is not a clean speed promotion:
        active-map direct/dynmap rows are slower in the diagnostic repeat-3,
        and prior non-diagnostic repeats already missed broad promotion.

    Selected-small closeout:
      The remaining 256B..2K gap is not solved by another active-map,
      SourceBlockRoute, SmallRunRoute, or dynmap-small8k toggle. Keep dynmap
      as selected-small candidate-watch/evidence, directlocalfreereuse as
      simple control, and move new optimization work to a different HZ6 target
      unless a new mechanism is proposed.
```

Latest HZ6 Larson / ElasticCapacity attack:

```text
2026-06-07:
  After closing selected-small toggles, the next ROI target moved back to the
  HZ6-specific low-RSS/cross-owner path:
    Larson / ElasticCapacity.

  Implemented runner lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
    depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run4096

  Meaning:
    Compose the current selected Elastic low-RSS sibling
      DepotOwnerDirect + DirectFreeTrustedLocalCache
    with the bounded descriptor materialization control
      DepotDescriptorRehomeBudget2048.

  Added selected-family preset:
    -LarsonElasticRehomeBudgetGuard

  Build fix:
    Diagnostic Larson builds had stale local projection variables that were
    computed but no longer printed after summary helper cleanup. They are now
    explicitly consumed in the diagnostic block; speed builds and allocator
    hot paths are unchanged.

  Guard run-1:
    results/hz6-elastic-dftlc-rehomebudget-guard-r1/

    main10k:
      dftlc-selected  42.369M / 227,444 KB
      budget2048      39.472M / 240,236 KB
      dftlc+budget    39.525M / 234,696 KB

    worker10k:
      dftlc-selected  44.130M / 214,820 KB
      budget2048      44.212M / 219,428 KB
      dftlc+budget    45.404M / 219,432 KB

    main4k:
      dftlc-selected  56.667M / 139,164 KB
      budget2048      51.258M / 146,472 KB
      dftlc+budget    51.969M / 146,500 KB

    worker4k:
      dftlc-selected  49.857M / 132,992 KB
      budget2048      49.508M / 134,908 KB
      dftlc+budget    50.467M / 134,904 KB

    main1k:
      dftlc-selected  56.029M / 92,124 KB
      budget2048      56.729M / 98,144 KB
      dftlc+budget    57.129M / 98,144 KB

    worker1k:
      dftlc-selected  58.084M / 91,860 KB
      budget2048      57.050M / 92,408 KB
      dftlc+budget    58.390M / 92,420 KB

  Safety:
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    remote_free_transfer_fail = 0
    lifecycle_foreign_free_invalid = 0
    alloc_fail = 0

  Read:
    The combined DFTLC + RehomeBudget lane is safety-clean and improves some
    worker/small rows, but it loses the critical main10k row and raises RSS
    versus the selected DirectFreeTrustedLocalCache lane.
    Keep it as composition guard/control evidence, not selected promotion.
    The current Larson / Elastic selected lane remains:
      depotownerdirect-directfree-trustedlocalcache source64-route16k-run512.

  Small-budget follow-up:
    Added DFTLC + RehomeBudget boundary rows:
      budget512
      budget1024
      budget2048

    results/hz6-elastic-dftlc-rehomebudget-smallbudget-guard-r1/

    main10k:
      dftlc-selected  38.845M / 224,732 KB
      budget2048      40.212M / 234,692 KB
      dftlc+budget512 39.805M / 240,676 KB
      dftlc+budget1024 39.888M / 240,684 KB
      dftlc+budget2048 39.101M / 234,696 KB

    main4k:
      dftlc-selected  49.997M / 139,028 KB
      dftlc+budget512 50.152M / 146,424 KB
      dftlc+budget1024 49.758M / 146,436 KB
      dftlc+budget2048 48.549M / 146,404 KB

    worker1k:
      dftlc-selected  57.011M / 91,832 KB
      dftlc+budget512 57.665M / 92,400 KB
      dftlc+budget1024 57.049M / 92,392 KB
      dftlc+budget2048 58.346M / 92,392 KB

    Safety:
      route_invalid = 0
      route_miss = 0
      route_register_fail = 0
      remote_free_transfer_fail = 0
      lifecycle_foreign_free_invalid = 0
      alloc_fail = 0

    Read:
      Smaller budgets do not rescue the composition.
      budget512/budget1024 are safety-clean and can improve isolated small rows,
      but main10k still fails the RSS requirement versus selected DFTLC.
      Keep all DFTLC + RehomeBudget budget rows as boundary/control evidence.
      Do not continue this family with another simple budget value unless a new
      conditional policy explains why main10k RSS would not rise.
```

Latest HZ6 Windows large-slice lane wiring:

```text
2026-06-06:
  The weak legacy large_slice_4k / 8k / 16k route4k rows now have enough
  diagnostic evidence:
    4K/8K route4k weakness is descriptor/source-block exhaustion.
    16K route4k weakness is direct-source/source-block pressure.
    No new hot-path counter is needed before the next step.

  The existing selected lane:
    largerlowrss-front8k-sourcerun-desc8k-route8k

  was wired into:
    win/build_win_allocator_suite.ps1
    win/run_win_allocator_matrix.ps1

  New legacy matrix allocator names:
    hz6-strict-largerlowrss
    hz6-speed-largerlowrss
    hz6-rss-largerlowrss

  Connectivity check:
    large_slice_4k:
      rss-largerlowrss 40.909M ops/s / 42,524 KB
    large_slice_8k:
      rss-largerlowrss 58.647M ops/s / 25,360 KB
    large_slice_16k:
      rss-largerlowrss 57.947M ops/s / 17,092 KB

  Safety:
    alloc_fail = 0
    route_invalid = 0
    route_miss = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0

  Read:
    route4k remains a tiny-capacity low-RSS control.
    largerlowrss is the selected HZ6 row for 4K..16K legacy large-slice checks.
    This is a single-run connectivity check, not a paper median.
```

Latest HZ6 small fixed-size attack:

```text
2026-06-06:
  Full legacy large_slices with largerlowrss connected:
    HZ6 wins 128K, 256K, 512K, and 1M.
    HZ6 is viable but not winner at 8K/16K.
    HZ6 remains weak at 256B..4K versus mimalloc/HZ3/HZ4.

  Diagnostic largerlowrss small rows:
    capacity is clean:
      alloc_fail = 0
      route_invalid = 0
      route_miss = 0
      descriptor_exhausted = 0
      source_block_exhausted = 0
    frontcache_reuse_hit ~= alloc count
    source_owned_route_hit_local_owner is very high

  Read:
    the remaining 256B..16K gap is hot same-owner/frontcache/route cost,
    not route4k-style capacity exhaustion.

  Implemented candidate-watch:
    sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k
    directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k

  HZ6-only repeat-10:
    256B: +23.2%
    512B: +19.5%
    1K:   +20.0%
    2K:   +12.9%
    4K:   +5.2%
    8K:   +13.0%
    16K:  +8.6%
    RSS essentially flat.

  Read after repeat-10:
    sameownerfast-largerlowrss is a real speed win for 256B..16K.
    2K is noisy, not a structural no-go.
    4K and 16K are smaller wins.

  Status:
    keep sameownerfast-largerlowrss as mechanism evidence/control.
    keep directlocalfreereuse-largerlowrss as a strong candidate-watch
    for same-owner fixed-size rows.
    do not default-promote yet.

  DirectLocal decomposition repeat-5:
    512B: directlocalfreereuse +25.1%
    2K:   directlocalfreereuse +21.3%
    4K:   directlocalfreereuse +31.2%
    8K:   directlocalfreereuse +19.5%

  Full 256B..16K repeat-5 with base/sameownerfast/directlocalfreereuse:
    256B: best directlocalfreereuse +30.9%
    512B: best sameownerfast +25.6%
    1K:   best directlocalfreereuse +27.6%
    2K:   best directlocalfreereuse +17.1%
    4K:   best directlocalfreereuse +25.3%
    8K:   best sameownerfast +11.3%
    16K:  best directlocalfreereuse +15.8%

  Read after decomposition:
    the win is not free-side only.
    directlocalfree alone is smaller, directlocalalloc is larger,
    directlocalfreealloc is close to SameOwnerFast, and
    directlocalfreereuse is the strongest decomposition shape in most rows.
    The remaining 256B..16K gap is mostly local alloc/reuse activation cost.

  Next:
    legacy runner connectivity for hz6-*-directlocalfreereuse-largerlowrss
    is verified.

  Legacy connectivity single-run:
    command:
      win/run_win_allocator_matrix.ps1
      -Profiles large_slice_256..large_slice_16k
      -Allocators hz6-speed-largerlowrss,
                  hz6-speed-sameownerfast-largerlowrss,
                  hz6-speed-directlocalfreereuse-largerlowrss,
                  mimalloc,tcmalloc

    delta vs hz6-speed-largerlowrss:
      256B: directlocalfreereuse +38.6%, sameownerfast +32.7%
      512B: directlocalfreereuse +62.6%, sameownerfast +59.1%
      1K:   directlocalfreereuse +12.3%, sameownerfast +20.0%
      2K:   directlocalfreereuse +13.2%, sameownerfast +19.9%
      4K:   directlocalfreereuse +18.3%, sameownerfast +9.1%
      8K:   directlocalfreereuse +20.0%, sameownerfast +8.5%
      16K:  directlocalfreereuse -28.0%, sameownerfast +14.5%

  Read after legacy connectivity:
    directlocalfreereuse is connected and valuable, but not a clean
    256B..16K promotion from the first legacy single-run alone because 16K
    regressed in that one run.
    sameownerfast looked safer at 16K in that first legacy single-run.
    The practical next step is a size-gated selected-small lane or a repeat
    matrix that decides where DirectLocalFreeReuse is allowed.
    mimalloc remains much faster on 256B..16K in this runner, so HZ6's near-term
    claim here is low-RSS/safety with improving speed, not small-object speed
    leadership.

  Implemented size-gated control:
    directlocalfreereuse-small8k-largerlowrss-front8k-sourcerun-desc8k-route8k

  Design:
    HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS controls DirectLocalFreeReuse eligibility.
    small8k sets MAX_CLASS=4, allowing Toy 256B/512B/1K/4K and MidPage 8K,
    while excluding MidPage 16K/32K class 5.

  HZ6-only repeat-5 with base/sameownerfast/directlocal/full/small8k:
    256B: best sameownerfast +30.9%
    512B: best sameownerfast +30.1%
    1K:   best directlocalfreereuse-small8k +26.5%
    2K:   best directlocalfreereuse +15.3%
    4K:   best sameownerfast +27.4%
    8K:   best directlocalfreereuse-small8k +19.2%
    16K:  best directlocalfreereuse +22.3%

  Legacy single-run with small8k connected:
    256B: best sameownerfast +31.1%
    512B: best directlocalfreereuse-small8k +32.7%
    1K:   best directlocalfreereuse-small8k +35.9%
    2K:   best directlocalfreereuse +21.2%
    4K:   best directlocalfreereuse +26.3%
    8K:   best directlocalfreereuse +21.7%
    16K:  best sameownerfast +33.7%

  Read after size-gated control:
    small8k gate is safe and useful, but it is not a universal winner.
    full DirectLocalFreeReuse remains strong at 2K/4K/8K and sometimes 16K.
    SameOwnerFast is a safer control row and often competitive at
    256B/512B/4K/16K.
    Do not create an overfit per-size hybrid yet.
    Keep all three as candidate/control lanes until repeat evidence stabilizes
    a clean selection rule.

  HZ6-only candidate repeat-10:
    Compared:
      largerlowrss
      sameownerfast-largerlowrss
      directlocalfreereuse-largerlowrss
      directlocalfreereuse-small8k-largerlowrss

    best per row:
      256B: directlocalfreereuse +28.1%
      512B: directlocalfreereuse-small8k +23.2%
      1K:   sameownerfast +24.6%
      2K:   directlocalfreereuse +23.3%
      4K:   directlocalfreereuse-small8k +19.7%
      8K:   directlocalfreereuse-small8k +17.1%
      16K:  directlocalfreereuse +14.3%

    simple lane average/min delta vs largerlowrss:
      sameownerfast:                avg +17.4%, min +8.2%
      directlocalfreereuse:         avg +19.8%, min +14.2%
      directlocalfreereuse-small8k: avg +17.9%, min +0.8%

  Current selected-small read:
    DirectLocalFreeReuse is the best simple candidate-watch for 256B..16K.
    It is not the best row at every size, but it is positive across all rows
    with the best average and best minimum improvement in repeat-10.
    SameOwnerFast and small8k remain control lanes.
    Do not build a per-size hybrid yet; that would overfit the current noise.

  Selected-family runner wiring:
    Added win/run_win_hz6_selected_family.ps1 -SelectedSmallFixed.
    The normal -SelectedFamily preset set now includes selected-small-fixed.

    selected-small-fixed:
      mixed_ws large_slice_256..large_slice_16k
      speed + directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k

    Smoke:
      powershell -NoProfile -ExecutionPolicy Bypass `
        -File .\win\run_win_hz6_selected_family.ps1 `
        -SelectedSmallFixed -Runs 3 -TimeoutSeconds 120 `
        -OutputDir .\results\hz6-selected-small-fixed-smoke `
        -ContinueOnFailure

    Result:
      completed and wrote selected-small-fixed summary.
      Treat smoke values as runner connectivity, not paper medians.

  Full selected-family smoke with selected-small included:
    command:
      powershell -NoProfile -ExecutionPolicy Bypass `
        -File .\win\run_win_hz6_selected_family.ps1 `
        -SelectedFamily -Runs 1 -TimeoutSeconds 180 `
        -OutputDir .\results\hz6-selected-family-with-small-smoke `
        -ContinueOnFailure

    Result:
      all five selected-family preset groups completed:
        selected-mixed-lowrss
        selected-random-sameowner
        selected-small-fixed
        selected-larger-lowrss
        larson-cross-owner-selected

      safety scan:
        no non-zero route_invalid / route_miss / alloc_fail /
        descriptor_exhausted / route_register_fail /
        source_block_exhausted / remote_free_transfer_fail /
        lifecycle_foreign_free_invalid found in the smoke summaries.

      selected-small-fixed smoke rows:
        256B: 76.056M ops/s / 13,668 KB
        512B: 57.295M ops/s / 25,996 KB
        1K:   57.401M ops/s / 25,860 KB
        2K:   38.994M ops/s / 75,152 KB
        4K:   54.122M ops/s / 41,916 KB
        8K:   59.678M ops/s / 25,364 KB
        16K:  55.191M ops/s / 17,088 KB

    Read:
      selected-small-fixed is now connected to the selected-family runner.
      These are run-1 smoke/connectivity values, not paper medians.
      The selected-family wrapper can now be used to produce an HZ6 profile
      family snapshot that includes the small fixed-size candidate-watch row.

  DirectLocalTrustedOwner-L1 smoke:
    Implemented candidate/control lane:
      directlocaltrusted-largerlowrss-front8k-sourcerun-desc8k-route8k

    Design:
      keeps DirectLocalFreeReuse, then enables
      HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1.
      The shortcut is intentionally scoped to local-cache/direct-local paths:
        alloc from an allocator-owned frontcache can activate LOCAL_FREE
        without re-setting owner;
        free after an already-computed local_owner check can cache ACTIVE
        without repeating descriptor owner equality.

    Smoke command:
      win/run_win_hz6_capacity_matrix.ps1
        -Families mixed_ws
        -BenchmarkProfiles large_slice_512,large_slice_2k,
                           large_slice_8k,large_slice_16k
        -Hz6Profiles speed
        -CapacityLanes largerlowrss,
                       directlocalfreereuse-largerlowrss,
                       directlocaltrusted-largerlowrss
        -Runs 3

    Result versus largerlowrss:
      512B:
        directlocalfreereuse +24.0%
        directlocaltrusted   +19.5%
      2K:
        directlocalfreereuse +21.3%
        directlocaltrusted   +15.8%
      8K:
        directlocalfreereuse +35.1%
        directlocaltrusted   +28.4%
      16K:
        directlocalfreereuse +14.5%
        directlocaltrusted   +17.6%

    Safety:
      no non-zero route_invalid / route_miss / alloc_fail /
      descriptor_exhausted / route_register_fail / source_block_exhausted /
      free_invalid_local_cache_direct / frontcache_reuse_invalid seen.

    Read:
      safety-clean, but not a promotion candidate.
      It only beats DirectLocalFreeReuse at 16K in this smoke and loses on
      512B/2K/8K, so owner re-check / owner set is not the main remaining
      256B..16K bottleneck. Keep as no-go/control evidence and do not wire
      into selected-family or legacy cross-allocator rows.

  DirectLocalPacked-L1 smoke:
    Implemented candidate/control lane:
      directlocalpacked-largerlowrss-front8k-sourcerun-desc8k-route8k

    Design:
      keeps DirectLocalFreeReuse and adds HZ6_FRONTCACHE_PACKED_META_L1.
      This tests whether the Larson FrontCachePackedMeta-L1 metadata shrink
      helps small fixed-size same-owner rows.

    Result versus largerlowrss:
      512B:
        directlocalfreereuse +23.8%
        directlocalpacked    +18.8%
      2K:
        directlocalfreereuse +19.6%
        directlocalpacked    +19.4%
      8K:
        directlocalfreereuse +14.2%
        directlocalpacked    +8.1%
      16K:
        directlocalfreereuse +19.7%
        directlocalpacked    +17.5%

    Safety:
      no non-zero route_invalid / route_miss / alloc_fail /
      descriptor_exhausted / route_register_fail / source_block_exhausted /
      frontcache_reuse_invalid seen.

    Read:
      safety-clean, but not a speed promotion candidate.
      FrontCachePackedMeta-L1 is still valuable in Larson RSS rows, but it does
      not improve the 512B/2K/8K/16K direct-local fixed-size speed shape.
      Keep as no-go/control evidence and do not wire into selected-family or
      legacy cross-allocator rows.

  DirectLocalExact-L1:
    Implemented candidate/control lane:
      directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k

    Design:
      keeps DirectLocalFreeReuse and adds HZ6_LOCAL_EXACT_FIRST_FREE_L1.
      This tests whether the remaining small fixed-size gap is mostly the
      general route lookup on free.

    Diagnostic witness before behavior:
      directlocalfreereuse diagnostic run showed route lookup pressure even
      when capacity/source counters were clean:
        512B route_lookup_probe_total=1,909,436 / route_valid=305,088
        2K   route_lookup_probe_total=2,111,815 / route_valid=305,088
        8K   route_lookup_probe_total=989,531   / route_valid=200,704
        16K  route_lookup_probe_total=1,021,285 / route_valid=160,256

    Repeat-5:
      DirectLocalExact improved the worst-row shape and beat DirectLocalFreeReuse
      on 256B, 1K, 2K, 8K, and 16K, but lost on 512B and 4K.
      avg/min delta vs largerlowrss:
        directlocalfreereuse avg +19.8%, min +8.7%
        directlocalexact     avg +20.8%, min +16.2%

    Repeat-10:
      avg/min delta vs largerlowrss:
        directlocalfreereuse avg +20.2%, min +9.2%
        directlocalexact     avg +20.2%, min +10.9%

      directlocalexact vs directlocalfreereuse by row:
        256B: -0.7%
        512B: +0.3%
        1K:   -1.3%
        2K:   -2.4%
        4K:   +6.4%
        8K:   -4.1%
        16K:  +1.5%

    Safety:
      no non-zero route_invalid / route_miss / alloc_fail /
      descriptor_exhausted / route_register_fail / source_block_exhausted /
      frontcache_reuse_invalid seen in repeat-10.

    Read:
      route lookup shortening is relevant, but simple exact-first free is not
      a clean selected-small replacement.  Keep DirectLocalFreeReuse as the
      selected-small candidate-watch and keep DirectLocalExact as a close
      route-pressure control.  Do not selected-family or legacy-wire it yet.

  Selected-small lane cleanup:
    Keep the selected-family and legacy cross-allocator wire narrow:
      selected:
        directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
      HZ6-only controls:
        directlocaltrusted-largerlowrss-front8k-sourcerun-desc8k-route8k
        directlocalpacked-largerlowrss-front8k-sourcerun-desc8k-route8k
        directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k

    Source cleanup:
      DirectLocalFreeReuse-derived flag builders now share one helper so the
      selected/control/no-go variants are less error-prone.  Lane names and
      output suffixes are unchanged, preserving old benchmark comparability.
      `win/run_win_hz6_selected_family.ps1` now forwards `-ListOnly` to the
      capacity matrix so selected-family lane enumeration cannot accidentally
      execute a long Larson row.
```

Latest HZ6 selected-family decision:

```text
2026-06-06 follow-up after selected-small lane cleanup:
  LargeSpan/LargeDirect status:
    detailed HZ6 ledger has already closed the LargeSpan256/512/1M and
    LargeDirect-L1 rollout.
    Treat 128K..1M as the current retained-reuse LargeSpan win range.
    Treat >1M..8M as direct-release/low-retain coverage, not speed-promotion.

  Cross-comparison doc update:
    HZ6_CROSS_ALLOCATOR_COMPARISON now separates route4k weakness-map rows
    from the current selected-small repeat-10 row.

  Current selected-small repeat-10 read:
    LargerLowRSS -> DirectLocalFreeReuse:
      256B: 58.212M -> 74.567M
      512B: 47.496M -> 56.798M
      1K:   45.752M -> 54.628M
      2K:   32.128M -> 39.623M
      4K:   44.426M -> 53.092M
      8K:   58.200M -> 66.468M
      16K:  52.993M -> 60.562M

    RSS shape remains essentially unchanged.  HZ6 is still not the small-object
    speed leader versus mimalloc/HZ3/HZ4, but the old route4k table is no
    longer the current selected-small representation.

  Next target:
    small/mid transition after DirectLocalFreeReuse.
    Do not add another directlocal owner/packed/exact knob yet; those were
    closed as controls.  The next useful work should either:
      1. collect a clean cross-allocator selected-small matrix, or
      2. inspect MidPage/Toy activation/source layout for a non-knob
         structural simplification.

  MidPage source-block unification:
    DONE:
      hz6_midpage_prefill_run() now delegates to the shared
      hz6_front_prefill_source_block_kind() helper.
      This removes the MidPage-only SourceBlock fill loop and lets 8K/32K
      MidPage use the same SourceRunReuse-L1 rollback/accounting path as Toy.

    Confirmation:
      build_win_hz6_capacity_suite mixed_ws/speed for LargerLowRSS and
      DirectLocalFreeReuse LargerLowRSS lanes passed.
      hz6-midpage-sourceblock-unified-repeat3:
        8K LargerLowRSS:             55.476M / 25,984 KB
        8K DirectLocalFreeReuse:     63.667M / 25,920 KB
        16K LargerLowRSS:            50.723M / 17,648 KB
        16K DirectLocalFreeReuse:    52.147M / 17,648 KB
      all rows:
        route_invalid=0
        route_miss=0
        alloc_fail=0
        route_register_fail=0
        source_run_reuse rollback/safety counters=0

    Read:
      This is a structural cleanup and contract unification, not a new speed
      knob.  Next performance decision still needs a repeat matrix if we want
      to promote any small/mid behavior.

    Lane fixed:
      selected-small candidate-watch remains:
        speed + directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
      directlocaltrusted / directlocalpacked / directlocalexact remain
      HZ6-only controls.
      directlocalfreereuse-small8k remains a size-gated control, not default.

    8K/16K/32K boundary follow-up:
      Source:
        results/hz6-boundary-8k16k32k-repeat3/
        results/hz6-boundary-directlocal-diag/
        results/hz6-boundary-directlocalexact-repeat3/

      LargerLowRSS / DirectLocalFreeReuse / small8k repeat-3:
        8K:
          50.52M -> 64.39M -> 60.75M
        16K:
          37.59M -> 52.54M -> 44.56M
        32K:
          48.31M -> 67.90M -> 60.08M

      DirectLocalFreeReuse / DirectLocalExact repeat-3:
        8K:
          57.75M -> 63.63M
        16K:
          40.52M -> 46.27M
        32K:
          52.88M -> 61.99M

      Diagnostic read:
        DirectLocalFreeReuse boundary rows are frontcache-reuse dominated after
        initial source refill.  source_run_reuse miss_no_block/source_alloc are
        bounded setup costs, while route_lookup_probe_total remains the visible
        free-side pressure.

      Decision:
        do not C-gate DirectLocalExact by class/size yet.  free() does not know
        size before route lookup, so a broad exact-first gate would still tax
        small rows before it can reject them.  Keep DirectLocalFreeReuse as the
        single-binary selected-small candidate-watch, and add a runner-level
        hybrid control that measures:
          256B..4K with DirectLocalFreeReuse
          8K..16K with DirectLocalExact
        This is evidence gathering, not a default allocator lane.

    Runner wiring:
      Added win/run_win_hz6_selected_family.ps1
        -SelectedSmallFixedHybridExactUpper

      It expands to two presets:
        selected-small-fixed-hybrid-lower:
          256B..4K + directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
        selected-small-fixed-hybrid-upper-exact:
          8K..16K + directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k

      Smoke/repeat-3:
        Source:
          results/hz6-selected-small-hybrid-exact-upper-repeat3/

        lower:
          256B: 70.439M / 13,712 KB
          512B: 53.952M / 25,924 KB
          1K:   53.352M / 25,876 KB
          2K:   39.114M / 75,036 KB
          4K:   52.542M / 41,908 KB

        upper exact:
          8K:   59.719M / 25,372 KB
          16K:  46.671M / 17,088 KB

        safety:
          no non-zero route_invalid / route_miss / alloc_fail /
          route_register_fail / descriptor_exhausted / source_block_exhausted.

        Read:
          the runner-level split works, but upper exact is not stable enough to
          displace selected-small.  It was strong in the boundary-only repeat
          and weaker in this selected-family wrapper repeat, so keep it as a
          control measurement path only.  Do not default DirectLocalExact and
          do not build a C-level exact-first size gate yet.

    Next allowed moves:
      1. clean repeat/cross-allocator selected-small matrix for promotion
         evidence, or
      2. another structural small/mid simplification with a new pressure
         signal.

    Do not do next:
      add another directlocal micro-knob
      promote per-size hybrids
      relabel MidPage source-block unification as a new speed lane

    Cross-allocator single-run confirmation:
      Source:
        results/hz6-selected-small-crossallocator-check/
        20260606_155209_allocator_matrix.md

      HZ6 selected-small vs external allocators:
        256B:
          HZ6 selected 72.024M / 14,160 KB
          loses to HZ3 315.673M, HZ4 227.264M, mimalloc 321.578M,
          and tcmalloc 141.643M
        512B:
          HZ6 selected 50.001M / 26,636 KB
          loses to HZ3 241.585M, HZ4 134.177M, mimalloc 361.620M,
          and tcmalloc 186.226M
        1K:
          HZ6 selected 54.219M / 26,536 KB
          loses to HZ3 247.934M, HZ4 167.729M, mimalloc 254.421M,
          and tcmalloc 108.170M
        2K:
          HZ6 selected 37.537M / 75,596 KB
          loses to HZ3 142.653M, HZ4 132.729M, mimalloc 190.186M,
          and tcmalloc 82.954M
        4K:
          HZ6 selected 52.730M / 42,468 KB
          beats HZ3 8.271M, HZ4 25.702M, and tcmalloc 35.027M
          but loses to mimalloc 166.165M
        8K:
          HZ6 selected 67.222M / 25,920 KB
          beats HZ3 9.400M, HZ4 32.947M, and tcmalloc 60.366M
          but loses to mimalloc 116.734M
        16K:
          HZ6 selected 55.120M / 17,656 KB
          beats HZ3 23.426M, HZ4 34.331M, and tcmalloc 41.811M
          but loses to mimalloc 139.373M

      Safety:
        HZ6 route_invalid=0
        HZ6 route_miss=0
        HZ6 alloc_fail=0
        HZ6 route_register_fail=0
        HZ6 source_block_exhausted=0
        HZ6 descriptor_exhausted=0

      Read:
        selected-small is not a small-object speed leader at 256B..2K.
        It becomes credible from 4K upward, where it beats HZ3/HZ4/tcmalloc
        in this runner while remaining safety-clean.  Treat as fixed
        candidate-watch / 4K..16K strength evidence, not a broad small-object
        promotion.  Mimalloc remains the speed ceiling in this matrix.

2026-06-06 historical LargeSpan attack after Windows bench modularization:
  Status:
    CLOSED as implementation history.  The active next target is the
    selected-small / small-mid transition listed above.

  The Windows bench source/output cleanup is now in a much healthier shape:
    bench_allocator_compare HZ6 summary is helper-owned
    Larson HZ6 summary/front diagnostics are helper-owned
    Redis HZ6 diagnostics are helper-owned

  HZ6 is strong enough to resume performance work:
    safety-oriented selected rows are route_invalid/route_miss clean
    RSS-oriented Larson siblings are meaningful evidence
    lane docs are good enough for targeted development

  Historical primary target:
    LargeSpan family work.

  Reason:
    HZ6 still reports large-object behavior as a Large128 seed, not as a broad
    large-object allocator.  The next high-ROI implementation should promote
    the large128 seed toward a real LargeSpan family before claiming broad
    ordinary-malloc coverage.

  First step:
    inspect current large128 / large front code and implement the smallest
    cleanup that makes 128K central span behavior more explicit and extensible.
    DONE:
      added LargeSpan class metadata in hakozuna-hz6/fronts/large/
      large128 request/class handling now goes through a one-class table
      Windows HZ6 R1 smokes pass
      mixed_ws large_slice_128k run1 is route_invalid/route_miss clean
      mixed_ws large_slice_128k repeat-3:
        median 75.404M ops/s / 6,736 KB
        route_invalid=0 route_miss=0 alloc_fail=0

  Pro consult / next LargeSpan order:
    Do not add all 128K/256K/512K/1M classes at once.
    NEXT:
      CentralSpanPoolBudget-L1
        add per-class retained bytes and global retained bytes accounting
        keep existing fallback when budget is full
        do not add decommit/release yet
      then LargeSpan256-L1

    Reason:
      descriptor-count central capacity is readable for a one-class 128K seed,
      but it becomes misleading once 256K/512K/1M spans share the same backend.

    CentralSpanPoolBudget-L1 DONE:
      per-class retained bytes and global retained bytes added
      default cap is 8MiB/class and 8MiB global
      budget full keeps existing fallback behavior
      allocator smoke asserts Large128 central bytes push/pop
      Windows HZ6 R1 smokes pass
      mixed_ws large_slice_128k repeat-3:
        median 72.608M ops/s / 6,740 KB
        route_invalid=0 route_miss=0 alloc_fail=0

  Secondary target:
    Elastic source-depot lifecycle cleanup for wide_ws stale-route pressure.

  Do not do next:
    add another ad hoc capacity knob
    weaken owned-invalid behavior into fallback
    mix LargeSpan work with Elastic source-depot lifecycle experiments

2026-06-06 wide_ws route-invalid closeout:
  mixed_ws wide_ws route_invalid/miss was traced to shared exact route
  directory deletion breaking open-address probe chains.  Fix is tombstone
  unregister + tombstone reuse on register, plus atomic SourceBlock ref_count
  and locked shared SourceBlock depot claim hardening.  Confirmed:
    normal wide_ws base and budget2048 route_invalid=0 route_miss=0
    diagnostic base elastic_route_overflow_register_fail=0
    pre_free_owns_false=0 after hz6_owns() uses allocator-level lookup
  See hakozuna-hz6/docs/current_task.md for the full closeout.

2026-06-05 cleanup/orientation:
  Short docs are the source of truth for the current working set:
    hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
    hakozuna-hz6/docs/HZ6_ELASTIC_CAPACITY_PLAN.md
    hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
    hakozuna-hz6/docs/HZ6_REPO_HYGIENE.md

  current production-ish selected rows:
    mixed_ws:
      rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-
      linearwrap-loopcarry

    random_mixed:
      strict + sameownerfast-descavail-noboost-route4k

    larger_sizes:
      speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

    Larson low-RSS balance:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
      noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
      ownersourcel2-source16k-route192k-run512

    Larson packed minimum-RSS sibling:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
      noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
      ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-
      run512

  current ElasticCapacity source-depot track:
    candidate-watch:
      depotownerdirect
      main10k repeat-3 = 46.273M / 224612 KB
      guard rows safety-clean

    recent transfer-scoped evidence:
      depotslottransfer
        full10k non-diagnostic = 41.596M, safety-clean
        sparse slot recording is safe when it stays scoped to transfer reuse

      depotdescrehomedry
        full10k diagnostic = 43.040M, safety-clean
        transfer_reuse_hit=80000
        depot_descriptor=71811
        run_match=71811
        local_descriptor_available=71811
        would_rehome=71811

      depotroutereplacedry
        full10k diagnostic = 41.332M, safety-clean
        transfer_reuse_hit=80000
        depot_descriptor=71811
        run_match=71811
        old_route_found=71811
        descriptor/generation/front/class match=71811
        current_route_same=71811
        current_route_conflict=0
        would_commit=71811
        would_rollback=8189 non-depot transfer objects

      depotdescrehome
        full10k non-diagnostic = 44.853M, safety-clean
        full10k diagnostic = 43.616M, safety-clean
        l1_attempt=80000
        l1_success=71811
        l1_ineligible=8189 non-depot transfer objects
        no_local_descriptor=0
        route_replace_fail=0
        detach_fail=0
        rollback=0
        watch item:
          descriptor_used rises to 77883 in diagnostic final stats because
          depot descriptors are materialized into consumer-local descriptors.
          This is valid behavior evidence, not promotion yet.

      depotdescrehome-capfree
        full10k non-diagnostic = 43.602M, safety-clean
        full10k diagnostic = 42.119M, safety-clean
        l1_attempt=80000
        l1_success=71811
        route_replace_fail=0
        detach_fail=0
        rollback=0
        read:
          control/no-go for the simple frontcache cap hypothesis.
          `descriptor_used` remains 77883, `active_descriptors` remains about
          72327, and `frontcache_total` is only 6072.  The retention watch item
          is dominated by live Larson objects materialized into consumer-local
          descriptors, not by a cold frontcache backlog.  Do not chase a broad
          cap-on-free policy for this track.

      depotdescrehome-budget2048
        full10k non-diagnostic = 44.919M, safety-clean
        full10k diagnostic = 40.961M, safety-clean
        l1_attempt=80000
        l1_success=30483
        l1_ineligible=8189
        l1_budget_denied=41328
        route_replace_fail=0
        detach_fail=0
        rollback=0
        descriptor_used=36539
        active_descriptors=33528
        descriptor_live_max=2448
        read:
          Strong candidate-control.  A per-allocator 2048 rehome budget cuts
          consumer-local descriptor materialization by more than half versus
          full rehome while keeping non-diagnostic throughput slightly above
          full rehome in this run.  This is a better next candidate than
          capfree; repeat/guard should decide whether budget2048 becomes the
          selected ElasticCapacity source-depot lane.

  immediate next attack order:
    1. RouteExactDescriptorReplace-L0/L1:
       implement a fail-closed exact-route descriptor replacement primitive.
       The dry-run shows the current allocator already has an exact route for
       the old depot descriptor, so the first behavior should replace the
       descriptor pointer/generation metadata in place rather than
       unregister-first.
       L0 primitive added as `hz6_route_replace_exact_descriptor()` /
       `hz6_allocator_route_replace_exact_descriptor()`.  It requires old
       descriptor, generation, front, class, and bytes to match before changing
       the exact entry, and route smoke verifies mismatch rejection.

    2. DepotDescriptorRehome-L1 behavior:
       clone/rehome the depot descriptor into a consumer-local descriptor only
       through the route replacement primitive.  Generation should be
       preserved because this is descriptor storage relocation for the same
       live object, not a new object lifetime.
       L1 behavior now succeeds for every eligible transfer-reuse depot
       descriptor and keeps safety clean.  Next optimization should not add
       another route knob.  The capfree control shows frontcache backlog is not
       the dominant retention source.  Budget2048 shows that consumer-local
       descriptor materialization can be bounded while keeping the speed signal,
       so the next validation should be repeat/guard for budgeted rehome rather
       than another route or broad owner shortcut.

    do not:
      revive broad slot-local storage-owner override
      unregister old route before a replacement/commit path is proven
      call source-release on the old depot descriptor during rehome

  2026-06-05 FreeLocalCacheOwnerPredicate-L0:
    implementation:
      Add HZ6_FREE_LOCAL_CACHE_OWNER_PREDICATE_DRYRUN_L0.
      It is diagnostic-only and records cheap descriptor/source-block
      predicates only for HZ6_OWNER_EQUAL_SITE_FREE and
      HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE.

    full10k:
      41.049M / 224628 KB
      flc_owner_predicate_probe=821934976
      flc_owner_predicate_site_free=411004460
      flc_owner_predicate_site_local_cache=410930516
      flc_owner_predicate_depot_descriptor=693768653
      flc_owner_predicate_local_descriptor=47477340
      flc_owner_predicate_foreign_descriptor=774457636
      flc_owner_predicate_source_block=821934976
      flc_owner_predicate_source_block_shared=698397445
      flc_owner_predicate_source_release=821934976
      flc_owner_predicate_source_run_active=0
      safety clean

    read:
      The predicate is not weak.  In the full cross-owner row, most free/local
      owner_equal pressure is depot/source-block backed and a large majority is
      foreign descriptor storage.  This supports one narrow behavior attempt:
      answer owner equality through depot owner metadata only when the
      descriptor is a depot descriptor, and keep the normal path for local
      descriptors / non-depot descriptors.  Do not use source_run_active as the
      gate in this lane; it is zero here.

  2026-06-05 DepotDescriptorOwnerEqualFastPath-L1:
    implementation:
      Add HZ6_DEPOT_DESCRIPTOR_OWNER_EQUAL_FASTPATH_L1.
      In owner_equal_at(), only for FREE/LOCAL_CACHE sites and depot
      descriptors, compare the depot owner directly before the normal owner
      path.  Non-depot descriptors and other sites fall back unchanged.

    full10k non-diagnostic:
      40.531M / 227708 KB
      safety clean

    full10k diagnostic:
      41.221M / 227744 KB
      depot_owner_equal_fastpath_probe=824802008
      depot_owner_equal_fastpath_hit=696139014
      depot_owner_equal_fastpath_miss=71811
      depot_owner_equal_fastpath_fallback=128591183
      depot_owner_equal_fastpath_other_site=0
      safety clean

    read:
      Safety is clean and the predicate hits heavily, but speed is no-go versus
      DepotOwnerDirectFastPath-L1 (`46.273M / 224612 KB`).  The existing
      descriptor_owner() depot-direct path already avoids the expensive L2
      owner lookup; adding an earlier free/local-cache branch costs more than
      it saves.  Keep as no-go/control.  Do not add another owner_equal
      short-circuit unless it removes a different expensive operation, not just
      reorders the same depot-owner read.

  2026-06-05 UnifiedDepotDrainDryRun-L1:
    implementation:
      Add HZ6_ELASTIC_DEPOT_DRAIN_DRYRUN_L1.
      It is diagnostic-only and runs during transfer reuse on the
      DepotRunMeta + DepotOwnerDirect source-depot lane.  It unifies the
      previous storage-owner, run metadata, ref-count, owner-match, and
      slot-localize projections into one owner-safe drain/localize witness.

    full10k diagnostic:
      43.101M / 235420 KB
      elastic_depot_drain_probe=79485
      elastic_depot_drain_storage_mismatch=79485
      elastic_depot_drain_run_match=79485
      elastic_depot_drain_ref_shared=79485
      elastic_depot_drain_owner_match=79485
      elastic_depot_drain_would_slot_localize=79485
      elastic_depot_drain_would_block_whole_localize=79485
      safety clean

    read:
      Whole-SourceBlock localize remains no-go: every probed depot transfer
      block is shared.  But every probe is run-matched, owner-matched after
      activation, and slot-localizable.  This is the strongest current witness
      for a slot-level depot drain/localize behavior.  The next behavior should
      localize per-slot storage/owner state for depot transfer reuse, not move
      whole SourceBlocks and not add another owner_equal branch.

  2026-06-05 DepotSlotLocalize-L1:
    implementation:
      Add HZ6_ELASTIC_DEPOT_SLOT_LOCALIZE_L1.
      It records slot-local storage in the sparse slot-owner table during
      depot transfer reuse and lets owner_source_side_meta_storage() return
      that per-slot storage owner before the SourceBlock-level storage owner.

    full10k non-diagnostic:
      44.658M / 240636 KB
      route_invalid=125
      remote_free_transfer_fail=125

    full10k diagnostic:
      36.191M / 240656 KB
      elastic_depot_slot_localize_attempt=30733367
      elastic_depot_slot_localize_success=30733367
      elastic_depot_slot_localize_storage_hit=401643367
      elastic_depot_slot_localize_storage_miss=4465070
      elastic_depot_slot_localize_storage_stale=0
      route_invalid=125
      remote_free_transfer_fail=125

    read:
      No-go/control.  The slot-local storage table is active and heavily hit,
      but returning slot-local storage through the general owner_source side
      metadata path lets a small number of stale/ownership mismatches reach
      real free/remote-transfer behavior.  Do not promote.  The next attempt
      must keep slot-localization scoped to transfer reuse or a fail-closed
      descriptor-local path; do not use it as a broad storage-owner override.

  2026-06-05 DepotSlotTransferScoped-L1:
    implementation:
      Add HZ6_ELASTIC_DEPOT_SLOT_TRANSFER_SCOPED_L1 and the
      depotslottransfer lane.  It records depot transfer-reuse slots in the
      sparse slot-owner table, but deliberately does not let
      owner_source_side_meta_storage() return the slot-local storage owner.
      This keeps the experiment scoped to transfer reuse and avoids the broad
      storage-owner override that made DepotSlotLocalize-L1 unsafe.

    full10k non-diagnostic:
      41.596M / safety clean
      route_invalid=0
      remote_free_transfer_fail=0

    full10k diagnostic:
      42.801M / safety clean
      elastic_depot_slot_localize_attempt=79485
      elastic_depot_slot_localize_success=79485
      elastic_depot_slot_localize_ineligible=515
      elastic_depot_slot_localize_storage_hit=0
      elastic_slot_owner_sparse_insert=79485
      elastic_slot_owner_sparse_hit=79485
      elastic_slot_owner_sparse_owner_match=79485
      route_invalid=0
      remote_free_transfer_fail=0

    read:
      Keep as safe transfer-scoped control/evidence, not promotion.  The sparse
      slot table can be populated from depot transfer reuse without safety
      leakage, and the prior `route_invalid=125` / `remote_free_transfer_fail=125`
      was caused by using slot-local storage as a general owner_source storage
      override.  The next useful behavior must consume this slot-local witness
      only in a fail-closed descriptor-local or transfer-local decision; do not
      re-enable broad storage-owner override.

  2026-06-05 DepotDescriptorRehomeDryRun-L1:
    implementation:
      Add HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1.
      It runs during transfer reuse after activation, on top of the safe
      depotslottransfer lane, and asks whether a depot descriptor could be
      cloned/re-homed into the consumer allocator's local descriptor table.
      This is diagnostic-only; it does not unregister/register routes, release
      depot descriptors, or change object ownership.

    full10k diagnostic:
      43.040M / safety clean
      transfer_reuse_hit=80000
      elastic_depot_descriptor_rehome_probe=80000
      elastic_depot_descriptor_rehome_depot_descriptor=71811
      elastic_depot_descriptor_rehome_already_local=0
      elastic_depot_descriptor_rehome_run_match=71811
      elastic_depot_descriptor_rehome_run_mismatch=0
      elastic_depot_descriptor_rehome_local_descriptor_available=71811
      elastic_depot_descriptor_rehome_no_local_descriptor=0
      elastic_depot_descriptor_rehome_would_rehome=71811
      route_invalid=0
      remote_free_transfer_fail=0

    read:
      Strong next-behavior witness.  Most transfer-reused depot descriptors are
      physically run-matched slots and every eligible depot descriptor had a
      local descriptor slot available in the consumer allocator.  The next
      behavior candidate is not storage-owner override; it is descriptor
      clone/rehome at transfer reuse, with fail-closed route exact replacement
      and depot descriptor release/rollback.  Keep this diagnostic separate
      from speed lanes until the route-swap safety contract is implemented.

  do not:
    use diagnostic-only lanes in speed-ranking tables
    revive whole-SourceBlock localize
    add another static route/descriptor/source cap cut without a specific
    pressure witness

2026-06-05 lane cleanup:
  HZ6_LANE_GUIDE now starts with a quick classification table:
    selected profile lanes
    ElasticCapacity candidate-watch lanes
    component/control lanes
    diagnostic-only lanes
    no-go / boundary lanes

  current Larson / ElasticCapacity read:
    source10k combined packed lane stays the selected minimum-RSS sibling.
    ElasticDescriptorRouteOverflow-L1 stays candidate-watch.
    ElasticDescriptorSourceRouteOverflow-L1 stays lower-RSS source-depot
    evidence/control, not promotion.
    SourceBlockLocalizeDryRun-L1 closes whole-SourceBlock localize as no-go:
    probes are blocked by shared SourceBlock ownership, so the next design
    should be slot-level/source-run ownership or descriptor storage locality.

  rule:
    dry-run / diagnostic lanes must not enter production speed-ranking tables.

2026-06-05 SourceRunLocalityDryRun-L1:
  implementation:
    Add HZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1.
    It runs only with diagnostic probes and observes transfer reuse from the
    elastic SourceBlock depot.

  purpose:
    After whole-SourceBlock localize was blocked by shared ref-counts, measure
    whether depot transfer objects are at least slot-level localizable.

  smoke:
    docs/benchmarks/windows/paper/
      hz6_elastic_source_run_locality_dryrun_smoke/
      20260605_155944_hz6_capacity_matrix_windows.md
    main1k:
      57.417M / 92644 KB
      elastic_source_run_locality_probe=7487
      storage_mismatch=7487
      run_miss=7487
      slot_match=7487
      would_rehome_slot=7487
      safety clean

  full10k:
    docs/benchmarks/windows/paper/
      hz6_elastic_source_run_locality_dryrun_full10k/
      20260605_160007_hz6_capacity_matrix_windows.md
    42.733M / 225168 KB
    elastic_source_run_locality_probe=79485
    storage_mismatch=79485
    run_miss=79485
    slot_match=79485
    would_rehome_slot=79485
    safety clean

  read:
    current source-depot lane does not carry source-run metadata, hence
    run_miss.  But every probed transfer object is physically identifiable as
    a block/offset/bytes slot.  Next behavior should be slot-level ownership or
    descriptor storage locality; do not revive whole-SourceBlock localize.

2026-06-05 SourceRunMetadataOnDepot-L1:
  implementation:
    Add HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1.
    It initializes source-run metadata on elastic source-depot blocks without
    enabling SourceRunReuse-L1 behavior.  The lane is diagnostic/prerequisite
    evidence, not a production speed row.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run4096

  smoke:
    docs/benchmarks/windows/paper/
      hz6_elastic_depot_source_run_meta_smoke3/
      20260605_164158_hz6_capacity_matrix_windows.md
    main1k:
      56.846M / 93552 KB
      elastic_source_run_locality_probe=7487
      elastic_source_run_locality_run_miss=0
      elastic_source_run_locality_slot_match=7487
      elastic_source_run_locality_would_rehome_slot=7487
      main-warmup depot metadata:
        init=937
        mark=14992
      safety clean

  full10k:
    docs/benchmarks/windows/paper/
      hz6_elastic_depot_source_run_meta_full10k2/
      20260605_163855_hz6_capacity_matrix_windows.md
    42.148M / 230032 KB
    elastic_source_run_locality_probe=79485
    elastic_source_run_locality_run_miss=0
    elastic_source_run_locality_slot_match=79485
    elastic_source_run_locality_would_rehome_slot=79485
    main-warmup depot metadata:
      init=9939
      mark=159024
    mismatch/failure counters:
      class_mismatch=0
      slot_misaligned=0
      too_many_slots=0
      used_count_mismatch=0
      route_invalid=0
      route_miss=0
      route_register_fail=0
      descriptor_exhausted=0
      source_block_exhausted=0
      alloc_fail=0

  read:
    SourceRunMetadataOnDepot-L1 closes the C prerequisite from the Pro consult.
    Depot SourceBlocks now carry source-run metadata and the prior run_miss
    signal drops to zero.  The next behavior should be SlotOwnerLocalityDryRun-L1
    with sparse per-slot ownership/locality projection; do not move whole
    SourceBlocks and do not enable SourceRunReuse-L1 in this depot lane yet.

2026-06-05 SlotOwnerLocalityDryRun-L1:
  implementation:
    Add HZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1 on top of
    SourceRunMetadataOnDepot-L1.  This is diagnostic-only: it does not change
    descriptor ownership, route ownership, SourceBlock storage owner, or
    transfer behavior.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerdryrun-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run4096

  smoke:
    docs/benchmarks/windows/paper/
      hz6_elastic_slot_owner_locality_dryrun_smoke/
      20260605_165044_hz6_capacity_matrix_windows.md
    main1k:
      57.435M / 113620 KB
      elastic_slot_owner_locality_probe=7487
      storage_mismatch=7487
      run_miss=0
      class_mismatch=0
      slot_match=7487
      owner_match=7487
      owner_mismatch=0
      would_set_owner=7487
      would_hit_owner=7487
      safety clean

  full10k:
    docs/benchmarks/windows/paper/
      hz6_elastic_slot_owner_locality_dryrun_full10k/
      20260605_165106_hz6_capacity_matrix_windows.md
    43.174M / 230020 KB
    elastic_slot_owner_locality_probe=79485
    storage_mismatch=79485
    run_miss=0
    class_mismatch=0
    slot_match=79485
    owner_match=79485
    owner_mismatch=0
    would_set_owner=79485
    would_hit_owner=79485
    safety clean:
      route_invalid=0
      route_miss=0
      route_register_fail=0
      descriptor_exhausted=0
      source_block_exhausted=0
      alloc_fail=0

  read:
    A0 gives the strongest locality signal so far.  Every depot transfer reuse
    probe remains a SourceBlock storage-owner mismatch, but every one is now
    a source-run slot match whose descriptor owner is current after transfer
    activation.  This supports a future sparse per-slot owner/locality metadata
    behavior.  Keep this as dry-run evidence; do not yet mutate slot owner
    state or rehome routes.

2026-06-05 SlotOwnerSparseMeta-L1:
  implementation:
    Add HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1 on top of
    SlotOwnerLocalityDryRun-L1.  This keeps behavior unchanged and records a
    sparse side-table entry keyed by SourceBlock pointer + slot index with the
    current owner16 token at transfer reuse.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownersparse-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run4096

  smoke:
    docs/benchmarks/windows/paper/
      hz6_elastic_slot_owner_sparse_meta_smoke/
      20260605_170219_hz6_capacity_matrix_windows.md
    main1k:
      56.550M / 116720 KB
      sparse_lookup=7487
      sparse_insert=7487
      sparse_hit=0
      sparse_update=0
      sparse_collision=259
      sparse_full=0
      safety clean

  full10k:
    docs/benchmarks/windows/paper/
      hz6_elastic_slot_owner_sparse_meta_full10k/
      20260605_170249_hz6_capacity_matrix_windows.md
    43.039M / 233124 KB
    elastic_slot_owner_locality_probe=79485
    elastic_slot_owner_locality_slot_match=79485
    elastic_slot_owner_locality_owner_match=79485
    elastic_slot_owner_sparse_lookup=79485
    elastic_slot_owner_sparse_miss=79485
    elastic_slot_owner_sparse_insert=79485
    elastic_slot_owner_sparse_hit=0
    elastic_slot_owner_sparse_update=0
    elastic_slot_owner_sparse_collision=62673
    elastic_slot_owner_sparse_full=0
    safety clean:
      route_invalid=0
      route_miss=0
      route_register_fail=0
      descriptor_exhausted=0
      source_block_exhausted=0
      alloc_fail=0

  read:
    The sparse side-table has enough capacity and no safety failure.  However,
    this workload inserts each observed transfer slot once and does not show
    same-slot sparse hits.  Treat SlotOwnerSparseMeta-L1 as metadata feasibility
    evidence, not as a performance behavior yet.  The next behavior should use
    this metadata to avoid storage-owner mismatch work or guide owner-local
    lookup; otherwise it remains a pure RSS-cost side table.

2026-06-05 next after SlotOwnerSparseMeta Pro consult:
  decision:
    Do not promote SlotOwnerSparseMeta-L1 directly.
    Do not use sparse slot owner as a descriptor storage-owner override.
    Logical slot owner and descriptor storage owner are different contracts.

  recommended order:
    1. DescriptorDepotOwnerDirectFastPath-L1.
       If a descriptor is from the shared descriptor depot, read/write the depot
       owner table directly before OwnerSourceSideMeta-L2 storage lookup.
       This has no sparse table, no RSS side table, and uses existing
       owner_source_side_meta_l2_lookup/hit counters as the effect signal.

    2. SlotOwnerConsumerDryRun-L1.
       Consume sparse owner entries from downstream free/owner-equal locations
       as diagnostic-only evidence.  Count would_skip_l2 and false_positive.

    3. SlotOwnerLogicalOwnerFastPath-L1.
       Only if consumer dry-run is strong: positive hit means logical owner
       match, miss/stale/mismatch falls back to the existing heavy owner path.

  acceptance for step 1:
    owner_source_side_meta_l2_lookup should drop on the source-depot lane.
    route_invalid=0, route_miss=0, route_register_fail=0.
    descriptor_exhausted=0, source_block_exhausted=0, alloc_fail=0.
    throughput should stay within -3% of the source-depot control, preferably
    recover toward ElasticDescriptorRouteOverflow.

2026-06-05 DescriptorDepotOwnerDirectFastPath-L1:
  implementation:
    Add HZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1.
    When a descriptor is in the elastic descriptor depot, descriptor owner
    get/set uses the shared depot owner table before OwnerSourceSideMeta-L2
    storage lookup.  This is a behavior lane, but it adds no sparse table, no
    route rehome, and no new production counter.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run512

  smoke:
    docs/benchmarks/windows/paper/hz6_depot_owner_direct_smoke/
      20260605_173815_hz6_capacity_matrix_windows.md
    main1k:
      57.649M / 105816 KB
      safety clean
      depot overflow not triggered, so owner_source_side_meta_l2_lookup=0

  full10k:
    docs/benchmarks/windows/paper/hz6_depot_owner_direct_full10k/
      20260605_173842_hz6_capacity_matrix_windows.md
    main10k:
      45.258M / 224600 KB
      safety clean:
        route_invalid=0
        route_miss=0
        route_register_fail=0
        descriptor_exhausted=0
        source_block_exhausted=0
        alloc_fail=0

    worker guard:
      docs/benchmarks/windows/paper/hz6_depot_owner_direct_worker_guard/
        20260605_174232_hz6_capacity_matrix_windows.md
      worker10k:
        47.481M / 214284 KB
        safety clean

  diagnostic A/B:
    control:
      docs/benchmarks/windows/paper/
        hz6_elastic_descsource_control_diag_full10k/
        20260605_174044_hz6_capacity_matrix_windows.md
      42.121M / 224608 KB
      owner_source_side_meta_l2_lookup=1547776055

    direct:
      docs/benchmarks/windows/paper/
        hz6_depot_owner_direct_diag_full10k/
        20260605_173933_hz6_capacity_matrix_windows.md
      42.946M / 227748 KB
      owner_source_side_meta_l2_lookup=489480577

  read:
    KEEP as a strong ElasticCapacity behavior candidate.  Depot owner direct
    cuts OwnerSourceSideMeta-L2 lookup by about 68% in diagnostic A/B and
    improves the non-diagnostic source-depot shape to 45.258M while preserving
    roughly the same low RSS.  L2 lookup does not drop to zero, so the remaining
    pressure is not depot-descriptor owner table work; the next step can be a
    repeat/guard closeout or SlotOwnerConsumerDryRun-L1 if more owner-path work
    needs to be explained.

2026-06-05 DescriptorDepotOwnerDirectFastPath-L1 repeat/guard closeout:
  repeat-3:
    docs/benchmarks/windows/paper/hz6_depot_owner_direct_repeat3/
      20260605_175337_hz6_capacity_matrix_windows.md
    main10k median:
      46.273M / 224612 KB
      safety clean:
        route_invalid=0
        route_miss=0
        route_register_fail=0
        descriptor_exhausted=0
        source_block_exhausted=0
        alloc_fail=0
        remote_free_transfer_fail=0

  guard matrix:
    docs/benchmarks/windows/paper/hz6_depot_owner_direct_guard_matrix/
      20260605_175453_hz6_capacity_matrix_windows.md
    main1k:
      58.318M / 92068 KB, safety clean
    worker1k:
      57.710M / 91784 KB, safety clean
    main4k:
      47.912M / 139052 KB, safety clean
    worker4k:
      52.784M / 132804 KB, safety clean
    worker10k:
      42.459M / 214292 KB, safety clean

  decision:
    Upgrade depotownerdirect from one-off behavior evidence to
    ElasticCapacity candidate-watch.  It does not become a broad default yet,
    but it is the new best source-depot speed/RSS shape in this track.  The
    next natural experiment is SlotOwnerConsumerDryRun-L1 only if we still need
    to explain the remaining non-depot owner path cost.

2026-06-05 SlotOwnerConsumerDryRun-L1:
  implementation:
    Add HZ6_ELASTIC_SLOT_OWNER_CONSUMER_DRYRUN_L1.
    The sparse slot-owner table is now shared by the producer note path and a
    diagnostic consumer dry-run.  The consumer runs only after the existing
    descriptor-owner equality path computes the real answer, then asks whether
    a sparse per-slot owner entry would have safely skipped heavier L2 owner
    lookup work.  It does not change allocation/free behavior.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-
    slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-
    noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
    ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-
    run4096

  smoke:
    docs/benchmarks/windows/paper/
      hz6_slot_owner_consumer_dryrun_smoke/
      20260605_180342_hz6_capacity_matrix_windows.md
    main1k:
      55.042M / 117620 KB
      elastic_slot_owner_consumer_probe=88111696
      elastic_slot_owner_consumer_hit=88096722
      elastic_slot_owner_consumer_miss=14974
      elastic_slot_owner_consumer_would_skip_l2=88096722
      elastic_slot_owner_consumer_false_positive=0
      safety clean

  full10k:
    docs/benchmarks/windows/paper/
      hz6_slot_owner_consumer_dryrun_full10k/
      20260605_180432_hz6_capacity_matrix_windows.md
    main10k:
      36.691M / 233556 KB
      elastic_slot_owner_consumer_probe=687695410
      elastic_slot_owner_consumer_hit=687536440
      elastic_slot_owner_consumer_miss=158970
      elastic_slot_owner_consumer_owner_match=687536440
      elastic_slot_owner_consumer_owner_mismatch=0
      elastic_slot_owner_consumer_stale_generation=0
      elastic_slot_owner_consumer_false_positive=0
      elastic_slot_owner_consumer_would_skip_l2=687536440
      elastic_slot_owner_consumer_fallback=158970
      owner_source_side_meta_l2_lookup=418621565
      safety clean:
        route_invalid=0
        route_miss=0
        route_register_fail=0
        descriptor_exhausted=0
        source_block_exhausted=0
        alloc_fail=0
        remote_free_transfer_fail=0

  read:
    KEEP as diagnostic evidence.  The sparse owner table is no longer merely a
    producer-side feasibility table: downstream owner-equality sites see very
    high sparse hits and would_skip_l2, with false_positive=0 and no generation
    staleness in both smoke and full10k.  The diagnostic overhead is heavy, so
    this row is not a speed-ranking lane.  It does justify one narrow behavior
    experiment: SlotOwnerLogicalOwnerFastPath-L1, where a generation-checked
    sparse hit may answer logical owner equality and miss/stale/mismatch falls
    back to the existing descriptor owner path.

2026-06-05 SlotOwnerLogicalOwnerFastPath-L1:
  implementation:
    Add HZ6_ELASTIC_SLOT_OWNER_LOGICAL_FASTPATH_L1.
    The sparse slot-owner table is updated outside diagnostic builds when this
    behavior is enabled, but sparse-table counters remain diagnostic-only.
    The fast path answers only a positive logical owner match:
      source-run slot match
      block pointer match
      descriptor generation match
      owner16 match
    Any miss, stale generation, or owner mismatch falls back to the existing
    descriptor owner path.  It never uses sparse metadata as a storage-owner
    override and never proves negative equality.

  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerlogical-
    desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
    routebytes16-storageowner16-ownersourcel2-frontcachepacked-
    sourceblockpacked-source64-route16k-run4096

  smoke diagnostic:
    docs/benchmarks/windows/paper/hz6_slot_owner_logical_smoke/
      20260605_183652_hz6_capacity_matrix_windows.md
    main1k:
      56.183M / 117616 KB
      logical_probe=89176100
      logical_hit=89161126
      logical_miss=14974
      stale_generation=0
      owner_mismatch=0
      fallback=14974
      owner_source_side_meta_l2_lookup=53848038
      safety clean

  full10k non-diagnostic:
    docs/benchmarks/windows/paper/hz6_slot_owner_logical_full10k/
      20260605_183723_hz6_capacity_matrix_windows.md
    main10k:
      38.494M / 239484 KB
      safety clean:
        route_invalid=0
        route_miss=0
        route_register_fail=0
        descriptor_exhausted=0
        source_block_exhausted=0
        alloc_fail=0
        remote_free_transfer_fail=0

  full10k diagnostic:
    docs/benchmarks/windows/paper/hz6_slot_owner_logical_diag_full10k/
      20260605_183811_hz6_capacity_matrix_windows.md
    main10k:
      38.275M / 233568 KB
      logical_probe=717941382
      logical_hit=717746510
      logical_miss=194872
      stale_generation=0
      owner_mismatch=0
      fallback=194872
      owner_source_side_meta_l2_lookup=366129578
      safety clean

  read:
    KEEP as safety/contract evidence, but mark speed no-go/control.  The
    logical match contract is safe in this slice and removes a large amount of
    L2 owner-path work.  However, trying the sparse table at every
    owner_equal() entry is too broad: non-diagnostic full10k falls to 38.494M,
    below depotownerdirect repeat-3 at 46.273M.  Do not promote this as a
    default or candidate-watch lane.  A future attempt would need a narrower
    admission gate that avoids probing sparse metadata on cheap local/depot
    owner checks.

2026-06-05 next attack after combined packed Pro consult:
  Larson cross-owner RSS:
    OwnerSourceSideMeta-L2 remains the selected speed/RSS balance sibling.
    Combined FrontCachePackedMeta-L1 + SourceBlockPackedFlags-L1 is the
    selected minimum-RSS sibling/candidate.
    FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 remain component
    controls/evidence.

  reason:
    combined packed repeat-3 is clean at 40.837M / 426084 KB.
    It is lower RSS than either component alone and close to L2 throughput,
    but it is not a universal default or broad throughput promotion.

  immediate implementation:
    LarsonRssResidualAudit-L1 diagnostic-only.
    Do not add another packing lane before measuring the remaining RSS source.

  audit lanes:
    OwnerSourceSideMeta-L2
    FrontCachePackedMeta-L1
    SourceBlockPackedFlags-L1
    combined packed

  audit target:
    split residual RSS into route/shared directory, owner-locality index,
    descriptor hot/cold/side metadata, source-block/source-run metadata,
    frontcache, transfer, and payload/source retention.

  next behavior only after reading residual:
    route/shared directory high:
      RouteDirectorySlim / OwnerLocalityDirectorySlim
    descriptor side metadata high:
      OwnerSourceSideMeta compaction or descriptor ownership representation
    payload/source retention high:
      source retention/scavenge checkpoint
    no clear dominant table:
      stop micro-packing and return to speed/RSS balance work

2026-06-04 next attack after Pro consult:
  mixed_ws / wide_ws:
    next main target is route representation / route probe shape, not
    frontcache/source knobs.

  reason:
    route17 -> route18 improves wide_ws while route20 does not add more.
    recent frontcache/source class diagnostics are effectively identical
    across desc17/route17, desc17/route18, desc17/route20, and desc20/route20.
    The remaining difference is therefore route load/probe/churn shape.

  immediate implementation:
    RouteProbeShape-L1 diagnostic only.
    Add lookup/register/unregister probe buckets:
      1, 2-4, 5-8, 9-16, 17-64, 65+
    Keep counters behind HZ6_DIAGNOSTIC_PROBES and do not mix them into
    production speed lanes.

  compare first:
    mixedclean-front16k-sourcerun-desc17k-source2k-route17k
    mixedclean-front16k-sourcerun-desc17k-source2k-route18k
    mixedclean-front16k-sourcerun-desc17k-source2k-route20k

  next behavior only after reading shape:
    tombstone/cluster high:
      RouteBackshiftCompact-L1 or tombstone compact policy
    invalid envelope scan high:
      exact/envelope route split or page/segment route directory
    cached/local-free exact pollution high:
      active/cached route representation split
    hash skew high:
      hash/layout change
    class-specific pressure:
      class-aware route admission/capacity

  Larson:
    freeze current selected lowest-RSS sibling for now.
    owner/source-aware descriptor ownership is valuable but L2 and should not
    be mixed with the mixed_ws route experiment.

  initial RouteProbeShape-L1 smoke:
    source:
      docs/benchmarks/windows/paper/hz6_route_probe_shape_l1/
        20260604_151113_hz6_capacity_matrix_windows.md
    wide_ws one-run, diagnostic-only:
      route17:
        20.429M / 140292 KB
        lookup_probe_total = 11.684M
        lookup b65+ = 35274
        register_probe_total = 1.856M
        register b65+ = 5459
      route18:
        21.534M / 140576 KB
        lookup_probe_total = 5.263M
        lookup b65+ = 12706
        register_probe_total = 793K
        register b65+ = 1987
      route20:
        19.282M / 141100 KB
        lookup_probe_total = 3.053M
        lookup b65+ = 2017
        register_probe_total = 449K
        register b65+ = 316
    read:
      route18 clearly cuts long probes relative to route17.
      route20 cuts probes further but loses speed in this one-run smoke, so
      the next read should not assume "lower probe count == faster".  Route
      table footprint/cache locality, hash/layout, and diagnostic variance are
      now plausible alongside tombstone/cluster pressure.
    next check:
      repeat route17/18/20 with balanced + wide_ws before behavior.

  repeat-3 confirmation:
    diagnostic repeat:
      source:
        docs/benchmarks/windows/paper/hz6_route_probe_shape_repeat/
          20260604_151811_hz6_capacity_matrix_windows.md
      balanced:
        route17 64.487M / 110940 KB, lookup_probe_total 1.273M, max 8
        route18 65.386M / 111068 KB, lookup_probe_total 1.269M, max 9
        route20 66.007M / 111628 KB, lookup_probe_total 1.242M, max 12
      wide_ws:
        route17 19.905M / 140312 KB, lookup_probe_total 13.449M, b65+ 33025
        route18 20.444M / 140576 KB, lookup_probe_total 5.495M, b65+ 14248
        route20 20.164M / 140952 KB, lookup_probe_total 3.036M, b65+ 2390
    non-diagnostic repeat:
      source:
        docs/benchmarks/windows/paper/hz6_route_probe_shape_nondiag_repeat/
          20260604_151900_hz6_capacity_matrix_windows.md
      balanced:
        route17 67.002M / 110960 KB
        route18 65.711M / 111152 KB
        route20 65.453M / 111668 KB
      wide_ws:
        route17 20.731M / 140248 KB
        route18 21.523M / 140580 KB
        route20 18.154M / 139752 KB
    read:
      selected split still stands:
        balanced low-RSS/speed = route17
        wide_ws sibling = route18
      route20 cuts probe count but loses speed in non-diagnostic wide_ws, so
      bigger route capacity is not the next behavior.  The next experiment
      should try to reduce route17/18 cluster shape without growing the table
      footprint.  Tombstones are not the mixed_ws pressure: unregister is zero
      in this workload.  Prefer a hash/probe-layout experiment before
      backshift/tombstone compaction.

  RouteDoubleHash-L1:
    implementation:
      HZ6_ROUTE_DOUBLE_HASH_L1 changes the first open-addressing pass from
      linear probing to double hashing.  It keeps exact/invalid route semantics
      unchanged and is exposed only as a capacity lane:
        mixedclean-front16k-sourcerun-desc17k-source2k-route18k-doublehash
    diagnostic smoke:
      source:
        docs/benchmarks/windows/paper/hz6_route_doublehash_l1_smoke/
          20260604_152425_hz6_capacity_matrix_windows.md
      route18 wide_ws:
        lookup_probe_total = 5.639M
        lookup b65+ = 13699
      route18 doublehash wide_ws:
        lookup_probe_total = 2.479M
        lookup b65+ = 34
      safety:
        route_invalid = 0
        route_miss = 0
        route_register_fail = 0
        alloc_fail = 0
    non-diagnostic repeat-3:
      source:
        docs/benchmarks/windows/paper/hz6_route_doublehash_l1_repeat/
          20260604_152734_hz6_capacity_matrix_windows.md
      route18 wide_ws:
        21.401M / 140496 KB
      route18 doublehash wide_ws:
        20.886M / 140564 KB
    read:
      no-go / control evidence.  Double hashing successfully removes the long
      probe tail, but the extra probe-step cost and less linear memory access
      do not improve throughput.  Do not promote.  The next route experiment
      should keep linear locality and reduce cluster/load cost by cheaper
      means, for example route-entry layout slimming, table split, or a cheaper
      hash perturbation that preserves linear probing.
    safety:
      Windows HZ6 R1 smoke suite passed after adding the route probe sequence
      abstraction and doublehash control lane:
        core-contract ok
        route ok
        transfer-contract ok
        source-contract ok
        allocator ok
        prefill ok
        sourceblock ok
        transfer ok
        reclaim ok
        safety ok
```

```text
mixed clean-lane update:
  selected-mixed-lowrss is now clean:
    rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k

  selected-family refresh repeat-3:
    balanced 55.504M / 110780 KB
    wide_ws  19.978M / 140236 KB
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0

  boundary scan reference:
    balanced 64.117M / 110976 KB
    wide_ws  21.119M / 140256 KB

  boundary:
    desc16 remains no-go for wide_ws:
      alloc_fail = 6943
    desc16 transfer2304/2560 still reports the same alloc_fail.
    desc17 is the current clean lower bound.

  siblings:
    desc20 is a clean wide-speed sibling:
      wide_ws 21.498M / 142676 KB
    desc18/22/24/32 and source3/source4 remain controls.

  descavail-noboost-route4k:
    demoted to pressure evidence only.

selected-family repeat-3:
  random_mixed:
    clean and stable:
      small 45.755M / 4968 KB
      medium 42.408M / 4964 KB
      mixed 41.306M / 4964 KB

  larger_sizes:
    clean:
      speed 26.404M / 71040 KB
      rss   27.178M / 71012 KB

  Larson full 10k:
    desc160k     44.754M / 808488 KB, clean
    front4k      45.092M / 716324 KB, clean
    thindesc16k  44.609M / 665704 KB, clean
    route192k    44.610M / 628844 KB, clean control
    run512       48.512M / 499820 KB, clean previous lowest-RSS sibling/control
    no-backptr   40.710M / 476784 KB, clean selected lowest-RSS sibling candidate

  mixed balanced / wide_ws:
    descavail-noboost-route4k is fast and very low RSS, but not clean:
      balanced 75.467M / 17376 KB with alloc_fail 1509729
      wide_ws  57.144M / 12524 KB with alloc_fail 1504489
    Treat these as capacity-failure pressure rows, not selected clean rows.

next HZ6 attack:
  clean balanced / wide_ws low-RSS lane is now found and selected.
  selected-family refresh is done under:
    docs/benchmarks/windows/paper/hz6_selected_family/
      selected-family-desc17-refresh/
  Larson MetadataSlim-L1 route192k and SourceBlockMetaSlim-L1 run512 are now
  found:
    docs/benchmarks/windows/paper/hz6_selected_family/
      larson-metadata-slim-route192-repeat/
    route192k repeat-3:
      44.610M / 628844 KB
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0
    route192k-run512 repeat-3:
      48.512M / 499820 KB
      alloc_fail = 0
      descriptor_exhausted = 0
      route_register_fail = 0
      source_block_exhausted = 0
    default-check one-run:
      40.688M / 499812 KB
      safety clean
    attribution-check one-run:
      40.978M / 499820 KB
      descriptor_table_bytes    = 127926272
      route_table_bytes         = 100663296
      source_block_table_bytes  = 37748736
    route under run512 one-run:
      route192k-run512:
        45.997M / 499824 KB, safety clean
      route160k-run512:
        warmup no-go
        route_active_current = 163840
        route_register_fail = 3
        alloc_fail = 1
      route128k-run512:
        warmup no-go
        route_active_current = 131072
        route_register_fail = 3
        alloc_fail = 1
    descriptor under run512:
      desc158k-route192k-run512 repeat-3:
        40.400M / 498080 KB, safety clean
      desc160k-route192k-run512 same repeat:
        40.578M / 499804 KB, safety clean
      desc156k-route192k-run512 and below:
        warmup no-go
        descriptor_exhausted = 3
        alloc_fail = 1
    descriptor layout under run512:
      no-backptr L1 removes the per-descriptor allocator back-pointer and
      passes allocator explicitly through descriptor lifecycle helpers.
      diagnostic:
        descriptor_entry_bytes = 48 -> 40
        descriptor_table_bytes = 127926272 -> 106954752
        descriptor_thin_hot_entry_bytes = 40
        descriptor_thin_hot_savings_bytes = 20971520
      repeat-3:
        baseline run512:
          40.498M / 499812 KB, safety clean
        no-backptr run512:
          40.710M / 476784 KB, safety clean
      read:
        strong keep. This saves about 23 MB peak RSS versus the run512
        baseline without hurting throughput. Treat no-backptr run512 as the
        new selected Larson lowest-RSS sibling candidate; keep desc158k as a
        tiny static-capacity control, not the main direction.
      runner:
        larson-cross-owner-selected and larson-cross-owner-lowest-rss now use
        the no-backptr lane as the selected low-RSS sibling. The old run512
        lane remains in larson-run512-descriptorlayout as the direct control.
      selected preset guard:
        larson-cross-owner-lowest-rss one-run:
          front4k:
            42.460M / 716340 KB, safety clean
          route192k:
            44.583M / 628848 KB, safety clean
          no-backptr run512:
            42.324M / 476868 KB, safety clean
        source:
          docs/benchmarks/windows/paper/hz6_selected_family/
            larson-nobackptr-selected-guard/
        read:
          selected-family wiring is correct and no-backptr remains clean as the
          lowest-RSS sibling in the selected preset path.
      descriptor layout L2 dry-run:
        source:
          docs/benchmarks/windows/paper/hz6_selected_family/
            larson-descriptor-layout-l2-dryrun-clean/
        baseline run512 diagnostic:
          descriptor_entry_bytes = 48
          descriptor_thin_hot_entry_bytes = 40
          descriptor_ownerless_hot_entry_bytes = 32
          descriptor_owner16_hot_entry_bytes = 40
          ownerless theoretical savings vs current table = 41943040 bytes
          owner16 theoretical savings vs current table = 20971520 bytes
        no-backptr run512 diagnostic:
          descriptor_entry_bytes = 40
          descriptor_ownerless_hot_entry_bytes = 32
          descriptor_ownerless_hot_table_bytes = 85983232
          descriptor_ownerless_hot_savings_bytes = 20971520
          descriptor_owner16_hot_entry_bytes = 40
          descriptor_owner16_hot_savings_bytes = 0
        read:
          owner16 packing does not improve the hot descriptor shape because
          alignment keeps it at 40 bytes. The next descriptor RSS target is
          ownerless/side-owner metadata, not 16-bit owner packing.

  next:
    SourceBlock is no longer the dominant table after run512.
    Route is capacity-bounded under run512; route192k remains the clean lower
    bound. Do not trim route capacity again without a new route representation.
    Static descriptor capacity can be trimmed only to desc158k under the current
    lifecycle, and the win is small. Descriptor no-backptr L1 is the first
    successful descriptor layout change: it converts the allocator back-pointer
    into explicit helper context and cuts the descriptor table by about 20 MiB.
    The next Larson RSS target should be a guarded selected-family promotion of
    no-backptr, or a follow-up descriptor representation change; do not return
    to blind static descriptor/route capacity cuts.
    same-run thindesc16k baseline:
      40.267M / 665700 KB
    route160k/route128k and route160k-run512/route128k-run512:
      warmup no-go due route-table saturation.

  next step is cross-allocator table cleanup, then choose the next focused
  optimization target:
    A. wide_ws throughput while preserving desc17 safety/RSS
    B. Larson metadata layout slimming only after checking the next table
       bottleneck

thindesc broad default:
  no

thindesc selected-family role:
  source16k-route192k-run512 is now the previous Larson / owner-locality
  lowest-RSS control. The selected lowest-RSS sibling candidate is the
  no-backptr source16k-route192k-run512 variant. source16k-route192k remains
  its route-capacity control; source16k without route trimming remains its
  baseline control.

next order:
  A. selected-family / cross-allocator table cleanup
  B. pick next optimization target from wide_ws throughput or Larson RSS
```

Historical selected-family result before desc17 refresh:

```text
Larson full 10k repeat-3:
  desc160k:
    43.506M ops/s, 808484 KB, safety clean
  front4k:
    42.925M ops/s, 716332 KB, safety clean
  front4k-thindesc:
    failed 3/3 during warmup
    source_alloc=12288
    alloc_fail=1
    source_block_exhausted=257

Read:
  front4k is the current full-10k lower-RSS sibling.
  thindesc remains compact/moderate evidence only until source-block
  capacity/retention is addressed.
  next attack is B: ownerlocality + largerlowrss/source-run retention.

Next experiment:
  add source16k/source32k thindesc siblings:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k
  use:
    win/run_win_hz6_selected_family.ps1 -LarsonThinDescSourceCap

Result:
  thindesc-source16k:
    repeat-3 full 10k:
      46.819M ops/s
      665704 KB peak
      safety clean
    read:
      selected low-RSS Larson sibling.

  front4k:
    repeat-3 full 10k:
      44.887M ops/s
      716328 KB peak
      safety clean

  thindesc-source32k:
    run-1:
      42.029M ops/s
      836644 KB peak
    read:
      over-retention control, not selected.

Source-cap tighten:
  source12k:
    repeat-3:
      44.308M ops/s
      623084 KB peak
      safety clean
    read:
      lowest-RSS control; throughput below source16k.
  source14k:
    run-1:
      44.471M ops/s
      644836 KB peak
      safety clean
  source16k:
    remains selected:
      47.040M ops/s
      665712 KB peak in the tighten repeat.
```

## HZ6 Lane Dashboard

Canonical lane guide:

```text
hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
```

Current lane read:

```text
route4k:
  current Windows candidate-control lane.
  Use first for low-RSS HZ6 capacity checks.

control:
  low-capacity / low-RSS baseline.

appcap:
  high-capacity completion/control lane.
  Do not treat as the production/default shape.

sourcerun-route4k:
  source-run reuse evidence/control.

sourcerun-sameclass-route4k:
  safer same-class reclaim evidence/control.

spill-route4k / borrow-route4k / cap-route4k / sourcerun-reclaim-route4k:
  frozen no-go controls.
```

Next design pressure remains descriptor lifecycle / frontcache-visible
descriptor reuse, not another frontcache spill/borrow/cap knob.

## Latest Windows Matrix Read

Latest full matrix:

```text
private/allocators/hakozuna/docs/benchmarks/windows/20260601_045004_allocator_matrix.md
```

Read:

```text
balanced / wide_ws:
  HZ6 strict/speed/rss are far below HZ3 / HZ4 / mimalloc / tcmalloc.
  broad-cap rows improve throughput but raise RSS substantially.

larger_sizes:
  HZ6 strict/speed/rss stay very low-RSS and are still throughput weak.
  broad-cap rows recover throughput well and stay well below the mixed-row RSS
  blow-up seen on balanced / wide_ws.

large_slice_64k / 128k:
  HZ6 strict/speed/rss are strong and low-RSS on the exact large-slice rows.
  128k is particularly strong in the broad lanes and beats tcmalloc in the
  current run.

RSS:
  the matrix now carries peak RSS KB for every allocator row, including
  HZ3 / HZ4 / mimalloc / tcmalloc.
```

Current interpretation:

```text
strong:
  large_slice_64k
  large_slice_128k
  exact large-slice low-RSS lanes

weak:
  balanced
  wide_ws
  small/mixed rows under strict/speed/rss controls

tradeoff:
  broad-cap HZ6 lifts throughput on many rows, but it also raises RSS.
```

Paper runner coverage:

```text
RSS-aware summaries now cover:
  allocator matrix
  Larson paper runner
  Redis workload paper runner
  MT remote paper runner

Next validation order:
  1. rerun the RSS-aware paper runners with the new peak_kb columns
  2. keep the balanced / wide_ws weakness as the main HZ6 warning
  3. keep large_slice_64k / 128k as the strong proof points
```

Latest weakness read:

```text
Larson:
  worker-warmup is good.
  stress and appcap are still the weak lanes, with HZ6 failing or
  underperforming on the heavier T=8/T=16 rows.

Redis:
  HZ6 appcap is weak across SET / GET / LPUSH / LPOP / RANDOM.
  mimalloc and tcmalloc stay ahead on every Redis pattern in the latest run.

Random mixed:
  strict / speed / rss controls still fail on the small and medium/mixed rows.
  broad and appcap survive, but they remain far behind mimalloc / tcmalloc on
  throughput, so mixed throughput is still a weak lane.

HZ6 owner-aware MT remote:
  3-run median is 17.388M to 19.471M ops/s depending on profile.
  peak RSS stays low at about 7.9 to 8.1 MB.
  alloc_failures are still huge at 21.9M, so this remains a pressure lane
  rather than a clean win lane.
```

Latest Larson lifecycle diagnostic:

```text
Diagnostic-only counters added:
  lifecycle_owner_mismatch
  lifecycle_foreign_free_attempt / handled / invalid
  frontcache_push_by_class[16]
  frontcache_pop_empty_by_class[16]

Smoke:
  bench_larson_hz6_speed_appcap.exe 2 8 1024 10000 1 12345 16 <mode>

main-warmup:
  throughput = 1249 ops/s
  foreign_free_attempt = 2499
  foreign_free_handled = 2499
  foreign_free_invalid = 0
  route_rehome_success = 2499
  route_rehome_fail = 0
  route_visibility_probe_max = 1
  route_lookup_probe_total = 2620397339
  route_lookup_probe_max = 524290

worker-warmup:
  throughput = 44.044M ops/s
  lifecycle/visibility/transfer counters stay 0
  route_lookup_probe_total = 99234056
  route_lookup_probe_max = 5

Read:
  remote handoff is safe in this smoke but not fast enough.
  visibility depth is not the current blocker.
  the main-warmup collapse is now dominated by the expensive worker-local
  route MISS before shared visibility.

Next:
  try a route negative-filter / visible-first diagnostic / shared directory
  experiment before adding more remote handoff capacity.
```

Next HZ6 route-lifecycle order:

```text
1. L0 VisibleAfterLocalMiss refactor:
   remove the duplicate worker-local route MISS inside visible lookup after
   hz6_free() has already missed locally.

2. A VisibleFirstFree-L1 diagnostic:
   non-strict diagnostic lane only.
   Try visible-only lookup before local lookup to measure the upper bound of
   skipping expensive local MISS scans.

3. B NegativeFilter-L1:
   production-near conservative local owned-range filter.
   Only skip local lookup for DEFINITELY_NOT_LOCAL; shadow-verify first.

4. C SharedRouteDirectory-L1:
   larger design if B cannot recover enough of A.
```

Latest route-lifecycle result:

```text
L0 VisibleAfterLocalMiss:
  safe refactor.
  main-warmup throughput improved in smoke, but expensive local route MISS
  remains the dominant cost.

VisibleFirstFree-L1 diagnostic:
  added visiblefirst-appcap as explicit diagnostic lane.
  small live-set T=16 shows the expected upper-bound:
    route_lookup_probe_total drops to 3481
    visible_first_local_lookup_skipped = 4677
    route_invalid = 0

  full appcap live-set T=16 crashes with exit -1073741819.
  The lane also sees many visible invalid envelopes before local fallback:
    visible_first_visible_invalid = 2250 in the small live-set smoke.

Decision:
  visiblefirst-appcap is no-go / upper-bound evidence.
  Do not promote visible-first behavior.

Next:
  B NegativeFilter-L1.
  Use conservative local owned-range hints plus shadow verification, rather
  than trying visible-first as a production policy.
```

Failure-row diagnosis:

```text
random_mixed 20260601_070706:
  small strict/speed/rss:
    fails at iter 79, size 1428
    source_alloc = 64
    descriptor_exhausted = 1
    route_register_fail = 0
    source_block_exhausted = 0

  medium strict/speed/rss:
    fails at iter 57, size 21472
    source_alloc = 26
    descriptor_exhausted = 0
    route_register_fail = 1
    source_block_exhausted = 11

  mixed strict/speed/rss:
    fails at iter 55, size 17001
    source_alloc = 26
    descriptor_exhausted = 0
    route_register_fail = 1
    source_block_exhausted = 8
```

Read:

```text
The strict/speed/rss failures are capacity-control failures, not a pure speed
verdict:
  default descriptor capacity is 64
  default route capacity is 64
  default source-block capacity is 16
  random_mixed uses WS=400

small fails first on descriptor capacity.
medium/mixed fail first on source-block plus route registration capacity.

The actual performance problem is the passing broad/appcap rows:
  they survive with low-to-moderate RSS
  but remain far behind HZ3 / HZ4 / mimalloc / tcmalloc
```

Control lane read:

```text
random_mixed 20260601_072659:
  hz6-strict-control / speed-control / rss-control
    pass on small, medium, and mixed
    peak RSS ~4.8 MB
    route_register_probe_total is much smaller than broad/appcap
    throughput is still below broad, but the capacity failures are gone

  broad:
    still the faster HZ6 lane on mixed
    peak RSS ~7.3 MB

  appcap:
    still similar on throughput to broad in some rows
    peak RSS ~196 MB
```

Route lookup diagnostic read:

```text
Implementation:
  route_lookup_probe_total / route_lookup_probe_max added under
  HZ6_DIAGNOSTIC_PROBES only.
  random_mixed diagnostic builds now write to out_win_random_mixed_diag so
  real benchmark executables in out_win_random_mixed are not overwritten.

random_mixed mixed RUNS=1 diagnostic:
  docs/benchmarks/windows/paper/20260601_074225_paper_random_mixed_windows.md

Passing HZ6 rows:
  broad:
    lookup probe total ~14.9M to 15.0M
    lookup max 5..6
    register probe total ~0.44M to 0.50M
    throughput ~28.4M to 33.1M ops/s
    RSS ~7.2 MB

  control:
    lookup probe total ~38.7M to 55.0M
    lookup max 55..144
    register probe total ~57K to 64K
    throughput ~24.0M to 30.9M ops/s
    RSS ~4.7 MB

  appcap:
    lookup probe total ~14.0M to 14.2M
    lookup max 2..4
    register probe total ~28.3M to 32.0M
    throughput ~27.5M to 32.2M ops/s
    RSS ~196 MB

Read:
  control is low-RSS and capacity-clean, but pays a repeated free-route lookup
  cost.

  appcap makes lookup cheap but burns huge registration probe work and RSS.

  broad is currently the best throughput/RSS compromise among HZ6 mixed rows,
  but still far behind HZ3 / mimalloc / tcmalloc.
```

Route4k lane read:

```text
Implementation:
  route4k keeps the control capacities for descriptor / transfer /
  source-block / front-cache, but raises only HZ6_ROUTE_TABLE_CAPACITY to
  4096.

random_mixed mixed RUNS=1 diagnostic:
  docs/benchmarks/windows/paper/20260601_075023_paper_random_mixed_windows.md

Throughput / RSS:
  hz6-strict-route4k:
    32.752M ops/s, 5.008 MB peak
  hz6-speed-route4k:
    28.102M ops/s, 5.016 MB peak
  hz6-rss-route4k:
    30.577M ops/s, 5.008 MB peak

Compared with control:
  strict route4k is about +18% throughput for about +0.8 MB RSS.
  speed route4k is about +21% throughput for about +0.8 MB RSS.
  rss route4k is about +20% throughput for about +0.8 MB RSS.

Probe read:
  route4k reduces lookup probe totals back to broad-like levels:
    route4k ~14.8M..15.4M
    control ~41.6M..71.1M

  route4k keeps appcap's lookup benefit without appcap RSS:
    appcap RSS ~195.7 MB
    route4k RSS ~5.0 MB

  route4k still has broad-like register probe totals:
    ~0.44M..0.50M
  but this is far below appcap's ~28M..32M register probe totals.

Read:
  route4k is the current best HZ6 random_mixed capacity shape:
  broad-class throughput, near-control RSS, and no appcap blow-up.
```

Route4k repeat-3 read:

```text
Source:
  small:
    docs/benchmarks/windows/paper/20260601_081612_paper_random_mixed_windows.md
  medium:
    docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md
  mixed:
    docs/benchmarks/windows/paper/20260601_082153_paper_random_mixed_windows.md

small:
  strict:
    33.013M ops/s, 4,556 KB
  speed:
    26.879M ops/s, 4,556 KB
  rss:
    28.921M ops/s, 4,560 KB

medium:
  strict:
    35.254M ops/s, 4,556 KB
  speed:
    30.282M ops/s, 4,556 KB
  rss:
    32.581M ops/s, 4,560 KB

mixed:
  strict:
    32.851M ops/s, 4,556 KB
  speed:
    28.399M ops/s, 4,552 KB
  rss:
    30.424M ops/s, 4,556 KB

Read:
  route4k stays stable across small / medium / mixed.
  It keeps RSS near the control lane while preserving broad-like throughput.
  This is strong enough to treat route4k as the current candidate-control lane.

Cross-family wiring:
  route4k is now also built into the mixed_ws allocator matrix plus the
  Larson and Redis app-like HZ6 matrices, so the same route-table-only
  capacity shape can be checked outside random_mixed.
```

Next step:

```text
1. Keep the new random_mixed control lane as the diagnostic bridge:
   it removes the capacity failures and keeps RSS low.

2. Promote route4k to the primary random_mixed HZ6 candidate-control lane.
   It isolates route-table capacity as the main cause of the control/broad
   throughput split.

3. Next measurement:
   run small / medium / mixed repeat-3 for route4k vs broad vs control.
   Then carry the same route4k shape into Larson and Redis app-like rows.
   If route4k stays stable, use it as the HZ6 Windows candidate-control
   capacity shape for random_mixed-style and app-like rows.

4. Keep large_slice_64k / 128k as the current strong proof points.
5. Leave Redis and Larson untouched until the random_mixed throughput gap is
   clearer.
```

Route4k cross-benchmark check:

```text
Sources:
  mixed_ws matrix:
    docs/benchmarks/windows/20260601_085515_allocator_matrix.md
  Redis workload:
    docs/benchmarks/windows/paper/20260601_085933_paper_redis_workload_windows.md
  Larson T=1 / compact / worker-warmup:
    docs/benchmarks/windows/paper/20260601_091055_paper_larson_windows.md

mixed_ws balanced:
  route4k improves over tiny control but still has heavy alloc_fail.
  rss-route4k is the best HZ6 route4k row:
    3.51M ops/s, 17.3 MB
  rss-broad:
    1.71M ops/s, 99.8 MB
  Read:
    route4k helps RSS/throughput balance, but balanced is not solved by route
    capacity alone.

mixed_ws wide_ws:
  route4k stays low-RSS and roughly broad-class for HZ6, but alloc_fail remains
  large.
  rss-route4k:
    0.437M ops/s, 12.1 MB
  strict-broad:
    0.452M ops/s, 141.5 MB
  Read:
    route4k is a better capacity shape than broad for RSS, but it does not fix
    wide working-set capacity.

mixed_ws larger_sizes:
  route4k is the strongest route4k proof point.
  strict-route4k:
    0.786M ops/s, 15.2 MB
  speed-route4k:
    0.762M ops/s, 14.2 MB
  rss-route4k:
    0.752M ops/s, 14.6 MB
  broad:
    0.751M..0.800M ops/s, 65..70 MB
  Read:
    route4k matches broad-class HZ6 throughput with far lower RSS for larger
    size matrix rows.

Redis workload:
  strict / broad / route4k all timed out at 60s.
  only appcap rows completed, but with very high RSS:
    ~584..647 MB
  Read:
    Redis is not a route-table-only problem. It needs descriptor/source/front
    capacity or a more adaptive admission shape before route4k matters.

Larson T=1:
  stress and worker-warmup:
    default / broad / route4k fail with rc1
    appcap runs, but at ~293..337 MB RSS
  compact control:
    route4k runs and matches broad/appcap throughput with far lower RSS.
    strict-route4k:
      16.25M ops/s, 7.2 MB
    speed-route4k:
      12.91M ops/s, 5.7 MB
    rss-route4k:
      14.97M ops/s, 5.9 MB
  Read:
    route4k is good for compact HZ6 controls, but Larson stress needs a
    non-route capacity or lifecycle fix.

Conclusion:
  route4k is a valid candidate-control capacity shape for compact/random_mixed
  and larger-size matrix checks.

  It is not enough for Redis or Larson stress. The next HZ6 work should attack
  non-route capacity/admission:
    descriptor capacity
    source block capacity
    front cache capacity
    adaptive source/front budget

  Start with a small fixed experiment:
    desc4k-route4k
  or:
    source512-route4k

  The goal is to find the minimum non-route capacity that removes alloc_fail
  without appcap's huge RSS footprint.
```

HZ6-only runner status:

```text
Implemented:
  win/build_win_hz6_capacity_suite.ps1
  win/run_win_hz6_capacity_matrix.ps1

Purpose:
  Run HZ6 capacity/profile lanes without rebuilding or executing
  HZ3 / HZ4 / HZ5 / mimalloc / tcmalloc every time.

Supported app-like families:
  mixed_ws
  random_mixed
  larson
  redis

Default HZ6 profiles:
  strict
  speed
  rss

Default capacity lanes:
  control
  route4k
  appcap

Additional focused lanes:
  desc4k-route4k:
    raises descriptor capacity to 4096 while keeping route4k and the other
    route4k/control capacities.
    Purpose: test whether balanced / Larson failures are descriptor-driven
    without moving to broad/appcap RSS.

  source512-route4k:
    raises source-block capacity to 512 while keeping route4k and the other
    route4k/control capacities.
    Purpose: test whether Redis / medium-mixed failures are source-block
    driven without moving to appcap RSS.

  desc4k-source512-route4k:
    raises descriptor capacity to 4096 and source-block capacity to 512 while
    keeping route4k plus control transfer/front-cache capacities.
    Purpose: test the smallest combined non-route shape before broad/appcap.

  front1k-desc4k-source512-route4k:
    raises front-cache bin capacity to 1024 on top of the combined descriptor /
    source / route lane, while keeping transfer capacity at 512.
    Purpose: isolate whether broad's win comes from front-cache capacity rather
    than transfer capacity.

Notes:
  Existing full comparison runners remain the source for allocator-vs-allocator
  tables. The HZ6-only runner is the fast iteration tool for capacity/admission
  experiments on the stable Windows machine.

  Diagnostic counters stay separate through -DiagnosticHz6Probes, which builds
  into out_win_hz6_capacity_diag instead of the normal speed artifacts.

Example:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 \
    -Families random_mixed \
    -BenchmarkProfiles small,mixed \
    -Hz6Profiles speed,rss \
    -CapacityLanes control,route4k,appcap \
    -Runs 3
```

Next focused capacity tests:

```text
1. random_mixed:
   route4k vs desc4k-route4k vs source512-route4k
   profiles:
     small, medium, mixed

2. mixed_ws:
   balanced / wide_ws / larger_sizes
   check whether desc4k-route4k removes alloc_fail without broad RSS.

3. Redis:
   source512-route4k first.
   If it still times out, route-only and source-block-only are insufficient.

4. Larson:
   desc4k-route4k first on compact/worker controls.
```

New lane smoke:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093158_hz6_capacity_matrix_windows.md

random_mixed small / speed / RUNS=1:
  route4k:
    27.082M ops/s, 4,548 KB
  desc4k-route4k:
    27.320M ops/s, 5,516 KB
  source512-route4k:
    27.415M ops/s, 5,048 KB

Read:
  Both focused non-route lanes build and run cleanly.
  On small, neither lane is needed for capacity correctness:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
  The next useful reads are medium/mixed, mixed_ws balanced/wide_ws, Redis,
  and Larson where route4k alone still failed or timed out.
```

Focused capacity read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093251_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_093323_hz6_capacity_matrix_windows.md

random_mixed medium/mixed:
  route4k remains the cleanest lane.
  desc4k-route4k and source512-route4k do not improve speed and add RSS.
  alloc_fail and descriptor/source/route exhaustion are already 0 on route4k.

mixed_ws balanced/wide_ws/larger_sizes:
  source512-route4k alone does not help while descriptor capacity remains 512.

  desc4k-route4k reduces alloc_fail dramatically, but moves pressure into
  source allocation and RSS:
    balanced:
      route4k 3.195M ops/s, 17.7 MB, alloc_fail 1.51M
      desc4k-route4k 0.469M ops/s, 130.2 MB, alloc_fail 11K
    wide_ws:
      route4k 0.388M ops/s, 12.2 MB, alloc_fail 1.50M
      desc4k-route4k 0.181M ops/s, 129.1 MB, alloc_fail 982K
    larger_sizes:
      route4k 0.747M ops/s, 14.6 MB, alloc_fail 459K
      desc4k-route4k 0.558M ops/s, 70.3 MB, alloc_fail 3K

Read:
  Descriptor capacity is a real blocker on mixed_ws, but lifting it alone
  exposes source-placement/RSS pressure. The next lane is the combined
  desc4k-source512-route4k check, not a broad/appcap jump.

Follow-up:
  desc4k-source512-route4k still trails broad and has broad-like RSS.
  Since mixed_ws rows report transfer_push/pop = 0, the broad delta is likely
  front-cache capacity rather than transfer capacity.
  Next check:
    front1k-desc4k-source512-route4k vs broad

Front-cache isolation read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093548_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_093735_hz6_capacity_matrix_windows.md

mixed_ws / rss / RUNS=1:
  balanced:
    desc4k-source512-route4k:
      0.564M ops/s, 122.1 MB, source_alloc 703K
    front1k-desc4k-source512-route4k:
      1.736M ops/s, 100.3 MB, source_alloc 191K
    broad:
      1.701M ops/s, 99.8 MB, source_alloc 191K

  wide_ws:
    front1k-desc4k-source512-route4k:
      0.191M ops/s, 93.4 MB
    broad:
      0.145M ops/s, 93.4 MB

  larger_sizes:
    front1k-desc4k-source512-route4k:
      0.728M ops/s, 65.3 MB
    broad:
      0.747M ops/s, 65.2 MB

Read:
  front1k-desc4k-source512-route4k reproduces broad behavior while keeping
  transfer capacity at 512. In these mixed_ws rows, transfer_push/pop remain 0,
  so broad's improvement is explained by front-cache capacity rather than
  transfer capacity.

  This is a good mechanistic result, but it is not a promotion lane:
    route4k is low-RSS but capacity-failing.
    broad/front1k shape is capacity-cleaner but RSS-heavy.

Next:
  attack front-cache/source placement rather than raising fixed capacities:
    reuse source-block slots more efficiently
    reduce source_alloc per successful object
    avoid broad-like RSS for balanced/wide_ws
```

HZ6 no-go checks after cleanup:

```text
Sources:
  docs/benchmarks/windows/paper/20260601_100622_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_100945_hz6_capacity_matrix_windows.md

Toy block prefill minimum:
  attempt:
    force non-strict toy block prefill to keep a minimum refill count instead
    of dropping to per-object source when source admission clamps to 1.

  result:
    smoke required strict opt-out.
    mixed_ws route4k lost throughput and did not solve descriptor exhaustion.

Descriptor-exhaustion scavenge-on-refill:
  attempt:
    call profile scavenge once when source/refill cannot find a free
    descriptor, then retry descriptor lookup.

  result:
    route4k still descriptor-exhausts on balanced/wide_ws/larger_sizes.
    balanced route4k regressed sharply:
      old route4k: ~3.2M ops/s, ~17 MB
      scavenge-on-refill route4k: 0.461M ops/s, 25.2 MB

Decision:
  Both behavior changes are no-go and were reverted.
  The remaining problem is not a one-shot refill tweak. It needs a clearer
  front-cache/source-lifetime design, likely class/run reuse or a bounded
  descriptor recycling policy that does not scan/release on the allocation
  slow path.
```

HZ6 diagnostic follow-up:

```text
Source:
  docs/benchmarks/windows/paper/20260601_101314_hz6_capacity_matrix_windows.md

mixed_ws / rss / diagnostic probes / RUNS=1:
  route4k / balanced:
    3.223M ops/s, 17.7 MB
    alloc_fail 1.51M
    descriptor_exhausted 3.39M
    descriptor_probe_total 1.736B
    source_block_probe_total 193.2M

  front1k-desc4k-source512-route4k / balanced:
    1.698M ops/s, 100.5 MB
    alloc_fail 45.8K
    route_register_fail 91.9K
    source_block_exhausted 31.9K
    route_register_probe_total 1.109B
    route_unregister_probe_total 25.3M

  broad / balanced:
    same shape as front1k-desc4k-source512-route4k.

Read:
  route4k is not bottlenecked by route capacity anymore. It is dominated by
  descriptor exhaustion and descriptor full-scan pressure.

  front1k/broad removes descriptor exhaustion, but shifts the cost into
  source allocation, route register/unregister probe pressure, and RSS.

  The next useful work is not another fixed capacity lane. It needs
  descriptor/source lifetime diagnostics and then a design that reuses
  descriptors/source slots without putting scan/release work on the allocation
  slow path.
```

HZ6 Redis focused read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_101345_hz6_capacity_matrix_windows.md

redis / rss / RUNS=1 / TimeoutSeconds=20:
  source512-route4k:
    all patterns timeout with rc124.

  front1k-desc4k-source512-route4k:
    all patterns timeout with rc124.

  appcap:
    completes all patterns, but peak RSS is about 597 MB.
    SET 12.06M
    GET 14.44M
    LPUSH 6.40M
    LPOP 15.98M
    RANDOM 7.63M

Read:
  Redis needs appcap-scale descriptor/source/front capacity or a smarter
  lifetime/source strategy. The focused non-route lanes are still too small,
  while appcap proves completion at unacceptable RSS.
```

Current HZ6 Windows direction:

```text
Stop:
  fixed capacity lane proliferation.

Keep:
  route4k as the low-RSS candidate-control for random_mixed and compact rows.
  broad/front1k as mechanism evidence for front-cache capacity.
  appcap as completion/control evidence only.

Next:
  add diagnostic-only state/lifetime attribution before behavior:
    descriptor state counts at failure/checkpoint
    source-block state counts at failure/checkpoint
    route register/unregister pressure by source/front if practical

  Then attack:
    descriptor recycling without full descriptor scans
    source-block/slot reuse by class or run
    route registration lifetime so broad-like completion does not require
    appcap-like RSS.

Do not:
  add hot-path atomics to real benchmark lanes.
  hide route4k failures with appcap defaulting.
  add another large fixed capacity lane without a lifetime hypothesis.
```

HZ6 lifetime-state diagnostic:

```text
Source:
  docs/benchmarks/windows/paper/20260601_102002_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_102217_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_102558_hz6_capacity_matrix_windows.md

Implementation:
  added diagnostic-only failure-state snapshots under HZ6_DIAGNOSTIC_PROBES.
  normal speed artifacts are not rebuilt with these counters unless
  -DiagnosticHz6Probes is requested.

mixed_ws / balanced / route4k:
  descriptor failure state:
    active_max = 512
    local_free_max = 457
    transfer/free/remote/orphan/released = 0

  source-block failure state:
    active_max = 128
    registered_max = 128
    ref_nonzero_max = 128
    ref_zero_max = 0

  frontcache distribution at descriptor failure:
    frontcache_total_max = 457
    largest_bin_max = 236
    nonempty_bins_max = 5

  class-aware spill dry-run:
    calls = 3.39M
    requested_empty = 3.39M
    candidate_calls = 834.9K
    reclaimable_total = 3.98M
    largest_donor_max = 235
    donor_bins_max = 4

mixed_ws / balanced / front1k-desc4k-source512-route4k:
  descriptor failure state:
    all zero because descriptor capacity no longer fails.

  source-block failure state:
    active_max = 512
    registered_max = 512
    ref_nonzero_max = 512
    ref_zero_max = 0

larger_sizes:
  route4k still descriptor-fails with active_max = 512.
  front1k-desc4k-source512-route4k still source-block-fails with
  active/registered/ref_nonzero = 512.

Read:
  The failure is real lifetime retention, not just a poor free-slot search:
    descriptors are full of live or locally cached objects.
    source blocks are all active, route-registered, and still referenced.

  For route4k, local_free_max and frontcache_total_max match. The allocator has
  plenty of cached objects, but they are concentrated in a few size classes.
  Current-class misses then fall through to source allocation and descriptor
  lookup even though other classes are hoarding descriptors/source-block refs.

  The dry-run confirms the route4k failure is often class-imbalance, not global
  object scarcity: the requested class is almost always empty, while other
  bins have spill candidates on a large minority of descriptor failures.

  Next behavior should avoid fixed capacity growth and instead reduce lifetime
  retention:
    class-aware frontcache spill or rebalance before new source blocks
    source-block slot reuse or partial-run reuse
    descriptor recycling only when route/fail-closed ownership remains valid

  A blind descriptor free-list alone is not enough because the route4k failure
  snapshot shows the table is full of ptr-backed descriptors, not missed null
  descriptors.
```

HZ6 spill-route4k behavior check:

```text
Sources:
  docs/benchmarks/windows/paper/20260601_102900_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_102927_hz6_capacity_matrix_windows.md

Implementation:
  added spill-route4k as an explicit experiment lane.
  route4k remains unchanged.
  behavior:
    on descriptor exhaustion, pop one cached object from the largest
    non-requested class bin, release its descriptor/source ownership, and retry
    descriptor allocation once.

Diagnostic run:
  route4k:
    2.47M ops/s, 17.3 MB
  spill-route4k:
    1.06M ops/s, 27.3 MB
    spill_success = 26.5K
    retry_success = 26.5K
    source_alloc = 11.4K versus route4k 1.0K

Non-diagnostic run:
  route4k:
    3.29M ops/s, 17.3 MB
  spill-route4k:
    1.32M ops/s, 27.3 MB

Decision:
  spill-route4k is no-go/control evidence.

Read:
  The dry-run hypothesis was real, but destructive donor spill is the wrong
  behavior. It frees descriptors, but increases source allocation,
  route unregister/register churn, and RSS. The issue is not just descriptor
  scarcity; it is retaining the wrong reusable objects in a way that preserves
  their source/run locality.

Next:
  do not promote spill-route4k.
  next experiment should be non-destructive reuse/rebalance:
    reuse donor source/run capacity without releasing and reallocating
    or add class/run-level partial reuse that can serve nearby classes
```

HZ6 borrow-route4k behavior check:

```text
Sources:
  docs/benchmarks/windows/paper/20260601_103209_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_103410_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_103437_hz6_capacity_matrix_windows.md

Dry-run:
  same-front larger cached-object candidates exist:
    borrow_dryrun_candidate_calls = 213K
    borrow_dryrun_candidate_total = 414K

Behavior:
  added borrow-route4k as an explicit experiment lane.
  route4k remains unchanged.
  behavior:
    after exact-class cache miss, borrow a larger same-front cached object
    instead of allocating a new object.

Diagnostic run:
  route4k:
    2.53M ops/s, 17.3 MB
  borrow-route4k:
    0.52M ops/s, 19.9 MB
    borrow_success = 29.9K

Non-diagnostic run:
  route4k:
    3.37M ops/s, 17.3 MB
  borrow-route4k:
    0.51M ops/s, 19.9 MB

Decision:
  borrow-route4k is no-go/control evidence.

Read:
  Reusing larger cached objects is non-destructive, but it still damages the
  mixed_ws route4k shape. It reduces some source-block exhaustion, but it
  increases descriptor exhaustion and collapses throughput. The likely issue is
  class-locality and repeated borrow scanning, not just object availability.

Next:
  stop one-object spill/borrow knobs.
  move to a cleaner front-cache policy:
    adaptive per-class cache caps
    source/run-level reuse accounting
    prefill allocation that avoids overfilling a few bins in the first place
```

HZ6 cap-route4k behavior check:

```text
Source:
  docs/benchmarks/windows/paper/20260601_104024_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_104157_hz6_capacity_matrix_windows.md

Dry-run:
  soft cap = frontcache bin capacity / 8, minimum 4.
  route4k / balanced:
    cap_dryrun_push = 251K
    over_cap / would_release = 33K
    bin_count_max = 256
  route4k / larger_sizes:
    cap_dryrun_push = 81K
    over_cap / would_release = 32.6K
    bin_count_max = 256

Behavior:
  added cap-route4k as an explicit experiment lane.
  route4k remains unchanged.
  behavior:
    when a class bin is at soft cap, release the returned object instead of
    keeping it in frontcache.

Diagnostic run:
  balanced:
    route4k 2.53M ops/s, 17.3 MB
    cap-route4k 0.44M ops/s, 36.6 MB
    cap_release = 42.7K

  larger_sizes:
    route4k 0.82M ops/s, 14.6 MB
    cap-route4k 0.67M ops/s, 17.2 MB
    cap_release = 44.1K

Decision:
  cap-route4k is no-go/control evidence.

Read:
  Per-class over-retention is real, but releasing over-cap objects directly
  causes source allocation and route register/unregister churn. This confirms
  the next fix must be at source/run lifecycle or prefill placement, not
  frontcache eviction after the fact.

Next:
  stop single-object frontcache knobs:
    spill-route4k
    borrow-route4k
    cap-route4k

  Next promising direction:
    source/run-local free-slot reuse and prefill placement, so bins do not
    over-retain source refs and descriptor ownership in the first place.
```

HZ6 source/run reuse dry-run:

```text
Source:
  docs/benchmarks/windows/paper/20260601_104527_hz6_capacity_matrix_windows.md

Implementation:
  added diagnostic-only source/run reuse projection.
  Before creating a new source block/run, count active source blocks with:
    same source kind
    same block size
    ref_count < slots_per_block

  No behavior change. Existing speed lanes remain unchanged unless
  -DiagnosticHz6Probes is requested.

mixed_ws / balanced / route4k:
  source_run_reuse_dryrun_calls = 1.51M
  candidate_calls = 1.51M
  candidate_blocks_total = 193.0M
  free_slots_total = 15.0B
  largest_free_slots_max = 4094

mixed_ws / balanced / front1k-desc4k-source512-route4k:
  source_run_reuse_dryrun_calls = 64.0K
  candidate_calls = 64.0K
  candidate_blocks_total = 31.6M
  free_slots_total = 1.42B
  largest_free_slots_max = 4095

mixed_ws / larger_sizes / route4k:
  source_run_reuse_dryrun_calls = 459.6K
  candidate_calls = 459.6K
  candidate_blocks_total = 25.7M
  free_slots_total = 359.5M
  largest_free_slots_max = 255

Read:
  This is the strongest signal so far. HZ6 is repeatedly trying to create new
  source blocks even though existing source blocks appear to have ref-count
  slack.

  The current source-block model only has ref_count and does not have slot
  occupancy metadata. Therefore behavior cannot safely reuse a projected free
  slot yet.

Next:
  design source-run slot metadata before behavior:
    per-source-block slot bitmap or free index stack
    front/class ownership for each run
    route/fail-closed invariants for reused slots
    release only when all slots dead and route invalid range is safe

Design-consult trigger:
  This is enough evidence to ask for a source-run reuse design review before
  implementing behavior.
```

HZ6 SourceRunReuse-L1 implementation plan:

```text
Decision:
  proceed with SourceRunReuse-L1.

Shape:
  Store lightweight slot metadata on Hz6SourceBlock itself.
  Rationale:
    descriptor release currently only has the descriptor/source_block pair,
    so block-owned metadata keeps slot release rollback local and avoids
    threading allocator-side parallel arrays through every release path.

Metadata:
  class_id
  slot_bytes
  slot_count
  used_count
  next_hint
  used bitmap capped by HZ6_SOURCE_RUN_MAX_SLOTS

Note:
  Pro suggested free_stack as the easiest L1 shape. The Windows implementation
  uses a bitmap instead because appcap source-block capacities can be large;
  bitmap keeps per-block metadata bounded while preserving the same source-run
  reuse contract.

L1 scope:
  route4k mixed_ws first.
  same source_kind / block_bytes / slot_bytes / class_id only.
  no frontcache spill / borrow / cap composition.

Safety contract:
  invalid range owns the source block boundary.
  exact route owns live/cached slot validity.
  slot USED commit only after descriptor prepare and exact route register
  succeed.
  rollback returns the slot to free stack.
  source block release requires used_count == 0 and invalid range unregister.

First lane:
  sourcerun-route4k

First acceptance:
  mixed_ws route4k balanced improves at least 10%.
  source_alloc and source_block_exhausted decrease.
  route_miss / route_invalid / route_register_fail do not increase.
  compact/control rows do not regress more than 5%.

No-go:
  used_count mismatch.
  route safety counters increase.
  source_alloc or route churn increases like spill/borrow/cap.
```

HZ6 SourceRunReuse-L1 smoke result:

```text
Implementation:
  added sourcerun-route4k lane with HZ6_SOURCE_RUN_REUSE_L1=1.
  source-run metadata is block-owned bitmap metadata, not production default.
  route4k remains unchanged.

Validation:
  HZ6 Windows R1 smoke suite passes.
  Results:
    results/hz6-sourcerun-probe/20260601_110611_hz6_capacity_matrix_windows.md
    results/hz6-sourcerun-probe/20260601_110745_hz6_capacity_matrix_windows.md

mixed_ws / balanced / speed:
  route4k:
    0.517M ops/s, 17.4 MB
  sourcerun-route4k:
    0.511M ops/s, 17.4 MB
    source_alloc 166
    source_run_reuse_attempt 1.51M
    source_run_reuse_candidate 409K
    source_run_reuse_hit 1.45K
    source_run_reuse_reserved 1.92M
    source_run_reuse_slot_fail 1.92M
    source_run_reuse_miss_no_slot 0
    source_run_reuse_used_count_mismatch 0

mixed_ws / larger_sizes:
  strict route4k:
    0.782M ops/s, 15.5 MB
  strict sourcerun-route4k:
    0.804M ops/s, 14.3 MB
  speed route4k:
    0.791M ops/s, 14.5 MB
  speed sourcerun-route4k:
    0.797M ops/s, 13.5 MB
  rss route4k:
    0.794M ops/s, 14.8 MB
  rss sourcerun-route4k:
    0.800M ops/s, 13.7 MB

Read:
  SourceRunReuse-L1 is safe enough as an evidence lane:
    route_miss = 0
    route_invalid = 0
    route_register_fail = 0
    source_block_exhausted = 0
    used_count_mismatch = 0

  It does not solve balanced route4k:
    source-run candidates exist, but most reserved slots fail at
    descriptor/route activation time.
    The next dominant pressure is still descriptor/frontcache lifecycle,
    not physical source-block slot availability.

Decision:
  keep sourcerun-route4k as evidence/control.
  do not promote SourceRunReuse-L1 yet.
  next likely ROI is descriptor lifecycle / frontcache-visible descriptor
  reuse, not more source-run placement.
```

HZ6 SourceRunDescriptorReclaim-L1 check:

```text
Implementation:
  added sourcerun-reclaim-route4k as an explicit experiment lane.
  It only reclaims a donor frontcache descriptor when source-run reuse has a
  candidate block but no free descriptor.
  route4k and sourcerun-route4k remain unchanged.

Validation:
  HZ6 Windows R1 smoke suite passes.
  Results:
    results/hz6-sourcerun-probe/20260601_113631_hz6_capacity_matrix_windows.md

mixed_ws / balanced / speed:
  route4k:
    0.530M ops/s, 17.6 MB
  sourcerun-route4k:
    0.494M ops/s, 17.5 MB
  sourcerun-reclaim-route4k:
    0.420M ops/s, 19.9 MB
    source_run_reuse_hit 65.5K
    descriptor_reclaim_success 64.5K
    route_register_probe_total 154.1M
    route_unregister_probe_total 3.91M

mixed_ws / larger_sizes:
  speed route4k:
    0.748M ops/s, 14.6 MB
  speed sourcerun-route4k:
    0.783M ops/s, 13.6 MB
  speed sourcerun-reclaim-route4k:
    0.746M ops/s, 14.4 MB

Read:
  Reclaiming donor descriptors proves the descriptor bottleneck, but it
  converts the bottleneck into route churn.
  The route register/unregister cost dominates and RSS does not improve.

Decision:
  sourcerun-reclaim-route4k is no-go/control evidence.
  do not promote descriptor reclaim.
  next ROI should avoid donor route unregister churn.
  Candidate directions:
    descriptor recycle within same class/source-run only
    or descriptor accounting/lifecycle redesign
    not cross-class donor reclaim.
```

HZ6 SourceRunSameClassReclaim-L1 check:

```text
Implementation:
  added sourcerun-sameclass-route4k as a narrower reclaim lane.
  It only reclaims from the same class frontcache bin, and only when the bin
  has more than one entry.
  donor cross-class reclaim remains off in this lane.

Validation:
  Windows HZ6 capacity matrix run:
    docs/benchmarks/windows/paper/20260601_120116_hz6_capacity_matrix_windows.md
  Comparison route4k run:
    docs/benchmarks/windows/paper/20260601_120346_hz6_capacity_matrix_windows.md

mixed_ws / balanced:
  route4k:
    0.511M ops/s, 24.2 MB
  sourcerun-sameclass-route4k:
    0.539M ops/s, 24.2 MB
    source_run_reuse_same_class_reclaim_success 73

mixed_ws / wide_ws:
  route4k:
    0.397M ops/s, 24.9 MB
  sourcerun-sameclass-route4k:
    0.409M ops/s, 24.9 MB
    source_run_reuse_same_class_reclaim_success 274

mixed_ws / larger_sizes:
  route4k:
    0.777M ops/s, 15.6 MB
  sourcerun-sameclass-route4k:
    0.796M ops/s, 14.4 MB

Read:
  Same-class reclaim is a much safer evidence lane than donor reclaim.
  It preserves route safety and gives a small throughput lift on balanced and
  wide_ws, but the reclaimed-descriptor success count is still tiny relative to
  the number of attempts.
  The main bottleneck still looks like descriptor/frontcache lifecycle pressure,
  not cross-class reuse.

Decision:
  keep same-class reclaim as evidence/control.
  do not promote it yet.
  next ROI remains descriptor lifecycle / frontcache-visible descriptor reuse.
```

HZ6 DescriptorFrontcacheReuse dry-run:

```text
Implementation:
  added diagnostic-only descriptor/frontcache reuse projection.
  On final descriptor allocation failure, count whether the requested class
  already has cached descriptors and how many descriptors are held by donor
  frontcache bins.

Validation:
  diagnostic mixed_ws route4k matrix:
    docs/benchmarks/windows/paper/20260601_121841_hz6_capacity_matrix_windows.md

mixed_ws / balanced / route4k:
  descriptor_frontcache_reuse_dryrun_calls = 4.53M
  requested_nonempty = 7
  requested_total = 39
  donor_total = 6.89M
  largest_donor_max = 239

mixed_ws / wide_ws / route4k:
  descriptor_frontcache_reuse_dryrun_calls = 4.51M
  requested_nonempty = 50
  donor_total = 16.1M

mixed_ws / larger_sizes / route4k:
  descriptor_frontcache_reuse_dryrun_calls = 1.14M
  requested_nonempty = 61
  donor_total = 6.08M

Read:
  Same-class cached descriptor reuse is not enough. Descriptor failures usually
  happen when the requested class is empty while other classes hold many
  cached descriptors.

  Cross-class destructive donor reclaim already regressed via route churn, so
  the next promising shape is not another spill/reclaim knob. The allocator
  needs cached physical slots to stop pinning descriptor capacity, or it needs
  a descriptor materialization reserve/admission rule.
```

HZ6 DescriptorlessFrontcache-L1 check:

```text
Implementation:
  added descriptorless-route4k as a source-run metadata prototype lane.
  Source-run cached frontcache entries may detach their descriptor and exact
  route while keeping the physical source slot retained. Reuse materializes a
  fresh descriptor and exact route before returning the object.

  Guard:
    descriptorless detach is restricted to source-run blocks whose class_id and
    slot_bytes match the frontcache entry. Non-source-run cached entries keep
    the normal descriptor-backed path.

Validation:
  HZ6 Windows capacity build passes for route4k and descriptorless-route4k.
  Latest diagnostic matrix:
    docs/benchmarks/windows/paper/20260601_122758_hz6_capacity_matrix_windows.md

mixed_ws / balanced:
  route4k:
    0.443M ops/s, 24.3 MB
  descriptorless-route4k:
    0.382M ops/s, 24.4 MB
    descriptorless_push = 3.3K
    descriptorless_pop = 3.0K
    descriptorless_descriptor_fail = 2.16M
    descriptorless_invalid = 0

mixed_ws / wide_ws:
  route4k:
    0.392M ops/s, 24.9 MB
  descriptorless-route4k:
    0.415M ops/s, 25.0 MB
    descriptorless_push = 14.0K
    descriptorless_pop = 12.7K
    descriptorless_descriptor_fail = 3.84M
    descriptorless_invalid = 0

mixed_ws / larger_sizes:
  route4k:
    0.701M ops/s, 15.6 MB
  descriptorless-route4k:
    0.653M ops/s, 14.6 MB
    descriptorless_push = 13.8K
    descriptorless_pop = 13.5K
    descriptorless_descriptor_fail = 499K
    descriptorless_invalid = 0

Read:
  The descriptorless source-run contract is viable as safety evidence:
    route_miss = 0
    route_invalid = 0
    route_register_fail = 0
    descriptorless_invalid = 0

  It is not a promotion lane. Descriptor materialization often happens while
  the descriptor table is still full, so descriptor_exhausted increases and
  balanced/larger_sizes regress. Wide_ws shows a small positive signal, but it
  is not enough to offset the descriptor_fail pressure.

Decision:
  keep descriptorless-route4k as evidence/control.
  do not promote it.
  next likely ROI:
    descriptor materialization reserve
    or admission that only pops descriptorless entries when a descriptor can be
    acquired without full-table failure loops.

  Do not mix this with spill/borrow/cap or cross-class donor reclaim.
```

HZ6 DescriptorMaterializationReserve-L1 check:

```text
Implementation:
  added descriptorreserve-route4k as a descriptorless extension lane.
  A descriptorless cached source-run slot can reserve its detached descriptor
  for later materialization. The reserved descriptor is skipped by normal
  descriptor allocation and reused only when that cached physical slot is
  popped.

Validation:
  HZ6 Windows capacity build passes for:
    route4k
    descriptorless-route4k
    descriptorreserve-route4k

  Diagnostic matrix:
    docs/benchmarks/windows/paper/20260601_124121_hz6_capacity_matrix_windows.md

mixed_ws / balanced:
  route4k:
    0.362M ops/s, 24.2 MB
  descriptorless-route4k:
    0.436M ops/s, 24.3 MB
    descriptorless_descriptor_fail = 2.16M
  descriptorreserve-route4k:
    0.357M ops/s, 24.6 MB
    descriptorless_descriptor_fail = 0
    descriptorreserve_push/pop = 1.5K / 1.5K

mixed_ws / wide_ws:
  route4k:
    0.304M ops/s, 24.8 MB
  descriptorless-route4k:
    0.286M ops/s, 24.8 MB
    descriptorless_descriptor_fail = 3.84M
  descriptorreserve-route4k:
    0.282M ops/s, 24.8 MB
    descriptorless_descriptor_fail = 0
    descriptorreserve_push/pop = 12.7K / 11.9K

mixed_ws / larger_sizes:
  route4k:
    0.719M ops/s, 15.5 MB
  descriptorless-route4k:
    0.668M ops/s, 14.5 MB
    descriptorless_descriptor_fail = 499K
  descriptorreserve-route4k:
    0.717M ops/s, 14.3 MB
    descriptorless_descriptor_fail = 0
    descriptorreserve_push/pop = 14.3K / 14.1K

Read:
  The reserve mechanism works mechanically:
    descriptorless materialization failures drop to 0
    route_miss = 0
    route_invalid = 0
    route_register_fail = 0
    descriptorreserve_missing = 0

  It is not a promotion lane. Reserving a descriptor for a cached physical slot
  removes the materialization failure loop, but it also withholds descriptors
  from normal allocation. Balanced and wide_ws do not improve, while
  larger_sizes only returns near route4k with slightly lower RSS.

Decision:
  keep descriptorreserve-route4k as evidence/control.
  do not promote it.

  The useful lesson is that the next design must be more selective than
  per-slot descriptor reservation:
    descriptorless only for cold / over-retained bins
    materialization admission based on class pressure
    or a separate descriptor credit policy that does not starve normal
    allocation.
```

HZ6 DescriptorCold-L1 check:

```text
Implementation:
  added descriptorcold-route4k as a selective descriptorless lane.
  It keeps descriptorless source-run slots, but only detaches descriptors from
  frontcache bins that are already at the same soft cap used by cap-route4k's
  dry-run. It does not reserve descriptors.

Validation:
  diagnostic matrix:
    docs/benchmarks/windows/paper/20260601_124434_hz6_capacity_matrix_windows.md

mixed_ws / balanced:
  route4k:
    0.384M ops/s, 24.2 MB
  descriptorless-route4k:
    0.398M ops/s, 24.3 MB
    descriptorless_descriptor_fail = 2.16M
  descriptorcold-route4k:
    0.407M ops/s, 24.6 MB
    descriptorless_push/pop = 107 / 89
    descriptorless_descriptor_fail = 232K

mixed_ws / wide_ws:
  route4k:
    0.322M ops/s, 24.8 MB
  descriptorcold-route4k:
    0.326M ops/s, 24.8 MB
    descriptorless_push/pop = 2.5K / 2.0K
    descriptorless_descriptor_fail = 0

mixed_ws / larger_sizes:
  route4k:
    0.777M ops/s, 15.5 MB
  descriptorcold-route4k:
    0.635M ops/s, 14.5 MB
    descriptorless_push/pop = 4.5K / 4.1K
    descriptorless_descriptor_fail = 181K

Read:
  Selective descriptorless is a better shape than broad descriptorless for
  balanced / wide_ws. It sharply reduces the descriptorless materialization
  failure loop and gives a small throughput signal.

  It is still not a promotion lane. The simple over-soft-cap gate hurts
  larger_sizes and does not materially reduce alloc_fail. The pressure is
  workload/class dependent, so one global soft-cap rule is too blunt.

Decision:
  keep descriptorcold-route4k as evidence/control.
  do not promote it.

  Next useful design is not more one-bit descriptorless gating. It should
  separate front/class policy:
    allow descriptorless only for small/mixed classes
    or drive descriptorless by descriptor-failure attribution per class
    or introduce an epoch/budgeted class pressure governor.
```

HZ6 DescriptorColdGovernor-L1 plan:

```text
Decision:
  descriptorless knobs are frozen as evidence/control.
  Do not promote:
    descriptorless-route4k
    descriptorreserve-route4k
    descriptorcold-route4k

  Proceed with one narrow governor experiment:
    descriptorcoldgov-route4k

Interpretation shift:
  descriptorless cached slots are not a hot reuse path.
  They are cold-cache descriptor compression for over-retained source-run
  physical slots.

Required L1 properties:
  failure-attributed detach:
    detach only when descriptor pressure identifies a requested class and a
    donor class/bin is over-retained.

  class-aware:
    avoid larger_sizes-sensitive classes in the first lane.
    start with small/mixed-heavy classes only.

  budgeted:
    limit descriptorless detach per class and globally.
    no unbounded detached backlog.

  materialization admission:
    do not reserve descriptors.
    do not pop descriptorless entries while descriptor pressure is high.
    if no descriptor can be acquired cheaply, leave detached slots cold.

First implementation order:
  1. Add diagnostic class attribution counters:
       descgov_requested_class[16]
       descgov_donor_class[16]
       descgov_materialize_fail_class[16]

  2. Add materialization admission:
       descriptorless pop only when descriptor availability/probe budget is
       acceptable.

  3. Add failure-attributed detach gate:
       requested class empty/failed and donor bin over-retained.

  4. Add class mask and budget:
       larger_sizes-sensitive classes off by default.

Acceptance:
  experimental:
    route_invalid = 0
    route_miss = 0
    route_register_fail = 0
    descriptorless_invalid = 0
    balanced >= route4k + 3%
    wide_ws >= route4k + 3%
    larger_sizes >= route4k - 3%
    descriptorless_descriptor_fail <= broad descriptorless / 10
    RSS <= route4k + 1 MiB

  promotion:
    balanced / wide_ws >= route4k + 5%
    larger_sizes >= route4k - 3%
    descriptor probe pressure improves vs route4k
    repeat-3 stable

No-go:
  any route safety counter nonzero
  larger_sizes regression > 5%
  descriptor_exhausted > route4k
  descriptor_probe_total > route4k + 5%
  route register/unregister probe per op > route4k + 10%
  detached_current grows across epochs

If descriptorcoldgov-route4k is no-go:
  stop descriptorless knob track.
  return to core descriptor pool / cached physical slot contract redesign.
```

HZ6 DescriptorColdGovernor-L1 result:

```text
Implementation:
  added descriptorcoldgov-route4k.
  It composes descriptorless source-run cached slots with:
    small/mixed class allow rule: bytes <= 2048
    detached slot budget
    materialization admission without descriptor reserve
    diagnostic class/pressure counters

Validation:
  diagnostic RUNS=1:
    docs/benchmarks/windows/paper/20260601_130901_hz6_capacity_matrix_windows.md

  non-diagnostic repeat-3:
    docs/benchmarks/windows/paper/20260601_130953_hz6_capacity_matrix_windows.md

Diagnostic read:
  balanced:
    descriptorless_push/pop = 29 / 26
    descriptorless_descriptor_fail = 0
    descgov_trigger_descriptor_fail = 4.78M
    descgov_detach_success = 29
    descgov_materialize_admit = 26
    descgov_materialize_fail = 0

  wide_ws:
    descriptorless_push/pop = 2048 / 1908
    descriptorless_descriptor_fail = 0
    descgov_detach_budget_denied = 1313
    descgov_materialize_fail = 0

  larger_sizes:
    descriptorless_push/pop = 497 / 458
    descriptorless_descriptor_fail = 0
    descgov_detach_class_denied = 18079
    descgov_materialize_block_no_descriptor = 14100
    descgov_materialize_fail = 0

repeat-3 non-diagnostic medians:
  balanced:
    route4k = 0.446M ops/s, 24.2 MB
    descriptorcoldgov-route4k = 0.469M ops/s, 24.7 MB
    read: about +5%, reaches the promotion-line signal for this row.

  wide_ws:
    route4k = 0.402M ops/s, 24.8 MB
    descriptorcoldgov-route4k = 0.407M ops/s, 24.8 MB
    read: only a small positive signal.

  larger_sizes:
    route4k = 0.784M ops/s, 15.5 MB
    descriptorcoldgov-route4k = 0.766M ops/s, 14.5 MB
    read: within -3% while lowering RSS, so the larger_sizes guard is held.

Safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  descriptorless_invalid = 0
  descgov_materialize_fail = 0

Read:
  This is the first descriptorless-family lane that looks structurally sane:
    broad materialization failure loop is gone
    reserve starvation is avoided
    larger_sizes is protected
    balanced has a real positive signal

  It is not default promotion yet:
    wide_ws improvement is weak
    alloc_fail remains high
    descriptor_exhausted can still exceed route4k
    detached budget/current accounting still needs repeat and guard review

Decision:
  keep descriptorcoldgov-route4k as the current HZ6 descriptor lifecycle
  candidate-control evidence lane.

  Next:
    repeat-5 or add one more guard slice if we want confidence.
    Then either:
      refine class/budget policy for wide_ws
      or stop and ask for design review before adding another knob.

HZ6 DescriptorColdGovernorWideWs-L1 result:
  added descriptorcoldgov-widews-route4k with a larger detach budget
  targeted at wide_ws only.

  repeat-3 non-diagnostic medians:
    balanced:
      route4k = 0.424M ops/s, 24.2 MB
      descgov-route4k = 0.420M ops/s, 24.2 MB
      widews = 0.413M ops/s, 24.2 MB
      read: balanced stays slightly below route4k, so this remains a narrow
      evidence lane rather than a broad promotion.

    wide_ws:
      route4k = 0.336M ops/s, 24.8 MB
      descgov-route4k = 0.344M ops/s, 24.8 MB
      widews = 0.395M ops/s, 24.8 MB
      read: wide_ws recovers strongly and is the main reason to keep the
      budget-expanded variant alive.

    larger_sizes:
      route4k = 0.787M ops/s, 15.5 MB
      descgov-route4k = 0.770M ops/s, 14.5 MB
      widews = 0.785M ops/s, 14.5 MB
      read: larger_sizes stays protected and RSS remains lower than route4k.

  decision:
    keep descriptorcoldgov-widews-route4k as a narrow wide_ws recovery lane.
    do not promote it as a new default yet because balanced is still a little
    below route4k and we have not checked the repeat-5 confidence line on this
    variant.
```

## HZ6 Windows Current Read

```text
Latest app-like matrix:
  results/windows-benchmark-suite/20260528_115721/app-like

Default HZ6 rows:
  hz6-strict / hz6-speed / hz6-rss
  small R1 capacities
  keep as smoke/control rows

Broad HZ6 rows:
  hz6-strict-broad / hz6-speed-broad / hz6-rss-broad
  same profiles with larger fixed capacities for broad working-set matrix rows
  use for large_slices and same-runner broad comparison

Appcap HZ6 rows:
  hz6-strict-appcap / hz6-speed-appcap / hz6-rss-appcap
  app-like fixed capacity rows for Larson/live-set separation

Capacity finding:
  the earlier 8K..64K Windows weakness was dominated by alloc_fail from
  descriptor/route/source capacity exhaustion.

Route finding:
  exact-route hash probing before fail-closed scan fallback restores much of
  the mid/large working-set performance while preserving invalid/interior
  pointer scan checks.
```

App-like repeat-3 read:

```text
Larson:
  HZ6 default and broad rows fail during warmup.
  HZ6 stats show descriptor exhaustion at default 64 and broad 4096.
  appcap rows run at T=1 with no alloc_fail, but throughput remains far below
  HZ3/tcmalloc/mimalloc, so the remaining issue is lifecycle/front-source
  behavior rather than raw live-set capacity.

Random mixed:
  small and medium/mixed rows still fail on the strict / speed / rss controls.
  broad and appcap rows survive, but they remain far behind HZ3/HZ4/mimalloc/
  tcmalloc on throughput.

MT remote:
  HZ6 rows are extremely slow under the legacy remote runner.
  Treat as remote-contract mismatch until an HZ6 owner/remote-aware runner is
  available.

Redis workload:
  HZ6 default rows are strong on SET / GET / LPUSH.
  HZ6 broad rows are weak, so broad-cap is not a Redis default.
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

Larson lifecycle/front-source diagnostic refresh:

```text
Source:
  docs/benchmarks/windows/paper/20260601_140855_paper_larson_windows.md

Stress T=16:
  hz6-strict / speed / rss / route4k / broad all fail during warmup.
  failures happen at tiny source_alloc counts, with no route visibility or
  transfer traffic yet, which keeps pointing at lifecycle/front-source rather
  than raw live-set capacity.

  appcap rows do run, but they stay tiny:
    strict-appcap 0.003M
    speed-appcap  0.001M
    rss-appcap    0.001M
  and remain far below HZ3/HZ4/mimalloc/tcmalloc.

Worker-warmup T=16:
  hz6-strict-appcap 37.475M
  hz6-speed-appcap  47.763M
  hz6-rss-appcap    41.689M
  same-owner control is viable, but still below HZ3/HZ4/mimalloc/tcmalloc.

Read:
  same-owner small-object placement is okay.
  the remaining weakness is the main-warmup / lifecycle / front-source path.
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

Post-route-hash large-slice signal on Windows:

```text
8K:
  hz6-strict-broad ~54.7M ops/s, near tcmalloc ~57.1M

16K / 32K / 64K:
  hz6-strict-broad beats or roughly matches tcmalloc in the latest local run

128K:
  hz6-speed-broad ~69.6M ops/s, above tcmalloc ~46.7M
```

Next HZ6 rule:

```text
Do:
  keep fixed broad-cap rows for reproducible broad comparison
  keep small-cap rows as controls
  inspect alloc_fail before interpreting a row
  expose HZ6 stats in app-like failure rows before adding new knobs

Do not:
  call hz6-*-broad the production/default lane yet
  hide small-cap controls
  add adaptive capacity to comparison rows before a dry-run/growth policy exists

Next order:
  1. Keep the HZ6 standalone local/remote/reuse runner as the first contract
     baseline.
  2. Compare HZ6 against HZ3 / HZ4 / HZ5 on the same machine and same runner.
  3. Keep HZ6 appcap rows as Larson capacity-separation evidence.
  4. Keep the HZ6 owner/remote-aware runner for MT remote as the contract-
     matched pressure lane.
  5. Larson lifecycle/front-source diagnostic.
  6. Redis default-row mechanism readout.
  7. hz6-adaptive-cap-dryrun only after the above evidence is clean.

Research candidate:
  hz6-adaptive-cap-dryrun
  then hz6-adaptive-cap only if growth counters justify it
```

Latest capacity-breakdown read:

```text
results/windows-hz6-capacity-breakdown/20260528_105215_allocator_matrix.md

8K default HZ6:
  route_register_fail dominates alloc_fail.

64K default HZ6:
  descriptor_exhausted dominates alloc_fail.

Broad-cap rows:
  clear all three tracked exhaustion counters in the checked 8K/64K slices.
```

Long historical task notes were archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```

## Current Claim

```text
MidPage:
  PageRun64 remains the strong keep for general malloc main/mid/cross64 rows.

LargeFront:
  128K remote-heavy rows are the active tcmalloc chase area.
  No single LargeFront diagnostic is broad enough to replace saved profiles.

Documentation:
  keep current_task short.
  put long result logs in docs/archive or dedicated result docs.

HZ6:
  keep separate from HZ5 implementation.
  design route-safe / transfer-first / RSS-aware contracts before writing code.
```

## HZ6 Design Seed

HZ6 docs now live under:

```text
hakozuna-hz6/
```

Read in this order:

```text
hakozuna-hz6/README.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
hakozuna-hz6/docs/current_task.md
```

Current HZ6 position:

```text
HZ3 lesson:
  thin local/TLS cache

HZ4 lesson:
  remote grouping and page/span reuse

HZ5 lesson:
  fail-closed descriptor ownership and low-RSS profile discipline

HZ6 plan:
  make RouteLayer, FrontCache, TransferLayer, SourceLayer, ScavengeLayer,
  OwnerLayer, and PolicyLayer explicit from the first implementation.
```

## Current LargeFront Read

Saved / comparison lanes:

```text
large128-rss:
  source batch4, low-RSS fixed profile.

large128-source16:
  source batch16, current best r90/t8 comparison lane.
```

Diagnostics:

```text
large128-r50-hold8:
  wins t8/r50 in one focused run.
  loses source16 badly on r90.
  keep as r50 diagnostic only.

large128-direct-header:
  unsafe ptr-4096 header lookup upper bound.
  improves t4/r50 and wins t8/r90 in one run.
  not promotable because it is not fail-closed for foreign pointers.

large128-base-directmap:
  safe exact-base one-slot route cache.
  improves t4/r90 versus source16.
  loses source16 on t8 rows.
  no-promote diagnostic.

large128-r50-drain-directmap:
  combines drain1 and base-directmap.
  current RUNS=3 no-go: it does not beat either parent and r90 RSS/ops regress.

large128-ownerfast:
  enables LargeFront same-owner load/store state transition.
  current RUNS=3 no-go: t4/t8 r50 and t8/r90 regress versus source16.

large128-drainbulk:
  source16 plus bulk local-list commit during owner remote drain.
  diagnostic only: t4 rows show signal, but t8 rows regress versus source16.
  useful evidence that local_push overhead is only a slice of the remaining
  drain/refill gap.

large128-draintrust:
  source16 plus trusted load/store for owner-drained
  REMOTE_PENDING->LOCAL_FREE transitions.
  diagnostic only: t8/r50 beats tcmalloc in RUNS=3, but t8/r90 loses source16.
  useful evidence that drain state CAS is a real slice, but not the broad fix.
```

Recent result roots:

```text
private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802
private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345
private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731
private/raw-results/linux/hz5_large128_current_focus_r3_20260526_052851
private/raw-results/linux/hz5_large128_drain_directmap_r3_20260526_053047
private/raw-results/linux/hz5_large128_t4r50_perf_current_20260526_053300
private/raw-results/linux/hz5_large128_rb_current_r3_20260526_053400
private/raw-results/linux/hz5_large128_batch32_smoke_20260526_053458
private/raw-results/linux/hz5_large128_base_directmap4_r3_20260526_053547
private/raw-results/linux/hz5_large128_ownerfast_r3_20260526_053858
private/raw-results/linux/hz5_large128_direct_header_recheck_r3_20260526_055156
private/raw-results/linux/hz5_large128_source_populate_r3_20260526_055548
private/raw-results/linux/hz5_large128_l0_compare_20260526_060606
private/raw-results/linux/hz5_large128_drainbulk_r3_20260526_061000
private/raw-results/linux/hz5_large128_drainbulk_tail_r3_20260526_061107
private/raw-results/linux/hz5_large128_draintrust_r3_20260526_061614
private/raw-results/linux/hz5_large128_draintrustbulk_manual_r3_20260526_061931
private/raw-results/linux/hz5_large128_draintrust_budget1_manual_r3_20260526_062121
private/raw-results/linux/hz5_large128_source16_draintrust_l0_compare_20260526_062558
private/raw-results/linux/hz5_large128_source16_draintrust_perf_20260526_062616
private/raw-results/linux/hz5_large128_source16_draintrust_perf_t4_20260526_062634
private/raw-results/linux/hz5_large128_source16_draintrust_median_r3_20260526_062644
private/raw-results/linux/hz5_large128_transfer128_smoke_r3_20260526_063953
private/raw-results/linux/hz5_large128_transfer128_flushmiss_r3_20260526_064056
private/raw-results/linux/hz5_large128_transfer128_tlsfirst_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_ownershard_r3
private/raw-results/linux/hz5_large128_transfer128_shard16_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_shard16_mask_smoke_r1
```

## Next Engineering Direction

```text
1. Keep source16 and large128-rss as the current comparison pair.
2. Treat hold8/base-directmap/direct-header/drain-directmap as diagnostics.
3. Current t4/r50 perf gap is instruction/branch count plus page-fault/refill
   pressure, not cache-miss rate alone.
4. rb32/rb64, batch32, and ownerfast do not fix t4/r50; keep them
   diagnostic/no-go.
5. Direct-header recheck does not improve t4/r50; free lookup alone is not
   the next primary fix.
6. MAP_POPULATE source refill is a hard no-go; page-fault prepopulation
   explodes RSS and throughput collapses.
7. L0 confirms r50-drain republish churn is huge; it is not a clean t4/r50
   fix. Source16 has no republish but has many REMOTE_PENDING->LOCAL_FREE
   conversions.
8. Drainbulk reduces part of that conversion overhead and can improve t4, but
   t8 regressions mean local-push batching is not the broad fix.
9. Draintrust shows the drain CAS cost matters. The latest RUNS=3 recheck is
   split: t8/r90 wins strongly with low RSS, but t8/r50 loses source16 and
   t4 rows remain far behind tcmalloc.
10. Draintrust+drainbulk composition is no-go in manual RUNS=3.
11. Draintrust+budget1 is a t4/r50-only local optimum: 29.56M on manual RUNS=3,
   but t4/r90 and t8 rows collapse. Do not add it as a named lane.
12. L0 source16/draintrust recheck shows draintrust increases source refill
    pressure on some rows. Example: t4/r90 source16 source_refill=392 versus
    draintrust source_refill=1143 in the L0 run. Trusted drain can reduce a
    state-transition slice, but can also perturb reuse/refill timing.
13. Perf recheck says instruction count is not always the only t8 limiter:
    at t8/r50 HZ5 and tcmalloc are close on aggregate instructions/op, while
    elapsed still differs. Treat parallel efficiency, owner-drain timing, and
    source/refill pressure as co-equal suspects.
14. Transfer128 class cache is a useful structural diagnostic: it improves
    t4/r50 with very low RSS, showing class-level transfer can cut owner-inbox
    cost. It is not broad yet; t8 rows regress, likely from global transfer
    contention or poor high-thread consumption order.
15. Transfer128-tlsfirst is no-go: producer-local retention starves consumer
    reuse and worsens RSS.
16. Transfer128-ownershard is no-go: routing by old owner slot preserves
    neither transfer128's t4/r50 signal nor source16's r90 behavior.
17. Transfer128-shard16 is no-go even after adding a nonempty mask to avoid
    empty-shard locks. Consumer-visible sharding loses the global transfer128
    t4/r50 win and does not recover t8.
18. Do not add another policy until a concrete hotspot explains the row split.
19. Keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters.
```

## Cleanup Status

Completed in this cleanup phase:

```text
HZ5_LINUX_PROFILE_MATRIX.md:
  shortened to current registry.
  history moved to docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md.

current_task.md:
  shortened to current state.
  history moved to docs/archive/current_task_2026-05-hz5-linux-post-largefront.md.

HZ5_LINUX_DEV_BRIEF.md:
  restored to a short entry point.
  history moved to docs/archive/HZ5_LINUX_DEV_BRIEF_HISTORY_2026-05.md.

HZ5_LINUX_ROUTE_LANE_MATRIX.md:
  shortened to current route/lane naming rules.
  history moved to docs/archive/HZ5_LINUX_ROUTE_LANE_MATRIX_HISTORY_2026-05.md.

HZ5_BENCH_RESULTS_INDEX.md:
  shortened to current active evidence index.
  history moved to docs/archive/HZ5_BENCH_RESULTS_INDEX_HISTORY_2026-05.md.

HZ5_P0_ALIGNED_RUN_8192.md:
  shortened to lifecycle index.
  history moved to docs/archive/HZ5_IMPLEMENTATION_LIFECYCLE_HISTORY_2026-05.md.

HZ5_LINUX_LOCAL2P_DESIGN.md:
  shortened to current Local2P profile boundary.
  history moved to docs/archive/HZ5_LINUX_LOCAL2P_DESIGN_HISTORY_2026-05.md.

HZ5_P43I_P43O_ALGO_CONSULT.md:
  shortened to current exact-route consultation summary.
  history moved to docs/archive/HZ5_P43I_P43O_ALGO_CONSULT_HISTORY_2026-05.md.

HZ5_SOURCE_CLEANUP_PLAN.md:
  added source-code split plan.
  active LargeFront/MidPageFront C files are intentionally not split until the
  current LargeFront tcmalloc-chase measurement direction stabilizes.

build_linux_hz5_standalone.sh:
  argument parser moved to linux/hz5_build_arg_parser.sh.

linux/hz5_build_arg_parser.sh:
  human-facing profile aliases moved to linux/hz5_build_profile_aliases.sh.
  parser is now a short dispatcher.
  low-level feature flags split into exact/midpage/front-end groups.

build_linux_hz5_standalone.sh:
  reduced below 1000 lines.
  long usage text, compound profile helpers, validation, and build-config
  output moved to focused linux/hz5_build_*.sh helpers.
```

Remaining cleanup candidates:

```text
hakozuna-hz5/largefront/hz5_largefront.c:
  large but acceptable during active optimization.
  only refactor after measurements stabilize.

hakozuna-hz5/midpagefront/hz5_midpagefront.c:
  large but active/stable enough to defer until allocator behavior settles.
```

## Reading Order

```text
1. current_task.md
2. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
3. hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
4. linux/HZ5_BUILD_SCRIPT_LAYOUT.md
5. hakozuna-hz5/docs/HZ5_SOURCE_CLEANUP_PLAN.md
6. hakozuna-hz5/docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md
7. hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```
