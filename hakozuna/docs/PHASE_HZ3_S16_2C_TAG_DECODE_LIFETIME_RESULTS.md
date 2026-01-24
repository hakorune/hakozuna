# PHASE_HZ3_S16-2C: tag decode lifetime 分割（結果）

目的:
- `hz3_free` hot 経路の **レジスタ圧（spills）**を下げる狙いで、tag decode を “使う直前” に分割し live range を短縮する。

参照（指示書）:
- `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_WORK_ORDER.md`

---

## 結果（要約）

結論: **NO-GO**

理由:
- mixed が改善せず（-1.2%）
- instructions が増加（+2.8%）
- prolog/spills が削減されず、命令数が変わらなかった

計測（mixed / perf stat）:

| 指標 | baseline | S16-2C | 変化 |
|------|----------|--------|------|
| mixed median | 91.3M | 90.2M | -1.2% |
| instructions | 1,408M | 1,447M | +2.8% |

観測（補足）:
- callee-saved registers（例: r12/r13）が関数エントリで push され、lifetime split の効果が出ていない可能性が高い。
- decode の式を分割しても、生成物の形（prolog/spills）に支配される。

---

## 学び（S16の次手）

- “decode 式を分割/簡略化する” だけでは改善しない可能性が高い。
- 改善するには、**プロローグ/エピローグ（callee-saved push/pop）や spills が発生しない形の hot 生成**が必要。

次の候補（Work Order）:
- `hakozuna/hz3/docs/PHASE_HZ3_S16_2D_FREE_FASTPATH_SHAPE_WORK_ORDER.md`
