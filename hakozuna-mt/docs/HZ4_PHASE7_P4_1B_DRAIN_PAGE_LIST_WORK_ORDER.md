# HZ4 Phase 7: P4.1b Drain‑Page List Work Order

目的:
- `hz4_drain_page` を **list 返却**に変更し、out[] store を本命で削減
- perf の hot spot (out[] store 22%) を直接潰す

---

## 0) ルール（Box Theory）

- 変更は **Collect / PageQ** の箱内のみ
- A/B は compile-time (`HZ4_COLLECT_LIST=1`)
- Fail‑Fast は維持

---

## 1) 方針

- `hz4_drain_page` → `hz4_drain_page_list` を追加
- list (head/tail/n) を返す
- `hz4_drain_segment_list` が list を splice して合成

---

## 2) API 追加案

```c
static inline uint32_t hz4_drain_page_list(hz4_page_t* page,
                                           void** head_out,
                                           void** tail_out,
                                           uint32_t budget);
```

返却:
- `head_out` / `tail_out` に prefix list
- 取得数 `n` を return

---

## 3) 実装案（drain_page_list）

```c
static inline uint32_t hz4_drain_page_list(hz4_page_t* page,
                                           void** head_out,
                                           void** tail_out,
                                           uint32_t budget) {
    void* head = NULL;
    void* tail = NULL;
    uint32_t n = 0;

    void* list = hz4_page_pop_local(page);
    if (!list) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    void* cur = list;
    while (cur && n < budget) {
        tail = cur;
        cur = hz4_obj_get_next(cur);
        n++;
    }

    // 余りは page->local_free に戻す
    if (cur) {
        hz4_page_push_local_list(page, cur);
    }

    *head_out = list;
    *tail_out = tail;
    return n;
}
```

---

## 4) セグメント側の splice

```c
static inline void hz4_list_splice(void** head, void** tail,
                                   void* h2, void* t2) {
    if (!h2) return;
    if (!*head) {
        *head = h2;
        *tail = t2;
    } else {
        hz4_obj_set_next(*tail, h2);
        *tail = t2;
    }
}
```

---

## 5) 変更箇所

- `core/hz4_collect.inc`
  - `hz4_drain_page_list` 追加
  - `hz4_drain_segment_list` で list 合成
- `src/hz4_tcache.c`
  - `hz4_collect_list` が list 返却を利用

---

## 6) SSOT

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 7) NO-GO 判定

- **+5% 未満**なら NO‑GO
- correctness 低下があれば即撤退

---

## 8) 記録フォーマット

```
P4.1b drain_page_list
T=16 R=90
hz4: <ops/s>
perf: out[] store が減るか
notes: <採用/NO-GO>
```
