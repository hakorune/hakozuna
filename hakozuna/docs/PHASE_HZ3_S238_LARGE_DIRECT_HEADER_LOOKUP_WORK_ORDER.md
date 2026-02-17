# PHASE_HZ3_S238: LargeDirectHeaderLookupBox（Work Order）

Status:
- scheduled as next execution (`CURRENT_TASK` lock, 2026-02-15).
- implemented at boundary (`hakozuna/hz3/src/hz3_large_map_ops.inc`) on 2026-02-15.
- screen A/B (RUNS=3, interleaved) passed:
  - artifact: `/tmp/hz3_s238_ab3_20260215_115335/summary.tsv`
  - `guard_r0`: `-0.70%`
  - `main_r50`: `+3.51%`
  - `cross128_r90`: `+10.37%`
- replay A/B (`RUNS=11`, interleaved) did not pass promotion gate:
  - artifact: `/tmp/hz3_s238_replay11_20260215_120454/summary.tsv`
  - `guard_r0`: `+2.27%`
  - `main_r50`: `-1.31%` (gate miss)
  - `cross128_r90`: `+3.02%`
- replay tie-break (`main_r50` only, `RUNS=21`) confirms regression:
  - artifact: `/tmp/hz3_s238_mainr50_replay21_20260215_121307/summary.tsv`
  - `main_r50`: `-1.90%`
- promotion decision:
  - **NO-GO for defaultization** (keep opt-in only).
- mechanism one-shot (`HZ3_S238_STATS=1`) confirms direct path engagement:
  - artifact: `/tmp/hz3_s238_ab3_20260215_115335/s238_stats_line.txt`
  - `direct_try=480099`, `direct_hit=480099`, `fallback_hash_hit=0`, `fallback_hash_miss=0`
- next: keep as opt-in reference and proceed to next box.

目的:
- `hz3_large_take()` の large free lookup 固定費を下げる。
- hash bucket 走査を「常時」使わず、直引き可能なケースを先に処理する。
- 正しさを最優先し、失敗時は必ず既存 hash map 経路へ戻す。

---

## 0) 背景

- 現状 `hz3_large_take(void* ptr)` は、`ptr -> bucket` の lock + list 走査で header を探索する。
- non-aligned large では `user_ptr = map_base + hz3_large_user_offset()` の形が多く、
  header 直引きが成立するケースがある。
- 一方で `hz3_large_aligned_alloc()` は pad が入るため、固定 offset 前提の直引きは常に成立しない。
- したがって、S238 は「direct-first + safe fallback」の二段構成にする。

---

## 1) Box 境界（1箇所）

- 変更境界は `hakozuna/hz3/src/hz3_large_map_ops.inc` の `hz3_large_take()` のみ。
- それ以外の large alloc/free policy は変更しない。
- `hz3_large_aligned_alloc()` の契約は変更しない。

---

## 2) 実装方針

### 2.1 Direct candidate 算出

- candidate:
  - `cand = (Hz3LargeHdr*)((uint8_t*)ptr - hz3_large_user_offset())`
- 検証:
  - `cand->magic == HZ3_LARGE_MAGIC`
  - `cand->user_ptr == ptr`
  - `cand->map_base == (void*)cand`（debug or fail-fast build）

上記すべて成立時のみ direct hit とみなす。

### 2.2 Fallback

- direct 検証が1つでも失敗したら、既存 hash bucket 探索へ即フォールバック。
- 既存 lock/unlink ロジックは保持する。

### 2.3 Removal

- direct hit 時も最終的には既存と同様に map list から unlink する。
- 取り外し処理は既存の map lock 下で行う（整合性維持）。

---

## 3) 追加ノブ（A/B 用）

- `HZ3_S238_LARGE_DIRECT_LOOKUP=0/1`（default 0 で導入開始）
  - 0: 既存挙動（hash only）
  - 1: direct-first + fallback

- `HZ3_S238_STATS=0/1`（default 0）
  - one-shot/stat 出力のための観測ノブ

注:
- S238 は最初は opt-in で評価し、GO のみ default 化を検討する。

---

## 4) カウンタ（最小）

`HZ3_S238_STATS=1` のときのみ終了時に 1 回出力:

- `direct_try`
- `direct_hit`
- `direct_miss_magic`
- `direct_miss_userptr`
- `fallback_hash_hit`
- `fallback_hash_miss`

期待:
- non-aligned large workload で `direct_hit` が有意に立つ。
- aligned でも correctness を壊さず、miss は fallback に吸収される。

---

## 5) GO/NO-GO ゲート

### 5.1 Screen（RUNS=7 or 10）

- 対象レーン:
  - `cross128`（large 混在）
  - `malloc-large`（large 専用）
  - `guard/main`（退行監視）

- GO 条件:
  - `cross128` または `malloc-large` で改善
  - `guard/main` はノイズ域以上の退行なし
  - crash/assert/abort なし

- NO-GO 条件:
  - 目立つ改善が出ない
  - `guard/main` に明確な退行
  - 整合性異常（map unlink 不整合、double-free 由来の崩れ）

### 5.2 Replay（RUNS=11+）

- screen 通過時のみ replay 実施。
- replay で再現しない場合は default 化しない。

---

## 6) Fail-Fast 条件

- debug/build-time で以下を強制:
  - direct hit で `map_size >= hz3_large_user_offset()` を満たさない場合 abort
  - direct hit 後の unlink 失敗は abort
- release では「危険側に倒さない」:
  - direct 検証に不安があれば即 fallback

---

## 7) 実装ステップ

1. `hz3_large_take()` に `#if HZ3_S238_LARGE_DIRECT_LOOKUP` ブロックを追加。
2. direct candidate 検証関数（`static inline`）を同ファイル内に追加。
3. stats（opt-in）を追加し one-shot dump を実装。
4. A/B（screen）を実施して GO/NO-GO 判定。
5. GO のみ replay gate へ進む。

---

## 8) 非対象（今回やらない）

- hash table 自体の全面削除
- large alloc policy（batch mmap, cache class tuning）の同時変更
- small/mid hot path の改造

---

## 9) 参照

- `CURRENT_TASK.md`（S238 queue）
- `hakozuna/hz3/docs/BACKLOG.md`
- `hakozuna/hz3/src/hz3_large_map_ops.inc`
- `hakozuna/hz3/src/hz3_large.c`
- `hakozuna/hz3/include/hz3_large_internal.h`
