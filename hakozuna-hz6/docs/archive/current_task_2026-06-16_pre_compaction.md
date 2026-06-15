# HZ6 Current Task Archive

Archived pre-compaction snapshot from 2026-06-16. This file intentionally keeps
the long chronological experiment ledger for source/reference lookup. Continue
active work from `../current_task.md` and the lane docs listed there.

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

## Current Continuation: MidPageDirectLocalReuseTrustedClass-L1

```text
latest continuation:
  Promote MidPage preload-boundary local-reuse success-path control to Ubuntu
  selected:
    HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1

design:
  The selected MidPage preload boundary already knows allocator/class and
  passes a descriptor out pointer.  The control keeps descriptor class,
  generation, and trusted-owner activation validation, but skips the generic
  direct-local-reuse entry checks for this known MidPage path.

acceptance:
  Passed focused/fixed speed and RSS guards in production and shared matrix
  shape.  Config default remains off; Ubuntu preload selected flags enable it.

raw:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004211
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004350
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004406
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004427
  private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_004725
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_004747
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_005109
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_005135
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_010900
  private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_010928
  private/raw-results/linux/hz6_ubuntu_size_slices_20260616_010948
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_010959

read:
  source-run route-after-maps retest:
    Existing runroute controls are not viable on current selected.  Stats raw
    showed runroute hit=0 and only fallback attempts, while RSS rose by roughly
    44-55 MiB on focused/fixed rows due to the additional source-block route
    indexes.  Keep this lane closed for default.

  repeat-15 no-stats:
    16..256      selected 57.358M, trusted-class 57.454M
    16..4096     selected 36.187M, trusted-class 35.978M
    1024..4096   selected 33.424M, trusted-class 33.752M
    4096..16384  selected 44.338M, trusted-class 45.576M
    fixed_4k     selected 31.693M, trusted-class 32.163M
    fixed_8k     selected 41.861M, trusted-class 43.045M
    fixed_16k    selected 44.036M, trusted-class 45.256M

  stats+diagnostics repeat-3:
    fail=0 and RSS/payload attribution is stable.  Diagnostics shape is not
    uniformly positive: 16..256 and 16..4096 regress, while 1024..4096 and
    fixed rows improve.

  selected-promotion repeat-15:
    16..256      selected 57.566M, trusted-class 57.894M
    16..4096     selected 36.221M, trusted-class 36.411M
    1024..4096   selected 33.566M, trusted-class 34.014M
    4096..16384  selected 44.515M, trusted-class 44.810M
    fixed_4k     selected 31.617M, trusted-class 32.206M
    fixed_8k     selected 42.358M, trusted-class 43.055M
    fixed_16k    selected 43.973M, trusted-class 44.947M

  selected-promotion shared matrices:
    focused matrix improved all rows:
      4096..16384 selected 44.497M, trusted-class 45.404M
    fixed matrix recovered the prior fixed_8k concern:
      fixed_8k selected 41.937M, trusted-class 43.368M
      fixed_16k selected 43.468M, trusted-class 44.926M

  final decision:
    Promote HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1 into the
    Ubuntu preload selected flags.
    Keep HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS=5 as a
    class5-only profile/control, not selected.
    Add build_hz6_preload_midpage_trusted_class_target.sh and aliases:
      hz6-midpage-trusted-class / hz6_midpage_trusted_class

  selected-balance matrix, repeat-3, 200K:
    16..256      selected 50.945M, trusted-class 50.946M
    16..4096     selected 28.853M, trusted-class 29.081M
    1024..4096   selected 26.517M, trusted-class 26.609M
    4096..16384  selected 34.451M, trusted-class 35.201M
    RSS was equal or slightly lower for trusted-class on focused rows.

  size-slices matrix, repeat-3, 200K:
    fixed_4k   selected 24.398M, trusted-class 24.987M
    fixed_8k   selected 33.164M, trusted-class 32.558M
    fixed_16k  selected 34.346M, trusted-class 34.744M

  class5-only split:
    Add min-class gate:
      HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS

    repeat-9 no-stats:
      16..256      selected 56.512M, class5-only 56.171M
      16..4096     selected 35.791M, class5-only 35.613M
      1024..4096   selected 33.719M, class5-only 33.174M
      4096..16384  selected 45.109M, class5-only 45.042M
      fixed_4k     selected 31.813M, class5-only 31.529M
      fixed_8k     selected 42.745M, class5-only 42.638M
      fixed_16k    selected 43.643M, class5-only 45.378M

    stats+diagnostics repeat-3:
      fail=0 and RSS/payload stable, but class5-only is not guard-clean under
      diagnostics and regresses target/fixed rows in that shape.

decision:
  Keep default-off for now, but retain as a promising selected-family control.
  It improves focused rows without raising RSS, but the fixed_8k matrix guard
  is not clean.  Keep the profile alias for broader comparison before any
  selected-default promotion.  The class5-only split is useful evidence for
  fixed_16k/profile work, but it is not a default candidate.
```

## Recent Continuation: MidPageBoundaryFused-L1

```text
latest continuation:
  Add default-off selected-boundary code-shape control:
    HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1=1

design:
  Keep the selected outer MidPage preload boundary and noinline isolation.
  For A/B only, resolve 8K/32K class in the preload boundary and call a narrow
  MidPage class helper.  This avoids rechecking the generic size policy inside
  the allocator helper, without changing transfer-skip semantics, route safety,
  active-map behavior, source retention, or selected default.

acceptance:
  Must pass selected-family tiny/mixed/target/fixed guards and RSS before any
  default discussion.  A target-only win is profile/control evidence, not a
  selected promotion.

raw:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004012
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_004031

read:
  repeat-7 no-stats:
    16..256      selected 57.206M, fused 56.395M
    16..4096     selected 36.094M, fused 36.211M
    1024..4096   selected 33.138M, fused 33.614M
    4096..16384  selected 45.013M, fused 44.177M
    fixed_4k     selected 31.858M, fused 32.360M
    fixed_8k     selected 42.513M, fused 42.161M
    fixed_16k    selected 44.566M, fused 45.303M

  repeat-3 stats+diagnostics:
    fail=0 and RSS/payload attribution unchanged, but fused is weaker under
    diagnostics on 1024..4096 and fixed rows.

decision:
  Keep as control/no-go for selected default.  The selected noinline boundary
  shape is already near the local optimum; class-fused helper shape changes
  code layout enough to lose target/fixed guard stability.
```

## Recent Continuation: PageKindFreeSelectorDryRun-L1

```text
latest continuation:
  Add diagnostic-only allocator-local page-kind selector:
    HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=1

  Add behavior control:
    HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1=1

design:
  Active-map registration records an advisory page kind:
    unknown / toy / midpage / mixed

  preload free probes the advisory table once and only compares it with the
  actual Toy/MidPage active-map hit.  It does not change free ordering, does
  not return route results, and does not bypass descriptor validation.

raw:
  private/raw-results/linux/hz6_page_kind_selector_dryrun_20260616_000420
  private/raw-results/linux/hz6_page_kind_selector_first_stats_20260616_000844
  private/raw-results/linux/hz6_page_kind_selector_first_prod_20260616_000902

read:
  repeat-3, stats+diagnostics, focused+fixed:
    wrong_toy_page_mid_hit=0
    wrong_midpage_page_toy_hit=0

  representative selector distribution:
    16..4096:
      probe=737475 toy=734782 midpage=12 mixed=255
      toy_hit=734482 midpage_hit=12 wrong=0
    4096..16384:
      probe=737319 toy=6 midpage=736514 mixed=600
      toy_hit=6 midpage_hit=735182 wrong=0
    fixed_4k:
      probe=748824 toy=694204 midpage=780 mixed=50673
      toy_hit=694127 midpage_hit=780 wrong=0

behavior result:
  Production repeat-9, no-stats/no-diagnostics:
    16..4096:
      selected 36.128M -> page_kind_first 33.663M
    1024..4096:
      selected 33.392M -> page_kind_first 31.086M
    4096..16384:
      selected 44.447M -> page_kind_first 39.926M
    fixed_4k:
      selected 31.259M -> page_kind_first 29.784M
    fixed_8k:
      selected 42.058M -> page_kind_first 39.514M
    fixed_16k:
      selected 43.797M -> page_kind_first 41.221M

decision:
  Close PageKind selector behavior as control/no-go for default.  The
  classifier is accurate, but a page-kind lookup on every free plus the extra
  allocator-local table is more expensive than the saved wrong first
  active-map probe.  Keep dry-run counters as evidence only.
```

