# S57-B: PartialSubrelease（Segment Free-Range Purge）— NO-GO（RSS）

## Status

- Verdict: ❌ NO-GO（RSS目標未達）
- Track: hz3

## Goal

- `mstress` mean RSS **-10%** vs baseline

## Results（summary）

- `mstress`: `madvise_calls=900`, RSS **-0.9%**
- `sh6bench`: `madvise_calls=32749`, RSS **0%**（instant re-fault）
- `burst-idle`（aggressive）: `madvise_calls=2`, RSS **0%**

## Root Cause（確定）

- purge 対象が（実質）**pack pool segment** に限定される一方、freed の大部分は **tcache local bins** に滞留。
- さらに workload によっては `madvise` しても即リフォールトし、steady-state RSS が動かない。

## Snapshot

- 実装: `hakozuna/hz3/archive/research/s57_b_partial_subrelease/snapshot/hz3_segment_purge.c`
- ヘッダ: `hakozuna/hz3/archive/research/s57_b_partial_subrelease/snapshot/hz3_segment_purge.h`

## Revival 条件

- S58（tcache shrink/decay/release to central）を先に実装し、segment 側に purge 候補が生まれる状態を作ってから再評価。
