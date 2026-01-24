# HZ4 Phase 6: P3.5 Store Reduction Work Order

目的:
- 無駄な store を削って hot path を軽くする

---

## 1) 変更内容

### 1.1 tcache_pop の obj->next=NULL を debug のみに

```c
static inline void* hz4_tcache_pop(hz4_tcache_bin_t* bin) {
    void* obj = bin->head;
    if (obj) {
        bin->head = hz4_obj_get_next(obj);
#if HZ4_FAILFAST
        hz4_obj_set_next(obj, NULL);  // P3.5: debug only
#endif
#if HZ4_TCACHE_COUNT
        if (bin->count > 0) bin->count--;
#endif
    }
    return obj;
}
```

### 1.2 bin->count を HZ4_TCACHE_COUNT でガード

```c
// P3.5: Tcache count tracking (statistics only, disable for perf)
#ifndef HZ4_TCACHE_COUNT
#define HZ4_TCACHE_COUNT 1  // default ON (A/B: try 0 for perf)
#endif
```

---

## 2) 結果 (2026-01-23)

### A/B Test (10 rounds, alternating)

| Round | COUNT=1 | COUNT=0 | Winner |
|-------|---------|---------|--------|
| 1 | 70.3M | 87.1M | COUNT=0 |
| 2 | 87.1M | 104.7M | COUNT=0 |
| 3 | 71.4M | 95.7M | COUNT=0 |
| 4 | 77.0M | 84.6M | COUNT=0 |
| 5 | 102.6M | 69.9M | COUNT=1 |
| 6 | 98.3M | 72.7M | COUNT=1 |
| 7 | 73.2M | 72.2M | COUNT=1 |
| 8 | 70.1M | 109.7M | COUNT=0 |
| 9 | 110.6M | 81.3M | COUNT=1 |
| 10 | 73.2M | 75.3M | COUNT=0 |

- **COUNT=0 勝率**: 6/10 (60%)
- **平均**: COUNT=1 ~83.4M vs COUNT=0 ~85.3M (**+2.3%**)

### 判定

1. **obj->next=NULL debug-only**: **GO** (release では常にスキップ)
2. **HZ4_TCACHE_COUNT=0**: **Marginal** (+2.3%、高分散で信頼性低)

### 採用方針

- `obj->next = NULL` の `#if HZ4_FAILFAST` ガード: **採用** (常時有効)
- `HZ4_TCACHE_COUNT`: **デフォルト ON のまま** (信頼性不足)

---

## 3) 教訓

- ベンチマーク環境の高分散で微細な最適化の効果測定が困難
- +1% 閾値の判定には安定した環境が必要
- store 削減の効果は理論的には存在するが、ノイズに埋もれやすい

---

## 4) 変更ファイル

- `core/hz4_config.h`: `HZ4_TCACHE_COUNT` フラグ追加
- `src/hz4_tcache.c`: `hz4_tcache_push`, `hz4_tcache_pop`, `hz4_refill_from_inbox_splice` 更新
