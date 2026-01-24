# S121: Page-Local Remote (Option A) Design

## Status: DRAFT

## 目的

xmalloc-testN (remote-heavy) で mimalloc に勝つため、remote-free のアーキテクチャを根本から変更。

## 背景

### 現状 (S44 MPSC Object Stash)

```
Thread A (producer)          Thread B (owner/consumer)
     |                              |
     | free(ptr)                    | malloc()
     v                              v
+------------+              +----------------+
| push_list  | --CAS push-> | owner_stash   |
|            |              | (MPSC queue)  |
+------------+              +----------------+
                                    |
                                    v pop_batch
                            +----------------+
                            | tcache (TLS)   |
                            +----------------+
```

**問題点**:
- 全 remote-free が `(owner, sc)` 1点に集中
- S112 で CAS retry は解決したが、spill_overflow の pointer chase が残る
- perf: `hz3_owner_stash_pop_batch` が 19.92%

### mimalloc のアプローチ

```
Thread A                    Thread B
     |                           |
     | free(ptr)                 |
     v                           |
+------------+                   |
| page->local_free | (page-local, no CAS contention)
+------------+                   |
     |                           |
     | (deferred)                |
     v                           v
+------------+              +------------+
| page->free | <-- collect -- | alloc path |
+------------+              +------------+
```

**特徴**:
- page-local remote list (競合がページ数に分散)
- lazy collection (alloc 時に回収)

---

## 設計方針

### 核心のアイデア

> **stash に object を積む → stash に page を積む**

- 競合が `(owner, sc) 1点` → `ページ数` に分散
- pop_batch で 1ページ drain = 複数 object 取得
- pointer chase が根本から減る

### 箱境界の維持 (A/B テスト可能)

S44 の外部 API は変更しない:
- `hz3_owner_stash_push_list(owner, sc, head, tail, n)`
- `hz3_owner_stash_pop_batch(owner, sc, out, want)`

中身だけを compile-time で切り替え:
- **A (現行)**: object-list stash (MPSC head + pop_batch)
- **B (Option A)**: page-local remote + page worklist (MPSC pages)

---

## フラグ設計

```c
// hz3_config_scale.h

// S121: Page-Local Remote (Option A)
// 0: current object-stash (default)
// 1: page-local remote + page worklist
#ifndef HZ3_S121_PAGE_LOCAL_REMOTE
#define HZ3_S121_PAGE_LOCAL_REMOTE 0
#endif

// Compile guards
#if HZ3_S121_PAGE_LOCAL_REMOTE
#if !HZ3_S44_OWNER_STASH
#error "HZ3_S121 requires HZ3_S44_OWNER_STASH=1"
#endif
#endif

// S121 Stats (opt-in)
#ifndef HZ3_S121_STATS
#define HZ3_S121_STATS 0
#endif
```

---

## データ構造

### 1. Page Header 拡張 (Hz3SmallV2PageHdr)

現在の構造 (16B):
```c
typedef struct {
    uint32_t magic;
    uint8_t  owner;
    uint8_t  sc;
    uint16_t flags_or_live;
    uint64_t reserved;  // ← ここを使う
} Hz3SmallV2PageHdr;
```

S121 拡張 (32B):
```c
typedef struct {
    uint32_t magic;
    uint8_t  owner;
    uint8_t  sc;
    uint16_t flags_or_live;

#if HZ3_S121_PAGE_LOCAL_REMOTE
    _Atomic(void*) remote_head;      // intrusive object list (MPSC)
    _Atomic(uint8_t) remote_state;   // 0=idle, 1=queued_or_processing
    uint8_t pad[7];
    void* page_qnext;                // worklist node link
#else
    uint64_t reserved;
#endif
} Hz3SmallV2PageHdr;
```

**remote_state 状態機械**:
- `0 (idle)`: worklist にいない、drain 対象外
- `1 (queued_or_processing)`: worklist に入っている or 処理中

### 2. Page Worklist (MPSC)

```c
// owner_stash/hz3_owner_stash_globals.inc

#if HZ3_S121_PAGE_LOCAL_REMOTE
typedef struct {
    _Atomic(void*) head;  // page header pointer (intrusive)
} Hz3PageWorklist;

static Hz3PageWorklist g_owner_pageq[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];
#endif
```

---

## 実装

### Push Path (producer)

```c
// owner_stash/hz3_owner_stash_push.inc

#if HZ3_S121_PAGE_LOCAL_REMOTE
HZ3_HOT void hz3_owner_stash_push_list(
    uint8_t owner, uint8_t sc,
    void* head, void* tail, size_t n)
{
    // Walk object list, group by page
    void* cur = head;
    while (cur) {
        void* next = hz3_obj_get_next(cur);
        Hz3SmallV2PageHdr* page = hz3_small_v2_obj_to_page(cur);

        // 1. Prepend to page->remote_head (MPSC)
        void* old_head;
        do {
            old_head = atomic_load_explicit(&page->remote_head, memory_order_relaxed);
            hz3_obj_set_next(cur, old_head);
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_head, &old_head, cur,
            memory_order_release, memory_order_relaxed));

        // 2. Enqueue page if state 0→1
        uint8_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(
                &page->remote_state, &expected, 1,
                memory_order_acq_rel, memory_order_relaxed)) {
            // Successfully transitioned 0→1, enqueue page
            pageq_push(owner, sc, page);
            HZ3_S121_STAT_INC(page_state_0to1);
        }

        cur = next;
        HZ3_S121_STAT_INC(stash_push_objs);
    }
}
#endif
```

