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

## Recent Closeout: HZ6 Ubuntu Toy ActiveMap Free FastSlot After RawPop-L1

```text
latest continuation:
  Add ToyActiveMapFreeProbeAudit-L1 as diagnostic-only.
  This does not change behavior; it only reports Toy/class4 active-map free
  hit base-slot ratio and hit probe length.

  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_215547
  selected stats+diagnostic, repeat-3, 200K, focused+fixed:
    16..4096:
      toy4_free_hit=916261
      toy4_free_base=94.3%
      toy4_free_avg_probe=1.06
      toy4_free_max_probe=4
    1024..4096:
      toy4_free_hit=1216713
      toy4_free_base=94.6%
      toy4_free_avg_probe=1.06
      toy4_free_max_probe=4
    fixed_4k:
      toy4_free_hit=1217140
      toy4_free_base=94.1%
      toy4_free_avg_probe=1.07
      toy4_free_max_probe=4

  read:
    Mid-small rows are already Toy class4 fast-hit dominated.
    Source/front refill is small, and Toy active-map free lookup is already
    near base-slot optimal. Register collision remains visible, but the free
    side is not paying a large probe wall.

  decision:
    Keep Toy map64k/probe8/mask/shift12 as controls/no-go.
    Do not default broad Toy preclassification; it remains a profile lane.
    Next speed work should avoid active-map capacity/probe tweaks and look for
    a new local-page/run metadata path or a deliberately separate profile DSO.

goal:
  Re-test low-risk Toy/small controls after the selected raw frontcache pop
  changed the production hot-path code shape.

implementation:
  Add runner controls:
    toy_free_fast
    toy_addr_envelope
    toy_preclassified_malloc
    current_bias_off
    direct_max5
  Select HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1.

evidence:
  production repeat-15:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_201517
    16..256       selected 55.832M -> toy_free_fast 56.483M
    16..4096      selected 35.258M -> toy_free_fast 35.726M
    1024..4096    selected 33.027M -> toy_free_fast 32.865M
    4096..16384   selected 43.657M -> toy_free_fast 44.074M
  diagnostic repeat-3:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_201548
    route_invalid=0
    route_miss=0
    route_register_fail=0
    alloc_fail=0
    4096..16384 route_after_maps sample: 1396 -> 1171
  selected baseline after default:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_201731
    4096..16384 selected 43.799M / 94.12 MiB
  selected balance refresh:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_201844
    4096..16384 hz6 41.718M / 94.25 MiB
    4096..16384 tcmalloc 35.058M / 97.88 MiB
    4096..16384 hz4 25.852M / 112.88 MiB
  selected-only free-order attribution:
    raw: private/raw-results/linux/hz6_preload_free_order_ab_20260615_202306
    route_after_maps is now only about 200..1100 per focused row
    all sampled route fallbacks are owned-valid; real_fallback=0
  preload_fast_free retest:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_202343
    broad prechecked free stayed no-go:
      16..256 56.333M -> 56.031M
      4096..16384 43.990M -> 43.603M
  boundary code-shape retest:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_202612
    selected noinline remains the target-balanced shape:
      boundary_inline 4096..16384 43.812M -> 42.896M
      boundary_off    4096..16384 43.812M -> 35.205M
    inline/off can help small guards, but the MidPage target loss blocks
    default selection.
  build-flag retest:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_202754
    keep selected -O2:
      -O3                         44.118M -> 43.222M
      -fno-semantic-interposition 44.118M -> 43.909M
      both                        44.118M -> 43.330M
  fixed-row runner/audit:
    run_hz6_midpage_payload_trim_ab.sh now supports:
      --rows focused
      --rows fixed
      --rows focused,fixed
    fixed rows:
      fixed_4k, fixed_8k, fixed_16k
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_203102
    isolated controls were not promotion-clean:
      toy_preclassified_malloc won fixed_4k only
      same_owner_fast won some fixed rows but remained target-negative later
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_203155
      same_owner_fast 4096..16384 43.781M -> 43.188M
  Toy prefill batch ladder:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_203243
    tested HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=64/96/192/256
    keep selected max128; fixed-row gains were split and 1024..4096/tiny were
    guard-negative.
  Source-run reuse/reclaim fixed-row retest:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_203339
    sourcerun, sourcerun_sameclass, and sourcerun_reclaim were all
    guard-negative or target-flat/negative. Keep SOURCE_RUN_REUSE off.
  cross-allocator fixed-size slice refresh:
    runner: linux/run_hz6_ubuntu_size_slices_matrix.sh
    fixed_mid raw: private/raw-results/linux/hz6_ubuntu_size_slices_20260615_203643
      fixed_4k  hz6 31.376M / 91.75 MiB
      fixed_8k  hz6 41.815M / 93.12 MiB
      fixed_16k hz6 44.770M / 93.12 MiB
    large_span raw: private/raw-results/linux/hz6_ubuntu_size_slices_20260615_203813
      fixed_32k  hz6 47.088M / 36.50 MiB
      fixed_64k  hz6 18.137M / 35.88 MiB
      fixed_128k hz6 17.276M / 38.00 MiB
      fixed_256k hz6 13.871M / 41.75 MiB
    read:
      HZ6 is not universally weak on fixed sizes. It trails HZ3/tcmalloc on
      fixed_4k, approaches HZ3 and beats tcmalloc/HZ4 on fixed_8k, edges HZ3
      and beats all measured allocators on fixed_16k speed, and is very strong
      on 32K..256K speed. RSS remains the main fixed_mid tradeoff.
  fixed-size residency audit:
    runner: linux/run_hz6_midpage_rss_audit.sh --rows fixed_mid
    raw: private/raw-results/linux/hz6_midpage_rss_audit_20260615_204203
    fixed_16k has 520.00 MiB of 32K all-local-free payload, 16384 matching
    frontcache entries, and ref mismatch 0.
    cold-retire fixed-row retest:
      production raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204233
      diagnostic raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204253
      existing free-time cold-retire gate does not fire on the fixed_16k final
      all-local-free shape; do not default it.
    fixed-row control refresh:
      raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204454
      direct_max5 fixed_4k 28.506M -> 28.965M, but 16..4096 regressed.
      same_owner_fast fixed_16k 39.192M -> 39.437M, but 4096..16384 regressed.
      current_bias_off helps some small/fixed rows, but target regresses.
      No fixed-row control is default-clean.
    fixed_4k attribution/control follow-up:
      free-order runner now supports --rows fixed and current_bias_off.
      frontcache-shape runner now supports --rows fixed.
      raw: private/raw-results/linux/hz6_preload_free_order_ab_20260615_204722
      fixed_4k selected has:
        toy_attempt ~= 413K
        toy_hit ~= 406K
        mid_attempt ~= 7.5K
        mid_hit ~= 6.4K
        route_after_maps ~= 1.1K
      raw: private/raw-results/linux/hz6_preload_free_order_ab_20260615_204830
      current_bias_2x helped fixed rows in stats-on attribution, but
      production repeat rejected it later.
      production raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205053
      current_bias_2x:
        fixed_4k 31.743M -> 31.908M
        4096..16384 43.784M -> 44.288M
        but 16..4096, 1024..4096, fixed_8k, and fixed_16k regressed.
      frontcache shape raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_205146
      fixed_4k class4 reaches cap4096 and has c4_empty ~= 2050, matching
      1024..4096 shape.
      frontcache controls:
        raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205229
        frontcache8192 improved target/fixed_8k/fixed_16k slightly but regressed
        tiny and 16..4096.
        raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205330
        storage_trim_c4_8192 kept static table smaller but destroyed fixed_16k
        because class5 storage trims to 3072.
        raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205408
        storage_trim_c4_8192_c5_4096 recovered class5 but still regressed target
        and fixed_8k/16k.
    Toy class4 malloc-path diagnostic:
      Added diagnostic-only toy_class4_* counters under
      HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1.
      raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205845
      fixed_4k selected:
        toy4_fast 1220355
        toy4_hit 1217280
        toy4_front 3075
        toy4_pop/activate 3075
        toy4_map_reg 1217280
        toy4_collision 65160
      read:
        fixed_4k is already almost entirely direct Toy class4 reuse. The
        remaining wall is not front dispatch/source refill; Toy active-map
        collision is visible but not automatically a behavior win.
      Toy active-map controls:
        raw diagnostic: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_205922
        toy_map64k roughly halves class4 collision but raises RSS and regresses
        important rows.
        toy_probe8 helped fixed_4k in diagnostic only.
        production repeat-15 raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_210005
        toy_probe8 improved 16..4096 and 1024..4096, but tiny, target,
        fixed_4k, and fixed_16k were flat/negative. Keep off.
      Toy active-map index controls:
        plan:
          Add control-only HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 and
          HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1.
        intent:
          mask_index tests code-shape only for power-of-two capacities.
          shift12_index tests whether low slot bits hurt fixed_4k collision,
          but is expected to be risky because Toy slots can share pages.
        accept:
          promote only if production repeat keeps tiny, 16..4096,
          1024..4096, target, fixed_4k, fixed_8k, and fixed_16k non-negative
          enough to beat selected noise.
        diagnostic raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_210443
        read:
          shift12_index and shift12_mask are immediate no-go. 16..256 fell
          from about 24.5M to about 0.78M in diagnostics, confirming that page
          granularity loses essential Toy slot entropy.
        production raw with runtime power-of-two guard:
          private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_210534
          mask_index was mixed: fixed_4k improved 30.460M -> 30.857M, but
          fixed_8k regressed 41.266M -> 40.761M and target stayed flat.
        production raw with branchless mask:
          private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_210625
          mask_index improved tiny/mixed/fixed_4k, but target regressed
          badly: 4096..16384 43.379M -> 40.302M.
        decision:
          Keep HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 and
          HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 as controls/no-go. Do not
          default Toy active-map index changes; code layout around the target
          row is too sensitive.

decision:
  selected/default. This is a small but balanced production code-shape win after
  raw-pop. Keep toy_addr_envelope, toy_preclassified_malloc, current_bias_off,
  and direct_max5 as controls/no-go unless a later baseline changes the shape.

next:
  Do not widen active-map capacity/probe next. Fixed-size RSS attribution is
  now clear, fixed_4k free-order/frontcache controls are closed, and Toy class4
  malloc-path attribution shows the fast reuse path is already dominant.
  Prefer a new quiescent/snapshot scavenge design next. The specific probe is
  HZ6_BenchQuiescentScavengeProbe-L1:
    Add a diagnostic-only benchmark hook that can call an exported preload
    scavenge function after worker threads join and before RSS reporting.
    This must not affect default benchmark runs unless explicitly enabled by an
    environment variable.
    Expect ru_maxrss/peak_kb to stay high; use final/current RSS and released
    object count to decide whether quiescent scavenge is useful outside the
    existing peak-RSS metric.
    implementation:
      Added exported preload function:
        hz6_preload_scavenge_local_free(size_t max_bytes)
      Added bench opt-in:
        HZ_BENCH_SCAVENGE_BEFORE_RSS=all
      Added runner variant:
        selected_scavenge_before_rss
      The hook runs after worker joins and after timed ops/s end, before RSS
      reporting. Default runs do not set the environment variable.
    evidence:
      stats-on raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_211346
        4096..16384 current RSS: 94.25 MiB -> 70.67 MiB
        fixed_16k current RSS:   93.25 MiB -> 59.94 MiB
        payload attribution after scavenge drops to about 0.25 MiB / 4 active
        source blocks, proving the final all-local-free payload is recoverable.
      no-stats raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_211402
        4096..16384 current RSS: 94.38 MiB -> 70.86 MiB
        fixed_16k current RSS:   93.25 MiB -> 59.95 MiB
        peak_kb stays essentially flat, as expected from ru_maxrss semantics.
    decision:
      Keep as diagnostic/control. This is strong evidence that HZ6 has good
      quiescent RSS recoverability, but it is not a default runtime behavior
      because it runs after timed work and does not improve peak RSS.
    follow-up:
      HZ6_PreloadedMallocTrim-L1:
        Implement malloc_trim(size_t pad) in the LD_PRELOAD shim. It first
        scavenges HZ6 local-free descriptors through
        hz6_preload_scavenge_local_free(0), then forwards to the real libc
        malloc_trim when available.
        This makes the quiescent RSS recovery available through a standard
        application-facing API without adding per-malloc/free hot-path work.
      validation:
        symbol export:
          malloc_trim and hz6_preload_scavenge_local_free are exported by the
          preload DSO.
        stats-on raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_211643
          malloc_trim path matches direct scavenge on current RSS:
            4096..16384 current RSS: 94.12 MiB -> 70.93 MiB
            fixed_16k current RSS:   93.25 MiB -> 59.80 MiB
          payload attribution after trim drops to about 0.25 MiB / 4 active
          source blocks.
          Runner scavenge count/result is 3 for malloc_trim because the hook
          returns a boolean success once per run, unlike direct scavenge which
          reports released object count.
        no-stats raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_211713
          4096..16384 current RSS: 94.38 MiB -> 70.78 MiB
          fixed_16k current RSS:   93.25 MiB -> 60.03 MiB
          peak RSS remains flat, so this is explicit quiescent trimming, not a
          peak-RSS optimization.
      decision:
        Keep malloc_trim hook as the standard quiescent release API for
        LD_PRELOAD. It is not automatic/default behavior; applications or
        diagnostic runners must call malloc_trim.
  Do not default the existing per-free cold-retire behavior, current_bias_2x,
  frontcache8192, storage-trim c4 variants, toy_map64k, toy_probe8,
  toy_mask_index, or toy_shift12_index.
```

