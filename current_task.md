# Current Task

This root file is the short project orientation ledger. Do not add chronological
benchmark logs here.

## Active Work

```text
Primary family:
  HZ6 Windows/Linux allocator development and benchmarking

Current Ubuntu direction:
  Keep selected/default stable.
  ToyTrustedDefault-L1 is now selected/default; continue measured profile or
  narrow code-shape optimization from HZ6 docs.
  Current preferred profile lane:
    hz6-small-boundary-trusted-target

Current strength:
  HZ6 is strongest as a speed/RSS balance allocator.
  Ubuntu LD_PRELOAD malloc_trim support recovers quiescent current RSS well.
  Peak RSS is still mostly touched MidPage payload residency.
```

## Read First

```text
HZ6 active orientation:
  hakozuna-hz6/docs/current_task.md

HZ6 selected rows and comparisons:
  hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
  hakozuna-hz6/docs/HZ6_UBUNTU_SELECTED_BALANCE.md

HZ6 lane decisions:
  hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
  hakozuna-hz6/docs/HZ6_UBUNTU_PRELOAD_LANES.md

Repo/source hygiene:
  hakozuna-hz6/docs/HZ6_REPO_HYGIENE.md
  hakozuna-hz6/docs/HZ6_SOURCE_MODULARIZATION.md
```

## Archive Map

```text
Root archive index:
  docs/archive/README.md

HZ6 archive index:
  hakozuna-hz6/docs/archive/README.md

Detailed historical ledgers:
  docs/archive/current_task_2026-06-hz6_linux_history.md
  hakozuna-hz6/docs/archive/current_task_2026-06_history.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_pre_compaction.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_profile_quiescent_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_post_toytrusted_controls_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_adaptive_profile_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_calloc_profile_snapshot.md
```

## Rules

```text
Keep this file below 80 lines.
Put HZ6 decisions in hakozuna-hz6/docs/current_task.md or stable HZ6 docs.
Move long benchmark logs and chronological notes to archive/.
Large archived ledgers may exceed 3000 lines; active current_task files should
not.
Do not promote profile-only lanes into selected/default without focused,
fixed, stats/diagnostic, RSS, and cross-allocator guard evidence.
```
