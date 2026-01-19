# PHASE_HZ3_S15-3: mixed gap（命令数差）perf 結果（SSOT固定）

Note: `HZ3_V2_ONLY_STATS` は 2026-01-10 の掃除で削除済み（本ドキュメントは履歴用）。

目的:
- `mixed (16–32768)` の tcmalloc 差（~ -16%）が **branch miss / cache miss** ではなく、**純粋な命令数差**であることを確定する。
- 次フェーズ（S16）で「どこを削るべきか」を **hz3_free 起点**で特定する。

前提:
- 比較は同一ベンチ・同一条件（RUNS/ITERS/WS/スレッド数）で、`hz3` と `tcmalloc` を LD_PRELOAD で切替して測る。
- `HZ3_V2_ONLY_STATS` 等の hot 統計は OFF（計測のために速い経路を汚さない）。

参照:
- S15 Work Order: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S15_MIXED_GAP_TRIAGE_WORK_ORDER.md`
- hz3 SSOT: `hakmem/hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## 結論（最重要）

- mixed の差は **branch miss** でも **cache miss** でもない。
- `hz3 mixed` は `tcmalloc mixed` より **命令数が約 18% 多い**。
- 主要な重さは `hz3_free` 側に寄っている（free 処理の CPU% が高い）。

したがって、mixed の gap を詰める主戦場は:
- shared path（central / inbox）ではなく、
- **hot の命令列（特に free の ptr→(kind/sc/owner) 判定〜dispatch）** の削減。

---

## 観測（要約）

### 1) branch miss は主因ではない

- `hz3 small → mixed` で branch-misses は増えるが、ops の落ち幅を説明できるほどではない。
- `hz3 vs tcmalloc (mixed)` では、hz3 の方が branch-misses が低いケースがある。

→ 「分岐予測が壊れているから mixed が遅い」は否定。

### 2) cache miss も主因ではない

- L1 miss / LLC miss / IPC は hz3 の方が良い（または同等）。

→ 「メモリ階層が負けているから mixed が遅い」は否定。

### 3) 決定要因は命令数

- `hz3 mixed` の instructions が `tcmalloc mixed` より多い（~18%）。

→ 「同じ仕事量に対して、hz3 が余計な命令を実行している」が本質。

---

## 主要ホットスポット（推定）

`hz3_free` が重い主因候補（現行実装パターン）:

- PageTagMap の decode:
  - `hz3_pagetag_decode_with_kind` の bit 操作（shift/and/sub）が多い
  - kind 判定の比較（複数分岐）
- arena range check:
  - base/end の atomic load + 比較が複数回

※ branch/cache より instructions が支配なので、ここを「短い命令列」に寄せるのが最優先。

---

## 次（S16）で狙うべき削減ポイント（優先順）

1. **tag decode を簡素化**（sub/shift の削減、decode の直線化）
2. **dispatch 分岐を削減**（v2-only を前提に dead code をコンパイル時に落とす）
3. **arena range check を軽量化**（atomic load を hot から押し出す／回数削減）
4. （必要なら）`malloc(size) → sc` を table 化（算術→1ロード、分岐削減）

Work Order: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`