## Recent Closeout: HZ6 Ubuntu MidPage ActiveMap Collision Layout Audit-L1

```text
goal:
  Decide whether the remaining MidPage active-map path has data-layout or
  collision-policy headroom before changing active-map behavior again.

implementation:
  Extend run_hz6_midpage_supply_map_ab.sh summary with:
    midpage_active_map_register_collision
    midpage_active_map_register_overwrite
    midpage_active_map_free_miss_found_elsewhere
  Keep behavior unchanged; use selected/cap/probe/free-fast controls as
  measurement references.

acceptance:
  Build and R1 smokes pass.
  A focused diagnostic run identifies whether collision/found-elsewhere is
  material enough to justify a behavior lane.
  If found_elsewhere and overwrite are low, close active-map layout as not the
  next target.

result:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_183000
  4096..16384 selected:
    reg_collision=68890
    reg_overwrite=1463
    miss_found_elsewhere=0
    reg/free avg probe=1.14
    base slot=88.9%
    route_after_maps=1465
  4096..16384 amap32k_p4:
    route_after_maps 1465 -> 114
    avg probe 1.14 -> 1.06
    peak RSS 94.00 -> 95.62 MiB
    speed essentially flat
  4096..16384 amap64k_p4:
    route_after_maps 1465 -> 2
    avg probe 1.14 -> 1.03
    peak RSS 94.00 -> 98.62 MiB
    speed regressed slightly
  decision:
    close as diagnostic/control. Collision count is real, but found-elsewhere
    is zero and wider maps mostly buy fewer route fallbacks at RSS cost. Do
    not pursue another active-map capacity/probe/layout behavior now.
```

