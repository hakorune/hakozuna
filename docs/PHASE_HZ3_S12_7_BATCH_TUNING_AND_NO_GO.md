# PHASE_HZ3_S12-7: batch tuning（4KB=12）+ NO-GO 記録（xfer / slow-start / dynamic cap）

目的:
- SSOT-HZ3（small/medium/mixed）で “mixed を詰める” ための batch tuning を試し、到達点を記録する。
- NO-GO（層が厚くなる/固定費が増える）試行も、再発防止のため SSOT と一緒に残す。

---

## S12-7A: 4KB refill batch を 12 に変更（GO）

変更:
- `hakozuna/hz3/include/hz3_types.h`
  - `HZ3_REFILL_BATCH[0] = 12`（4KB class）
- `hakozuna/hz3/src/hz3_knobs.c`
  - `g_hz3_knobs.refill_batch[0] = 12`（既定値の整合）

SSOT（RUNS=10 median）:
- baseline: `/tmp/hz3_ssot_7b02434b8_20260101_060336`
- batch=12: `/tmp/hz3_ssot_7b02434b8_20260101_061616`

結果（ops/s）:
- small: `94.33M → 94.66M`（+0.35%）
- medium: `106.97M → 107.17M`（+0.19%）
- mixed: `98.75M → 100.98M`（+2.26%）

結論:
- mixed が改善し、small/medium 退行も無し。`4KB=12` を既定として採用。

---

## S12-7B: TransferCache（xfer）追加（NO-GO）

SSOT（RUNS=10 median）:
- baseline（4KB=12, xfer OFF）: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- xfer ON: `/tmp/hz3_ssot_7b02434b8_20260101_062654`

結果（ops/s）:
- small: `94.66M → 94.22M`（-0.47%）
- medium: `107.17M → 106.36M`（-0.76%）
- mixed: `100.98M → 98.29M`（-2.66%）

結論:
- 層の追加による固定費が勝ってしまい、全体で悪化。NO-GO。

---

## S12-7C: slow-start（TLS batch ramp-up）（NO-GO）

SSOT（RUNS=10 median）:
- baseline（4KB=12, slow-start OFF）: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- slow-start ON: `/tmp/hz3_ssot_7b02434b8_20260101_063620`

結果（ops/s）:
- small: `94.66M → 90.64M`（-4.25%）
- medium: `107.17M → 103.34M`（-3.58%）
- mixed: `100.98M → 93.23M`（-7.67%）

結論:
- hot/slow の分岐・状態管理コストが支配し、全体で悪化。NO-GO。

---

## S12-7D: bin cap 動的調整（epochベース）（NO-GO寄り）

SSOT（RUNS=10 median）:
- baseline（4KB=12）: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- dynamic ON: `/tmp/hz3_ssot_7b02434b8_20260101_064505`

結果（ops/s）:
- small: `94.66M → 93.75M`（-0.96%）
- medium: `107.17M → 106.68M`（-0.46%）
- mixed: `100.98M → 101.34M`（+0.36%）

結論:
- mixed は微増だが small/medium が微減で、総合では NO-GO 寄り（現状は採用しない）。