## Current Continuation: Narrow Preload Code-Shape Controls

```text
latest follow-up:
  SmallBoundaryFastTarget-L1:
    Add a profile DSO candidate that combines the existing small-boundary
    profile with:
      HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
      HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1=1

    raw:
      private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_002421
      private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_002519
      private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_002549

    repeat-15, no-stats, include-tiny:
      16..256      selected 55.491M -> fast profile 76.683M
      16..4096     selected 34.920M -> fast profile 41.712M
      1024..4096   selected 32.236M -> fast profile 38.243M
      4096..16384  selected 43.406M -> fast profile 44.843M
      fixed_4k     selected 30.347M -> fast profile 45.505M
      fixed_8k     selected 41.474M -> fast profile 44.449M
      fixed_16k    selected 43.544M -> fast profile 44.233M

    stats safety:
      fail=0 on selected and fast profile across tiny/focused/fixed rows.
      Diagnostics are not used for speed ranking because raw-push intentionally
      falls back to the wrapper path when stats/diagnostics are active.

    decision:
      Keep selected default unchanged.  Promote this only as a named
      profile/control DSO for small/fixed-boundary workloads.

    cross-allocator position:
      raw:
        private/raw-results/linux/hz6_ubuntu_size_slices_20260616_003055
        private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_003133

      focused repeat-5, no-stats:
        16..256:
          hz6 selected 57.385M / 30.50 MiB
          fast profile 76.473M / 30.50 MiB
          tcmalloc 224.336M / 9.38 MiB
          hz3 236.598M / 6.75 MiB

        16..4096:
          hz6 selected 35.245M / 79.50 MiB
          fast profile 40.055M / 80.00 MiB
          hz4 46.915M / 59.12 MiB
          tcmalloc 80.710M / 41.25 MiB

        1024..4096:
          hz6 selected 31.777M / 91.00 MiB
          fast profile 38.417M / 91.50 MiB
          hz4 42.886M / 52.88 MiB
          tcmalloc 75.718M / 49.50 MiB

        4096..16384:
          hz6 selected 42.746M / 94.25 MiB
          fast profile 45.235M / 94.25 MiB
          hz3 50.581M / 73.38 MiB
          tcmalloc 34.500M / 94.25 MiB

      fixed repeat-5:
        fixed_4k:
          fast profile 45.722M / 92.75 MiB
          tcmalloc 43.398M / 69.50 MiB
          hz3 59.154M / 68.38 MiB
        fixed_8k:
          fast profile 43.383M / 93.00 MiB
          tcmalloc 28.091M / 71.38 MiB
          hz3 53.661M / 69.88 MiB
        fixed_16k:
          hz6 selected 44.584M / 93.12 MiB
          fast profile 43.969M / 93.12 MiB
          hz3 43.317M / 73.12 MiB

      read:
        Fast profile is now a strong named HZ6 lane for small/fixed-boundary
        workloads. It beats selected HZ6 on most target rows and beats
        tcmalloc/HZ4/mimalloc on 4096..16384 and fixed_8k/fixed_16k speed.
        fixed_4k also beats tcmalloc speed, but HZ3 remains the best
        speed/RSS frontier for tiny/small and most fixed rows.

latest continuation:
  Add default-off controls:
    HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
    HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1=1

  Extend existing Toy active-map mask-index control so probe wrap also uses the
  mask helper instead of only masking the base index.

raw:
  private/raw-results/linux/hz6_preload_boundary_trusted_owner_20260616_001246
  private/raw-results/linux/hz6_preload_boundary_trusted_owner_safety_20260616_001306
  private/raw-results/linux/hz6_preload_boundary_trusted_owner_guard_20260616_001324
  private/raw-results/linux/hz6_preload_boundary_trusted_owner_tiny_20260616_001347
  private/raw-results/linux/hz6_preload_boundary_trusted_owner_tiny_repeat_20260616_001406
  private/raw-results/linux/hz6_toy_mask_wrap_ab_20260616_001459
  private/raw-results/linux/hz6_raw_frontcache_push_ab_20260616_001644
  private/raw-results/linux/hz6_raw_frontcache_push_safety_20260616_001706
  private/raw-results/linux/hz6_raw_frontcache_push_guard_20260616_001724

read:
  preload boundary trusted-owner:
    repeat-15 focused/fixed looked useful on target/fixed:
      4096..16384 41.446M -> 43.300M
      fixed_4k     30.819M -> 31.406M
      fixed_8k     40.770M -> 41.547M
      fixed_16k    43.473M -> 43.726M
    but tiny repeat-15 failed:
      16..256      56.457M -> 54.229M

  Toy mask-wrap:
    repeat-9 mixed:
      16..256      56.876M -> 56.988M
      16..4096     35.688M -> 35.867M
      1024..4096   33.537M -> 33.113M
      4096..16384  44.598M -> 44.185M
    The full mask shape still loses the mid-small/target guards.

  raw frontcache push:
    repeat-9 was all-positive, but repeat-15 failed the target:
      4096..16384  44.689M -> 42.174M
    safety stats kept fail=0.

decision:
  Keep all three as controls/no-go for selected default.  The useful signal is
  that trusted-owner and raw-push can move fixed rows, but each still has a
  selected-family guard failure.  Do not promote without a narrower profile or
  code-layout isolation.
```

## Current Continuation: Narrow Toy Direct-Class Gates

```text
latest:
  Add reusable runner controls:
    preload_toy_direct_class_max256
    preload_toy_direct_class_max512
    preload_toy_direct_class_fast_reuse_max256
    preload_toy_direct_class_fast_reuse_max512

raw:
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_003343
  private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_003438

read:
  selected diagnostic attribution:
    Toy direct-class eligible is still large, but selected does not enter it:
      16..4096    eligible 1220604, enter 0
      1024..4096  eligible 1220382, enter 0
      fixed_4k    eligible 1220379, enter 0

  repeat-9 no-stats:
    max256:
      16..256      57.121M -> 61.495M
      16..4096     34.888M -> 31.062M
      1024..4096   33.414M -> 32.733M
      4096..16384  44.524M -> 43.852M

    fast_reuse_max512:
      16..256      57.121M -> 75.462M
      16..4096     34.888M -> 36.270M
      1024..4096   33.414M -> 33.964M
      fixed_8k     41.565M -> 42.454M
      fixed_16k    43.454M -> 44.720M
      but 4096..16384 44.524M -> 38.024M

decision:
  Keep selected default unchanged.  Narrow Toy direct-class gates are useful
  evidence for profile lanes, but the selected-family target guard still
  rejects them.  The small-boundary-fast DSO remains the right place for the
  Toy direct fast-reuse family.
```

## Recent Closeout: HZ6 Ubuntu RealAlignedFreeSkip-L1