## Recent Closeout: HZ6 Ubuntu Preload Phase Counter Compile-Out-L1

```text
goal:
  Remove preload hook phase-counter call/branch overhead from production
  stats-off DSOs without changing diagnostic builds.

design:
  Add HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 and promote it to the selected
  production preload flags.
  When enabled, hz6_preload_phase_count(), phase_add(), and
  phase_count_size_bucket() become no-op macros in the preload stats header.
  The stats registry and allocator stats remain buildable.
  Stats/diagnostic runners force the flag back to 0 unless explicitly testing
  the phase_count_off variant, so normal attribution still has phase/hook
  counters.

acceptance:
  selected and diagnostic preload builds pass.
  stats-off focused rows improve or stay flat, especially 16..256 and
  4096..16384.
  stats-on diagnostic builds keep current counter behavior with the flag off.

production read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_194710
  command:
    RUNS=5 ITERS=300000 \
      run_hz6_midpage_payload_trim_ab.sh --skip-bench --no-stats \
      --include-tiny --variants selected,phase_count_on

  16..256:
    selected 51.875M vs old phase_count_on 50.587M
  16..4096:
    selected 33.809M vs old phase_count_on 32.999M
  1024..4096:
    selected 31.335M vs old phase_count_on 31.338M
  4096..16384:
    selected 43.570M vs old phase_count_on 40.482M

diagnostic read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_194723
  selected keeps nonzero HZ6_PRELOAD_PHASE_STATS under --stats --diagnostics.
  phase_count_off intentionally reports zero phase/hook counters.

decision:
  selected/default for production preload builds.
  Keep phase_count_on/phase_count_off variants for A/B and attribution sanity.
```

