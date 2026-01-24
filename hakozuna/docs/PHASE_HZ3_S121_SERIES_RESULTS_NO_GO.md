# PHASE HZ3 S121: PageLocalRemote / PageQ experiments — Series Summary（NO-GO）

目的:
- owner stash / remote drain の “ページ単位の集約” で、MT remote（r50/r90 など）の throughput を押し上げられるかを検証する。
- 追加の複雑さが `hz3_small_v2_alloc_slow()` に波及（cascade）しないことを重視する。

前提:
- S112 FullDrainExchange（atomic_exchange で stash を全量 drain）は既に **GO かつ scale lane 既定**。
- S44-4 の early prefetch など、stash pop の pointer chase latency 隠しは既に導入済み。

---

## 結果（SSOT: no-S121 vs S121）

結論: **S121_PAGE_LOCAL_REMOTE は NO-GO**（MT remote で大幅退行）。

| 条件 | no-S121 | S121-G3 | 退行 |
|------|---------|---------|------|
| R=90% | 56.63M | 39.85M | **-29.6%** |
| R=50% | 68.89M | 45.23M | **-34.3%** |

比較条件（代表）:
- T=4 / R=50%（mt remote）

注意:
- ここでは “S121 なし” を基準にする（S121 内部の亜種比較だけだと見誤る）。

---

## 結果（S121シリーズ内の亜種）

| 実装 | 方式 | T=4 R=50% | 判定 | 失敗原因 |
|------|------|-----------|------|----------|
| S121-C | drain-all（atomic_exchange 1回） | 67M（baseline） | **GO（既存）** | - |
| S121-D | Page Packet（batch push） | -82% | **NO-GO** | バッチ管理の固定費が勝つ |
| S121-E | Cadence Collect（page scan） | 40M（-40%） | **NO-GO** | scan 固定費（O(n)）が重い |
| S121-F v1 | 4-way sub-shard | 45M（-25%） | **NO-GO** | pop 側の固定費が増える |
| S121-F v2 | per-subshard requeue | 47M（-25%） | **NO-GO** | 配列index等の固定費が増える |
| S121-F v3 | merge-once | 45M（-35%） | **NO-GO** | 4x atomic_exchange の固定費 |
| S121-H | Budget Drain（K pages） | 44M（-30%） | **NO-GO** | per-page CAS/pop が重い |

perf 観測（代表）:
- `alloc_slow` self% が **3.74% → 53.57%**（S121-C → S121-F v2、+14x）
- pop path に複雑さを足すと alloc_slow が cascade して throughput が崩壊する。

設計的結論:
- sub-sharding は **push（multi-producer）**には理屈があるが、**pop（single-consumer）**では恩恵が薄く、固定費だけ増えやすい。

---

## 運用（台帳）

- S121（page-local remote/pageq 系）は **当面見送り（NO-GO）**。
- 既定の勝ち筋は S112 + S44-4 のラインで維持。
- 研究を続けるなら “構造変更” 側（例: page-local を hot path から外す / 別レーンに隔離）で箱を切る。
