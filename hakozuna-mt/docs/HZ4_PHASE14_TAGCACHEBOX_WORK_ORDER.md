# HZ4 Phase 14: TagCacheBox（TLS page-tag cache）指示書

目的: R=0 の `hz4_free()` / `hz4_small_free_with_page_tls()` で発生する page header load を、
超小さい TLS キャッシュで削り、throughput を押し上げる。

重要: page base の再利用があるため、**世代(generation)一致**でのみヒット扱いにして正しさを守る。

箱理論（Box Theory）
- **箱**: `TagCacheBox`
- **境界1箇所**: `hz4_free()` の small routing 内だけ
- **戻せる**: `HZ4_TAGCACHE=0/1`（default OFF）
- **見える化**: SSOT 10runs median（R=0）+ R=90 sanity 1回
- **Fail-Fast**: generation mismatch を “miss 扱い” に落とす（abort しない）

---

## 0) A/B フラグ案

`hakozuna/hz4/core/hz4_config.h`

- `HZ4_TAGCACHE=0/1`（default OFF）
- `HZ4_TAGCACHE_CAP=4`（default 4, direct-mapped）

---

## 1) データ構造（TLS）

`hakozuna/hz4/core/hz4_tls.h`

追加（例）:
```c
typedef struct {
  uintptr_t key[HZ4_TAGCACHE_CAP];   // page base
  uint32_t  gen[HZ4_TAGCACHE_CAP];   // page generation
  uint32_t  tag[HZ4_TAGCACHE_CAP];   // PTAG32-lite (owner|sc)
} hz4_tagcache_t;
```

`hz4_tls` に `hz4_tagcache_t tagc;` を追加。

---

## 2) 世代(generation) の導入（page header）

`hakozuna/hz4/core/hz4_page.h`

small page に `uint32_t gen;` を追加（meta separation では扱いが変わるため、**perf lane のみ**）。

更新点:
- `hz4_alloc_page()` の初期化時に `page->gen++`（wrap OK）または seg 内の counter を採番
- page を再利用するたびに gen が変わることで、同一 base のキャッシュが誤ヒットしない

---

## 3) lookup（TagCacheBox）

`hakozuna/hz4/src/hz4_tcache.c` の `hz4_free()`（small path）で:

- `base = ptr & ~(PAGE_SIZE-1)`
- `idx = (base >> HZ4_PAGE_SHIFT) & (HZ4_TAGCACHE_CAP-1)`
- `if (key[idx]==base && gen[idx]==page->gen) tag=tag[idx] else tag=hz4_page_tag_load(page) and update cache`

注意:
- ヒット時は `tag` 読みを省略できるが、`gen` 読みは必要（4B）
- 期待値は小さいので、`CAP` は増やしすぎない（L1汚染で逆効果）

---

## 4) A/B（SSOT 10runs）

baseline:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
```

TagCache ON:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_TAGCACHE=1 -DHZ4_TAGCACHE_CAP=4'
```

判定:
- GO: R=0 **+2% 以上**
- NO-GO: **+1% 未満**（即アーカイブ）

---

## 5) 成功した場合の次

- CAP sweep（2/4/8）を SSOT 10runs で実施
- TagCache を RouteBox に統合（境界は `hz4_free()` 冒頭のまま）

---

## 結果（2026-01-26）

結論: **NO-GO**（改善が +2% 未満）

- R=0（SSOT 10runs median）:
  - baseline: 276.69M ops/s
  - TagCache ON（CAP=4）: 279.49M ops/s（+1.01%）
- R=90 sanity: 109.28M ops/s ✓

アーカイブ:
- `hakozuna/hz4/archive/research/hz4_phase14_tagcachebox/README.md`