## Recent Closeout: HZ6 Ubuntu Preload MidPage Boundary Min Audit-L1

```text
goal:
  Check whether the selected preload MidPage malloc boundary shortcut should
  keep both 8K/32K classes, or whether raising the shortcut lower bound reduces
  guard-row incidental boundary work without losing target throughput.

design:
  Add HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES as a compile-time control.
  selected remains 4096 so behavior is unchanged.
  A/B variants:
    boundary_min8k  -> shortcut only for sizes >8192
    boundary_min16k -> shortcut only for sizes >16384

acceptance:
  Build and R1 smokes pass.
  Production stats-off A/B shows whether 8K boundary work is worth keeping.
  Diagnostic run confirms boundary_attempt/hit shape moves as expected.

production read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_195357
  selected vs boundary_min8k vs boundary_min16k:
    16..256:
      50.471M / 51.654M / 51.625M
    16..4096:
      33.646M / 33.929M / 33.640M
    1024..4096:
      31.837M / 31.767M / 31.759M
    4096..16384:
      42.609M / 39.728M / 35.090M

diagnostic read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_195415
  4096..16384 boundary attempts:
    selected       406760
    boundary_min8k 270863
    boundary_min16k     4
  boundary_min8k reintroduces 135897 empty 8K direct transfer probes.
  boundary_min16k routes nearly all 32K allocs through generic MidPage alloc
  (`midpage_32k_alloc_call=270863`).

decision:
  Keep selected lower bound at 4096.
  boundary_min8k/boundary_min16k are controls/no-go.
```

## Recent Closeout: HZ6 Ubuntu Direct Local Raw Frontcache Pop Audit-L1

```text
goal:
  Check whether direct-local reuse should bypass the generic frontcache_pop
  wrapper in production stats-off builds.

design:
  Reuse existing HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1.
  Add raw_frontcache_pop A/B runner variants.
  The control is disabled under HZ6_DIAGNOSTIC_PROBES, so stats-off production
  repeat is the promotion signal; diagnostic selected remains the attribution
  source.

acceptance:
  Production stats-off rows improve or stay flat across tiny, mixed, and
  MidPage target rows.
  R1 smokes pass if promoted.

production read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_195530
  repeat-15 selected vs raw_frontcache_pop:
    16..256:
      50.873M -> 56.845M
    16..4096:
      33.516M -> 35.772M
    1024..4096:
      31.583M -> 33.437M
    4096..16384:
      43.136M -> 43.528M

stats safety read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_195546
  fail=0 on all rows.
  source_alloc unchanged on all rows.

decision:
  Promote HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1 to selected/default production
  preload flags. It is disabled under HZ6_DIAGNOSTIC_PROBES, so diagnostic
  attribution keeps the wrapper counters.

follow-up:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_200858
  Current-bias variants were rechecked after raw-pop selected. They still do
  not pass production target balance:
    selected 4096..16384 44.164M
    current_bias_fast    43.546M
    current_bias_2x      43.598M
    current_bias_4x      43.268M

  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_201029
  Existing fast-path controls were rechecked after raw-pop selected:
    selected 4096..16384                  44.492M
    HZ6_SAME_OWNER_FAST_L1=1              44.339M
    HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1    43.482M
  Keep both controls off.
```

