# Current Task: HZ5 Linux General Front-End

## Read First

Short current-status entry point:

```text
hakozuna-hz5/docs/HZ5_LINUX_DEV_BRIEF.md
```

Use that brief for the active decision tree. This file is now the short
current-status index. The detailed chronological history, measurements, failed
candidates, and exact command history moved to:

```text
docs/archive/current_task_2026-05-hz5-linux_history.md
```

Current active question:

```text
HZ5 Linux now covers:
  SmallFront-S1   ordinary malloc <= 2KiB
  MidFront-M1     ordinary malloc 2049..65536
  LargeFront-L1   ordinary malloc 65537..1MiB
  Local2P         exact 64K/a8192 appendix/profile lane

The current general candidate preset is:
  --linux-hz5-general-region-outbox

Latest result:
  LargeFront region-map removed the per-page insertion timeout tail while
  preserving interior invalid-free attribution.
  SmallFront remote outbox cap8 modestly improves r90, but does not close the
  HZ4 cross128 gap.

Next decision:
  Pro review agrees with the shared owner handoff direction, but this branch
  already tested OwnerHub-R1/R2/R3 and found no broad default win.
  Next work should not reimplement OwnerHub generically; it should either:
    1. make OwnerHub selective/mixed-only with lower fixed publish cost, or
    2. reduce MidFront remote-heavy instruction/branch cost versus HZ4.
```

## Goal

Build an Ubuntu/Linux development lane for HZ5 that can be moved to native
Ubuntu later for paper-facing measurement.

Current direction:

- do not merge to `main`
- keep Windows P43i/P45 behavior separate and unchanged
- use full `LD_PRELOAD` lanes for general allocator coverage
- keep exact standalone lanes fallback-free for route attribution
- route unsupported exact cases fail-closed instead of counting them as HZ5 wins

## Paper Benchmark Source

The previous paper benchmark tree is available at:

```text
/mnt/workdisk/public_share/hakmem
```

This maps to the user's `Y:\hakmem` path. Use it as the default paper-main
source for fresh runs. The archived copy at
`/mnt/workdisk/public_share/hakozuna_paper/hakmem` has matching MT matrix
runner and benchmark source, but it is not a git checkout.

Current policy:

- do not copy the whole `hakmem` tree into this branch
- run paper-main through `linux/run_paper_allocator_suite.sh --tier paper-main`
  when a general LD_PRELOAD allocator comparison is needed
- if a self-contained paper-compat suite is needed, import only the paper result
  docs, `scripts/run_mt_lane_remote_matrix.sh`,
  `scripts/lib/ssot_preload_guard.sh`,
  `hakozuna/bench/bench_random_mixed_mt_remote.c`, and
  `hakozuna/scripts/run_realworld_4pack.sh`
- keep these paper-main workloads separate from HZ5 standalone exact
  `64K/a8192` appendix benchmarks

2026-05-24 audit:

- paper-main benchmark mapping is recorded in
  `hakozuna-hz5/docs/HZ5_PAPER_BENCH_SUITE.md`
- the refreshed paper-main MT entry point is
  `/mnt/workdisk/public_share/hakmem/scripts/run_mt_lane_remote_matrix.sh`
- the latest paper-main results are documented in
  `/mnt/workdisk/public_share/hakmem/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md`
- a short HZ5 preload hit probe against `/mnt/workdisk/public_share/hakmem`
  produced:

```text
private/raw-results/linux/hz5_hakmem_hit_probe_20260524_051807

guard/main:
  malloc_hz5=0

cross128:
  malloc_hz5=1 out of about 40393 malloc calls
```

Interpretation:

```text
Do not add hz5-preload-hybrid as a paper-main allocator row.
The paper MT matrix mostly measures libc passthrough for current HZ5.
Use paper-main for hz3/hz4/mimalloc/tcmalloc, and use HZ5 exact appendix
benchmarks for Local2P claims unless a true general HZ5 LD_PRELOAD lane is
implemented.
```

## Current Development Focus: General HZ5 Linux Front-End

Direction change, 2026-05-24:

```text
Goal:
  Move from HZ5 exact sidecar back toward a real hz3/hz4 successor.

Immediate target:
  paper-main LD_PRELOAD should exercise HZ5 for ordinary malloc traffic instead
  of delegating almost everything to libc.

Do not weaken:
  existing Local2P exact 64K/a8192 profiles
  existing hybrid preload diagnostic adapter
  fail-closed invalid/double-free behavior
```


## Archive

The detailed chronological history moved to:

```text
docs/archive/current_task_2026-05-hz5-linux_history.md
```

Use that file for the older benchmark logs, phase history, and archived candidate notes.
