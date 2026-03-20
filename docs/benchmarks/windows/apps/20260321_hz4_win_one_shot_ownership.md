# Windows HZ4 One-Shot Ownership Box

Date: 2026-03-21

This follow-up box keeps the Windows foreign-pointer safety boundary, but moves
the ownership question back to one place.

Box:

- `HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1`

Goal:

- keep `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` as the Windows HZ4 safety default
- stop asking the same ownership question twice across:
  - [hakozuna-mt/win/hz4_win_api.c](/C:/git/hakozuna-win/hakozuna-mt/win/hz4_win_api.c)
  - [hakozuna-mt/src/hz4_tcache_free.inc](/C:/git/hakozuna-win/hakozuna-mt/src/hz4_tcache_free.inc)

Boundary:

- the Windows wrapper probes `foreign / seg-owned / large-owned` once
- foreign pointers still reject safely
- seg-owned pointers now route through `hz4_free_known_seg_owned()`
- large-owned pointers now route directly to `hz4_large_free()`

## Safety Check

The Windows foreign-pointer probe still passes after promotion:

- `usable_size -> 0`
- `free -> returned`
- `realloc -> NULL`

Reference entrypoint:

- [win/build_win_hz4_foreign_probe.ps1](/C:/git/hakozuna-win/win/build_win_hz4_foreign_probe.ps1)

## Mixed Bench A/B

Shape:

- benchmark: `bench_mixed_ws`
- params: `threads=4 iters=1000000 ws=8192 size=16..1024`
- runs: `5`
- statistic: `median ops/s`

Comparison:

- box on: `HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1`
- rollback: `HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=0`

Results:

- box on runs: `417.340M`, `413.578M`, `410.164M`, `388.576M`, `342.668M`
- box on median: `410.164M ops/s`
- rollback runs: `383.991M`, `361.259M`, `345.214M`, `386.320M`, `343.183M`
- rollback median: `361.259M ops/s`
- delta: about `+13.54%` for the one-shot box

## Counter Read

Diagnostic build:

- `HZ4_FREE_ROUTE_STATS_B26=1`
- `HZ4_SMALL_STATS_B19=1`

Results on the same mixed shape:

- one-shot on:
  - `free_calls=2001152`
  - `seg_checks=0`
  - `large_validate_calls=0`
  - `small_page_valid_hits=2001152`
- rollback:
  - `free_calls=2001152`
  - `seg_checks=2001152`
  - `large_validate_calls=0`
  - `small_page_valid_hits=2001152`

Interpretation:

- the foreign-pointer safety stays in place
- the Windows wrapper already proved `seg-owned`
- the new box removes the duplicate core `seg_check`
- the recovered throughput lands exactly where the earlier slowdown was observed

## Decision

This box is `GO/default` for Windows HZ4 build entrypoints.

Keep the rollback path explicit:

- `-ExtraDefines HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=0`

Use that rollback only for A/B replay or if a later Windows-specific regression
needs bisect help.
