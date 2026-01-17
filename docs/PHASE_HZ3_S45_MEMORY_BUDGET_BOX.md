# S45: Memory Budget Box

## Overview

16GB arena で `arena_alloc_full` が発生する問題を解決する。

**根本原因**: medium (4KB-32KB) の均等消費で segment が枯渇。Central pool に push された "free な run" が segment 内に残り続け、arena slot が解放されない。

## Phase 1: Segment-level Reclaim (NO-GO)

### 設計
- `hz3_arena_alloc()` 失敗直前に `hz3_mem_budget_reclaim_segments()` を呼び出し
- `free_pages == HZ3_PAGES_PER_SEG` (512) の segment を arena に返却

### 結果: NO-GO
- **原因**: medium free 時に `hz3_segment_free_run()` が呼ばれない
- free された object は tcache/central に戻る
- segment の `free_pages` は増えない（pages は「使用中」のまま）
- `free_pages == 512` の条件を満たす segment がない

## Phase 2: Run-level Reclaim (GO)

### 設計
Central pool から medium run を pop し、`hz3_segment_free_run()` で segment に返却する。
Segment が完全に空になったら (`free_pages == 512`)、Phase 1 で arena に返す。

### アルゴリズム

```c
hz3_mem_budget_reclaim_medium_runs():
  if (!t_hz3_cache.initialized) return 0  // TLS guard

  my_shard = t_hz3_cache.my_shard  // CRITICAL: my_shard only (data race prevention)

  for sc in 0..7 (medium size classes):
    while central[my_shard][sc] is not empty AND reclaimed < MAX_RECLAIM_PAGES:
      obj = hz3_central_pop(my_shard, sc)
      seg_base = obj & ~(HZ3_SEG_SIZE - 1)
      meta = hz3_segmap_get(seg_base)
      page_idx = (obj - seg_base) >> HZ3_PAGE_SHIFT
      pages = sc + 1
      hz3_segment_free_run(meta, page_idx, pages)
      reclaimed += pages
```

### Object → Segment/Page 逆算 (O(1))

| ステップ | 計算 | コスト |
|----------|------|--------|
| seg_base | `ptr & ~(HZ3_SEG_SIZE - 1)` | O(1) マスク |
| meta | `hz3_segmap_get(seg_base)` | O(1) radix lookup |
| page_idx | `(ptr - seg_base) >> HZ3_PAGE_SHIFT` | O(1) シフト |
| pages | `sc + 1` | O(1) |

### 呼び出しタイミング

`hz3_arena_alloc()` 失敗時の reclaim 順序:

```c
void* hz3_arena_alloc(uint32_t* out_idx) {
    void* seg = hz3_arena_try_alloc_slot(out_idx);
    if (seg) return seg;

#if HZ3_MEM_BUDGET_ENABLE
    // Emergency flush: local medium bins/inbox → central
    hz3_mem_budget_emergency_flush();

    // Phase 2: Central から medium run を回収 → segment に返す
    hz3_mem_budget_reclaim_medium_runs();

    // Phase 1: 空になった segment を arena に返す
    size_t reclaimed = hz3_mem_budget_reclaim_segments(HZ3_SEG_SIZE);
    if (reclaimed > 0) {
        seg = hz3_arena_try_alloc_slot(out_idx);
        if (seg) return seg;
    }
#endif

    hz3_oom_note("arena_alloc_full", ...);
    return NULL;
}
```

## 安全策

### 1. my_shard 制約 (CRITICAL)

**`hz3_segment_free_run()` はロックを持たない。** 他 shard の segment を触るとデータレースになる。

```c
// SAFETY: Only operate on my_shard to avoid data races.
// hz3_segment_free_run() modifies free_bits/free_pages without internal lock.
int my_shard = t_hz3_cache.my_shard;
```

### 2. TLS 初期化ガード

```c
if (!t_hz3_cache.initialized) {
    return 0;
}
```

### 3. 暴走防止

```c
#define HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES 4096  // default: 16MB worth of runs

if (reclaimed_pages >= HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES) {
    break;
}
```

### 4. 境界チェック

```c
if (page_idx + pages > HZ3_PAGES_PER_SEG) {
    continue;  // Safety: invalid run bounds
}
```

