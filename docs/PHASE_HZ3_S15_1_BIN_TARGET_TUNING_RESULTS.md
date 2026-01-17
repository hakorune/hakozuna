# PHASE_HZ3_S15-1: bin_target tuning（結果）

目的:
- mixed（16–32768）の gap を詰める仮説として「4KB(sc=0) が薄くて枯渇→slow増」を検証する。

変更（概要）:
- `sc=0` の bin（4KB class）を厚くする（bin_target を増やす）。
- hot path の固定費追加は無し（保持量の調整のみ）。

ベンチ結果（SSOT, RUNS=5, ops/s median）

| range | baseline | S15-1 | diff |
|---|---:|---:|---:|
| small (16–2048) | 95.0M | 97.6M | +2.7% |
| medium (4096–32768) | 94.0M | 100.1M | +6.5% |
| mixed (16–32768) | 92.8M | 92.4M | -0.4% |

結論:
- mixed は改善せず（仮説Aは外れ）
- small/medium は改善（sc=0 の厚みが効いた）
- 回帰なし（mixed は -0.4% でノイズ級）

判定:
- **partial GO**（small/medium を底上げするが、mixed gap の主因ではない）

次の一手（S15）:
- mixed の主因は別（分岐混在/サイズクラス密度/PTAG固定費/小サイズの “希少クラス” による miss 等）を疑う。
- `PHASE_HZ3_S15_MIXED_GAP_TRIAGE_WORK_ORDER.md` の S15-0/S15-2 に沿って割る。

