# HZ4 File Split Candidates (2026-02-14)

## Scope
- Checked files under `hakozuna/hz4` (`*.c/*.h/*.inc/*.md`).
- Goal: identify files over 1000 lines and whether they can be split safely.

## Over-1000 Files (before split)

| File | Lines | Split Feasibility | Priority |
|---|---:|---|---|
| `hakozuna/hz4/src/hz4_large.c` | 1486 | High (already boundary-rich; can split by cache layer/free path) | P1 |
| `hakozuna/hz4/src/hz4_os.c` | 1363 | Medium (stats counters are dense; split by stats-domain) | P2 |
| `hakozuna/hz4/core/hz4_remote_flush.inc` | 1035 | Medium (hotpath; split only with strict perf recheck) | P3 |

## Status (2026-02-14, latest)

- `P1-0` completed (move-only):
  - extracted helper path block from
    `hakozuna/hz4/src/hz4_large.c` into
    `hakozuna/hz4/src/hz4_large_paths.inc`.
  - file size after split:
    - `hakozuna/hz4/src/hz4_large.c`: `352` lines
    - `hakozuna/hz4/src/hz4_large_paths.inc`: `1138` lines
  - public API boundary remains in `hz4_large.c`:
    `hz4_large_malloc/free/usable_size`.
- `P1-1` completed (move-only):
  - split `hakozuna/hz4/src/hz4_large_paths.inc` into:
    - `hakozuna/hz4/src/hz4_large_cache_box_layers.inc` (`643` lines)
    - `hakozuna/hz4/src/hz4_large_cache_box_paths.inc` (`260` lines)
    - `hakozuna/hz4/src/hz4_large_cache_box_retry.inc` (`125` lines)
  - wrapper `hakozuna/hz4/src/hz4_large_paths.inc` is now `115` lines.
- `P2` completed (move-only):
  - extracted stats increment/flush function block from
    `hakozuna/hz4/src/hz4_os.c` into
    `hakozuna/hz4/src/hz4_os_stats_funcs.inc` (`709` lines).
  - `hakozuna/hz4/src/hz4_os.c` is now `657` lines.
- `P3` completed (move-only):
  - split `hakozuna/hz4/core/hz4_remote_flush.inc` into:
    - `hakozuna/hz4/core/hz4_remote_flush_core.inc` (`728` lines)
    - `hakozuna/hz4/core/hz4_remote_free_entry.inc` (`281` lines)
  - wrapper `hakozuna/hz4/core/hz4_remote_flush.inc` is now `30` lines.
- post-check:
  - no `hz4` `*.c/*.h/*.inc` file exceeds `1000` lines.
  - build/smoke pass after split (`make -C hakozuna/hz4 libhakozuna_hz4.so`, short `LD_PRELOAD` bench).

## Recommended Split Plan

1. `hz4_large.c` / `hz4_large_paths.inc` (P1 complete)
- Keep public API boundary in `hz4_large.c`:
  - `hz4_large_malloc()`
  - `hz4_large_free()`
  - `hz4_large_usable_size()`
- done with move-only split:
  - wrapper: `hz4_large_paths.inc`
  - internals: `hz4_large_cache_box_layers.inc`, `hz4_large_cache_box_paths.inc`,
    `hz4_large_cache_box_retry.inc`.

2. `hz4_os.c` (P2 complete)
- Keep one-shot dump entrypoint in `hz4_os.c`.
- extracted `hz4_os_stats_funcs.inc` (move-only).
- Preserve existing output format (`[HZ4_OS_STATS]`, `[HZ4_OS_STATS_B*]`).

3. `hz4_remote_flush.inc` (P3 complete)
- split into `hz4_remote_flush_core.inc` and `hz4_remote_free_entry.inc` (move-only).

## Guardrails
- Split is move-only first (no algorithm change).
- Keep compile-time flags and behavior unchanged.
- Verify with:
  - build success (`make -C hakozuna/hz4 libhakozuna_hz4.so`)
  - `RUNS=7` quick parity on `guard/main/cross64/cross128`
  - one-shot stats format unchanged.