## Recent Closeout: HZ6 Ubuntu Front Prefill Descriptor-Out Audit-L1

```text
goal:
  Remove the self route lookup immediately after a source-block prefill slot is
  created, without changing route registration or source-block ownership.

design:
  Add HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1 as a default-off control.
  The source-block slot helper already has the descriptor before registering
  the exact route. The control returns that descriptor to
  hz6_front_prefill_source_block_kind() and caches it directly instead of
  looking up the same ptr through RouteLayer.

acceptance:
  Build and R1 smokes pass.
  Production stats-off selected vs prefill_descriptor_out improves or stays
  flat on guards and the 4096..16384 target.
  Diagnostic run shows lower route_lookup_probe_total on source-prefill-heavy
  rows, with route_register_fail/alloc_fail/source failures at zero.

production read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_200006
  repeat-7 selected vs prefill_descriptor_out:
    16..256:
      52.964M -> 51.391M
    16..4096:
      31.973M -> 32.660M
    1024..4096:
      30.547M -> 30.102M
    4096..16384:
      40.655M -> 37.893M

diagnostic read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_200043
  4096..16384 route_lookup_probe_total:
    selected               7767
    prefill_descriptor_out 7341
  safety counters stayed clean:
    alloc_fail=0
    route_register_fail=0
    source_prefill_fallback=0

decision:
  Keep as default-off control/no-go.
  The attribution premise is valid, but the code shape loses production speed.
```

## Recent Closeout: HZ6 Ubuntu Preload Current-Bias Fast Predicate-L1

