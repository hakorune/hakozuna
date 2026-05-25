# Current Task

HZ5 Linux is in profile-stabilization plus targeted tcmalloc-chase mode.

Long historical task notes were archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```

## Current Claim

```text
MidPage:
  PageRun64 remains the strong keep for general malloc main/mid/cross64 rows.

LargeFront:
  128K remote-heavy rows are the active tcmalloc chase area.
  No single LargeFront diagnostic is broad enough to replace saved profiles.

Documentation:
  keep current_task short.
  put long result logs in docs/archive or dedicated result docs.
```

## Current LargeFront Read

Saved / comparison lanes:

```text
large128-rss:
  source batch4, low-RSS fixed profile.

large128-source16:
  source batch16, current best r90/t8 comparison lane.
```

Diagnostics:

```text
large128-r50-hold8:
  wins t8/r50 in one focused run.
  loses source16 badly on r90.
  keep as r50 diagnostic only.

large128-direct-header:
  unsafe ptr-4096 header lookup upper bound.
  improves t4/r50 and wins t8/r90 in one run.
  not promotable because it is not fail-closed for foreign pointers.

large128-base-directmap:
  safe exact-base one-slot route cache.
  improves t4/r90 versus source16.
  loses source16 on t8 rows.
  no-promote diagnostic.
```

Recent result roots:

```text
private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802
private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345
private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731
```

## Next Engineering Direction

```text
1. Keep source16 as the current r90/t8 baseline.
2. Treat hold8/base-directmap/direct-header as diagnostics, not defaults.
3. If continuing LargeFront optimization, focus on t4/r50 with perf evidence.
4. Do not add another policy until a concrete hotspot explains the row split.
5. Keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters.
```

## Cleanup Status

Completed in this cleanup phase:

```text
HZ5_LINUX_PROFILE_MATRIX.md:
  shortened to current registry.
  history moved to docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md.

current_task.md:
  shortened to current state.
  history moved to docs/archive/current_task_2026-05-hz5-linux-post-largefront.md.

build_linux_hz5_standalone.sh:
  argument parser moved to linux/hz5_build_arg_parser.sh.

linux/hz5_build_arg_parser.sh:
  human-facing profile aliases moved to linux/hz5_build_profile_aliases.sh.
  parser now owns low-level feature flags and value parsing.
```

Remaining cleanup candidates:

```text
linux/build_linux_hz5_standalone.sh:
  still large because defaults, flag emission, meta output, and build commands
  are in one file.

linux/hz5_build_arg_parser.sh:
  still long because all low-level feature flags are centralized.
  next split candidate is Local2P / preload / front-end flag groups.

hakozuna-hz5/largefront/hz5_largefront.c:
  large but acceptable during active optimization.
  only refactor after measurements stabilize.
```

## Reading Order

```text
1. current_task.md
2. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
3. hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
4. hakozuna-hz5/docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md
5. hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```
