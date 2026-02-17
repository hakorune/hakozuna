# HZ4 Phase22: NO-GO Summary

Phase22 (mid lock-path optimization) で試行した最適化の NO-GO 記録の索引。
詳細は phase 分割ドキュメントを参照。

---

## Overview

| 最適化 | 目標 | 結果 | main_r0 変化 | main_r50 変化 | 日付 |
|--------|------|------|--------------|---------------|------|
| B71 SpanLiteBox | +3% | NO-GO | -1% | 0% | 2026-02-15 |
| B72 SpanLeafRemoteQueue | +3% | NO-GO | 0% | 0% | 2026-02-15 |
| B73 TransferCache v1 | +3% | NO-GO | -1% | +3% | 2026-02-16 |
| B73 TransferCache v2 (adaptive) | +3% | NO-GO | 0% | -5% | 2026-02-16 |
| B73 TransferCache v3 (bug fix) | +3% | NO-GO | +1% | -1% | 2026-02-16 |
| B74 Stage A (outlock create) | +3% | NO-GO | -1% | +4% | 2026-02-16 |
| B74 Stage B (outlock refill) | +3% | NO-GO | -2% | 0% | 2026-02-16 |
| B75 Pre-Observation | - | NO-GO | - | - | 2026-02-16 |
| B76 SmallAllocPageInitLite | +3% | NO-GO | -0.1% / -0.6% | +4.5% / +5.4% | 2026-02-17 |
| B77 MidOwnerLocalStackAdaptive | +3% | NO-GO | -6.85% | +2.12% | 2026-02-17 |
| B77a (hot=1, COOLDOWN=16) | +3% | NO-GO | -3.09% | -0.10% | 2026-02-17 |
| B77b (hot=1, COOLDOWN=32) | +3% | NO-GO | -3.32% | +5.12% | 2026-02-17 |
| B77c Event-driven Adaptive | +3% | NO-GO | -4.80% | +1.58% | 2026-02-17 |
| B78 MidMallocHotColdSplit | +3% | NO-GO | +0.24% | -0.14% | 2026-02-17 |
| B79 SupplyResvAdaptiveChunk | +3% | NO-GO | +0.98% / +0.08% | +1.76% / +2.68% | 2026-02-17 |
| B80 LockShards Sweep | +3% | NO-GO | -0.39% / +8.40% | -4.15% / -18.72% | 2026-02-17 |
| B81 MidCache Knob Sweep | +3% | NO-GO | -1.20% .. +0.32% | -4.80% .. +0.28% | 2026-02-17 |

---

## Detail Docs

- B71/B72:
  - `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_B71_B72.md`
- B73/B74/B75:
  - `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_B73_B75.md`
- B76:
  - `hakozuna/hz4/docs/HZ4_PHASE21_B76_SMALL_ALLOC_PAGE_INIT_LITE_RESULTS.md`
- B77/B77a/B77b/B77c:
  - `hakozuna/hz4/docs/HZ4_PHASE23_B77C_NO_GO_20260217.md`
- B78/B79:
  - `hakozuna/hz4/docs/HZ4_PHASE23_B78_B79_NO_GO_20260217.md`
- B80/B81:
  - `hakozuna/hz4/docs/HZ4_PHASE23_B80_B81_KNOB_SWEEP_RESULTS_20260217.md`

---

## Cross-Phase Lessons

1. lock-path 頻度削減だけでは不十分
- lock-path 外に work を逃がしても、総 work 増で相殺されやすい。

2. remote-heavy 最適化は lane split を起こしやすい
- cross128 で効く箱が main/guard で退行するパターンが多発。

3. 仮説先行より観測先行
- B75 で periodic drain 仮説が実測 0% で否定された。

4. default baseline
- `B70 MidPageSupplyReservation` を維持し、常時固定費を増やす経路は避ける。

5. hz4_free() hot path is extremely sensitive
- Even "event-driven" conditional code adds measurable overhead (code bloat, TLS access).
- Parameter tuning is preferred over code changes for this boundary.
