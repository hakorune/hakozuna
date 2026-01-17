# Question Pack: S121 Page-Local Remote 退行分析

## 背景

hz3 (custom memory allocator) の remote-free 最適化として S121 Page-Local Remote を実装したが、深刻な退行が発生した。

### 元の設計意図

- **目的**: xmalloc-testN (MT remote-heavy workload) で mimalloc に勝つ
- **アイデア**: owner stash (centralized MPSC queue) から page-local remote list に変更
  - 競合を `(owner, sc)` 1点から「ページ数」に分散
  - pop_batch で 1ページ drain = 複数 object 取得

### 実装概要

```
Push path (freeing thread):
1. ptr → page header 取得
2. atomic CAS で page->remote_head に prepend
3. remote_count >= threshold なら pageq に enqueue

Pop path (owner thread):
1. pageq から page を dequeue
2. atomic_exchange で page->remote_head 全量取得
3. 取得した objects を tcache に入れる
```

## ベンチ結果

xmalloc-testN (16 threads, remote-heavy):

| Config | free/sec | vs mimalloc | vs S121=0 |
|--------|----------|-------------|-----------|
| **mimalloc** | **113M** | baseline | +35% |
| **S121=0 baseline** | **84M** | -26% | baseline |
| S121=1 (any threshold) | **9.8M** | **-91%** | **-88%** |

**S121 で 8-9x 遅くなった。** (84M → 9.8M)

## 観測データ

S121=1, threshold=4 の stats:
```
push_objs=48M
pop_calls=1.5M
pageq_deq=35M
drain_total=48M
objs_per_page=1.4    <-- ここが問題
```

- **objs_per_page=1.4**: 1回の page drain で平均 1.4 objects しか取れない
- pageq enqueue/dequeue 回数が多すぎ（35M回）
- page->remote_head への atomic 操作が多い

## 仮説

1. **Page queue オーバーヘッドが支配的**
   - 1.4 objects/page では queue 操作コストが償却されない
   - centralized stash の方がバッチ効率が良かった

2. **Pointer chase が増えた**
   - remote_head への CAS が per-object
   - page header lookup (ptr → page) が毎回発生

3. **競合分散が逆効果**
   - 競合は少なくなったが、操作回数が増えた
   - 元の centralized stash でも競合は軽かった可能性

## 質問

1. **page-local remote アーキテクチャは根本的に間違いか?**
   - それとも実装の問題か？

2. **objs_per_page を上げる方法はあるか?**
   - Lazy enqueue (threshold) は効果なし
   - Page 単位のバッチは本質的に難しい？

3. **代替アプローチの提案**
   - Producer-consumer 型で batching を効かせる方法
   - mimalloc / tcmalloc / jemalloc がどうやっているか

4. **このベンチ (xmalloc-testN) の特性**
   - remote-heavy だが page 分散も激しい？
   - 実際のワークロードでも同様か？

## Perf 分析結果

### Stats ON vs OFF

| Config | free/sec | vs S121=0 |
|--------|----------|-----------|
| S121=0 | 84M | baseline |
| S121=1 stats=0 | 59M | **-30%** |
| S121=1 stats=1 | 9.8M | -88% |

Stats カウンタ (`lock addl` to global) で 6x 遅くなっていた。

### Push 操作の atomic 比較

**S121=0 (centralized stash):**
- `lock cmpxchg` to g_hz3_owner_stash: **30%**
- **合計: 1 atomic/push**

**S121=1 (page-local remote):**
- `lock cmpxchg` to page->remote_head: **15%**
- `lock xadd` to remote_count: **6%**
- `lock cmpxchg` for state 0→1: **11%**
- `lock cmpxchg` to pageq: **8%**
- **合計: 2-4 atomics/push**

### 結論

- S121=1 は atomic 操作が **2-4 倍** 多い
- 競合を分散しても、操作回数増加で **逆効果**
- centralized stash の競合は実際には軽い（16 threads で 30% overhead）

## コードスニペット

### Push path (hz3_owner_stash_push.inc)
```c
// 1. Prepend object to page->remote_head (MPSC push)
void* old_head;
do {
    old_head = atomic_load_explicit(&page->remote_head, memory_order_acquire);
    *((void**)obj) = old_head;
} while (!atomic_compare_exchange_weak_explicit(
    &page->remote_head, &old_head, obj,
    memory_order_release, memory_order_relaxed));

// 2. Lazy enqueue: only enqueue when count >= threshold
uint8_t cnt = atomic_fetch_add_explicit(&page->remote_count, 1, memory_order_acq_rel);
if ((cnt + 1) >= HZ3_S121_LAZY_THRESHOLD) {
    uint8_t expected = 0;
    if (atomic_compare_exchange_strong_explicit(
            &page->remote_state, &expected, 1,
            memory_order_acq_rel, memory_order_relaxed)) {
        hz3_pageq_push(owner, sc, page);
    }
}
```

### Pop path (hz3_owner_stash_pop.inc)
```c
void* page = hz3_pageq_pop(owner_id, sc);
if (page) {
    // Drain all objects from page->remote_head
    void* head = atomic_exchange_explicit(&page->remote_head, NULL, memory_order_acq_rel);
    atomic_store_explicit(&page->remote_state, 0, memory_order_release);

    while (head && got < want) {
        out[got++] = head;
        head = *((void**)head);
    }
    // Requeue if objects remain...
}
```

## 参考: S121=0 (既存 owner stash)

Owner stash は centralized MPSC queue per (owner, sc):
- Push: atomic CAS to global list head
- Pop: atomic exchange to take entire batch

既存実装で 83M free/sec を達成していた。
