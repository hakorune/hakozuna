# Current Task

This root file is the short project orientation ledger. Long chronological
HZ6/HZ5 experiment history was archived so the active task stays readable.

## Read First

```text
HZ6 stable lane docs:
  hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
  hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
  hakozuna-hz6/docs/HZ6_UBUNTU_PRELOAD_LANES.md
  hakozuna-hz6/docs/HZ6_REPO_HYGIENE.md
  hakozuna-hz6/docs/HZ6_SOURCE_MODULARIZATION.md

Source cleanup lane:
  docs/HZ3_SOURCE_MODULARIZATION_LANE.md

Full archived root history:
  docs/archive/current_task_2026-06-hz6_linux_history.md
```

## Current Focus

```text
HZ6 remains the active Windows/Linux implementation and benchmarking family.
HZ5 Linux remains profile-stabilized; new HZ5 work should not blur the HZ6
contract.

Source cleanup status:
  ./linux/audit_large_source_files.sh --top 20
  default source/script audit is clean after the HZ3/HZ6/Windows benchmark
  helper splits.

Next allocator work:
  continue from the HZ6 selected lane docs, not from this archived root ledger.
  Keep new benchmark rows mirrored into the HZ6 stable docs when promoted.
```

## Recent Cleanup Commits

```text
ce50edb Split Windows PowerShell benchmark helpers
f4ff246 Split Windows benchmark HZ6 stats helpers
e8b9d70 Split HZ3 scale part8 config helpers
61cdf18 Split HZ3 arena helpers
986634a Split HZ3 tcache slowpath helpers
a2b15cb Split HZ3 tcache helpers
```

## Rules

```text
Do not add long chronological benchmark logs to this root file.
Use current_task.md for orientation only.
Use family-specific docs for selected/default/no-go lane decisions.
Use docs/archive/current_task_2026-06-hz6_linux_history.md for old root notes.
```
