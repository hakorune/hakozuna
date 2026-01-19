# S32-2: dst compare を消す (row_off 方式)

Status: **NO-GO**

Date: 2026-01-02

## 目的

free hot path の `dst == my_shard` 比較を排除し、分岐を削減する。

## 変更

- `Hz3TCache` に `local_row[HZ3_BIN_TOTAL]` と `row_off[HZ3_NUM_SHARDS]` を追加
- init 時に row_off を設定
- free hot path で dst compare を row_off lookup に置換

## GO 条件

- dist=app (RUNS=30) で +1% 以上

## 結果

| 構成 | Median | vs Baseline |
|------|--------|-------------|
| Baseline | 43.55M | - |
| S32-1+2 | 42.47M | **-2.48%** |

**判定: NO-GO（退行）**

## 敗因分析

1. **分岐予測の効率**: dist=app では 80% が local free
   - `dst == my_shard` の分岐は分岐予測でほぼ完璧に当たる
   - 比較命令自体のコストはほぼゼロ

2. **row_off のオーバーヘッド**:
   - `row_off[dst]` のテーブル lookup (メモリ読み込み)
   - ポインタ演算 `(char*)&t_hz3_cache + offset`
   - これらが分岐予測の利点を上回るコスト

3. **教訓**: 分岐予測が効く状況では、分岐を消しても得にならない

## ログ

- 結果ドキュメント（RUNS=30の集計と /tmp ログ参照）:
  - `hakozuna/hz3/docs/PHASE_HZ3_S32_RESULTS.md`

## アーカイブ方針

- この案は NO-GO のため、本線（`hakozuna/hz3/src`）から実装コードを撤去し、研究箱としてこのフォルダに固定した。
- 再現したい場合は `PHASE_HZ3_S32_RESULTS.md` のログと、当時の差分（git履歴）を参照して復元する。

## 復活条件

- 分岐予測が効かないワークロード（random dst pattern など）で改善が見込める場合
- ただし、実アプリケーションでは local free が dominant なので、復活の可能性は低い
