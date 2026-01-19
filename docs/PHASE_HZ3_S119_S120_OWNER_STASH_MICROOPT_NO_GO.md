# PHASE HZ3 S119/S120: OwnerStash MicroOpt（prefetch/PAUSE）— NO-GO 記録

目的:
- `hz3_owner_stash_pop_batch()` 周辺の micro-opt（prefetch 距離/位置、CAS loop の `pause`）で、
  xmalloc-testN の伸び代が残っていないかを確認する。

前提（既に効いているもの）:
- S112 FullDrainExchange（scale lane 既定）により、bounded drain の CAS retry 起因の O(n) re-walk は解消済み。
- S44-4（early prefetch 等）は scale lane 既定で有効化済みで、pointer chase latency は既に軽減されている。

---

## 結果（A/B）

| 改善案 | 内容 | 結果 | 判定 |
|--------|------|------|------|
| S119 | `PREFETCH_DIST=2`（legacy path） | -1.08% | **NO-GO** |
| S119-2 | S112 path に prefetch 追加 | -0.43% | **NO-GO** |
| S120 | CAS loop に `PAUSE` 追加 | -0.09% | **NO-GO** |

解釈:
- S112 パスへの追加 prefetch は、固定費（命令/分岐/レジスタ）増が benefit を上回る。
- CAS loop の `pause` は、競合（retry）が低い workload ではほぼ効果が出ない。

運用:
- scale lane 既定には反映しない。
- 追加フラグ化（opt-in）も現時点では不要（勝ち筋が薄い）。

