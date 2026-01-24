# PHASE_HZ3_S12-8: v2-only（PTAG-only dispatch）coverage 確認と残差

Note: `HZ3_V2_ONLY` / `HZ3_V2_ONLY_STATS` は 2026-01-10 の掃除で削除済み（本ドキュメントは履歴用）。

目的:
- “v2-only（range check + PTAG load だけで dispatch）” を成立させ、segmap fallback 依存を外す。
- まず正しさ（PTAG coverage）を確認し、その後に性能差分を詰める。

前提:
- false positive は NG（他allocatorの ptr を誤って hz3 で free しない）
- false negative は OK（tag==0 は fallback で安全側）

---

## 実行ログ（SSOT）

- v2-only (stats=1 coverage): `/tmp/hz3_ssot_7b02434b8_20260101_083338`
- v2-only (stats=0 perf): `/tmp/hz3_ssot_7b02434b8_20260101_085230`
- 混在ベース: `/tmp/hz3_ssot_7b02434b8_20260101_070653`

v2-only SSOT median（ops/s）:
- stats=1:
  - small(16–2048): `85.73M`
  - medium(4096–32768): `102.34M`
  - mixed(16–32768): `97.54M`
- stats=0:
  - small(16–2048): `89.27M`
  - medium(4096–32768): `102.48M`
  - mixed(16–32768): `94.68M`

混在ベースとの差分:
- stats=1:
  - small: `-8.4%`
  - medium: `-4.0%`
  - mixed: `-3.3%`
- stats=0:
  - small: `-4.7%`
  - medium: `-3.9%`
  - mixed: `-6.1%`

---

## Coverage（PTAG）

v2-only stats（PTAGカバレッジ）:
- small: `tag_zero=0`（coverage OK）
- medium: `tag_zero=0`（coverage OK）
- mixed: `tag_zero=0`（coverage OK）

補足:
- `HZ3_V2_ONLY_STATS=1` は hot path にカウンタ増分が入るため、性能比較では必ず OFF にする。
  - coverage 計測（1回）と SSOT到達点（RUNS=10 median）は分ける。

---

## 次の一手（性能差分を詰める）

1) `HZ3_V2_ONLY_STATS=0` で再測定（比較の土台を揃える）
- stats 増分が入ると small/medium/mixed が不利になりやすい。
- stats=0 でも mixed が落ちるため、主要ボトルネックは alloc 側の固定費に移った可能性が高い。

2) それでも v2-only が負ける場合は “alloc 側” を疑う
- 現状の v2-only は free の分類は PTAG で薄いが、alloc slow path の refill/central の固定費が支配しうる。
- 先に `perf` で `hz3_small_v2_alloc` / `hz3_small_v2_central_pop_batch` の比率を確認する。
