# HZ5 Linux Development Brief

Short entry point for current HZ5 Linux allocator work. Long historical notes
live in:

```text
hakozuna-hz5/docs/archive/HZ5_LINUX_DEV_BRIEF_HISTORY_2026-05.md
```

## Current Direction

```text
HZ5 Linux:
  general LD_PRELOAD allocator candidate

SmallFront-S1:
  ordinary malloc <= 2048

MidPage PageRun64:
  current strong keep for main / mid_only / cross64

LargeFront 128K:
  active tcmalloc chase area
  source16, draintrust, and transfer128 are the current comparison controls
  transfer128-tlsfirst / ownershard / shard16 are no-go diagnostics

Local2P:
  exact 64K/a8192 appendix/special profiles

Windows P43i/P45:
  separate; do not weaken from Linux experiments
```

## Current Profile Registry

Read these first:

```text
current_task.md
hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
linux/HZ5_BUILD_SCRIPT_LAYOUT.md
```

## Current LargeFront Read

```text
source16:
  best current r90/t8 direction
  not universal

large128-rss:
  saved low-RSS fixed profile

transfer128:
  t4/r50 signal with excellent RSS
  broad no-go because the single global transfer cache hurts t8 rows

transfer128-tlsfirst:
  no-go diagnostic
  TLS-held TRANSFER_FREE spans improved neither t4 nor t8, and RSS worsened

transfer128-ownershard:
  no-go diagnostic
  routes TRANSFER_FREE spans by old owner slot so consumers can see incoming
  remote frees without a single global transfer lock

transfer128-shard16:
  no-go diagnostic
  consumer-visible 16-shard transfer cache with shard stealing; nonempty-mask
  version still loses transfer128's t4/r50 win and does not recover t8

known no-go diagnostics:
  rb32/rb64 for t4/r50
  batch32
  ownerfast
  transfer128-tlsfirst
  transfer128-ownershard
  transfer128-shard16
  drain-directmap
  base-directmap4
```

## Engineering Rules

```text
Do:
  keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters
  record focused results in HZ5_LINUX_PROFILE_MATRIX.md
  archive long historical logs
  add human lane aliases in linux/hz5_build_profile_aliases.sh

Do not:
  promote unsafe direct-header style lanes
  mix Windows P43i/P45 changes into Linux LargeFront work
  add another policy until a concrete hotspot explains the row split
```

## Next Cleanup Target

```text
linux/hz5_build_arg_parser.sh:
  still a long low-level case table
  next split should group Local2P, MidPage, MidFront, and LargeFront flags
```
