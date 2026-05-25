# HZ5 P43i / P43o Algorithm Consultation

This file is the short current-status note for historical P43i/P43o/P45
algorithm consultation. The full consultation ledger is archived at:

```text
hakozuna-hz5/docs/archive/HZ5_P43I_P43O_ALGO_CONSULT_HISTORY_2026-05.md
```

## Current Judgment

```text
P43i:
  keep as balanced / RSS-bounded candidate-watch
  not default yet

P43o:
  keep as source-churn evidence/control
  not a replacement for P43i

P44:
  dry-run evidence only
  no actual trim under the P43i lockless contract

P43m/P43n:
  RSS lower-bound no-go controls

P25a/P33:
  practical speed/RSS baselines
```

## Important Boundary

P43i/P43o are exact-route research lines. They are not the current Linux
ordinary malloc front-end work.

Current Linux general allocator work is tracked in:

```text
current_task.md
hakozuna-hz5/docs/HZ5_LINUX_DEV_BRIEF.md
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
```

## Keep / Do Not

```text
Keep:
  P43i/P43o/P45 notes as evidence and historical design context.

Do not:
  use P43i/P43o as the default answer for LargeFront or MidPageFront ordinary
  malloc rows.
```
