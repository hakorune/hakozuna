# HZ4 Phase 6: P3 Inbox / Message-Passing Lane Work Order

目的:
- remote-heavy (T=16/R=90) で **collect のメタ仕事**をさらに削減
- remote_free の流れを **message passing (inbox)** に寄せる
- 既存の collect/segq/pageq は温存し、**A/B で切替可能**にする

---

## 0) ルール（Box Theory）

- 新規箱: **Inbox Box**（remote_flush/collect の境界でのみ使用）
- hot path (malloc/free fast) は触らない
- A/B は compile-time (`HZ4_REMOTE_INBOX=1`)
- Fail-Fast は debug 限定で最小限

---

## 1) 最小設計（方針）

- **owner x sizeclass の inbox** を持つ
- inbox の中身は **intrusive list (free object 自体)**
- extra alloc / message node なし
- collect は **inbox exchange -> out[] に詰めるだけ**
- 余りは **TLS stash[sc]** に退避（pushback しない）

---

## 2) データ構造（最小）

```c
// 64B pad (false sharing 回避)
typedef struct {
    _Atomic(void*) head;
    uint8_t pad[64 - sizeof(_Atomic(void*))];
} hz4_inbox_head_t;

// グローバル inbox: owner x sc
extern hz4_inbox_head_t g_hz4_inbox[HZ4_NUM_SHARDS][HZ4_SC_MAX];
```

TLS に stash を追加:

```c
// hz4_tls.h
void* inbox_stash[HZ4_SC_MAX];  // sc ごとの余り stash
```

---

## 3) Inbox Box API

```c
// push list (MPSC)
static inline void hz4_inbox_push_list(uint8_t owner, uint8_t sc,
                                       void* head, void* tail) {
    _Atomic(void*)* slot = &g_hz4_inbox[owner][sc].head;
    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // CAS retry ごとに再設定 (Lane16 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
}

static inline void* hz4_inbox_pop_all(uint8_t owner, uint8_t sc) {
    return atomic_exchange_explicit(&g_hz4_inbox[owner][sc].head, NULL,
                                    memory_order_acq_rel);
}
```

consume (collect 前段):

```c
static inline uint32_t hz4_inbox_consume(hz4_tls_t* tls, uint8_t sc,
                                         void** out, uint32_t budget) {
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
    void* list = tls->inbox_stash[sc];
    if (!list) {
        list = hz4_inbox_pop_all(owner, sc);
    } else {
        tls->inbox_stash[sc] = NULL;
    }
    if (!list) return 0;

    uint32_t got = 0;
    void* cur = list;
    while (cur && got < budget) {
        void* next = hz4_obj_get_next(cur);
        out[got++] = cur;
        cur = next;
    }

    // 余りは stash へ（pushback しない）
    tls->inbox_stash[sc] = cur;
    return got;
}
```

---

## 4) 境界の接続（最小差分）

### remote_flush 側
- `hz4_remote_flush.inc` で **page remote list を inbox に publish**
- `HZ4_REMOTE_INBOX=1` のときだけ有効
- bucket は **(owner, sc)** で分ける（local 集約して push_list 1回）

### collect 側
- `hz4_collect.inc` の冒頭で **Phase -1: inbox_consume** を挿入
- `got >= budget` なら即 return（既存の carry/segq は触らない）

---

## 5) Fail-Fast（debug 限定）

- `page->sc < HZ4_SC_MAX`
- owner shard 範囲 (`hz4_owner_shard(page->owner_tid) < HZ4_NUM_SHARDS`)
- `hz4_page_valid(page)` / `hz4_seg_valid(seg)`（必要なら）
- `obj` の align/範囲（page header より前に被らない）

---

## 6) A/B 方式

- `-DHZ4_REMOTE_INBOX=1` で inbox lane ON
- OFF 時は既存経路 (page remote + segq/pageq) を使用

---

## 7) ベンチ（SSOT）

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 8) NO-GO 判定

- **+5% 未満**: NO-GO
- **+10〜15%** なら採用検討
- RSS/blowup 兆候が出たら撤退

---

## 11) 結果（SSOT, 2026-01-23）

- T=16 R=90 (16–2048)
- Throughput: **+60–77% 改善** → **GO**
- 追加: `core/hz4_inbox.inc` / `src/hz4_inbox.c`
- default ON: `HZ4_REMOTE_INBOX=1` へ更新予定

---

## 9) 実装ステップ（最小）

1) Inbox Box を追加
   - `core/hz4_inbox.inc` を新設（push/pop/consume）
   - `hakozuna/hz4/Makefile` の `CORE_INCS` に追加
2) TLS 拡張
   - `core/hz4_tls.h` に `inbox_stash[HZ4_SC_MAX]`
   - `src/hz4_tls_init.c` で NULL 初期化
3) remote_flush 経路切替
   - `core/hz4_remote_flush.inc` に `#if HZ4_REMOTE_INBOX` 分岐
4) collect 冒頭に inbox consume を挿入
   - `core/hz4_collect.inc`
5) test/bench
   - `make -C hakozuna/hz4 test`
   - SSOT ベンチ + perf

---

## 10) 記録フォーマット

```
P3 (inbox)
T=16 R=90
hz4: <ops/s>
perf: hz4_collect <X%>, hz4_remote_flush <Y%>
notes: <採用/NO-GO>
```

---

## 11) 結果 (SSOT, 2026-01-23)

### Throughput (T=16 R=90)

| Mode | ops/s | vs OFF |
|------|-------|--------|
| **Inbox OFF** | ~42-44M | baseline |
| **Inbox ON** | ~68-97M (median ~70M) | **+60-77%** |

### Perf (self%)

| Function | Inbox OFF | Inbox ON |
|----------|-----------|----------|
| `hz4_collect` | 9.62% | 14.59% |
| `hz4_remote_flush` | 3.54% | 5.13% |
| `bench_thread` | 12.03% | 22.51% |

### 評価

- **+60-77% 改善** → **GO**
- overflow 発生 (650K-1.2M/iter) - collect 頻度調整で改善余地あり
- `hz4_collect` の self% は増加しているが、全体時間が短縮されたため（絶対時間は減少）

### 次のステップ

- P3 を default ON に変更
- overflow 削減のための collect 頻度調整（P4 候補）
