# PHASE_HZ3_S64_P1: Purge Coalesce + Still-Free Check

目的: S64 の purge コストを下げつつ RSS を改善する（SSOT T=16/R=90）。
現状は **S64 ON が baseline より遅く、RSS も増える**ため、S64 を使う価値が薄い。
P1 は Box 境界（epoch/tick）を守ったまま **madvise 回数と粒度を改善**する。

## 方針（最小差分）
1) **Purge entry の tail coalesce**
- `hz3_s64_purge_delay_enqueue()` で **直近の tail と隣接する run を結合**
- 例: `tail.page_idx + tail.pages == new.page_idx` のとき `tail.pages += new.pages`
- これだけで **madvise の粒度が増える**（4KB→連続ページ）

2) **Still-free check**
- `hz3_s64_purge_delay_tick()` の madvise 前に **free_bits を再確認**
- free でない場合は **purge をスキップ**（安全性 + 無駄 purge 回避）

## 実装対象
- `hakozuna/hz3/src/hz3_s64_purge_delay.c`
- `hakozuna/hz3/include/hz3_config_rss_memory.h`

## 追加フラグ
`hz3_config_rss_memory.h` に A/B フラグ追加:

```
#ifndef HZ3_S64_PURGE_COALESCE
#define HZ3_S64_PURGE_COALESCE 0
#endif

#ifndef HZ3_S64_PURGE_CHECK_FREE
#define HZ3_S64_PURGE_CHECK_FREE 0
#endif
```

## 実装詳細

### 1) Tail coalesce（enqueue 側）
- queue が空でなく、`tail` 直前 entry と隣接なら結合
- 同一 `seg_base` のみ結合
- `retire_epoch` は「新しい方」を採用

擬似コード:
```
if (!s64_queue_is_empty(q)) {
  Hz3S64PurgeEntry* last = &q->entries[(q->tail - 1) & QUEUE_MASK];
  if (HZ3_S64_PURGE_COALESCE && last->seg_base == seg_base) {
    if (last->page_idx + last->pages == page_idx) {
      last->pages += pages;
      last->retire_epoch = t_hz3_cache.epoch_counter;
      return;  // merged
    }
  }
}
// fallback: normal enqueue
```

### 2) Still-free check（tick 側）
- `Hz3SegHdr* hdr = (Hz3SegHdr*)ent->seg_base;`
- free_bits を走査して **全ページ free なら purge**
- free でなければ skip（queue からは取り除く）

最低限の helper（purge_delay 内に static inline でOK）:
```
static inline int s64_run_is_free(const uint64_t* bits, size_t page_idx, size_t pages) {
  for (size_t i = 0; i < pages; i++) {
    size_t idx = page_idx + i;
    size_t word = idx / 64; size_t bit = idx % 64;
    if ((bits[word] & (1ULL << bit)) == 0) return 0;
  }
  return 1;
}
```

## A/B 計測
SSOT（T=16, R=90, 16–2048, RUNS=10 median）

### Baseline
```
-DHZ3_S64_RETIRE_SCAN=1 -DHZ3_S64_PURGE_DELAY=1 -DHZ3_S64_STATS=1
```

### P1a: Coalesce only
```
-DHZ3_S64_PURGE_COALESCE=1
```

### P1b: Coalesce + Still-free check
```
-DHZ3_S64_PURGE_COALESCE=1 -DHZ3_S64_PURGE_CHECK_FREE=1
```

### P1c: P1b + Light A (delay=8)
```
-DHZ3_S64_PURGE_COALESCE=1 -DHZ3_S64_PURGE_CHECK_FREE=1 -DHZ3_S64_PURGE_DELAY_EPOCHS=8
```

## 判定基準
- **速度**: baseline 比 -2% 以内（-3% なら NO-GO）
- **RSS**: baseline 比 -10% 以上で GO
- **madvise_calls** が減ること（S64 stats で確認）

## 記録
結果は次に追記:
- `docs/benchmarks/2026-01-24_S64_LIGHT_TUNING_RESULTS.md`
- 必要なら別ファイル `2026-01-24_S64_P1_PURGE_COALESCE_RESULTS.md`

NO-GO の場合は `hakozuna/archive/research/hz3_s64_p1_purge_coalesce/README.md` に移管。
