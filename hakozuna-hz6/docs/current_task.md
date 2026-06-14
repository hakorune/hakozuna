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

Next Ubuntu MidPage work should not try more transfer-skip code-shape tweaks
until a broader selected matrix confirms the promoted outer-guard noinline
boundary. Do not chase route fallback, deeper free probing, source-run-slot
route registration, broad malloc code-shape changes, or whole-helper free-cache
replacement first.

Use HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md as the implementation order for the next
MidPage pass. TransferProbeAudit-L1, target DSO, and guard-isolated helper
attempts are done. The final outer-guard noinline preload-boundary shape passed
the focused repeat-15 promotion guard.

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
