# MidOwnerRemoteQueueBox 設計案

## ステータス（2026-02-13）

- 本箱は **研究箱（default OFF）**。
- 本線デフォルトは `HZ4_MID_OWNER_REMOTE_QUEUE_BOX=0` を維持。
- `MidOwnerClaimGateBox` は NO-GO 判定で archive 管理へ移行し、本線コードから切り離し済み。

## 背景

mt_remote mainレーンで70%のmallocがロックパスを通過している。
ロックを「減らす」より「細くする」ことで、owner threadはlockless、
cross-thread freeはatomic pushのみ、という設計が必要。

## 設計概要

```
[Per-SizeClass Owner Page Design]

Owner Thread (Thread A):
  - lockless alloc from local_freelist
  - lockless free to local_freelist
  - periodic drain: remote_freelist → local_freelist (with lock)

Cross-Thread Free (Thread B):
  - atomic push to page->remote_head only
  - no lock required

Global/SC Lock:
  - page acquisition/return only
  - not on alloc/free hot path
```

## データ構造変更

### hz4_mid_page_t (追加フィールド)

```c
typedef struct hz4_mid_page {
    uint32_t magic;
    uint16_t sc;
    uint16_t _pad0;
    uint32_t obj_size;
    uint32_t capacity;
    uint32_t free_count;

    // Owner thread's local freelist (lockless)
    void* local_free;
    uint32_t local_free_count;

    // Cross-thread remote freelist (atomic)
    _Atomic(void*) remote_head;
    _Atomic(uint32_t) remote_count;

    // Owner identification
    pthread_t owner_tid;

    struct hz4_mid_page* next;
} hz4_mid_page_t;
```

### Thread-Local State (追加)

```c
// Per-thread, per-SC owner page
static __thread hz4_mid_page_t* g_mid_owner_page[HZ4_PAGE_SIZE / HZ4_MID_ALIGN];
static __thread uint32_t g_mid_owner_drain_counter[HZ4_PAGE_SIZE / HZ4_MID_ALIGN];
```

## アロゴリズム

### malloc (fast path - lockless)

```c
void* hz4_mid_malloc(size_t size) {
    uint16_t sc = size_to_sc(size);
    hz4_mid_page_t* page = g_mid_owner_page[sc];

    // 1. Try local freelist (lockless)
    if (page && page->local_free_count > 0) {
        return pop_local(page);
    }

    // 2. Periodic drain: remote → local (amortized cost)
    if (page && should_drain(page)) {
        drain_remote_to_local(page);  // atomic collect + lock
    }

    // 3. Slow path: acquire new page with lock
    return malloc_slow_path(sc);
}
```

### free (fast path - atomic)

```c
void hz4_mid_free(void* ptr) {
    hz4_mid_page_t* page = ptr_to_page(ptr);
    pthread_t self = pthread_self();

    // 1. Owner thread: lockless local free
    if (page->owner_tid == self) {
        push_local(page, ptr);
        return;
    }

    // 2. Cross-thread: atomic push to remote
    atomic_push_remote(page, ptr);
}
```

### atomic_push_remote

```c
static inline void atomic_push_remote(hz4_mid_page_t* page, void* ptr) {
    void* prev;
    do {
        prev = atomic_load(&page->remote_head);
        hz4_obj_set_next(ptr, prev);
    } while (!atomic_compare_exchange_weak(&page->remote_head, &prev, ptr));

    atomic_fetch_add(&page->remote_count, 1);
}
```

### drain_remote_to_local

```c
static void drain_remote_to_local(hz4_mid_page_t* page) {
    // Snapshot atomic list
    void* remote_list = atomic_exchange(&page->remote_head, NULL);
    uint32_t remote_cnt = atomic_exchange(&page->remote_count, 0);

    if (!remote_list) return;

    // Prepend to local list (no lock - owner only)
    void* tail = remote_list;
    while (hz4_obj_get_next(tail)) {
        tail = hz4_obj_get_next(tail);
    }
    hz4_obj_set_next(tail, page->local_free);
    page->local_free = remote_list;
    page->local_free_count += remote_cnt;
}
```

## 実装計画

### Phase 1: 基盤実装

1. **ヘッダー変更** (`hz4_mid.h`)
   - `hz4_mid_page_t` に新フィールド追加
   - 設定フラグ追加: `HZ4_MID_OWNER_REMOTE_QUEUE`

2. **新ファイル作成** (`hz4_mid_owner_remote.c`)
   - `atomic_push_remote()`
   - `drain_remote_to_local()`
   - `hz4_mid_owner_malloc()` / `hz4_mid_owner_free()`

### Phase 2: 統合

3. **既存コード変更** (`hz4_mid.c`)
   - `hz4_mid_malloc()` で新パスを分岐
   - `hz4_mid_free()` でownerチェック追加
   - 既存ロックパスはフォールバックとして維持

### Phase 3: チューニング

4. **Drain頻度調整**
   - `HZ4_MID_DRAIN_THRESHOLD` (default: 16)
   - `HZ4_MID_DRAIN_PERIOD` (default: every 64 allocs)

## GO/NO-GO ゲート

| 指標 | 基準 | 測定方法 |
|------|------|----------|
| malloc_lock_path削減 | 30%以上 | HZ4_MID_STATS_B1 |
| main性能 | +5%以上 | mt_remote RUNS=10 |
| guard品質 | -1%以内 | guard RUNS=10 |
| larson安定性 | rc=0 | larson 3回連続 |

## リスクと対策

| リスク | 対策 |
|--------|------|
| ABA問題 | atomic_compare_exchange_weak + ポインタタグ |
| Remoteリスト過剰増加 | Periodic drain + 閾値チェック |
| Ownerスレッド終了 | ページ返却時のownerクリア |
| 複数ページ所有 | SCごとに1ページのみ所有可 |

## 既存コードとの互換性

- `HZ4_MID_OWNER_REMOTE_QUEUE_BOX=0` で既存動作（ロックパス）
- `HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1` で新動作
- A/Bテスト可能な設計

## 実装ステータス（2026-02-13）

- 実装先:
  - `hakozuna/hz4/include/hz4_mid.h`
  - `hakozuna/hz4/src/hz4_mid.c`
  - `hakozuna/hz4/core/hz4_config_collect.h`
- 実装形:
  - 当初案の別ファイル化（`hz4_mid_owner_remote.c`）ではなく、`hz4_mid.c` 内の Box 境界関数として実装。
  - `owner_tag / remote_head / remote_count` と、bin整合用 `in_bin` を導入。

### A/B結果（クリーン再計測）

- artifact: `/tmp/hz4_ab_mid_owner_remote_20260213_104121`
- main (`16..32768 r90`, RUNS=7): `63.915M -> 56.651M` (`-11.37%`)
- guard (`16..2048 r90`, RUNS=7): `105.844M -> 109.096M` (`+3.07%`)
- cross128 (`16..131072 r90`, RUNS=5): `8.258M -> 8.286M` (`+0.34%`)

### 判定

- 現時点では `main` レーンの回帰があるため、**default OFF 維持**（`HZ4_MID_OWNER_REMOTE_QUEUE_BOX=0`）。
- 研究箱として opt-in 継続し、drain policy の追加調整余地を残す。
- `MidOwnerClaimGateBox` は archive 化済みのため、本ドキュメントの対象外（RemoteQueue本体のみ再検討対象）。