### Pop Path (consumer)

```c
// owner_stash/hz3_owner_stash_pop.inc

#if HZ3_S121_PAGE_LOCAL_REMOTE
HZ3_HOT int hz3_owner_stash_pop_batch(
    uint8_t owner, uint8_t sc,
    void** out, int want)
{
    int got = 0;
    HZ3_S121_STAT_INC(stash_pop_calls);

    // Step 0: TLS spill から取得 (既存の S67 spill_array/overflow を流用)
    got = hz3_spill_pop(sc, out, want);
    if (got >= want) return got;

    // Step 1: Page worklist から drain
    while (got < want) {
        Hz3SmallV2PageHdr* page = pageq_pop(owner, sc);
        if (!page) break;

        HZ3_S121_STAT_INC(pageq_deq);

        // 2. atomic_exchange で全量取得
        void* list = atomic_exchange_explicit(
            &page->remote_head, NULL, memory_order_acquire);

        // 3. out[] に必要分を埋める
        int page_got = 0;
        void* cur = list;
        while (cur && got < want) {
            out[got++] = cur;
            cur = hz3_obj_get_next(cur);
            page_got++;
        }

        // 4. 余剰を TLS spill に保存
        if (cur) {
            hz3_spill_push_list(sc, cur);
        }

        HZ3_S121_STAT_ADD(page_drain_objs_total, page_got);
        HZ3_S121_STAT_MAX(page_drain_objs_max, page_got);

        // 5. Lost wakeup 防止: state を戻す
        // drain 後に remote_head が空なら state=0 に戻す
        if (atomic_load_explicit(&page->remote_head, memory_order_acquire) == NULL) {
            atomic_store_explicit(&page->remote_state, 0, memory_order_release);
            HZ3_S121_STAT_INC(page_state_1to0);

            // 二度見: 戻した直後に新規 push が来てないか
            if (atomic_load_explicit(&page->remote_head, memory_order_acquire) != NULL) {
                uint8_t expected = 0;
                if (atomic_compare_exchange_strong_explicit(
                        &page->remote_state, &expected, 1,
                        memory_order_acq_rel, memory_order_relaxed)) {
                    pageq_push(owner, sc, page);
                    HZ3_S121_STAT_INC(page_requeue_due_to_new_remote);
                }
            }
        } else {
            // drain 中に新規 push → page を再 enqueue
            pageq_push(owner, sc, page);
            HZ3_S121_STAT_INC(page_requeue_due_to_new_remote);
        }
    }

    HZ3_S121_STAT_ADD(stash_pop_objs, got);
    return got;
}
#endif
```

---

## 統計カウンタ (SSOT)

```c
// S121 Stats (atexit 1回)
#if HZ3_S121_STATS
HZ3_DTOR_STAT(g_s121_stash_push_objs);
HZ3_DTOR_STAT(g_s121_stash_pop_objs);
HZ3_DTOR_STAT(g_s121_stash_pop_calls);
HZ3_DTOR_STAT(g_s121_pageq_enq);
HZ3_DTOR_STAT(g_s121_pageq_deq);
HZ3_DTOR_STAT(g_s121_page_drain_objs_total);
HZ3_DTOR_STAT(g_s121_page_drain_objs_max);
HZ3_DTOR_STAT(g_s121_page_state_0to1);
HZ3_DTOR_STAT(g_s121_page_state_1to0);
HZ3_DTOR_STAT(g_s121_page_requeue_due_to_new_remote);
#endif
```

**期待する形**:
- `page_drain_objs_total / pageq_deq` が 2以上 (できればもっと)
- `page_requeue_due_to_new_remote` が極端に多くない

---

## 変更ファイル一覧

| ファイル | 変更内容 |
|----------|----------|
| hz3_config_scale.h | S121 フラグ + compile guards |
| small_v2/hz3_small_v2_types.inc | Page header 拡張 |
| small_v2/hz3_small_v2_fill.inc | ヘッダサイズ反映 |
| owner_stash/hz3_owner_stash_globals.inc | pageq globals + init |
| owner_stash/hz3_owner_stash_push.inc | push を page-local へ |
| owner_stash/hz3_owner_stash_pop.inc | pop を pageq drain へ |
| owner_stash/hz3_owner_stash_stats.inc | S121 stats |

---

## A/B テスト

```bash
# Baseline (S121 OFF)
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s121_base.so

# Treatment (S121 ON)
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S121_PAGE_LOCAL_REMOTE=1 -DHZ3_S121_STATS=1'
cp ./libhakozuna_hz3_scale.so /tmp/s121_treat.so
```

**テストスイート**:
- xmalloc-testN T=8 (primary target)
- r90, r50 (regression check)
- dist_app, dist_uniform

---

## GO/NO-GO 基準

| 条件 | GO | NO-GO |
|------|-----|-------|
| xmalloc-testN | +10% 以上 | - |
| r90 | +5% 以上 | - |
| r50 | -3% 以内 | -5% 超退行 |
| correctness | 全テスト pass | any fail |

---

## リスク

| リスク | 対策 |
|--------|------|
| Lost wakeup | remote_state の二度見で防止 |
| Page header サイズ増加 | 16B→32B (4KB中0.4%→0.8%) |
| 混在によるデータ破壊 | compile-time 切替で防止 |
| pageq contention | ページ数で分散 (改善方向) |

---

## 参照

- ChatGPT Pro consultation (2025-01-14)
- mimalloc page-local free design
- S44 owner stash: owner_stash/*.inc
- S112 full drain exchange: hz3_config_scale.h