```text
goal:
  Check whether the selected preload free-order current-bias predicate can be
  simplified without changing semantics.

design:
  Add HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1 as a default-off control.
  When enabled, the selected 1:1/delta0 predicate uses:
    midpage_active_map_current > toy_small_active_map_current
  instead of the generic numerator/denominator/delta expression.

acceptance:
  R1 smokes pass.
  free-order diagnostic rows stay safety-clean.
  4096..16384 improves or stays flat, with Toy/tiny guard rows not regressing
  materially.

diagnostic read:
  raw: private/raw-results/linux/hz6_preload_free_order_ab_20260615_174543
  current_bias_fast is not a winner in diagnostic shape:
    4096..16384 selected 16.116M, current_bias_fast 16.356M
    1024..4096 selected 16.145M, current_bias_fast 15.319M
  current_bias_4x is more interesting:
    4096..16384 16.941M
    16..4096 17.274M
    1024..4096 16.254M
    16..256 26.062M vs selected 26.822M
  Next: verify current_bias_4x in production shape before any default decision.

production read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174700
  stats-off repeat-7:
    4096..16384 selected 31.516M, current_bias_fast 31.781M,
    current_bias_4x 31.422M
    16..256 selected 45.567M, current_bias_fast 44.894M,
    current_bias_4x 44.369M
    1024..4096 selected 24.334M, current_bias_fast 24.011M,
    current_bias_4x 23.790M
  decision:
    keep current_bias_fast and current_bias_4x as controls/no-go for default.
    The tiny target win from fast predicate is not broad enough to justify the
    guard regressions.
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
  latest raw-pop selected cross repeat-3:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_200259
    4096..16384 hz6 54.836M / 94.50 MiB
    4096..16384 tcmalloc 46.507M / 99.00 MiB
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
  free fast-slot, cold-retire behavior, current-bias variants, and active-map
  layout attempts are closed for now. Next work should start with a broader
  hot-path attribution refresh or a non-active-map preload boundary/code-shape
  lane. Do not reopen active-map capacity/probe or free-path source release
  without new evidence.

MidPagePayloadResidencyAudit-L1 task:
  goal:
    Split MidPage source payload residency by source block and descriptor state
    before adding any release/decommit behavior. The key question is whether
    2MiB 32K runs are pinned by ACTIVE descriptors or by LOCAL_FREE/frontcache
    descriptors.
  implementation:
    Add diagnostic-only snapshot counters in Hz6StatsSnapshot and
    HZ6_PRELOAD_MEMORY_ATTR. The audit scans descriptor/source-block state from
    hz6_stats_snapshot(); no production hot-path branch or behavior changes.
  required read:
    midpage_8k/32k source blocks and payload bytes
    ACTIVE / LOCAL_FREE / TRANSFER_FREE / REMOTE_PENDING descriptor counts
    all-local-free payload bytes
    low-active 1..4 blocks and payload bytes
    ref mismatch blocks
  next gate:
    If all-local-free or low-active 32K payload is material, implement a dry-run
    cold source-block retire candidate counter. If most payload is ACTIVE, stop
    RSS release work and return to speed/free-hit lanes.

MidPagePayloadResidencyAudit-L1 result:
  implementation:
    Hz6StatsSnapshot and HZ6_PRELOAD_MEMORY_ATTR now split MidPage source
    payload by 8K/32K source block and descriptor residency. The RSS audit
    runner prints the new residency columns.
  raw:
    private/raw-results/linux/hz6_midpage_rss_audit_20260615_171658
  read:
    4096..16384:
      total payload 399.25 MiB
      MidPage 8K payload 45.06 MiB
      MidPage 32K payload 354.00 MiB
      MidPage 32K source blocks 177
      MidPage 32K active descriptors 0
      MidPage 32K local-free descriptors 11328
      MidPage 32K all-local-free payload 354.00 MiB
      ref mismatch 0
    16..4096 and 1024..4096 also show MidPage all-local-free payload, but the
    target row is the clear RSS opportunity.
  decision:
    proceed to MidPageColdSourceBlockRetireDryRun-L1. The target payload is not
    pinned by ACTIVE descriptors; it is retained by LOCAL_FREE/frontcache
    descriptors, so a batch/out-of-line retire design is worth auditing.

MidPageColdSourceBlockRetireDryRun-L1 task:
  goal:
    Estimate how much MidPage all-local-free payload could be retired without
    changing behavior. This is still diagnostic-only; it does not drain
    frontcache, unregister routes, release source blocks, or madvise memory.
  implementation:
    Extend the residency snapshot with retire-candidate blocks, payload bytes,
    descriptor count, and matching frontcache entries. Candidate blocks require
    active=0, local_free>0, transfer_free=0, and remote_pending=0.
  promotion gate:
    If 4096..16384 shows large 32K retire-candidate payload and frontcache
    entries match the candidate descriptors, design an out-of-line behavior
    helper. If frontcache coverage is low or guards show similar pressure,
    remain diagnostic/control.

MidPageColdSourceBlockRetireDryRun-L1 result:
  implementation:
    Residency snapshot now reports retire-candidate blocks, payload,
    descriptors, and matching frontcache entries. Candidate blocks are
    all-local-free blocks; this still performs no behavior change.
  raw:
    private/raw-results/linux/hz6_midpage_rss_audit_20260615_172905
  read:
    4096..16384:
      MidPage 32K retire candidate payload 354.00 MiB
      MidPage 32K retire candidate descriptors 11328
      MidPage 32K retire candidate frontcache entries 11328
      ref mismatch 0
    16..4096 and 1024..4096 each have only 8.00 MiB of 32K retire candidate
    payload; the main opportunity is clearly the MidPage-heavy target row.
  decision:
    proceed to a default-off MidPage32K cold-retire behavior control. The
    helper should be out-of-line and should drain only complete all-local-free
    32K source blocks. It must not add a scan to every free; use a high-water
    trigger and max-blocks-per-call guard.

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

Use HZ6_UBUNTU_PRELOAD_LANES.md as the current implementation ledger. The older
HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md file is historical context; TransferProbeAudit,
target DSO, and guard-isolated helper attempts are done. The final outer-guard
noinline preload-boundary shape passed the focused repeat-15 promotion guard
and the selected-vs-boundary-off confirmation lane.

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
  preload/hz6_preload.c is a thin translation unit.
  preload/hz6_preload_stats.c owns preload stats aggregation/printing plus
  allocator registry state.
  preload/hz6_preload_stats.h is the narrow shared hook/stats boundary.
  A later cleanup-only pass can split narrower print helpers inside
  preload/hz6_preload_stats.c if the diagnostic body grows further.

Do not append long run logs here. Promote stable conclusions into the focused
HZ6 docs and move raw chronological evidence to archive docs.
```

## Recent Closeout: HZ6 Ubuntu MidPage 32K Cold Retire Behavior-L1

```text
goal:
  Convert the proven MidPageColdSourceBlockRetireDryRun-L1 evidence into a
  default-off behavior control and measure the speed/RSS tradeoff.

evidence:
  raw: private/raw-results/linux/hz6_midpage_rss_audit_20260615_172905
  4096..16384 has 354.00 MiB of MidPage 32K retire-candidate payload,
  11328 candidate descriptors, 11328 matching frontcache entries, and
  ref mismatch=0.

design:
  Keep selected/default unchanged.
  Add HZ6_MIDPAGE_32K_COLD_RETIRE_L1 as a behavior control.
  Trigger only when class5 frontcache reaches a high-water mark and the
  MidPage active-map is near quiescence.
  Use a noinline helper with bounded source-block scan and max blocks per call.
  Candidate must be all local-free, no active/transfer/remote descriptors,
  ref_count must match local-free descriptors, and every descriptor must have a
  matching class5 frontcache entry before release begins.

acceptance:
  R1 smokes pass.
  fail counters stay zero.
  4096..16384 RSS drops materially.
  Guard rows do not regress enough to justify keeping the lane control-only.

first read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_173814
  Eager high-water retire is too aggressive. It retired 3267 blocks / 6534 MiB
  cumulatively on 4096..16384, but peak RSS only moved 94.00 -> 93.38 MiB while
  source_alloc jumped 723 -> 3831 and speed fell sharply. Keep eager as a
  negative control; behavior needs quiescent or low-active gating.

second read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_173940
  Quiescent/active_low_water=1 stopped source churn but retired only 12 blocks
  cumulatively on 4096..16384 because max blocks per call was 1.
  active_low_water=256 behaved like eager and is not viable.

third read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174116
  max16 quiescent retired 192 blocks / 384 MiB cumulatively without increasing
  source_alloc, but still paid release cost in the timed path.

production shape:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174140
  stats-off repeat-5:
    4096..16384 selected 32.246M / 93.88 MiB
    cold_retire 30.679M / 94.00 MiB
    cold_retire_max16 27.628M / 93.88 MiB
    cold_retire_eager 9.979M / 93.12 MiB
  decision:
    keep HZ6_MIDPAGE_32K_COLD_RETIRE_L1 as a default-off control/no-go for
    selected. Free-path source-block release does not improve maxRSS enough
    and release cost is visible even when source churn is avoided.
```