```text
latest continuation:
  Extend bench/bench_aligned64k.c with a mode argument:
    posix   -> posix_memalign/free
    aligned -> aligned_alloc/free

  Add:
    hakozuna-hz6/linux/run_hz6_preload_aligned_wrapper_audit.sh

  Add default-off behavior control:
    HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1=1

design:
  posix_memalign/aligned_alloc requests with alignment > 16 still use the real
  allocator.  The control records successful real aligned fallback pointers in
  a small global hash table.  free() checks the table only while its active
  atomic count is nonzero; on hit, it removes the pointer and calls real free
  directly, skipping the HZ6 route lookup.

aligned evidence:
  raw: private/raw-results/linux/hz6_preload_aligned_wrapper_audit_20260615_224612
  repeat-3, 2K iters/thread, 4 threads:
    posix64_2k:
      selected ~= 14.2K ops/s
      phase_count_on free_route_miss_real=24000
      aligned_free_skip ~= 7.24M ops/s
      skip_hit=24000, free_route_miss_real=0
    aligned4k_4k:
      selected ~= 14.1K ops/s
      phase_count_on free_route_miss_real=24000
      aligned_free_skip ~= 8.03M ops/s
      skip_hit=24000, free_route_miss_real=0
    posix8k_64k:
      selected ~= 14.3K ops/s
      phase_count_on free_route_miss_real=24000
      aligned_free_skip ~= 8.11M ops/s
      skip_hit=24000, free_route_miss_real=0

selected guard:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_224657
  selected vs aligned_free_skip, repeat-9, 300K, focused+fixed:
    16..256       57.285M -> 56.395M
    16..4096      35.953M -> 36.143M
    1024..4096    33.480M -> 33.440M
    4096..16384   44.639M -> 44.002M
    fixed_4k      31.567M -> 31.979M
    fixed_8k      42.633M -> 39.876M
    fixed_16k     43.890M -> 44.561M

decision:
  Keep RealAlignedFreeSkip as an aligned-heavy profile/control, not selected
  default.  It removes the catastrophic HZ6 route miss on real aligned fallback
  pointers, but the extra free-side atomic gate still weakens fixed_8k enough
  to reject broad default promotion.

lane plumbing:
  Add persistent profile builder:
    hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh
    output:
      hakozuna-hz6/out/linux/hz6_preload_aligned_target/libhakozuna_hz6_preload.so
  Add shared allocator aliases:
    hz6-aligned-target / hz6_aligned_target
  run_linux_bench_compare_matrix.sh now builds the profile DSO when the
  allocator list includes either alias.
  alias smoke:
    raw: private/raw-results/linux/hz6_aligned_target_alias_smoke_20260615_225209
    result:
      bench_common.sh resolves hz6-aligned-target to
      out/linux/hz6_preload_aligned_target/libhakozuna_hz6_preload.so.
  matrix alias smoke:
    raw: private/raw-results/linux/hz6_aligned_target_matrix_alias_smoke_20260615_225209
    result:
      run_linux_bench_compare_matrix.sh built the aligned target DSO and
      forwarded hz6-aligned-target through the shared compare runner.
```

## Recent Closeout: HZ6 Ubuntu Codegen/Realloc Recheck-L1

```text
latest continuation:
  Add runner variant:
    realloc_in_place_off

  Recheck preload codegen controls:
    boundary_inline
    no_semantic_interposition
    opt_o3_no_semantic_interposition

worker read:
  The best default-looking follow-up was realloc capacity-in-place.  Source
  review shows HZ6 descriptors already store slot capacity in descriptor->bytes
  for source-run/Toy/MidPage allocations, not the original requested bytes.
  Therefore the current HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1 already performs the
  safe capacity in-place rule; remaining copies are class/slot-boundary growth
  such as 4K->4112, 8K->8208, or foreign/real fallback cases.

codegen A/B:
  raw: private/raw-results/linux/hz6_codegen_boundary_ab_20260615_225540
  selected vs codegen controls, no-stats, repeat-7, 300K:
    4096..16384:
      selected 42.982M
      boundary_inline 43.946M
      no_semantic_interposition 44.369M
      opt_o3_no_semantic_interposition 44.443M
    fixed_16k:
      selected 44.050M
      boundary_inline 39.555M
      no_semantic_interposition 38.723M
      opt_o3_no_semantic_interposition 39.224M

decision:
  Do not default -fno-semantic-interposition or O3.  The target lift is real in
  this run, but the fixed_16k guard regression is too large.
  Keep selected -O2 and noinline MidPage boundary.

realloc guard:
  raw: private/raw-results/linux/hz6_realloc_in_place_guard_20260615_225828
  selected vs realloc_in_place_off, stats-on, repeat-5, 200K:
    16..4096:
      20.949M vs 19.668M
      realloc_in_place median 6254 vs 0
      copy_calls median 102 vs 6356
      copy_bytes median 0.13 MiB vs 12.45 MiB
    4096..16384:
      18.096M / 93.88 MiB vs 15.420M / 136.50 MiB
      realloc_in_place median 6345 vs 0
      copy_calls median 11 vs 6356
      copy_bytes median 0.09 MiB vs 62.72 MiB
    fixed_16k:
      17.988M / 93.12 MiB vs 16.001M / 109.12 MiB
      realloc_in_place median 6356 vs 0
      copy_calls median 0 vs 6356
      copy_bytes median 0.00 MiB vs 99.41 MiB

decision:
  Keep HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1 selected/default.
  Do not add a new realloc capacity-growth behavior: the safe capacity check is
  already represented by descriptor->bytes.  Boundary-growth copies require a
  different allocation policy, not a realloc hotfix.

follow-up diagnostic:
  Add realloc copy class attribution:
    realloc_copy_same_class
    realloc_copy_cross_class
    realloc_copy_boundary_toy_to_midpage
    realloc_copy_boundary_mid8_to_mid32
    realloc_copy_boundary_midpage_to_large

  Production selected keeps HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1, so the
  classification is compiled out unless stats/diagnostic runners preserve phase
  counters.

  raw: private/raw-results/linux/hz6_realloc_copy_class_audit_20260615_230209
  selected, stats-on, repeat-3, 200K:
    16..4096:
      copy_calls median 102
      same_class 0
      cross_class 102
      toy_to_midpage 27
    4096..16384:
      copy_calls median 11
      same_class 0
      cross_class 11
      mid8_to_mid32 11
    fixed_4k:
      copy_calls median 6356
      toy_to_midpage 6356
    fixed_8k:
      copy_calls median 6356
      mid8_to_mid32 6356
    fixed_16k:
      copy_calls median 0

decision:
  Remaining realloc copy pressure is boundary-crossing, not missed same-class
  in-place.  Any next behavior would be an allocation-policy/profile lane
  around boundary slack, not a realloc correctness shortcut.

boundary slack profile:
  Add default-off control:
    HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1=1
    HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1=1
    HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1=1

  behavior:
    malloc(4096) -> allocate from MidPage 8K slot
    malloc(8192) -> allocate from MidPage 32K slot
    The combined flag keeps both enabled for the existing profile DSO; the
    split flags allow 4K-only and 8K-only A/B without changing selected.

  Add persistent profile builder:
    hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh
    output:
      hakozuna-hz6/out/linux/hz6_preload_realloc_boundary_target/libhakozuna_hz6_preload.so

  Add shared allocator aliases:
    hz6-realloc-boundary-target / hz6_realloc_boundary_target

  production A/B:
    raw: private/raw-results/linux/hz6_realloc_boundary_slack_ab_20260615_230639
    selected vs realloc_boundary_slack, no-stats, repeat-7, 300K:
      fixed_4k:
        31.931M -> 44.525M
      fixed_8k:
        41.961M -> 44.491M
      4096..16384:
        44.047M -> 44.540M
      16..4096:
        35.910M -> 35.402M
      1024..4096:
        33.517M -> 33.132M
      fixed_16k:
        44.519M -> 44.212M

  cross-allocator fixed-row profile read:
    raw: private/raw-results/linux/hz6_realloc_boundary_cross_20260615_231203
    hz6 vs hz6-realloc-boundary-target vs hz3/hz4/tcmalloc,
    no-stats, repeat-5, 300K:
      fixed_4k:
        hz6 32.588M / 91.88 MiB
        hz6-realloc-boundary-target 46.037M / 92.62 MiB
        hz3 60.702M / 68.38 MiB
        hz4 14.322M / 63.25 MiB
        tcmalloc 43.006M / 70.50 MiB
      fixed_8k:
        hz6 41.908M / 93.12 MiB
        hz6-realloc-boundary-target 44.357M / 93.12 MiB
        hz3 54.584M / 70.00 MiB
        hz4 13.542M / 64.00 MiB
        tcmalloc 27.568M / 73.50 MiB
      fixed_16k:
        hz6 44.610M / 93.12 MiB
        hz6-realloc-boundary-target 44.553M / 93.12 MiB
        hz3 44.836M / 73.12 MiB
        hz4 11.185M / 63.88 MiB
        tcmalloc 12.966M / 99.09 MiB

  split-lane A/B:
    raw: private/raw-results/linux/hz6_realloc_boundary_split_ab_20260615_232000
    selected vs 4k-only vs 8k-only vs combined, no-stats, repeat-7, 300K:
      fixed_4k:
        selected 32.132M
        4k-only 46.185M
        8k-only 32.374M
        combined 46.052M
      fixed_8k:
        selected 42.637M
        4k-only 42.605M
        8k-only 44.396M
        combined 44.535M
      1024..4096:
        selected 33.404M
        4k-only 31.806M
        8k-only 33.898M
        combined 33.348M

  8k-only promotion guard:
    raw: private/raw-results/linux/hz6_realloc_boundary_8k_promotion_guard_20260615_232300
    selected vs 8k-only, no-stats, repeat-15, 300K:
      16..4096:
        36.212M -> 36.255M
      1024..4096:
        33.613M -> 34.008M
      4096..16384:
        44.533M -> 44.510M
      fixed_4k:
        32.247M -> 32.080M
      fixed_8k:
        43.004M -> 44.615M
      fixed_16k:
        44.348M -> 44.181M

  stats safety:
    raw: private/raw-results/linux/hz6_realloc_boundary_split_stats_20260615_232100
    stats-on repeat-3 confirms the split targets the intended copy wall:
      fixed_4k selected copy 19068 -> 0 under 4k-only
      fixed_8k selected copy 19068 -> 0 under 8k-only

  8k shape ladder:
    raw: private/raw-results/linux/hz6_realloc_boundary_8k_midpath_ab_20260615_233000
    raw: private/raw-results/linux/hz6_realloc_boundary_8k_shape_ladder_20260615_233700
    raw: private/raw-results/linux/hz6_realloc_boundary_8k_shape_guard_20260615_234100
    temporary controls tested branch-only, unlikely, likely, after-toy, and
    inside-MidPage-boundary placement.  Branch-only removed the fixed_8k win,
    confirming the win is allocation-policy/copy-wall removal rather than
    branch layout.  The shape variants did not pass guards:
      unlikely hurt 16..4096/fixed_4k in repeat-15
      after-toy hurt 4096..16384 and fixed_16k in repeat-15
      inside-MidPage-boundary hurt fixed_16k
      likely hurt target/fixed guards in repeat-7
    These temporary controls were not kept in source; keep only the clean
    split 4K/8K flags.

decision:
  Keep realloc-boundary slack as a fixed-boundary profile/control DSO, not
  selected default.  It is very strong for fixed_4k/fixed_8k realloc-growth
  rows, but the mixed-small and fixed_16k guards are not clean enough for the
  balanced default allocator.  As a profile, it beats tcmalloc on fixed_4k and
  fixed_8k speed, and leaves fixed_16k essentially unchanged; HZ3 still keeps
  the better speed/RSS frontier on fixed_4k/fixed_8k.

  The split read says 4K slack is the fixed_4k profile win but pollutes
  1024..4096.  8K slack is cleaner and improves fixed_8k, but repeat-15 still
  has small fixed_4k/fixed_16k/target negatives.  Keep both split flags as
  controls and do not default either yet.

small boundary combo profile:
  raw: private/raw-results/linux/hz6_toy_realloc_boundary_combo_20260615_235000
  selected vs toy_target vs realloc_boundary_target vs combo, no-stats,
  repeat-7, 300K:
    16..4096:
      selected 36.272M
      toy_target 37.704M
      realloc_boundary_target 36.114M
      combo 37.151M
    1024..4096:
      selected 33.772M
      toy_target 34.436M
      realloc_boundary_target 33.394M
      combo 34.439M
    4096..16384:
      selected 45.040M
      toy_target 44.884M
      realloc_boundary_target 44.954M
      combo 44.053M
    fixed_4k:
      selected 31.930M
      toy_target 33.237M
      realloc_boundary_target 43.929M
      combo 45.585M
    fixed_8k:
      selected 42.479M
      toy_target 42.259M
      realloc_boundary_target 44.632M
      combo 44.754M
    fixed_16k:
      selected 44.644M
      toy_target 45.167M
      realloc_boundary_target 44.126M
      combo 44.532M

  Add persistent profile builder:
    hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh
    output:
      hakozuna-hz6/out/linux/hz6_preload_small_boundary_target/libhakozuna_hz6_preload.so

  Add shared allocator aliases:
    hz6-small-boundary-target / hz6_small_boundary_target
  matrix alias smoke:
    raw: private/raw-results/linux/hz6_small_boundary_target_matrix_alias_smoke_20260615_235500
    result:
      run_linux_bench_compare_matrix.sh resolved hz6-small-boundary-target to
      out/linux/hz6_preload_small_boundary_target/libhakozuna_hz6_preload.so.

  cross-allocator profile read:
    raw: private/raw-results/linux/hz6_small_boundary_cross_20260616_000000
    hz6 vs hz6-small-boundary-target vs hz3/hz4/tcmalloc,
    no-stats, repeat-5, 300K:
      16..4096:
        hz6 36.250M / 79.62 MiB
        hz6-small-boundary-target 37.602M / 80.00 MiB
        hz3 67.134M / 53.38 MiB
        hz4 48.035M / 59.75 MiB
        tcmalloc 82.472M / 41.12 MiB
      1024..4096:
        hz6 33.775M / 91.00 MiB
        hz6-small-boundary-target 34.671M / 91.38 MiB
        hz3 62.465M / 63.00 MiB
        hz4 43.574M / 53.00 MiB
        tcmalloc 78.786M / 49.38 MiB
      fixed_4k:
        hz6 31.918M / 91.75 MiB
        hz6-small-boundary-target 45.696M / 92.75 MiB
        hz3 59.773M / 68.50 MiB
        hz4 14.184M / 61.25 MiB
        tcmalloc 42.226M / 70.50 MiB
      fixed_8k:
        hz6 42.747M / 93.25 MiB
        hz6-small-boundary-target 44.947M / 93.12 MiB
        hz3 55.236M / 70.00 MiB
        hz4 13.734M / 64.38 MiB
        tcmalloc 28.272M / 74.12 MiB
      4096..16384:
        hz6 43.767M / 94.25 MiB
        hz6-small-boundary-target 43.467M / 94.12 MiB
        hz3 51.808M / 73.50 MiB
        hz4 26.535M / 113.25 MiB
        tcmalloc 34.878M / 99.12 MiB

decision:
  Keep as profile/control, not selected default.  It is the strongest named
  profile so far for fixed_4k/fixed_8k while retaining Toy target's 16..4096
  and 1024..4096 gains, but it gives up too much on 4096..16384 to become the
  balanced default.  Cross-allocator comparison confirms the fixed_4k/fixed_8k
  profile is strong enough to beat tcmalloc on speed, but HZ3 still owns the
  speed/RSS frontier and tcmalloc/HZ4 remain ahead on mixed mid-small rows.
```

