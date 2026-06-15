# HZ6 Current Task

HZ6 is the active Windows/Linux implementation and benchmarking family. Keep
this file short; it is an orientation entry point, not the chronological
experiment ledger.

## Read First

```text
Selected rows and current comparisons:
  HZ6_SELECTED_FAMILY_SUMMARY.md

Lane names, status, controls, and no-go boundaries:
  HZ6_LANE_GUIDE.md

Ubuntu LD_PRELOAD selected bundle and controls:
  HZ6_UBUNTU_PRELOAD_LANES.md

Ubuntu selected speed/RSS balance:
  HZ6_UBUNTU_SELECTED_BALANCE.md

Ubuntu MidPage next design:
  HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md

Ubuntu MidPage 32K run closeout:
  HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md

Ubuntu preload free-order closeout:
  HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md

Repo cleanup and documentation rules:
  HZ6_REPO_HYGIENE.md

Source/module cleanup:
  HZ6_SOURCE_MODULARIZATION.md

Archived chronological ledger:
  archive/current_task_2026-06_history.md
```

## Current Status

```text
R1 implementation remains modular across API, route, frontcache, transfer,
source, owner, policy, and fronts.

Windows selected-family lane status is maintained in HZ6_LANE_GUIDE.md and
HZ6_SELECTED_FAMILY_SUMMARY.md.

Ubuntu LD_PRELOAD status is maintained in HZ6_UBUNTU_PRELOAD_LANES.md.
Current Ubuntu selected default includes MidPage descriptor-out. The latest
MidPage register callsite audit shows route fallback is already eliminated on
the target row; register pressure is split between direct reuse and front alloc.
MidPage trusted activation source-block-check skip was tested and is no-go for
preload default because the target and tiny guard did not improve.
MidPage preload-boundary malloc skip is now selected with an unlikely size
guard plus noinline helper; it avoids empty transfer-first probes on the
MidPage direct-local path without adding a helper call to small rows.
Balanced current-bias free ordering is now selected/default; it tries MidPage
before Toy only when allocator-local MidPage active entries exceed Toy active
entries.
The confirmation lane now compares selected default against an explicit
boundary-off control DSO.
The latest Ubuntu selected balance matrix shows HZ6 is strongest on
4096..16384: faster and lower-RSS than HZ4, much stronger than mimalloc/system,
and now slightly ahead of tcmalloc on both speed and RSS. HZ3 remains the
speed/RSS ceiling on this row.
MidPageFrontcacheRSSAudit-L1 is now implemented as a diagnostic lane. The first
200K audit shows a large fixed allocator-local table cost plus expected
MidPage source payload pressure:
  16..4096     peak 100.25 MiB, static 61.73 MiB, frontcache 20.00 MiB
  1024..4096   peak 111.62 MiB, static 61.73 MiB, frontcache 20.00 MiB
  4096..16384  peak 114.88 MiB, static 61.73 MiB, frontcache 20.00 MiB
  raw: private/raw-results/linux/hz6_midpage_rss_audit_20260614_164214
AllocatorStaticTableTrim-L1 is now selected/default for Ubuntu preload:
  route table 131072 -> 65536
  descriptor table 32768 -> 16384
  source block table 4096 -> 2048
  frontcache bin capacity 8192 -> 4096
  confirm repeat-5 without stats:
    16..4096     41.519M / 100.62 MiB -> 43.581M / 79.75 MiB
    1024..4096   39.966M / 111.75 MiB -> 41.849M / 91.00 MiB
    4096..16384  40.863M / 115.25 MiB -> 42.904M / 94.38 MiB
  safety repeat-3: route/descriptor/source/frontcache/fallback failures all 0
  raw: private/raw-results/linux/hz6_static_table_trim_confirm_20260614_165003
Earlier cross-allocator refresh after static table trim:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
  16..256     hz6 60.381M / 30.38 MiB
  16..4096    hz6 42.216M / 79.75 MiB
  1024..4096  hz6 39.672M / 91.00 MiB
  4096..16384 hz6 41.264M / 94.38 MiB
  At that checkpoint on 4096..16384, HZ6 trailed tcmalloc speed
  (41.264M vs 44.812M) but had lower RSS and better ops-per-MiB.
Earlier cross-allocator refresh after current-bias and 8K run768:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_050834
  16..256     hz6 57.545M / 30.50 MiB
  16..4096    hz6 40.441M / 79.75 MiB
  1024..4096  hz6 38.812M / 91.00 MiB
  4096..16384 hz6 45.984M / 94.38 MiB
  On 4096..16384, HZ6 now beats tcmalloc on speed/RSS:
    hz6      45.984M / 94.38 MiB
    tcmalloc 45.310M / 103.25 MiB
MidPage32KRun1536-L1 is now selected/default for Ubuntu preload:
  HZ6_MIDPAGE_32K_RUN_BYTES 786432 -> 1572864
  focused repeat-15 without stats:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_103231
    16..256      57.738M -> 57.228M  (-0.88%)
    16..4096     41.985M -> 41.791M  (-0.46%)
    1024..4096   40.525M -> 40.302M  (-0.55%)
    4096..16384  46.078M -> 47.991M  (+4.15%)
  stats repeat-3:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_103250
    fail counters 0; 4096..16384 source_alloc 1599 -> 900.
  post-promotion HZ6-only repeat-5:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_103405
    16..256      57.985M / 30.38 MiB
    16..4096     41.868M / 79.75 MiB
    1024..4096   40.253M / 91.12 MiB
    4096..16384  48.563M / 94.50 MiB
  full cross repeat-3:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_103414
    4096..16384 hz6 45.283M / 94.38 MiB
    4096..16384 tcmalloc 44.034M / 103.75 MiB
  decision: promote HZ6_MIDPAGE_32K_RUN_BYTES=1572864 for Ubuntu preload.
MidPage32KRun2048-L1 is now selected/default for Ubuntu preload:
  HZ6_MIDPAGE_32K_RUN_BYTES 1572864 -> 2097152
  focused repeat-15 without stats:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_112005
    16..256      57.147M -> 57.264M  (+0.21%)
    16..4096     41.829M -> 41.560M  (-0.64%)
    1024..4096   39.667M -> 40.017M  (+0.88%)
    4096..16384  48.278M -> 49.789M  (+3.13%)
  stats repeat-3:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_112031
    fail counters 0; 4096..16384 source_alloc 900 -> 723.
  post-promotion HZ6-only repeat-5:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_112129
    16..256      57.306M / 30.50 MiB
    16..4096     41.608M / 79.75 MiB
    1024..4096   39.868M / 91.00 MiB
    4096..16384  49.480M / 94.38 MiB
  full cross repeat-3:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_112139
    4096..16384 hz6 48.327M / 94.25 MiB
    4096..16384 tcmalloc 44.795M / 102.38 MiB
  decision: promote HZ6_MIDPAGE_32K_RUN_BYTES=2097152 for Ubuntu preload.
MidPage32KRunFineLadder-L1 keeps run2048 selected:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_113052
  repeat-7 without stats:
    4096..16384 selected 49.494M / 94.50 MiB
    4096..16384 run2048  49.675M / 94.50 MiB
    4096..16384 run2304  48.864M / 94.50 MiB
    4096..16384 run2560  48.411M / 94.50 MiB
    4096..16384 run3072  44.866M / 94.38 MiB
    4096..16384 run4096  46.384M / 94.38 MiB
  decision: keep run2048 selected; 2M is the local peak and larger runs are
  controls/no-go for the balanced preload lane.
MidPage8KBorrow32-L1 is implemented as a default-off control:
  flag: HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1
  production repeat-7:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_113433
    16..256      selected 57.947M -> mid8_borrow32 57.372M
    16..4096     selected 42.100M -> mid8_borrow32 42.460M
    1024..4096   selected 40.156M -> mid8_borrow32 40.290M
    4096..16384  selected 50.077M -> mid8_borrow32 49.896M
  stats repeat-3:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_113452
    fail counters 0; 4096..16384 mid8_borrow32 had borrow_success=0.
  read: broad borrow_larger is guard-negative; narrow MidPage 8K->32K borrow
  does not find real candidates on the selected target row, so keep off.
MidPageActiveMapMaskIndex-L1 is now selected/default for Ubuntu preload:
  flag: HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1
  reason: selected MidPage active-map capacity is 16384, so index/probe wrap can
  use a power-of-two mask instead of modulo without changing behavior.
  production repeat-15:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_131659
    16..256      57.755M -> 57.523M  (-0.40%)
    16..4096     41.770M -> 41.752M  (-0.04%)
    1024..4096   40.007M -> 40.019M  (+0.03%)
    4096..16384  49.443M -> 50.231M  (+1.59%)
  stats repeat-3:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_131717
    fail counters 0.
  post-promotion HZ6-only repeat-5:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_131844
    16..256      57.443M / 30.50 MiB
    16..4096     41.409M / 79.75 MiB
    1024..4096   40.162M / 90.88 MiB
    4096..16384  49.639M / 94.38 MiB
  full cross repeat-3:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_131852
    4096..16384 hz6 48.459M / 94.50 MiB
    4096..16384 tcmalloc 43.632M / 103.88 MiB
  decision: promote HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1 for Ubuntu preload.
MidPageActiveMapRegisterFastSlot-L1 is now selected/default for Ubuntu preload:
  flag: HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1
  reason: after mask-index, diagnostic probe attribution showed the selected
  4096..16384 row has active-map register/free hits at about 1.15 average
  probes with 88.3% base-slot hits. Register fast-slot avoids entering the
  bounded probe loop for empty/same-pointer base-slot registrations without
  changing free behavior.
  diagnostic probe audit:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145056
    4096..16384 selected:
      register avg_probe=1.15, base_slot=88.3%
      free hit avg_probe=1.15, base_slot=88.3%
      fail counters 0
  production repeat-15:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145147
    16..256      56.980M -> 56.787M  (-0.34%)
    16..4096     41.344M -> 41.409M  (+0.16%)
    1024..4096   39.847M -> 39.646M  (-0.50%)
    4096..16384  48.584M -> 50.160M  (+3.24%)
  comparison:
    amap_fast_both reached 50.253M on 4096..16384, but 1024..4096 was
    weaker (-0.99%), so keep free fast-slot as a control for now.
  stats repeat-3:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145220
    fail counters 0.
  post-promotion HZ6-only repeat-5:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_145322
    16..256      56.227M / 30.38 MiB
    16..4096     40.939M / 79.88 MiB
    1024..4096   40.079M / 91.00 MiB
    4096..16384  50.574M / 94.38 MiB
  full cross repeat-3:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_145328
    4096..16384 hz6 48.961M / 94.50 MiB
    4096..16384 tcmalloc 43.192M / 106.62 MiB
  decision: promote HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1 for Ubuntu
  preload; keep HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1 as a control.
MidPageActiveMapFreeFastSlotFollowup-L1 is closed as control/no-go:
  flags:
    HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1
    HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1
  reason: after register fast-slot promotion, free fast-slot still does not
  clear the balanced guard gate. The current-bias gated shape adds branch cost
  and is weaker than the plain free-fast control.
  production repeat-7:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_152942
    4096..16384 selected 50.566M -> free_fast 50.604M
    4096..16384 selected 50.566M -> free_fast_bias 50.081M
    1024..4096 selected 39.486M -> free_fast_bias 39.462M
    16..256 selected 57.027M -> free_fast_bias 55.097M
  production repeat-15:
    raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_153012
    16..256      57.028M -> free_fast 56.508M  (-0.91%)
    16..4096     41.478M -> free_fast 41.581M  (+0.25%)
    1024..4096   39.763M -> free_fast 39.269M  (-1.24%)
    4096..16384  50.375M -> free_fast 50.408M  (+0.07%)
  decision: keep both free-fast controls off. Do not promote the current-bias
  gated shape; it is a useful negative control showing the extra branch is not
  free.
Earlier repeat-3 refresh after free-hint/free-fastslot no-go closeouts:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_004605
  16..256     hz6 58.046M / 30.50 MiB
  16..4096    hz6 40.757M / 79.75 MiB
  1024..4096  hz6 38.917M / 91.00 MiB
  4096..16384 hz6 41.961M / 94.38 MiB
  At that checkpoint on 4096..16384, HZ6 was below tcmalloc speed
  (41.961M vs 45.086M) but kept lower RSS (94.38 vs 100.62 MiB).
Earlier MidPage32KRun768-L1 promotion:
  HZ6_MIDPAGE_32K_RUN_BYTES 524288 -> 786432
  512K remains the direct control.
  repeat-7 versus 512K improved 4096..16384 and 16..4096, kept 16..256
  positive, and only softened 1024..4096 slightly. Safety spot-check is clean.
  Superseded by MidPage32KRun1536-L1; full evidence:
    HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md.
MidPageSupplyMapResume-L1 is now observed after run768:
  Diagnostic selected768 shows free route fallback is no longer the main wall:
    4096..16384 free_route_lookup_after_maps is about 2.2K for 1M frees.
  Remaining pressure is supply/frontcache shape:
    4096..16384 midpage_source_alloc=649
    midpage_8k_alloc_call=180
    midpage_32k_alloc_call=469
    midpage_8k_frontcache_pop_empty=362
    midpage_32k_frontcache_pop_empty=938
  8K run widening is now selected at run768 after current-bias:
    run8_512K repeat-7: 4096..16384 source_alloc 653 -> 565, but speed was
    essentially flat/slightly weak; 1024..4096 improved.
    after current-bias, production-shape repeat-7:
      raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_044047
      run8_512K improved 4096..16384 45.301M -> 46.000M, but weakened
      16..4096 42.246M -> 41.862M.
    focused repeat-15:
      raw: private/raw-results/linux/hz6_run8_512k_repeat15_20260615_044225
      16..256      58.373M -> 57.192M  (-2.02%)
      16..4096     42.070M -> 41.820M  (-0.59%)
      1024..4096   40.338M -> 40.525M  (+0.46%)
      4096..16384  45.389M -> 46.014M  (+1.38%)
    decision: keep 8K run512 as target-positive guard-negative control.
    run8_768K focused repeat-15:
      raw: private/raw-results/linux/hz6_prefill_cache_run8_768_repeat15_20260615_050459
      16..256      57.307M -> 57.679M  (+0.65%)
      16..4096     41.591M -> 41.638M  (+0.11%)
      1024..4096   39.628M -> 40.163M  (+1.35%)
      4096..16384  45.649M -> 45.971M  (+0.71%)
    post-promotion selected repeat-5:
      raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_050543
      16..256      58.484M / 30.38 MiB
      16..4096     42.150M / 79.75 MiB
      1024..4096   40.106M / 91.00 MiB
      4096..16384  46.496M / 94.25 MiB
    safety stats raw: private/raw-results/linux/hz6_run8_768_selected_stats_safety_20260615_050543
      route_miss=0 route_invalid=0 alloc_fail=0 register_fail=0
      source_block_exhausted=0 route_register_fail=0 on 16..256, 16..4096,
      and 4096..16384.
    decision: promote HZ6_MIDPAGE_RUN_BYTES=786432 for Ubuntu preload.
  MidPage prefill-cache-only retry is no-go:
    flag: HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1
    raw: private/raw-results/linux/hz6_prefill_cache_run8_768_repeat15_20260615_050459
    16..256 regressed -7.11%; 4096..16384 regressed -2.08%.
  Active-map capacity/probe widening removes most route-after-map fallbacks but
  regresses speed and RSS because the larger map is hotter than the remaining
  fallback cost:
    cap32K/probe4: route_after_maps about 2199 -> 124, but target speed fell
    and RSS rose.
  Keep HZ6_MIDPAGE_RUN_BYTES=786432 and MidPage active-map cap16K/probe4 as
  selected. Use run_hz6_midpage_supply_map_ab.sh for reproducible controls.
Recent MidPage/free-order lanes are closed for selected default:
  low-water refill is no-go; extra eager supply/layout cost did not translate.
  aligned-first is no-go; alignment was not selective enough for Toy-heavy rows.
  balanced current-bias is selected/default:
    repeat-15 raw: private/raw-results/linux/hz6_current_bias_repeat15_20260615_041255
    16..256      58.235M -> 57.971M  (-0.45%)
    16..4096     42.330M -> 42.333M  (+0.01%)
    1024..4096   40.580M -> 40.643M  (+0.15%)
    4096..16384  44.163M -> 45.596M  (+3.24%)
    post-promotion selected repeat-5:
      raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_041438
      16..256      57.367M / 30.50 MiB
      16..4096     42.130M / 79.62 MiB
      1024..4096   39.670M / 91.00 MiB
      4096..16384  46.357M / 94.50 MiB
    safety stats raw: private/raw-results/linux/hz6_current_bias_selected_stats_safety_20260615_041430
      route_miss=0 route_invalid=0 alloc_fail=0 register_fail=0 on
      16..256, 16..4096, and 4096..16384.
    bias2x is target-stronger but guard-negative.
  Full evidence: HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md.
FrontcacheCapacityShapeAudit-L1 is now implemented:
  diagnostic adds class-level frontcache push/pop-empty/bin-max attribution.
  raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260614_215447
  read:
    1024..4096 uses class4 up to the selected cap4096, so broad cap shrink is
    unsafe.
    4096..16384 uses class5 up to about 2832, but mid32k cap3072 did not win
    speed/RSS, and cap2560/2048 increased empty pops and slowed the target.
    midpage cap3072 is no-go because it badly regresses 1024..4096.
  keep selected global frontcache4096 for now.
FrontcacheClassStorageTrim-L1 is now implemented as a default-off control:
  flag: HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1
  runner: run_hz6_frontcache_shape_ab.sh now supports
    --no-stats --no-diagnostics for production-shape speed/RSS ranking
    --stats --diagnostics for class max/empty attribution
  production-shape repeat-7:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_043630
    16..256      selected 55.108M / 30.50 MiB -> trim 57.733M / 30.38 MiB
    16..4096     selected 42.318M / 79.75 MiB -> trim 41.818M / 79.75 MiB
    1024..4096   selected 40.216M / 91.00 MiB -> trim 40.355M / 91.00 MiB
    4096..16384  selected 45.239M / 94.38 MiB -> trim 45.567M / 94.50 MiB
  diagnostic repeat-1:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_043714
    storage trim kept c4/c5 max behavior but did not show meaningful RSS
    reduction and was target-weak under diagnostic overhead.
  decision: keep off; fixed frontcache storage is not the next selected RSS win.

Latest MidPage closeout:
  keep descriptor-out selected
  keep register callsite counters as diagnostic-only
  keep free-cache counters as diagnostic-only
  keep transfer-probe counters as diagnostic-only
  keep trusted activation skip off
  keep trusted cache push off
  keep MidPage direct-local skip-transfer-first off
  keep noinline/branch-isolated transfer-skip off
  keep preload-boundary noinline transfer-skip selected
  keep static table trim selected
  keep MidPage 32K run2048 selected
  keep MidPage active-map mask-index selected
  keep MidPage active-map register fast-slot selected
  keep preclassified malloc shape out of source
  keep MidPage target DSO as selected/control alias

Next Ubuntu MidPage work should not try more transfer-skip code-shape tweaks;
the broader cross-allocator matrix already confirmed the promoted outer-guard
noinline boundary. Do not chase route fallback, deeper free probing,
source-run-slot route registration, broad malloc code-shape changes, or
whole-helper free-cache replacement first.

Next recommended optimization lane:
  FrontcacheStorageLayoutAuditV2-L1 is closed as control/no-go for default.
  active-map register fast-slot is selected and safety-clean; run-size, borrow,
  and free fast-slot attempts are closed for now. Next work should target source
  payload/RSS pressure or a free-hit shape that does not add a branch to every
  MidPage free attempt.

MidPage32KRunFineLadder-L1 task:
  goal:
    Re-check the 32K MidPage run-size ridge after active-map register fast-slot
    and frontcache storage closeout. The selected 2048 KiB run is strong, but
    source payload dominates RSS attribution; a narrow ladder may find a better
    speed/RSS balance without changing behavior.
  implementation:
    Extend run_hz6_midpage_payload_trim_ab.sh with 1920/1984/2112/2176/2240
    KiB variants and payload/active-source attribution in stats summaries.
  promotion gates:
    production 4096..16384 must beat selected or keep speed flat with lower
    peak/payload attribution; 16..4096 and 1024..4096 must stay guard-clean;
    stats counters must keep fail=0.

MidPage32KRunFineLadder-L1 result:
  runner updates:
    run_hz6_midpage_payload_trim_ab.sh now supports 1920/1984/2112/2176/2240
    KiB variants, optional --diagnostics, --stats-value, and payload/active
    source attribution in the summary.
  production repeat-7:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163111
    selected/run2048 remains the 4096..16384 speed peak:
      selected 50.895M / 94.50 MiB
      run2112k 48.394M / 94.50 MiB
      run2176k 48.825M / 94.38 MiB
  diagnostic repeat-3:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163443
    larger runs reduce 4096..16384 source_alloc but increase payload
    attribution and lose speed:
      selected source_alloc=723, payload=399.25 MiB
      run2112k source_alloc=708, payload=400.00 MiB
      run2176k source_alloc=696, payload=402.25 MiB
    fail counters stayed 0.
  decision:
    keep HZ6_MIDPAGE_32K_RUN_BYTES=2097152 selected. The next RSS work should
    not be a wider 32K run; it needs either payload release/reuse behavior or a
    separate no-branch free-hit path.

FrontcacheStorageLayoutAuditV2-L1 task:
  goal:
    Keep the current 4096..16384 speed/RSS balance while checking whether
    class-specific frontcache backing storage can recover allocator-local RSS
    or improve cache locality after run2048 and active-map register fast-slot.
  implementation:
    Extend run_hz6_frontcache_shape_ab.sh with focused --variants support and
    cold/class-specific storage trim variants. Do not change selected preload
    defaults before measurement.
  candidate variants:
    storage_trim
    storage_trim_cold32
    storage_trim_cold16
    storage_trim_c1_512_c3_512_cold32
    storage_trim_c0_64_c1_512_c3_512_cold32
  promotion gates:
    fail counters stay zero, 4096..16384 improves or stays flat, 16..4096 and
    1024..4096 do not regress materially, and peak RSS or table bytes improve
    enough to justify making the lane default. Otherwise close as control.

FrontcacheStorageLayoutAuditV2-L1 result:
  runner updates:
    run_hz6_frontcache_shape_ab.sh now supports --variants and reports
    frontcache/static table attribution when stats are enabled.
  production repeat-5:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161719
    selected remains best on the target row:
      4096..16384 selected 51.332M / 94.38 MiB
      storage_trim_cold32 50.271M / 94.38 MiB
    storage_trim_cold32 helped 16..4096 and 1024..4096, but the target loss and
    unchanged peak RSS do not justify default promotion.
  diagnostic repeat-3:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161900
    storage_trim_cold32 cuts frontcache table attribution from about 10242 KiB
    to 2152 KiB and static table attribution from about 31609 KiB to 23519 KiB,
    with fail counters clean. Peak RSS is still flat because source payload is
    the dominant resident pressure in the measured rows.
  decision:
    keep storage trim variants as control/evidence; do not change selected
    preload defaults.

Closed MidPage controls:
  free-order/page-hint/current-bias details:
    HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md
  32K run-size details:
    HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md
  frontcache and supply controls:
    HZ6_UBUNTU_PRELOAD_LANES.md

Use HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md as the implementation order for the next
MidPage pass. TransferProbeAudit-L1, target DSO, and guard-isolated helper
attempts are done. The final outer-guard noinline preload-boundary shape passed
the focused repeat-15 promotion guard and the selected-vs-boundary-off
confirmation lane.

Long historical benchmark notes and failed experiments live in:
  archive/current_task_2026-06_history.md
```

## Cleanup Status

```text
The root repository source/script large-file audit is currently clean for the
1000-line threshold:
  ../../linux/audit_large_source_files.sh --top 20
  no output

Ubuntu preload script hygiene:
  selected flags are centralized in linux/hz6_preload_flags.sh
  A/B runners should use key-based define replacement, not positional flag
  array indexes.

Source modularity:
  core HZ6 modules remain healthy.
  preload/hz6_preload_hooks.c now owns libc hook control flow and allocator
  route/free/realloc behavior.
  preload/hz6_preload.c is below the large-source threshold and owns preload
  stats aggregation/printing plus allocator registry state.
  preload/hz6_preload_stats.h is the narrow shared hook/stats boundary.
  A later cleanup-only pass can split stats printing into
  preload/hz6_preload_stats.c if the diagnostic body grows again.

Do not append long run logs here. Promote stable conclusions into the focused
HZ6 docs and move raw chronological evidence to archive docs.
```
