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
The confirmation lane now compares selected default against an explicit
boundary-off control DSO.
The latest Ubuntu selected balance matrix shows HZ6 is strongest on
4096..16384: faster and lower-RSS than HZ4, much stronger than mimalloc/system,
but still behind HZ3 and tcmalloc on the speed/RSS frontier.
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
Latest cross-allocator refresh after static table trim:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
  16..256     hz6 60.381M / 30.38 MiB
  16..4096    hz6 42.216M / 79.75 MiB
  1024..4096  hz6 39.672M / 91.00 MiB
  4096..16384 hz6 41.264M / 94.38 MiB
  On 4096..16384, HZ6 still trails tcmalloc speed
  (41.264M vs 44.812M) but now has lower RSS and better ops-per-MiB.
MidPage32KRun512-L1 is now selected/default for Ubuntu preload:
  HZ6_MIDPAGE_32K_RUN_BYTES 262144 -> 524288
  payload trim controls below 256K were no-go: RSS was flat while source_alloc
  increased and 4096..16384 slowed.
  confirm repeat-7 without stats:
    16..256      59.531M / 30.38 MiB -> 60.730M / 30.50 MiB
    16..4096     41.811M / 79.88 MiB -> 43.235M / 79.88 MiB
    1024..4096   40.856M / 91.12 MiB -> 41.568M / 91.00 MiB
    4096..16384  42.176M / 94.38 MiB -> 45.298M / 94.50 MiB
  safety repeat-3: route/source/alloc/fallback failures all 0
  raw: private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
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
  keep MidPage 32K run512 selected
  keep preclassified malloc shape out of source
  keep MidPage target DSO as selected/control alias

Next Ubuntu MidPage work should not try more transfer-skip code-shape tweaks;
the broader cross-allocator matrix already confirmed the promoted outer-guard
noinline boundary. Do not chase route fallback, deeper free probing,
source-run-slot route registration, broad malloc code-shape changes, or
whole-helper free-cache replacement first.

Next recommended optimization lanes:
  PreloadHookHotPathAudit-L1:
    implemented. The first short read shows 4096..16384 spends nearly every
    free() on a Toy active-map miss before the MidPage hit:
      free_toy_active_map_attempt=406801
      free_toy_active_map_hit=39
      free_toy_active_map_miss=406762
      free_midpage_active_map_hit=405883
    ToyActiveMapAddrEnvelope-L1 was added as a default-off control, but the
    first repeat-3 was not promotion-clean: tiny was slightly positive while
    1024..4096 and 4096..16384 were weak. Keep off for now.

  PreloadFreeMidPageFirst-L1:
    implemented as a default-off control.
    Read: it directly recovers part of the 4096..16384 Toy-miss wall, but it
    badly regresses Toy/tiny guard rows.
      16..256 median shape fell from about 29M to about 21M ops/s
      16..4096 fell from about 20M to about 16M ops/s
      1024..4096 fell from about 19M to about 15.5M ops/s
      4096..16384 rose from about 14.2M to about 15.4M ops/s
    Keep HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1 off in selected default. It remains
    useful evidence that a class-aware/free-hint gate could be valuable, but
    unconditional MidPage-first ordering is not selected-safe.

  PreloadFreeMidPageHintGateDryRun-L1:
    implemented as diagnostic-only recent-envelope dry-run.
    Result:
      4096..16384 coverage is high:
        mh_would=406799, mh_true=405878, mh_false=39
      16..256 is clean:
        mh_would=4, mh_true=4, mh_false=0
      16..4096 and 1024..4096 are not clean:
        16..4096 mh_would=252022, mh_true=31, mh_false=251906
        1024..4096 mh_would=260955, mh_true=40, mh_false=260851
    Decision: do not behaviorize the broad recent envelope. It proves a
    selective MidPage-first gate needs a tighter range/page table or source-run
    side hint, not a simple min/max envelope.

  PreloadFreeMidPagePageHintDryRun-L1:
    implemented as a default-off diagnostic page-hint table for selective
    MidPage-first free ordering. Capacity32768 was much cleaner than the broad
    min/max envelope, reducing guard false positives from hundreds of thousands
    to hundreds while covering most 4096..16384 MidPage frees. Keep as
    diagnostic evidence only; see HZ6_UBUNTU_PRELOAD_FREE_HINT_CLOSEOUT.md.

  PreloadFreeMidPagePageHintFirst-L1:
    implemented as a default-off behavior control and closed as no-go. It
    reduced target Toy active-map attempts, but short repeat-5 regressed every
    focused row because the all-free hint probe/table overhead outweighed the
    saved Toy-miss wall. Keep off.

  Next MidPage/free direction:
    do not add another per-free classification probe. Next work should use
    decision data already available on the hot free path, or attach a
    source-run/class/front hint to existing active-map or descriptor flows.

  MidPageActiveMapFreeFastSlot-L1:
    implemented as a default-off control and closed as no-go for the selected
    preload shape. It stays inside the existing MidPage active-map try_free path
    and tests the base hash slot before entering the bounded probe loop. Short
    repeat-5 was not better:
      16..256 median 46.777M -> 47.014M
      16..4096 median 27.289M -> 27.095M
      1024..4096 median 25.017M -> 24.945M
      4096..16384 median 26.295M -> 25.422M
    Decision: keep off. Even without a new classifier, this free-path code
    shape does not improve the target.

  Next direction:
    pause MidPage free-order/free-map code-shape attacks. The recent lanes all
    show that the target can expose Toy/MidPage ordering pressure, but the
    control overhead beats the saved path. Prefer a malloc/source/frontcache
    lane or a broader selected-balance rerun before another free-path edit.

  FrontcacheCapacityShapeAudit-L1:
    closed as diagnostic/control for now. Class-specific MidPage cap behavior
    did not pass promotion; keep the class-max attribution for future lazy
    storage design.

  MidPagePayloadTrimAudit-L1:
    closed. Smaller 32K runs are no-go for now; 512K is selected for speed.

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