## Previous Closeout: HZ6 Ubuntu Preload Wrapper Attribution-L1

```text
latest continuation:
  Add diagnostic-only wrapper attribution for LD_PRELOAD surface calls:
    calloc_zero_bytes
    calloc size buckets
    posix_memalign calls / HZ6 path / real fallback
    posix_memalign alignment and size buckets
    aligned_alloc calls / HZ6 path / real fallback
    aligned_alloc alignment and size buckets

  Output lines:
    [HZ6_PRELOAD_WRAPPER_DETAIL]
    [HZ6_PRELOAD_WRAPPER_SIZE_DETAIL]

  Production shape:
    HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1 compiles these counters out.
    The selected malloc/free hot path remains unchanged.

smoke:
  temporary aligned-wrapper smoke under the diag DSO:
    posix_memalign_calls=1
    posix_memalign_real_fallback=1
    posix_memalign_align_17_64=1
    aligned_alloc_calls=1
    aligned_alloc_real_fallback=1
    aligned_alloc_align_65_4096=1
    calloc_calls=1
    calloc_zero_bytes=4096

mixed_ws phase-count attribution:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_223042
  phase_count_on, repeat-1, 20K, focused:
    wrapper aligned calls are zero on the current selected benchmark rows
    calloc and realloc attribution are visible:
      16..4096:
        calloc_calls=11
        calloc_zero_bytes=410944
        realloc_calls=640
        realloc_copy_calls=9
        realloc_copy_bytes=6144

decision:
  Keep this as diagnostic-only attribution.
  Do not behaviorize >16-byte aligned allocation yet; current selected rows do
  not exercise it.  Revisit only with real-app or aligned-allocation benchmark
  evidence showing material libc fallback and later free route-miss pressure.
```

## Previous Closeout: HZ6 Ubuntu `malloc_trim` Retain Flush-L1

