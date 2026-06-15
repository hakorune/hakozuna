# HZ6 Archive Index

This directory stores long HZ6 chronological ledgers. Active HZ6 work should
start from `../current_task.md`, then follow the stable lane docs linked there.

## Files

| File | Use |
| --- | --- |
| `current_task_2026-06_history.md` | Older HZ6 implementation and optimization chronology. Large by design. |
| `current_task_2026-06-16_pre_compaction.md` | Detailed Ubuntu HZ6 optimization ledger immediately before `../current_task.md` was compacted. Large by design. |
| `current_task_2026-06-16_profile_quiescent_snapshot.md` | Snapshot of the profile/alias/quiescent RSS ledger before the active HZ6 current task was shortened again. |
| `current_task_2026-06-16_toy_trusted_default_snapshot.md` | Evidence snapshot for promoting Toy direct-class fast reuse plus boundary trusted-owner into Ubuntu selected/default. |
| `current_task_2026-06-16_post_toytrusted_controls_snapshot.md` | Post-promotion retest snapshot for realloc-boundary adaptive, codegen/boundary, class5 controls, and worker follow-up candidates. |
| `current_task_2026-06-16_adaptive_profile_snapshot.md` | Profile-frontier, realloc copy attribution, and named adaptive realloc-boundary profile snapshot before active task compaction. |
| `current_task_2026-06-16_calloc_profile_snapshot.md` | Large-calloc real-fallback profile, calloc cross-allocator matrix, and adaptive profile decision snapshot. |

## Reading Order

```text
For current selected/default state:
  ../current_task.md
  ../HZ6_UBUNTU_PRELOAD_LANES.md
  ../HZ6_UBUNTU_SELECTED_BALANCE.md

For historical investigation:
  use this index first, then open only the specific archived ledger needed.
```

## Rule

```text
Do not append new benchmark logs here unless archiving a completed compaction
snapshot. Keep active decisions in the stable HZ6 docs and keep current_task.md
short.

Large files in this directory are intentional historical ledgers. If a
current_task-style file grows past a few hundred lines outside docs/archive/,
archive a dated snapshot here and replace the active file with a short
orientation ledger plus links.

When archiving:
  1. copy only the evidence needed to reconstruct the decision;
  2. keep raw result paths, key rows, and the selected/default decision;
  3. update this index and `../current_task.md`;
  4. do not duplicate long raw logs in active docs.
```
