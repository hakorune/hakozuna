# S140: PTAG32 Leaf Micro-Optimizations (ARCHIVE / NO-GO)

## Goal
Reduce `hz3_free_try_ptag32_leaf()` overhead by shaving decode and branch costs
without adding hot-path instructions or function boundaries.

## Changes (A/B)
- A1: `HZ3_S140_PTAG32_DECODE_FUSED`
  - `tag32 - 0x00010001` to fuse bin/dst `-1` decode.
- A2: `HZ3_S140_EXPECT_REMOTE`
  - Invert local/remote branch hint for remote-heavy workloads.

## Results
All variants regressed, especially at high remote ratios.

Throughput (median, 5 runs):
- r90: **-7.16% ~ -14.39%**
- r50: **-3.45% ~ -6.10%**
- r0: ~neutral

Perf cause:
- A1 reduces instruction count but creates a dependency chain, collapsing ILP and IPC.
- A2 increases branch-miss rate in practice (prediction got worse).

## Decision
**NO-GO**. Archived and disabled in `hz3_config_archive.h` with `#error`.

## References
- Report: `hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md`
- Disabled flags: `hakozuna/hz3/include/hz3_config_archive.h`