```text
latest continuation:
  Add hz6_linux_mmap_retain_flush(size_t max_bytes).
  Wire LD_PRELOAD malloc_trim(size_t pad) to:
    1. scavenge HZ6 local-free descriptors
    2. flush Linux mmap retained mappings
    3. forward to real libc malloc_trim when present

  This is explicit trim API behavior only.  It adds no malloc/free hot-path
  branch and does not change peak RSS during the timed workload.

worker read:
  The next low-risk RSS lane is explicit trim/retain-cache flush, not another
  active-map or route shortcut.  The already-closed source-run route-after-maps
  lane produced zero hits and large RSS cost.

residency audit:
  raw: private/raw-results/linux/hz6_midpage_rss_audit_20260615_221838
  selected diagnostic, 200K:
    4096..16384:
      mid32 all-local-free payload = 354.00 MiB
      retire candidate payload = 354.00 MiB
      retain retained bytes = 0.00 MiB at snapshot
    fixed_16k:
      mid32 all-local-free payload = 520.00 MiB
      retire candidate payload = 520.00 MiB
    fixed_64k:
      retain retained bytes = 255.94 MiB

  read:
    Final all-local-free MidPage payload is highly recoverable at quiescence.
    Runtime cold-retire remains a control because it can create source churn
    during active rows; explicit trim is the safer default API boundary.

validation:
  stats+diagnostic raw:
    private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_222328
    selected vs selected_malloc_trim_before_rss, repeat-3, 200K:
      16..4096 current RSS:    79.62 -> 27.32 MiB
      1024..4096 current RSS:  90.75 -> 27.08 MiB
      4096..16384 current RSS: 94.12 -> 28.49 MiB
      fixed_16k current RSS:   93.38 -> 28.29 MiB
      payload attribution after trim stays about 0.25 MiB
      fail counters remain 0

  production-shape raw:
    private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_222345
    selected vs selected_malloc_trim_before_rss, repeat-3, 300K:
      16..4096 current RSS:    79.75 -> 27.14 MiB
      1024..4096 current RSS:  91.00 -> 27.10 MiB
      4096..16384 current RSS: 94.38 -> 28.32 MiB
      fixed_16k current RSS:   93.12 -> 28.26 MiB

decision:
  Keep malloc_trim as the standard quiescent RSS return API.
  The selected LD_PRELOAD allocator now returns both HZ6-local cached payload
  and Linux mmap retain-cache mappings on explicit trim.
```

## Previous Closeout: HZ6 Ubuntu Toy ActiveMap Free FastSlot After RawPop-L1

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

Toy target DSO:
  Add linux/build_hz6_preload_toy_target.sh.
  Add linux/run_hz6_preload_toy_target_ab.sh for direct selected-vs-profile
  DSO comparison.
  This builds selected preload plus:
    HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1
    HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=4096

  validation:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_220021
    production repeat-7, 300K, focused+fixed:
      16..256       56.457M -> 60.441M
      16..4096      35.364M -> 37.573M
      1024..4096    33.455M -> 34.507M
      fixed_4k      31.632M -> 33.147M
      4096..16384   44.997M -> 44.665M
      fixed_8k      42.309M -> 41.478M
      fixed_16k     44.739M -> 44.164M

  decision:
    Good profile DSO for Toy/mid-small workloads.
    Still not selected default because MidPage/fixed guards lose.

  direct DSO runner:
    raw: private/raw-results/linux/hz6_toy_target_preload_ab_20260615_220312
    production repeat-7, 300K, focused+fixed:
      16..256       +8.49%
      16..4096      +5.19%
      1024..4096    +4.10%
      fixed_4k      +4.68%
      fixed_8k      +0.15%
      4096..16384   -1.42%
      fixed_16k     -1.80%

  final read:
    The profile DSO is real and useful, but selected default remains the
    balanced DSO. Do not promote Toy direct-class into hz6_preload_flags.sh.

  lower-bound gate follow-up:
    Add HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES as a default-off control
    and runner variants:
      preload_toy_direct_class_min1025
      preload_toy_direct_class_min2049
      preload_toy_direct_class_min3073

    raw: private/raw-results/linux/hz6_toy_direct_class_min_gate_ab_20260616_001000
    production repeat-9, 300K, focused+fixed:
      16..4096:
        selected     36.234M
        direct       37.394M
        min1025      36.362M
        min2049      35.464M
        min3073      35.696M
      1024..4096:
        selected     33.577M
        direct       34.535M
        min1025      34.208M
        min2049      33.563M
        min3073      33.427M
      4096..16384:
        selected     44.811M
        direct       44.289M
        min1025      43.204M
        min2049      45.079M
        min3073      44.698M
      fixed_4k:
        selected     31.949M
        direct       33.156M
        min1025      33.117M
        min2049      33.530M
        min3073      33.582M
      fixed_8k:
        selected     42.417M
        direct       42.101M
        min1025      42.659M
        min2049      42.553M
        min3073      42.782M
      fixed_16k:
        selected     44.475M
        direct       44.385M
        min1025      44.560M
        min2049      44.624M
        min3073      44.683M

    read:
      The min gate does not create a selected-default path. min1025 keeps only a
      weak part of the mid-small win and is target-negative in this run.
      min2049/min3073 recover target/fixed guards better but give up the
      broad mid-small win. Keep the gate as a reusable control for profile
      shaping; do not promote it into selected.

  direct-class phase attribution:
    Add stats-only preload phase counters:
      malloc_toy_direct_class_eligible
      malloc_toy_direct_class_eligible_le1024
      malloc_toy_direct_class_eligible_1025_4096
      malloc_toy_direct_class_enter
      malloc_toy_direct_class_enter_le1024
      malloc_toy_direct_class_enter_1025_4096

    raw: private/raw-results/linux/hz6_toy_direct_phase_stats_20260616_002200
    stats+diagnostic repeat-3, 200K, focused+fixed:
      16..4096 selected:
        eligible_total=1220604
        eligible_1025_4096=918756
        toy4_fast_attempt=918756
        toy4_fast_hit=916392
      1024..4096 selected:
        eligible_total=1220382
        eligible_1025_4096=1219968
        toy4_fast_attempt=1219968
        toy4_fast_hit=1216893
      fixed_4k selected:
        eligible_total=1220379
        eligible_1025_4096=1220355
        realloc_toy_to_mid=19068

    read:
      The broad mid-small rows are almost entirely Toy class4 eligible after
      the preload boundary split. min1025 is structurally plausible for
      1024..4096 because it enters nearly all useful direct-class candidates,
      but production A/B still shows the selected guard balance is not clean.
      Future speed work should not repeat max/min gating as a default path;
      it needs a thinner local-run/page metadata route or a profile DSO.

  direct-class fast-reuse profile:
    Add HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 as a default-off
    control. When Toy direct-class is already enabled, it skips the generic
    direct-local alloc front/transfer gates and calls direct local reuse.

    production guard:
      raw: private/raw-results/linux/hz6_toy_direct_fast_reuse_guard_20260616_003500
      repeat-15, 300K, focused+fixed:
        16..4096      selected 35.946M -> fast 41.896M
        1024..4096    selected 33.332M -> fast 38.238M
        4096..16384   selected 44.208M -> fast 44.544M
        fixed_4k      selected 31.937M -> fast 36.024M
        fixed_8k      selected 41.038M -> fast 41.647M
        fixed_16k     selected 44.010M -> fast 44.189M

    promotion guard:
      raw: private/raw-results/linux/hz6_toy_direct_fast_reuse_key_guard_20260616_004400
      repeat-21, 300K, focused+fixed, selected temporarily set to fast:
        16..4096      old 35.900M -> fast 41.983M
        1024..4096    old 33.161M -> fast 37.760M
        4096..16384   old 43.713M -> fast 44.297M
        fixed_4k      old 31.955M -> fast 35.630M
        fixed_8k      old 42.450M -> fast 42.081M
        fixed_16k     old 44.423M -> fast 44.387M

    safety:
      raw: private/raw-results/linux/hz6_toy_direct_fast_reuse_safety_20260616_003800
      stats+diagnostic repeat-3 kept fail=0 on focused+fixed.

    decision:
      Do not select by default because fixed_8k/fixed_16k guards are not clean
      on the stronger repeat-21. Adopt it in profile DSOs:
        build_hz6_preload_toy_target.sh
        build_hz6_preload_small_boundary_target.sh

    DSO validation:
      raw: private/raw-results/linux/hz6_toy_target_fast_reuse_preload_ab_20260616_004700
      repeat-9, selected DSO vs toy-target DSO:
        16..256       +30.39%
        16..4096      +15.27%
        1024..4096    +14.90%
        4096..16384   -1.01%
        fixed_4k      +12.77%
        fixed_8k      -0.80%
        fixed_16k     +0.95%

  runner integration:
    Add shared allocator alias:
      hz6-toy-target / hz6_toy_target
    Add matrix build support when allocator list includes the profile alias.
    Harden run_hz6_preload_toy_target_ab.sh option parsing and README metadata
    with arch, git sha, and DSO sha256.
    alias smoke:
      raw: private/raw-results/linux/hz6_toy_target_alias_smoke_20260615_220800
      command:
        ./linux/run_linux_bench_compare.sh --allocators hz6-toy-target \
          --runs 1 --bench-args "1 1000 64 16 256" \
          --skip-build --skip-prepare-allocators
      result:
        hz6-toy-target resolved to out/linux/hz6_preload_toy_target and ran.
    matrix alias smoke:
      raw: private/raw-results/linux/hz6_toy_target_matrix_alias_smoke_20260615_220832
      result:
        run_linux_bench_compare_matrix.sh forwarded hz6-toy-target through the
        shared compare runner and resolved the profile DSO correctly.

