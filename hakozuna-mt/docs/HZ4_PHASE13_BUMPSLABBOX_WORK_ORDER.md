# HZ4 Phase 13: BumpSlabBox（populate 廃止 / bump refill）指示書

目的: 新規 page の “全obj初期化(populate)” を避け、refill 境界で必要分だけ carve して R=0 を押し上げる。

箱理論（Box Theory）
- **箱**: `BumpSlabBox`（研究箱）
- **境界1箇所**: `hz4_refill()` の “alloc_page → populate” 部分のみを差し替える
- **戻せる**: **NO-GO のため本線から除去済み**（研究箱でのみ再実装して A/B）
- **Fail-Fast**: N/A（本線から除去済み）

---

## 実装概要（perf lane only）

- `page->capacity` を算術で一度だけ算出（obj本体は触らない）
- `page->used_count` を bump cursor として利用
- TLS に `bump_page[sc]` を持ち、同じ page を exhaustion まで使い切る
- refill 時に `HZ4_BUMP_REFILL_BATCH` 個だけ carve し、bin に push（従来の intrusive list）

---

## A/B（SSOT 10runs）

baseline:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
```

BumpSlab ON:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_BUMP_SLAB=1'
```
※ 現状は `HZ4_BUMP_SLAB` の実装が本線に無いので、そのままではビルドできません（研究箱で復活させた場合のみ）。

判定:
- GO: R=0 **+5% 以上**
- NO-GO: R=0 **+2% 未満** または R=90 が明確に退行

---

## 結果（2026-01-26）

結論: **NO-GO**

SSOT（T=16, iters=2,000,000, runs=10 median, pinning=0-15）
- R=0:
  - baseline: 275.03M ops/s（min 246.82M / max 280.17M）
  - bump:     277.00M ops/s（+0.72%）（min 266.69M / max 283.99M）
- R=90:
  - baseline: 108.08M ops/s
  - bump:     104.72M ops/s（-3.1%）

アーカイブ:
- `hakozuna/hz4/archive/research/hz4_phase13_bump_slab/README.md`
