# HZ6 Archive Index

This directory stores long HZ6 chronological ledgers. Active HZ6 work should
start from `../current_task.md`, then follow the stable lane docs linked there.

## Files

| File | Use |
| --- | --- |
| `current_task_2026-06_history.md` | Older HZ6 implementation and optimization chronology. Large by design. |
| `current_task_2026-06-16_pre_compaction.md` | Detailed Ubuntu HZ6 optimization ledger immediately before `../current_task.md` was compacted. Large by design. |
| `current_task_2026-06-16_profile_quiescent_snapshot.md` | Snapshot of the profile/alias/quiescent RSS ledger before the active HZ6 current task was shortened again. |

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
```