PreloadMidPageRunRouteAfterMaps-L1:
  Add default-off controls:
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1
    runner variants:
      runroute_dryrun_after_maps
      runroute_after_maps

  intent:
    After Toy/MidPage active maps miss, try descriptor-validated source-run
    metadata before full RouteLayer lookup. Keep RouteLayer fallback on any
    miss/mismatch. Exclude Toy and accept only MidPage route results.

  diagnostic read:
    raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_221341
    selected vs dryrun, stats+diagnostics, 200K:
      16..4096:
        runroute_attempt=938
        runroute_hit=0
        runroute_fallback=938
        RSS 79.62 MiB -> 132.12 MiB
      1024..4096:
        runroute_attempt=1138
        runroute_hit=0
        runroute_fallback=1138
        RSS 90.88 MiB -> 145.50 MiB
      4096..16384:
        runroute_attempt=602
        runroute_hit=0
        runroute_fallback=602
        RSS 94.00 MiB -> 139.38 MiB
        runmeta detail:
          range_index_hit=602
          source_block_route_behavior_valid=0
          source_block_route_behavior_invalid_front=602

  decision:
    No-go for default and no need to run behavior promotion.
    The range index can find source blocks, but the source-run metadata is not
    valid as a preload free route after active-map miss. The lane adds large
    static/RSS cost and produces zero route hits.

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

## Recent Closeout: HZ6 Trusted-Class Selected Baseline Refresh

```text
goal:
  Fix the selected/default baseline after promoting
  HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1 and keep the speed/RSS
  read separate from profile DSOs.

focused cross refresh:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_011316
  repeat-3, bench_mixed_ws_crt, ws=4096, iters=300000

  16..256:
    hz6 58.047M / 30.25 MiB
    hz3 235.546M / 6.75 MiB
    tcmalloc 243.995M / 9.25 MiB

  16..4096:
    hz6 36.461M / 79.62 MiB
    hz3 67.823M / 53.25 MiB
    tcmalloc 82.383M / 40.50 MiB

  1024..4096:
    hz6 34.193M / 90.75 MiB
    hz3 61.867M / 63.00 MiB
    tcmalloc 76.482M / 49.12 MiB

  4096..16384:
    hz6 45.315M / 94.25 MiB
    hz3 51.230M / 73.38 MiB
    hz4 26.683M / 112.75 MiB
    tcmalloc 34.618M / 100.25 MiB

fixed-size cross refresh:
  raw: private/raw-results/linux/hz6_ubuntu_size_slices_20260616_011443
  repeat-3, bench_mixed_ws_crt, ws=4096, iters=300000

  fixed_4k:
    hz6 31.542M / 91.88 MiB
    hz3 60.786M / 68.50 MiB
    tcmalloc 41.821M / 69.62 MiB

  fixed_8k:
    hz6 43.506M / 93.25 MiB
    hz3 54.549M / 69.88 MiB
    tcmalloc 27.702M / 72.75 MiB

  fixed_16k:
    hz6 45.586M / 93.25 MiB
    hz3 43.766M / 73.12 MiB
    tcmalloc 12.375M / 99.84 MiB

read:
  HZ6 selected is now a strong balanced MidPage lane. It beats tcmalloc/HZ4 on
  4096..16384 speed while keeping lower RSS than tcmalloc, and fixed_16k now
  edges HZ3 on speed in this repeat-3 read. HZ3 remains the better speed/RSS
  frontier on tiny, fixed_4k, fixed_8k, and broad small/mid rows.

RSS read:
  The major RSS win is currently quiescent recoverability through the HZ6
  malloc_trim interpose/scavenge path:
    4096..16384 current RSS: 94.38 MiB -> 28.32 MiB
    fixed_16k current RSS:   93.12 MiB -> 28.26 MiB
  Peak RSS is not yet materially lower because touched MidPage source payload
  still dominates peak reads.

next:
  Continue from selected docs. Good next checks are refreshed attribution for
  mid-small/Toy gaps and a cautious retest of boundary/profile controls against
  the trusted-class selected baseline. Do not default cold-retire/free-path
  release behavior just because quiescent trim works; it is a different RSS
  mechanism and can tax hot frees.
```

## Recent Continuation: Realloc-Boundary Split Profile DSOs

```text
goal:
  Use the latest selected stats to decide whether fixed-boundary realloc-copy
  pressure is still worth a lane, without polluting selected/default.

diagnostic read:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_011918
  stats+diagnostics selected, repeat-3, iters=200000

  fixed_4k:
    realloc_copy=19068, realloc_toy_to_mid=19068
  fixed_8k:
    realloc_copy=19068, realloc_mid8_to_mid32=19068
  fixed_16k:
    realloc_copy=0

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_011939
  no-stats repeat-9, focused+fixed, iters=300000

  fixed_4k:
    selected 31.968M / 91.88 MiB
    realloc_boundary_slack_4k 46.758M / 92.75 MiB
    realloc_boundary_slack_8k 32.333M / 92.00 MiB
    combined 46.282M / 92.62 MiB

  fixed_8k:
    selected 43.156M / 93.12 MiB
    realloc_boundary_slack_4k 43.343M / 93.12 MiB
    realloc_boundary_slack_8k 45.203M / 93.12 MiB
    combined 44.348M / 93.00 MiB

  guards:
    16..4096 regressed for all slack variants.
    4096..16384 regressed for all slack variants, especially 8K/combined.

decision:
  Keep realloc-boundary slack out of selected/default. It remains a strong
  fixed-boundary profile lane, especially exact fixed_4k and fixed_8k rows.

implementation:
  Added split profile build wrappers:
    linux/build_hz6_preload_realloc_boundary_4k_target.sh
    linux/build_hz6_preload_realloc_boundary_8k_target.sh
  Added bench aliases:
    hz6-realloc-boundary-4k-target / hz6_realloc_boundary_4k_target
    hz6-realloc-boundary-8k-target / hz6_realloc_boundary_8k_target
  Added matrix auto-build hooks for the split profile DSOs.

validation:
  bash -n for build wrappers, matrix wrappers, and bench_common passed.
  git diff --check passed.
  Both profile DSOs built successfully.
  Alias smoke raw:
    private/raw-results/linux/hz6_ubuntu_size_slices_20260616_012248

next:
  Continue speed work from selected. Boundary trusted-owner and MidPage fused
  boundary were retested after trusted-class selection and remain no-go for
  selected because mixed/focused guards regress. The next default-safe work
  should avoid top-level boundary branches and all-free lookup tables.
```

## Recent Recheck: Current-Bias Fast Shape Still No-Go

