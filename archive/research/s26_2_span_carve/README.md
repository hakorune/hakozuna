# S26-2: Span carve（16KB–32KB）NO-GO

目的:
- S25 triage で疑われた `16KB–32KB` 近傍の供給効率（segment/bitmap/tag の固定費）を amortize するため、
  “run=1obj” をやめて **1 span を確保→複数objに切って bin を満たす**（event-only）方式を試す。

GO条件:
- S25-D（`16385–32768` を 20% に増幅）で hz3↔tcmalloc gap が改善（目安 +1% 以上、または gap ≤ 1%）。
- `dist=app` mixed の gap が改善（回帰なし）。

結果（要約）:

| Config             | S25-D gap | dist=app gap | 状態  |
|--------------------|-----------|--------------|-------|
| S26-1A (batch=3)   | +0.5%     | -5.0%        | GO    |
| S26-2 (span carve) | -0.7%     | -4.4%        | NO-GO |

判定:
- **NO-GO**
  - `dist=app` は僅かに改善したが、S25-D（16KB–32KB増幅）で悪化し、主戦場で負け。

補足:
- “span carve が悪い” というより、現段階の実装は「span生成/タグ付け/返却」の固定費や局所性で負けた可能性がある。
- 16KB–32KB は `HZ3_REFILL_BATCH[5..7]=3` の tuning が最も素直に効いたため、当面は S26-1A を維持する。

次:
- `dist=app` の残差は small（16–2048）側が主犯という切り分けがあるため、次は small 側の箱へ（S27）。

