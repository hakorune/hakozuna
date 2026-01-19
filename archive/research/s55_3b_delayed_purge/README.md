# S55-3B: DelayedMediumSubreleaseBox（NO-GO / Archive）

## 目的
- medium run（4KB–32KB）の subrelease を **遅延実行**し、
  “すぐ再利用される run” を避けて RSS 効率を上げる（purge delay）。

## 結論
- **steady-state（mstress/larson）で NO-GO**。
- `reused` 検出は成立したが、RSS が下がらない/増えるケースがあり目標未達。

## 主要な観測
- 例: `reused=61.3%` / `madvised=38.7%` で hot run 検出は成立。
- ただし RSS 目標（mean -10%）に届かず、steady-state では不利。

## 既知の修正点（Step2で反映済み）
- TLS ring head/tail の 16-bit wrap 問題 → 32-bit monotonic
- TLS epoch の time axis split → global epoch に統一

## NO-GO理由
1. `mstress/larson` の constant high-load では RSS が動かない/増える。
2. 省メモリ目的では効果が不足（-10%目標未達）。

## 復活条件（もし再評価するなら）
- “burst→idle” が明確な **長寿命ワークロード**でのみ再評価。
- `cache-thrash` 等の短時間ベンチでは判定不能。
- 評価手順は以下を参照。

## 参照
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_DELAYED_MEDIUM_SUBRELEASE_WORK_ORDER.md`
- Step2 A/B: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_STEP2_GATE_AB_WORK_ORDER.md`
- NO-GO整理: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_STEP3_NO_GO_AND_NEXT_WORK_ORDER.md`

## スナップショット（実装の保存）
- 本線からは S55-3B の実装を除去（NO-GO隔離）し、この研究箱に保存。
- 当時の実装は `hakozuna/hz3/archive/research/s55_3b_delayed_purge/snapshot/` にコピーしてある（参照用・ビルド対象外）。