```text
goal:
  Recheck a small selected-free-order code-shape control against the current
  trusted-class selected baseline before moving to larger work.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_012437
  no-stats repeat-7, focused+fixed, iters=300000

  16..256:
    selected 57.713M -> current_bias_fast 57.544M
  16..4096:
    selected 36.709M -> current_bias_fast 36.706M
  1024..4096:
    selected 33.785M -> current_bias_fast 33.462M
  4096..16384:
    selected 45.581M -> current_bias_fast 45.175M
  fixed_4k:
    selected 31.983M -> current_bias_fast 32.366M
  fixed_8k:
    selected 42.785M -> current_bias_fast 42.525M
  fixed_16k:
    selected 45.283M -> current_bias_fast 45.337M

decision:
  Keep HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1=0. The fixed_4k/fixed_16k
  nudge is not worth the 1024..4096 and 4096..16384 regressions.
```

## Recent Recheck: Raw Frontcache Push Class5 Still Profile-Only

```text
goal:
  Recheck the class5-only free raw-push code-shape control after trusted-class
  became part of selected, because the earlier read had a fixed_16k signal.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_012729
  no-stats repeat-15, focused+fixed+tiny, iters=300000

  16..256:
    selected 57.384M -> raw_frontcache_push_class5 56.625M
  16..4096:
    selected 35.915M -> raw_frontcache_push_class5 35.751M
  1024..4096:
    selected 33.334M -> raw_frontcache_push_class5 33.600M
  4096..16384:
    selected 45.745M -> raw_frontcache_push_class5 44.429M
  fixed_4k:
    selected 32.231M -> raw_frontcache_push_class5 32.165M
  fixed_8k:
    selected 42.963M -> raw_frontcache_push_class5 42.760M
  fixed_16k:
    selected 45.664M -> raw_frontcache_push_class5 46.034M

decision:
  Keep HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1=0 and keep class5 gating as
  profile/control only. The fixed_16k win is real enough to preserve the lane,
  but the 4096..16384 target loss is too large for selected default.
```

## Recent RSS Recheck: Quiescent Trim Holds, Peak Retire Deferred

```text
goal:
  Recheck the current RSS floor and compare it with cold-retire after the
  trusted-class selected baseline.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_012801
  stats+diagnostics repeat-3, focused+fixed, iters=200000, ws=4096

  selected vs selected_malloc_trim_before_rss current RSS:
    16..4096:    79.88 MiB -> 27.27 MiB
    1024..4096:  91.00 MiB -> 27.18 MiB
    4096..16384: 94.25 MiB -> 28.52 MiB
    fixed_4k:    92.00 MiB -> 28.37 MiB
    fixed_8k:    93.38 MiB -> 28.25 MiB
    fixed_16k:   93.38 MiB -> 28.38 MiB

  cold_retire_max16:
    4096..16384 payload attribution drops 399.25 -> 271.25 MiB and retires
    192 blocks / 12288 descriptors / 384.00 MiB, but current RSS remains
    94.25 MiB and diagnostic throughput drops 17.070M -> 16.759M.
    Fixed rows do not get a peak/current RSS win.

decision:
  Keep malloc_trim as the selected quiescent RSS recovery API. Do not promote
  cold-retire behavior for selected; peak RSS needs a different lane than
  final all-local-free reclamation.
```

## Recent Recheck: PageKind Free Selector Still No-Go

```text
goal:
  Recheck the page-kind free-order selector after trusted-class entered
  selected, because it is one of the few remaining free-order ideas.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_013204
  no-stats repeat-7, focused+fixed+tiny, iters=300000

  16..256:
    selected 57.687M
    page_kind_selector_dryrun 56.075M
    page_kind_selector_first 55.401M
  16..4096:
    selected 36.831M
    page_kind_selector_dryrun 34.582M
    page_kind_selector_first 34.543M
  1024..4096:
    selected 33.827M
    page_kind_selector_dryrun 32.337M
    page_kind_selector_first 32.136M
  4096..16384:
    selected 45.440M
    page_kind_selector_dryrun 41.751M
    page_kind_selector_first 40.060M
  fixed_4k:
    selected 31.931M
    page_kind_selector_dryrun 30.594M
    page_kind_selector_first 30.370M
  fixed_8k:
    selected 43.259M
    page_kind_selector_dryrun 40.414M
    page_kind_selector_first 39.541M
  fixed_16k:
    selected 45.057M
    page_kind_selector_dryrun 41.263M
    page_kind_selector_first 41.761M

decision:
  Keep HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=0 and
  HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1=0. Even dryrun loses everywhere and
  raises RSS by about 2.5 MiB, so page-kind all-free lookup is closed for
  selected/default.
```

## Recent Recheck: Toy Direct Fast-Reuse Still Profile-Only

```text
goal:
  Recheck Toy direct-class fast-reuse after trusted-class selected baseline,
  because it has the largest remaining tiny/mid-small upside.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_013426
  no-stats repeat-15, focused+fixed+tiny, iters=300000

  selected vs preload_toy_direct_class_fast_reuse:
    16..256:     57.499M -> 74.244M
    16..4096:    35.747M -> 41.596M
    1024..4096:  28.494M -> 37.729M
    4096..16384: 45.074M -> 44.378M
    fixed_4k:    31.685M -> 32.071M
    fixed_8k:    38.159M -> 37.318M
    fixed_16k:   44.259M -> 43.777M

  narrower max gates:
    max256/max512 improve some small/fixed guards but lose the selected
    target more clearly. They are not balanced default candidates.

decision:
  Keep Toy direct-class fast-reuse in the Toy/small-boundary profile DSOs,
  not selected default. The tiny/mid-small win is large, but the target and
  fixed 8K/16K guards still reject promotion.
```

## Recent Recheck: Same-Owner Trusted Free Still No-Go

```text
goal:
  Test the worker-suggested same-owner fast path with trusted local free. This
  skips the second owner-equality check only after hz6_free() has already
  proven local ownership.

implementation:
  Added runner variants:
    same_owner_trusted
    same_owner_trusted_class4
    same_owner_trusted_class5

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_013714
  no-stats repeat-15, focused+fixed+tiny, iters=300000

  selected vs same_owner_trusted:
    16..256:     57.892M -> 57.066M
    16..4096:    36.253M -> 35.870M
    1024..4096:  33.722M -> 33.698M
    4096..16384: 45.466M -> 44.775M
    fixed_4k:    32.069M -> 31.992M
    fixed_8k:    42.769M -> 43.172M
    fixed_16k:   45.472M -> 45.957M

  class gates:
    class4 is mostly flat/down.
    class5 helps 16..4096, 1024..4096, and fixed_8k, but still loses target
    and fixed_16k.

decision:
  Keep HZ6_SAME_OWNER_FAST_L1=0 and
  HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1=0 for selected/default. The duplicated
  owner check is not the primary remaining wall.
```

## Recent Lane Cleanup: Profile Alias Autobuild Hooks

```text
goal:
  Make the named HZ6 profile DSOs usable from the selected/fixed matrix
  runners without requiring manual pre-builds.

implementation:
  Added auto-build hooks to:
    linux/run_hz6_ubuntu_selected_balance_matrix.sh
    linux/run_hz6_ubuntu_size_slices_matrix.sh

  Newly covered aliases:
    hz6-toy-target / hz6_toy_target
    hz6-aligned-target / hz6_aligned_target
    hz6-small-boundary-target / hz6_small_boundary_target

  Existing profile hooks remain:
    hz6-small-boundary-fast-target / hz6_small_boundary_fast_target
    hz6-realloc-boundary-target / hz6_realloc_boundary_target
    hz6-realloc-boundary-4k-target / hz6_realloc_boundary_4k_target
    hz6-realloc-boundary-8k-target / hz6_realloc_boundary_8k_target
    hz6-midpage-trusted-class / hz6_midpage_trusted_class

validation:
  bash -n passed for both matrix runners.
  Raw smoke:
    private/raw-results/linux/hz6_profile_alias_autobuild_smoke_20260616_014024
  Command used selected-balance matrix with:
    --allocators hz6-toy-target,hz6-small-boundary-target,hz6-aligned-target
    --runs 1 --iters 10000 --ws 1024 --skip-prepare

  Smoke confirmed all three DSOs built and resolved through the shared compare
  runner:
    hz6-toy-target -> out/linux/hz6_preload_toy_target/...
    hz6-small-boundary-target -> out/linux/hz6_preload_small_boundary_target/...
    hz6-aligned-target -> out/linux/hz6_preload_aligned_target/...

decision:
  Profile lane ergonomics are now cleaner. Future cross-allocator reads can
  request named HZ6 profiles directly from the matrix runners.
```