## Recent Closeout: HZ6 Preload MidPage Direct Class-L1

```text
goal:
  Keep attacking the selected Ubuntu LD_PRELOAD lane, but avoid reopening broad
  malloc-path preclassification. Test only the already-isolated MidPage preload
  boundary helper.

design:
  Add HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1.
  In hz6_allocator_preload_midpage_malloc_skip_transfer(), classify directly:
    4097..8192   -> HZ6_MIDPAGE_8K_CLASS_ID
    8193..32768  -> HZ6_MIDPAGE_32K_CLASS_ID
  Fall back to hz6_malloc outside the boundary.
  Keep the generic hz6_midpage_policy_for_size() path when the flag is off.

important distinction:
  This is not the older broad preclassified malloc no-go lane.
  The preload hook has already checked the MidPage boundary before calling this
  helper, so the selected change removes policy-struct work only from the
  narrow LD_PRELOAD MidPage path.

evidence:
  A/B stats-on safety:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212243
    safety counters clean; diagnostic throughput was mixed and is not the
    promotion gate.

  A/B production repeat-9:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212258
    all focused/fixed rows improved.

  A/B production repeat-15:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212317
    16..256      selected 56.549M -> direct_class 56.946M
    16..4096     selected 35.463M -> direct_class 35.874M
    1024..4096   selected 32.908M -> direct_class 32.926M
    4096..16384  selected 43.285M -> direct_class 44.309M
    fixed_4k     selected 31.174M -> direct_class 31.903M
    fixed_8k     selected 41.374M -> direct_class 42.503M
    fixed_16k    selected 43.362M -> direct_class 44.061M

promotion:
  HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1 is selected/default in
  linux/hz6_preload_flags.sh.

post-promotion selected-only checks:
  production repeat-5:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212533
    16..256      58.141M / 30.62 MiB
    16..4096     35.805M / 79.62 MiB
    1024..4096   33.437M / 91.00 MiB
    4096..16384  44.720M / 94.25 MiB
    fixed_4k     32.257M / 91.75 MiB
    fixed_8k     41.978M / 93.12 MiB
    fixed_16k    44.608M / 93.12 MiB

  stats-on safety:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212542
    fail=0 on all focused/fixed rows.

post-promotion cross refresh:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_212811
  16..256:
    hz6 53.758M / 30.50 MiB
    hz3 236.104M / 6.75 MiB
    hz4 208.664M / 7.00 MiB
    tcmalloc 240.099M / 9.38 MiB
    read: HZ6 beats mimalloc, but tiny remains an architecture mismatch.
  16..4096:
    hz6 34.858M / 79.62 MiB
    hz4 46.646M / 59.12 MiB
    tcmalloc 75.857M / 41.25 MiB
    read: HZ6 is stable but not the speed/RSS frontier here.
  1024..4096:
    hz6 31.574M / 90.88 MiB
    hz4 42.840M / 53.25 MiB
    tcmalloc 72.726M / 49.25 MiB
    read: mid-small remains the biggest gap.
  4096..16384:
    hz6 40.833M / 94.12 MiB
    hz3 51.152M / 73.50 MiB
    hz4 26.301M / 114.12 MiB
    tcmalloc 32.849M / 107.50 MiB
    read: HZ6 is a strong balanced target lane; it beats HZ4/tcmalloc/mimalloc
      on the MidPage target row, but HZ3 remains the speed/RSS ceiling.

next:
  With active-map capacity/probe/layout, cold-retire behavior, source-run reuse,
  and broad malloc/free shortcuts mostly closed, prefer one of:
    1. source/payload residency shape for fixed 8K/16K RSS polish
    2. mid-small 16..4096 / 1024..4096 attribution refresh
    3. narrow preload boundary/code-shape audit outside active-map/free-path
```

## Recent Closeout: HZ6 Frontcache Class4/Class5 Storage Trim-L1

