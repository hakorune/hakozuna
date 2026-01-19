# PHASE_HZ3_S15-2B: arena range check（delta>>32）A/B結果

目的:
- `hz3_arena_page_index_fast()` の hot path 固定費を削る。
- `end` の atomic load を消し、4GB 即値（`movabs`）も避けて mixed を改善できるか検証する。

変更（S15-2B）:
- `HZ3_ARENA_SIZE == 4GB` の場合に限り、
  - `delta = (uintptr_t)ptr - (uintptr_t)base`
  - `if ((delta >> 32) != 0) return 0;`
  - `page_idx = (uint32_t)(delta >> 12)`
  で range check を行う（`g_hz3_arena_end` load を削除）。

結果（SSOT, RUNS=10, median）

| range | median | 前回比 |
|---|---:|---:|
| small | 95.6M | -1.4% |
| medium | 98.8M | -1.2% |
| mixed | 92.5M | +0.5% |

結論:
- mixed は +0.5% と誤差内で、明確な改善なし。
- small/medium が -1〜-2% と若干落ちる。
- 判定: **NO-GO（本線には残さない）**

対応:
- 実装は revert（`hz3_arena_page_index_fast()` を元の `end` load 方式に戻す）。
- 学びは `PHASE_HZ3_S15_MIXED_GAP_TRIAGE_WORK_ORDER.md` の S15-2 メモに反映済み。

