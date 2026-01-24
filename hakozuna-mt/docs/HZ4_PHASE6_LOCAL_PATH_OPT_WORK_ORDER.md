# HZ4 Phase 6: Local Path Optimization Work Order

目的:
- R=0 での **local 経路の遅さ (-29%)** を改善
- hot path の固定費（magic check / TLS / ownership 依存）を削減

---

## 0) ルール（Box Theory）

- 変更は **Small/TLS/Metadata Box** 内で完結
- 境界は 1 箇所（alloc/free hot path）
- A/B は compile-time で切替可能に

---

## 1) 観測されたボトルネック

優先度順:
1. **magic check + page meta cache miss** (10–15 cycles)
2. **double TLS indirection (init check)** (5–10 cycles)
3. **ownership check の data dependency** (5–10 cycles)
4. **metadata フィールド分散** (3–5 cycles)

hz3 の強み:
- bin_plus1==0 の **magic check なし**
- TLS direct access
- metadata packed

---

## 2) 施策案（A/B）

### A) magic check を hot path から外す

- `HZ4_LOCAL_MAGIC_CHECK=0` で release では省略
- debug only (`HZ4_FAILFAST`) では維持

### B) TLS アクセスを 1 回に統合

- `hz4_tls_get()` を hot path で 1 回だけ
- init 判定を fast path 外へ寄せる

### C) ownership check を reduce

- local fast path の直前で only once
- `owner_tid` の読みは 1 回に固定

### D) metadata packed

- page header で locality を改善
- 影響範囲が大きいので別 A/B lane 推奨

---

## 3) 変更箇所（候補）

- `core/hz4_types.h`
- `core/hz4_page.h`
- `core/hz4_tls.h`
- `src/hz4_tcache.c`
- `core/hz4_collect.inc` (local 分岐があれば)

---

## 4) A/B 例

```
# magic check OFF
CFLAGS="-O2 -Wall -Wextra -Werror -std=c11 -DHZ4_LOCAL_MAGIC_CHECK=0"

# TLS fastpath merge
CFLAGS="-O2 -Wall -Wextra -Werror -std=c11 -DHZ4_TLS_SINGLE=1"
```

---

## 5) SSOT

```
ITERS=20000000 WS=400 MIN=16 MAX=2048
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_args $ITERS $WS $MIN $MAX
```

---

## 6) NO-GO 判定

- **R=0 が +10% 未満**なら NO‑GO
- correctness 低下があれば即撤退

---

## 7) 結果（SSOT, 2026-01-23）

Baseline (R=0, T=16, 16–2048): **~257M**（hz3 **~360M**, ratio **0.71**）

| Mode | ops/s | vs Baseline | vs hz3 |
|------|-------|-------------|--------|
| Baseline | ~257M | - | 0.71 |
| A (magic OFF) | ~257M | 0% | 0.71 |
| B (TLS+page dedup) | ~255M | -1% | 0.71 |
| A+B | ~262M | +2% | 0.73 |
| hz3 | ~360M | - | - |

**結論**: +10% 目標未達 → **NO‑GO**（default OFF 維持）

### 実装した変更（フラグ化）
- `HZ4_LOCAL_MAGIC_CHECK` / `HZ4_TLS_SINGLE`
- `hz4_page_valid()` 条件分岐
- `hz4_small_free_with_page()`（page 二重計算削減）
- `hz4_alloc_tls_get()` に `__builtin_expect`

### perf 追加分析（R=0）
- TLS 取得: hz4 `hz4_tls_get@plt` vs hz3 直接 `%fs`（~6%）
- Magic routing: hz4 3回比較 vs hz3 page_tag32 1回（~9%）
- 命令数: hz4 3.70B vs hz3 2.92B（+27%）
- L1 miss: hz4 58.9M vs hz3 38.2M（+54%）
- IPC: hz4 1.09 vs hz3 1.27（-14%）

### 次の優先案
1. **TLS 直接アクセス化**（中 / +6%）
2. **Page Tag Table** 導入（高 / +9%）
3. **TLS 構造統合**（中 / +3%）

---

## 7) 結果（SSOT, 2026-01-23）

### 実装内容

- `core/hz4_config.h`: `HZ4_LOCAL_MAGIC_CHECK`, `HZ4_TLS_SINGLE` フラグ追加
- `core/hz4_page.h`: `hz4_page_valid()` を `#if HZ4_FAILFAST || HZ4_LOCAL_MAGIC_CHECK` で条件分岐
- `src/hz4_tcache.c`:
  - `hz4_alloc_tls_get()` に `__builtin_expect` 追加
  - `hz4_small_free_with_page(ptr, page)` を新設（page 二重計算削減）
  - `hz4_free()` から page を渡すように修正

### ベンチ結果 (T=16 R=0 16-2048, x3 median)

| Mode | ops/s | vs Baseline | vs hz3 |
|------|-------|-------------|--------|
| Baseline | ~257M | - | 0.71 |
| A (magic OFF) | ~257M | 0% | 0.71 |
| B (TLS + page dedup) | ~255M | -1% | 0.71 |
| A+B (combined) | ~262M | +2% | 0.73 |
| hz3 | ~360M | - | - |

### 評価

- **A (magic check OFF)**: 効果なし（small の二重 magic は元々軽い）
- **B (TLS + page dedup)**: 効果なし（TLS 最適化はコンパイラが既に行っている）
- **A+B (combined)**: +2%（測定誤差の範囲）
- hz4/hz3 = 0.73 (-27%)、改善前の 0.71 (-29%) とほぼ同等

### 判定

**NO-GO** (+10% 未満)

### 次のアクション

- ~~TLS 直接アクセス化（中 / +6%）~~ → **TLS_DIRECT_ACCESS で GO (+8.8%)**
  - 詳細: `HZ4_PHASE6_TLS_DIRECT_ACCESS_WORK_ORDER.md`
- Page Tag Table 導入（高 / +9%）
- TLS 構造統合（中 / +3%）

※ 他のボトルネック（metadata packed / C: ownership check）は影響範囲が大きいため保留