```text
goal:
  Re-test frontcache storage trim after direct_class changed the selected
  preload code shape. Earlier broad trim attempts were control/no-go, but the
  current selected row has lower hot-path overhead and different guard balance.

selected change:
  HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1
  HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY=8192
  HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY=4096

evidence:
  broad production sweep:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213251
    mid32k_cap3072 helped target slightly but destroyed fixed_16k.
    storage_trim_c4_8192_c5_4096 was the only balanced candidate.

  production repeat-15:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213336
    16..256      selected 57.097M -> trim 57.137M
    16..4096     selected 33.998M -> trim 35.676M
    1024..4096   selected 33.232M -> trim 33.731M
    4096..16384  selected 44.144M -> trim 44.174M
    fixed_4k     selected 31.220M -> trim 31.306M
    fixed_8k     selected 41.790M -> trim 41.925M
    fixed_16k    selected 43.952M -> trim 44.358M

  stats-on safety/attribution:
    raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213400
    fail counters 0.
    frontcache table attribution 10242 KiB -> 3002 KiB.
    static table attribution 31609 KiB -> 24369 KiB.

  post-promotion selected-only:
    production raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213453
    stats raw: private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213504
    selected DSO keeps the trimmed attribution and fail counters clean.

  post-promotion cross refresh:
    raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_213629
    16..256:
      hz6 55.910M / 30.38 MiB
      tcmalloc 230.273M / 9.25 MiB
      read: HZ6 beats mimalloc but tiny remains out of scope for this lane.
    16..4096:
      hz6 34.828M / 79.62 MiB
      hz4 47.064M / 59.50 MiB
      tcmalloc 77.056M / 41.50 MiB
      read: mid-small speed/RSS gap remains.
    1024..4096:
      hz6 31.870M / 90.88 MiB
      hz4 42.076M / 52.62 MiB
      tcmalloc 77.413M / 48.75 MiB
      read: this is still the weakest selected-family comparison row.
    4096..16384:
      hz6 42.006M / 94.12 MiB
      hz3 52.193M / 73.38 MiB
      hz4 26.034M / 111.88 MiB
      tcmalloc 34.873M / 100.25 MiB
      read: target lane remains strong; HZ6 beats HZ4/tcmalloc/mimalloc on
        speed and RSS, while HZ3 remains the ceiling.

read:
  This is a small but broad selected/default win. It does not materially lower
  peak RSS because source payload and active pages still dominate peak reads,
  but it removes about 7 MiB of frontcache/static attribution and improves the
  mid-small rows that were weakest after the direct_class closeout.

next:
  Mid-small still deserves attribution work if continuing performance polish.
  Do not chase tiny first. Target row is already a strong HZ6 balance lane.
```

## Recent Closeout: HZ6 Preload Toy Direct Class-L1

```text
goal:
  Attack the weak mid-small rows without reopening broad hz6_malloc()
  preclassification. The prior toy_preclassified_malloc control helped some
  Toy rows but polluted target/fixed code shape.

implementation:
  Add HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 as default-off control.
  Add hz6_allocator_preload_toy_malloc_direct_class() and call it only from
  the LD_PRELOAD malloc boundary when size <= 4096.
  Keep helper noinline and use an unlikely hook branch so larger selected rows
  keep their preferred layout.

evidence:
  first production repeat-9:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_214226
    16..256      selected 57.027M -> preload_toy 60.376M
    16..4096     selected 35.690M -> preload_toy 37.665M
    1024..4096   selected 32.640M -> preload_toy 34.986M
    4096..16384  selected 44.877M -> preload_toy 44.658M
    fixed_4k     selected 30.999M -> preload_toy 33.568M
    fixed_8k     selected 42.615M -> preload_toy 42.501M
    fixed_16k    selected 44.699M -> preload_toy 44.226M

  noinline+unlikely repeat-9:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_214404
    16..256      selected 55.940M -> preload_toy 60.712M
    16..4096     selected 35.841M -> preload_toy 37.122M
    1024..4096   selected 33.368M -> preload_toy 35.093M
    4096..16384  selected 44.073M -> preload_toy 44.273M
    fixed_4k     selected 31.393M -> preload_toy 33.208M
    fixed_8k     selected 42.381M -> preload_toy 41.864M
    fixed_16k    selected 44.738M -> preload_toy 43.917M

decision:
  Keep as control/no-go for selected default.
  The mid-small gain is real and useful diagnostic evidence, but selected
  default still needs fixed_8k/fixed_16k and target-family guard stability.

worker follow-up:
  Worker proposed capped Toy direct-class and class4-only frontcache storage
  ladders as the two safest next checks.

  capped Toy direct-class / class4 storage ladder:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_214901
    variants:
      preload_toy_direct_class_max1024
      preload_toy_direct_class_max2048
      preload_toy_direct_class_max3072
      storage_trim_c4_12288_c5_4096
      storage_trim_c4_16384_c5_4096
    read:
      Toy caps still improve 16..4096 and 1024..4096, but all caps regressed
      4096..16384 versus selected in this run.
      Wider class4 storage did not beat the selected c4=8192/c5=4096 balance
      and weakened target/fixed guards.
    decision:
      keep all as controls/no-go for selected default.

next:
  Do not pursue broad Toy preclassification further unless a profile-specific
  DSO/lane is explicitly split. Continue with narrow attribution or a different
  mid-small mechanism that does not tax fixed 8K/16K.
```
