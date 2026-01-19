# S57-A: SegmentDecayTrim（Fully-Free Segment Return）— NO-GO（steady-state RSS）

## Status

- Verdict: ❌ NO-GO（steady-state RSS）
- Track: hz3
- Commits: `848267d88`（実装）, `c80f76670`（結果追記）

## Goal

- `mstress` mean RSS **-10%** vs baseline

## Result（mstress T=10, L=100, I=100, RUNS=10）

- Median peak RSS: **-0.32%**（noise level）

## Root Cause

- S57-A は segment が完全に空（`free_pages == PAGES_PER_SEG`）になったときだけ発火。
- steady-state churn では segment が完全に空にならず、`munmap` が実質発生しない。

## Snapshot

- 指示書/結果: `hakozuna/hz3/archive/research/s57_a_segment_decay_trim/snapshot/PHASE_HZ3_S57_A_DECAY_TRIM_WORK_ORDER.md`
- 実装: `hakozuna/hz3/archive/research/s57_a_segment_decay_trim/snapshot/hz3_segment_decay.c`
- ヘッダ: `hakozuna/hz3/archive/research/s57_a_segment_decay_trim/snapshot/hz3_segment_decay.h`

## Revival 条件

- S58（tcache shrink/decay）で `tcache → central/segment` の返却パイプを先に作ってから再評価。
