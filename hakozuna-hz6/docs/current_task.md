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
  keep preclassified malloc shape out of source
  keep MidPage target DSO as selected/control alias

Next Ubuntu MidPage work should not try more transfer-skip code-shape tweaks;
the broader cross-allocator matrix already confirmed the promoted outer-guard
noinline boundary. Do not chase route fallback, deeper free probing,
source-run-slot route registration, broad malloc code-shape changes, or
whole-helper free-cache replacement first.

Next recommended optimization lanes:
  AllocatorStaticTableTrimAudit-L1:
    check whether preload can avoid multiplying full descriptor/route/source/
    frontcache tables for helper allocators or cold per-thread allocators.

  FrontcacheCapacityShapeAudit-L1:
    test narrower frontcache table capacity or class-specific caps against
    16..4096 / 1024..4096 / 4096..16384 speed and RSS.

  MidPagePayloadTrimAudit-L1:
    only after table/frontcache fixed cost is understood; 32K source payload
    is real on 4096..16384, but earlier larger-run attempts regressed locality.

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
The root repository source/script large-file audit is clean:
  ../../linux/audit_large_source_files.sh --top 20

Do not append long run logs here. Promote stable conclusions into the focused
HZ6 docs and move raw chronological evidence to archive docs.
```
