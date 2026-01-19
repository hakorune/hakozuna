# S28-A: size→bin shift最適化 NO-GO

目的:
- dist=app の small 残差の主犯が `tiny(16–256)` と判明したため、hot path の `hz3_small_sc_from_size()` を軽量化して命令数を削減し、tiny gap を詰める。

---

## 背景

S27 トリアージで tiny (16-256) が -8.9% gap の主犯と判明:
- tiny (16–256): hz3 ~59.2M vs tcmalloc ~65.0M（-8.9%）
- 257–2048: hz3 ~53.5M vs tcmalloc ~50.9M（+5.1%）

そこで S28-A で `hz3_small_sc_from_size()` の最適化を試みた。

---

## 試した最適化

### v1: shift 最適化
- `/16` を `>>4` に変更し、分岐を削減しようとした。
- `size | !size` パターンを使用。

### v2: unified check 最適化
- 複数の条件チェックを統合し、5命令に圧縮。

---

## 結果（RUNS=10, dist=app triage）

| Version      | tiny (16–256) | 257–2048      | 判定  |
|--------------|---------------|---------------|-------|
| baseline     | 59.2M         | 53.5M         | -     |
| v1 (shift)   | 57.8M (-2.4%) | 52.3M (-2.2%) | NO-GO |
| v2 (unified) | 57.1M (-3.5%) | 52.6M (-1.7%) | NO-GO |

---

## 発見

1. **コンパイラ最適化済み**: コンパイラは既に `/16` を `>>4` に最適化済みだった。手動変更は冗長。

2. **cmov の罠**: v1 の `size | !size` は cmovne を生成したが、cmov は遅い（2 uops + data dependency）。分岐を消したつもりが、より遅い命令を生成してしまった。

3. **assembly の美しさと性能の乖離**: v2 の unified check は assembly としては綺麗（5命令）だったが、実性能は退行した。命令数削減が必ずしも性能向上に繋がらない例。

4. **ボトルネック別の場所**: `sc_from_size()` は hotpath の主犯ではなかった。ボトルネックは別の場所（page supply、PTAG lookup、bin refill など）にある可能性が高い。

---

## NO-GO理由

- v1/v2 ともに tiny/非tiny 両方で退行（-2〜3%）。
- hot path への命令数削減アプローチは `hz3_small_sc_from_size()` に対しては効果なし。
- 原因究明には別のアプローチ（slow path 率の観測）が必要。

---

## 次のアクション

- S28 Work Order に従い、まず slow path 率を観測して「遅さが hot path なのか supply (slow path) なのか」を確定する。
- 参照: `hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md`

---

## 参照

- S27 triage: `hakozuna/hz3/docs/PHASE_HZ3_S27_DIST_APP_SMALL_GAP_TRIAGE_WORK_ORDER.md`
- NO-GO 索引: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
