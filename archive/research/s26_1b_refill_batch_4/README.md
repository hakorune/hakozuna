# S26-1B: Refill batch（sc=5..7 を 4 に）NO-GO

目的:
- `16KB–32KB`（`sc=5..7`）の供給効率を上げるため、`HZ3_REFILL_BATCH[5..7]` を `3→4` に上げて A/B する。

変更:
- `HZ3_REFILL_BATCH[5,6,7]`: `3 → 4`

GO条件（主戦場）:
- S25-D（`16385–32768` を 20% に増幅）で hz3↔tcmalloc gap が改善（目安 +1% 以上）。
- `dist=app` mixed の gap が改善（回帰なし）。

結果:

| Config           | S25-D gap | dist=app gap |
|------------------|-----------|--------------|
| Baseline         | -2.3%     | -6.7%        |
| S26-1A (batch=3) | +0.5%     | -5.0%        |
| S26-1B (batch=4) | -0.1%     | -6.0%        |

判定:
- **NO-GO**
  - `batch=4` は `16KB–32KB` の勝ちが崩れ、`dist=app` も悪化傾向。

メモ:
- “batchを上げるほど良い” ではなく、critical section / refillの局所性 / cache pressure のバランスで当たりが変わる。
- 現時点の固定値は S26-1A（`HZ3_REFILL_BATCH[5..7]=3`）を採用する。

次:
- `dist=app` の残差（約 -5%）を詰めるには S26-2（span carve）など “供給の形” を変える箱が候補。