## Recent Measurement: HZ6 Profile Position Refresh

```text
goal:
  Re-measure the named HZ6 profile DSOs through the selected/fixed matrix
  runners after profile alias autobuild hooks were added, and pin the current
  selected/profile split.

focused matrix:
  raw: private/raw-results/linux/hz6_profile_position_focused_20260616_014250
  command shape:
    run_hz6_ubuntu_selected_balance_matrix.sh
    --allocators hz6,hz6-toy-target,hz6-small-boundary-fast-target,
      hz6-realloc-boundary-4k-target,hz6-realloc-boundary-8k-target
    --runs 5 --iters 300000 --ws 4096 --skip-builds --skip-prepare

  selected vs best profile:
    16..256:     58.182M / 30.38 MiB -> small-boundary-fast 78.220M / 30.50 MiB
    16..4096:    36.874M / 79.62 MiB -> toy-target 43.649M / 79.50 MiB
    1024..4096:  34.045M / 90.88 MiB -> small-boundary-fast 39.610M / 91.50 MiB
    4096..16384: 45.518M / 94.25 MiB -> small-boundary-fast 46.164M / 94.12 MiB

fixed matrix:
  raw: private/raw-results/linux/hz6_profile_position_fixed_20260616_014402
  command shape:
    run_hz6_ubuntu_size_slices_matrix.sh
    --allocators hz6,hz6-toy-target,hz6-small-boundary-fast-target,
      hz6-realloc-boundary-4k-target,hz6-realloc-boundary-8k-target
    --rows fixed_mid --runs 5 --iters 300000 --ws 4096 --skip-builds --skip-prepare

  selected vs best profile:
    fixed_4k:  32.005M / 91.75 MiB -> small-boundary-fast 48.188M / 92.75 MiB
    fixed_8k:  43.184M / 93.12 MiB -> realloc-boundary-8k 45.960M / 93.12 MiB
    fixed_16k: 45.614M / 93.12 MiB -> small-boundary-fast 45.923M / 93.12 MiB

decision:
  HZ6 has a clear profile DSO story now. selected remains the balanced default;
  small-boundary-fast is the best general profile for tiny/mid-small/fixed_4k
  and remains target-safe in this read; realloc-boundary-8k is the exact
  fixed_8k profile. RSS stays roughly in the selected range, so these profiles
  mainly buy speed rather than peak-RSS reduction.
```

## Recent Cleanup: HZ6 Profile Alias Helper

```text
goal:
  Keep profile DSO lane wiring clean before adding more profile variants. The
  selected-balance and fixed-size matrix runners had duplicated dash/underscore
  alias build checks.

implementation:
  Added:
    linux/hz6_preload_aliases.sh

  Updated:
    linux/run_hz6_ubuntu_selected_balance_matrix.sh
    linux/run_hz6_ubuntu_size_slices_matrix.sh
    linux/run_hz6_midpage_payload_trim_ab.sh

  The matrix runners now call:
    hz6_preload_build_requested_aliases "$ALLOCATORS" "$ROOT_DIR"

  The A/B runner now accepts:
    small_boundary_fast
  as an alias for:
    small_boundary_raw_push_trusted_owner

validation:
  bash -n passed for the helper and all touched runners.
  git diff --check passed.
  Alias predicate smoke passed for dash/underscore aliases.

  Matrix source smoke:
    private/raw-results/linux/hz6_matrix_alias_helper_source_smoke_20260616_015014
    private/raw-results/linux/hz6_size_alias_helper_source_smoke_20260616_015021

  A/B alias smoke:
    private/raw-results/linux/hz6_small_boundary_fast_alias_smoke_20260616_014954

decision:
  This is hygiene only; selected flags and profile macro bundles are unchanged.
  Future profile DSOs should add one alias entry in hz6_preload_aliases.sh and
  the bench resolver, instead of duplicating build checks in each matrix runner.
```

## Recent Measurement: Small-Boundary Fast Component Split

```text
goal:
  Decompose the current small-boundary-fast profile to avoid guessing which
  profile-only controls are carrying the win.

A/B:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_014717
  no-stats repeat-9, focused+fixed+tiny, iters=300000

  selected vs notable profile components:
    16..256:
      selected 58.086M
      small_boundary_target 75.548M
      small_boundary_trusted_owner 77.104M
      small_boundary_fast 76.506M

    16..4096:
      selected 36.098M
      small_boundary_target 34.187M
      small_boundary_trusted_owner 40.278M
      small_boundary_fast 42.365M

    1024..4096:
      selected 33.109M
      small_boundary_target 38.127M
      small_boundary_trusted_owner 39.060M
      small_boundary_fast 39.063M

    4096..16384:
      selected 44.713M
      small_boundary_target 44.247M
      small_boundary_trusted_owner 44.227M
      small_boundary_fast 45.561M

    fixed_4k:
      selected 31.728M
      small_boundary_target 46.389M
      small_boundary_trusted_owner 47.440M
      small_boundary_fast 47.056M

    fixed_8k:
      selected 42.697M
      small_boundary_target 38.789M
      small_boundary_trusted_owner 42.525M
      small_boundary_fast 45.727M

    fixed_16k:
      selected 44.795M
      small_boundary_target 45.201M
      small_boundary_trusted_owner 45.438M
      small_boundary_fast 46.086M

decision:
  Raw-push alone is not the lever. The useful profile shape is the
  trusted-owner/raw-push combination: it carries target, fixed_8k, and fixed_16k.
  Trusted-owner-only is slightly better on fixed_4k, so a future exact fixed_4k
  profile can be considered, but selected/default remains unchanged.
```

## Recent Profile: Small-Boundary Trusted Target

```text
goal:
  Turn the trusted-owner-only small-boundary shape into a named profile DSO and
  compare it against selected, small-boundary-fast, and fixed_8k profile lanes.

implementation:
  Added:
    linux/build_hz6_preload_small_boundary_trusted_target.sh

  Added aliases:
    hz6-small-boundary-trusted-target
    hz6_small_boundary_trusted_target

  Alias wiring:
    linux/hz6_preload_aliases.sh
    bench/lib/bench_common.sh

build/smoke:
  build_hz6_preload_small_boundary_trusted_target.sh succeeded.
  alias smoke:
    private/raw-results/linux/hz6_small_boundary_trusted_alias_smoke_20260616_015323

focused profile position:
  raw: private/raw-results/linux/hz6_small_boundary_trusted_focused_20260616_015341

  selected vs trusted vs fast:
    16..256:
      selected 58.439M / 30.50 MiB
      trusted  78.065M / 30.50 MiB
      fast     77.954M / 30.50 MiB

    16..4096:
      selected 36.312M / 79.62 MiB
      trusted  42.438M / 79.88 MiB
      fast     43.031M / 80.00 MiB

    1024..4096:
      selected 33.111M / 90.88 MiB
      trusted  39.308M / 91.50 MiB
      fast     39.097M / 91.38 MiB

    4096..16384:
      selected 45.165M / 94.12 MiB
      trusted  45.215M / 94.25 MiB
      fast     45.033M / 94.25 MiB

fixed profile position:
  raw: private/raw-results/linux/hz6_small_boundary_trusted_position_20260616_015331

  selected vs trusted vs fast vs realloc8:
    fixed_4k:
      selected 32.529M / 91.88 MiB
      trusted  47.946M / 92.62 MiB
      fast     47.619M / 92.50 MiB
      realloc8 32.310M / 91.75 MiB

    fixed_8k:
      selected 43.820M / 93.12 MiB
      trusted  47.314M / 93.12 MiB
      fast     46.549M / 93.12 MiB
      realloc8 45.651M / 93.00 MiB

    fixed_16k:
      selected 45.896M / 93.00 MiB
      trusted  47.114M / 93.00 MiB
      fast     45.786M / 93.12 MiB
      realloc8 45.518M / 93.12 MiB

decision:
  Prefer hz6-small-boundary-trusted-target as the current broad small/fixed
  profile. It keeps the target row neutral, beats selected on focused small rows,
  and beats fast/realloc8 on all fixed_mid rows in this repeat. Keep selected
  default unchanged because this is still a profile macro bundle, not a
  selected-family behavior promotion.
```