### 5. SegMap NULL チェック

```c
Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
if (!meta) {
    continue;  // Safety: external allocation or unmapped
}
```

## Fail-fast 条件

`arena_alloc_full` が再発した場合:

```c
// hz3_config.h
#define HZ3_OOM_FAILFAST 1  // abort() on arena_alloc_full
```

これにより `hz3_oom_note("arena_alloc_full", ...)` で即座に abort し、原因調査が可能になる。

## 設定フラグ

| フラグ | デフォルト | 説明 |
|--------|-----------|------|
| `HZ3_MEM_BUDGET_ENABLE` | 0 | Memory Budget Box 有効化 (scale lane のみ 1) |
| `HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES` | 4096 | Phase 2 の 1回あたり最大回収ページ数 |
| `HZ3_S45_FOCUS_RECLAIM` | 0 | S45-FOCUS: 1セグメント集中回収 |
| `HZ3_S45_FOCUS_PASSES` | 4 | S45-FOCUS: 多パス上限 |
| `HZ3_S45_EMERGENCY_FLUSH_REMOTE` | 0 | S45-FOCUS: remote stash 全量 flush |
| `HZ3_ARENA_PRESSURE_BOX` | 0 | S46: Global Pressure Box |
| `HZ3_OWNER_STASH_FLUSH_BUDGET` | 256 | S46: pressure 時 owner_stash flush 上限 |
| `HZ3_OOM_SHOT` | 0 | reclaim 時に one-shot ログ出力 |
| `HZ3_OOM_FAILFAST` | 0 | arena_alloc_full 時に abort() |

**デバッグ時**: `HZ3_OOM_SHOT=1 HZ3_OOM_FAILFAST=1` でビルドすると、reclaim 統計を出力しつつ、reclaim 失敗時に即座に停止する。

## 検証結果

### Phase 2 GO (2026-01-04)

| 条件 | 結果 |
|------|------|
| T=32 R=50 4096-32768, 1M iters | 70-82M ops/s, arena_alloc_full なし |
| T=32 R=50 4096-32768, 5M iters | 149M ops/s, arena_alloc_full なし |

### S45-FOCUS + S46 GO (2026-01-04)

| 条件 | 結果 |
|------|------|
| T=32 R=50 16-32768 | alloc failed = 0 / OOM ログなし |

### 実装メモ（Pressure + Focus の経路）

- arena 枯渇時に pressure epoch を進め、各スレッドが slow path 入口で flush する。
- 失敗スレッドは `sched_yield()` を挟んで回収を繰り返す（集中回収 + 再試行）。
- Focus は pressure 側のループと mem_budget 側の fallback の両方で実行（event-only）。
  - 現状は「alloc failed ゼロ」を優先して二重化しているため、後日整理する場合は A/B で切り分ける。

### ビルドフラグ

```makefile
HZ3_SCALE_DEFS := -DHZ3_NUM_SHARDS=32 -DHZ3_REMOTE_STASH_SPARSE=1 \
    -DHZ3_ARENA_SIZE=0x400000000ULL \
    -DHZ3_S42_SMALL_XFER=1 \
    -DHZ3_S44_OWNER_STASH=1 \
    -DHZ3_MEM_BUDGET_ENABLE=1 \
    -DHZ3_S45_FOCUS_RECLAIM=1 \
    -DHZ3_S45_EMERGENCY_FLUSH_REMOTE=1 \
    -DHZ3_ARENA_PRESSURE_BOX=1
```

## ファイル変更

| ファイル | 変更内容 |
|----------|----------|
| `include/hz3_config.h` | `HZ3_MEM_BUDGET_ENABLE`, `HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES` 追加 |
| `include/hz3_mem_budget.h` | Phase 1/2 API 宣言 |
| `src/hz3_mem_budget.c` | Phase 1/2 実装 |
| `src/hz3_arena.c` | reclaim 呼び出し (Phase 2 → Phase 1 順序) |

## 参照

- Phase 1 NO-GO 原因分析: medium free → tcache/central、segment free_pages 不変
- Phase 2 解決策: central から pop → segment_free_run → free_pages 増加 → reclaim 可能
