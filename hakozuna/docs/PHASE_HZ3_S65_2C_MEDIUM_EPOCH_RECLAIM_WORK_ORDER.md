# PHASE_HZ3_S65_2C: Medium Epoch Reclaim (Coverage Extension)

Status: READY  
Date: 2026-01-09  
Track: hz3  
Depends: S65 ReleaseBoundaryBox + ReleaseLedgerBox already integrated

---

## 0) 目的（SSOT）

S65 の coverage を **medium (4KB–32KB)** に拡張する。  
epoch で central → segment reclaim を行い、**release boundary 経由で ledger に載せる**。

狙い:
- RSS 主成分に当たる medium の “返せる状態” を作る
- free_bits 更新を S65 boundary に集約（箱の境界 1 箇所）

---

## 1) 設計方針（境界の固定）

**Medium reclaim の free_bits 更新はすべて boundary で行う。**

新規 API（S65 boundary に追加）:

```
int hz3_release_range_definitely_free_meta(
    Hz3SegMeta* meta,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason);
```

特徴:
- meta->free_bits / meta->free_pages を更新
- meta->sc_tag をクリア（hz3_segment_free_run と同等）
- ledger enqueue（range coalesce）
- arena 範囲チェック
- **page_idx==0 を許可**（medium は header page 無し）

---

## 2) 実装手順

### Step 1: Boundary に meta版を追加（必須）

Files:
- `hakozuna/hz3/include/hz3_release_boundary.h`
- `hakozuna/hz3/src/hz3_release_boundary.c`

追加:
- `hz3_release_range_definitely_free_meta(...)`
  - safety: meta != NULL / seg_base != NULL / page_idx+pages <= HZ3_PAGES_PER_SEG
  - arena 範囲チェック: `hz3_os_in_arena_range(meta->seg_base, HZ3_SEG_SIZE)`
  - `hz3_bitmap_mark_free(meta->free_bits, page_idx, pages)`
  - `meta->free_pages += pages`
  - `meta->sc_tag[start_page..start_page+pages)` を 0 クリア
  - ledger enqueue（reason = HZ3_RELEASE_MEDIUM_RUN_RECLAIM）

注意:
- small/sub4k は **既存の hdr 版**を維持（page_idx==0 禁止）
- “single entry point” は **boundary モジュール**で保証する（関数2本でも箱は1つ）

---

### Step 2: Medium reclaim tick を追加（epoch）

新規ファイル:
- `hakozuna/hz3/include/hz3_s65_medium_reclaim.h`
- `hakozuna/hz3/src/hz3_s65_medium_reclaim.c`

API:
```
void hz3_s65_medium_reclaim_tick(void);
```

アルゴリズム（S61 の dtor を epoch へ移植）:
1. `my_shard = t_hz3_cache.my_shard`
2. budget 以内で central を pop
3. obj → seg_base → meta（`hz3_segmap_get(seg_base)`）
4. `page_idx` / `pages` を算出（`hz3_sc_to_pages(sc)`）
5. `hz3_release_range_definitely_free_meta(meta, page_idx, pages, HZ3_RELEASE_MEDIUM_RUN_RECLAIM)`
6. pack pool 更新（`hz3_pack_on_free`）

budget:
- `HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS`（デフォルト 256）
- または pages 予算（必要なら `..._BUDGET_PAGES`）

Fail-Fast:
- `page_idx + pages > HZ3_PAGES_PER_SEG` → skip/abort（debug opt-in）

---

### Step 3: epoch に接続

File:
- `hakozuna/hz3/src/hz3_epoch.c`

接続位置（S65 ledger の前）:
```
#if HZ3_S65_RELEASE_COVERAGE
    hz3_s65_medium_reclaim_tick();
#endif
```

順序:
1. S64 retire scan
2. **S65 medium reclaim**
3. S65 ledger purge

---

### Step 4: config フラグ

File:
- `hakozuna/hz3/include/hz3_config_rss_memory.h`

追加:
```
#ifndef HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS
#define HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS 256
#endif
```

適用:
- `HZ3_S65_RELEASE_COVERAGE=1` を ON のときに効く設計で良い

---

## 3) 可視化（one-shot）

必要なら統計追加:
- `reclaim_runs`, `reclaim_pages`, `boundary_calls`

atexit 1回のみ。

---

## 4) ビルド & 評価

Build:
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S65_RELEASE_BOUNDARY=1 -DHZ3_S65_RELEASE_LEDGER=1 -DHZ3_S65_RELEASE_COVERAGE=1 -DHZ3_S65_STATS=1'
```

Bench:
```
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 200000 10 3 0 4 256 4096 1 100 1
```

観測:
- `[HZ3_S65]` の `purged_pages` が増えるか
- RSS が idle 後に下がるか
- `madvise_calls` が pages と同数になっていないか（coalesce 効果）

---

## 5) GO / NO-GO

GO:
- medium reclaim が boundary 経由で動く
- RSS 主成分に変化が出る（idle 後に低下傾向）

NO-GO:
- reclaim が動かない（meta 取得/境界失敗）
- RSS が不動

