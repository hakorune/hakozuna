# HZ4 Phase 6: Large Free Header Fix Work Order

目的:
- full‑range (16–65536) crash の原因である **hz4_large_free の header 位置ズレ**を修正
- Fail‑Fast を維持しつつ正しい munmap を実行

---

## 0) ルール（Box Theory）

- 変更は **Large Box** 内で完結
- 境界は `hz4_large_malloc/free` の 1 箇所
- A/B は不要（バグ修正）

---

## 1) 根本原因（要約）

- malloc: `return base + hdr_offset`（例: +32）
- free: `ptr & ~(HZ4_PAGE_SIZE-1)` で header を探す → **位置不一致**
- magic check が失敗して abort

---

## 2) 修正方針（最小）

### A案（推奨）: header を ptr 直前に置く

- malloc:
  - `h = (hdr*)base;`
  - `ret = (void*)((uint8_t*)base + sizeof(hdr));`
- free:
  - `h = (hdr*)((uint8_t*)ptr - sizeof(hdr));`
  - `base = (void*)h;`
- `h->total` で munmap

### B案: header に base を保存

- malloc:
  - `h = (hdr*)((uint8_t*)base + hdr_offset);`
  - `h->base = base;`
- free:
  - `h = (hdr*)((uint8_t*)ptr - hdr_offset);`
  - `munmap(h->base, h->total);`

**推奨**: A案（シンプルで fast path が短い）

---

## 3) 変更箇所

- `hakozuna/hz4/src/hz4_large.c`
- `hakozuna/hz4/include/hz4_large.h`（必要なら header layout を明記）

---

## 4) Fail‑Fast（維持）

- `magic` / `total` / `size` の整合チェック
- `total` が `HZ4_PAGE_SIZE` 境界であることを検証

---

## 5) テスト / SSOT

```
make -C hakozuna/hz4 clean test

# full-range MT remote
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 65536 90 65536
```

---

## 6) 記録フォーマット

```
LargeFreeFix
status: <OK/FAIL>
notes: <crash 解消 여부>
```

---

## 7) 結果（SSOT, 2026-01-23）

**採用案**: A案（ptr 直前に header 配置）

**変更内容**:
- `hz4_large_free`: `ptr & ~(PAGE_SIZE-1)` → `ptr - hz4_large_hdr_size()`
- `hz4_large_usable_size`: 同様

**テスト結果**: All tests passed

**ベンチ結果** (T=16 R=90 16-65536):

| Allocator | ops/s | 比率 |
|-----------|-------|------|
| hz4 | 1.66M | - |
| hz3 | 1.76M | baseline |
| hz4/hz3 | - | 0.94 (-6%) |

**status: OK**
**notes: crash 解消、full-range 完走確認**
