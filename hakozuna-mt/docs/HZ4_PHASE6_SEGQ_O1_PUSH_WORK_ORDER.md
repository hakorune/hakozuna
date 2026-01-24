# HZ4 Phase 6: SegQ O(1) Push Work Order

目的:
- `hz4_segq_push_list()` の **O(n) tail 走査**を排除し、`hz4_collect` の 33% を削減する
- T=16/R=90 の IPC 低下を **SegQ Box**で改善する

前提:
- Phase6-1 (Small/Mid/Large) 完了
- perf で `hz4_collect` 内の tail 走査がボトルネックであることを確認済み

---

## 1) 方針（Box Theory）

- 変更範囲は **SegQ Box 内**に限定（境界は 1 箇所）
- A/B 可能にする（`HZ4_SQ_O1_PUSH=1`）
- Fail‑Fast を維持（壊れた queue を隠さない）

---

## 2) データ構造（SegQ）

`core/hz4_segq.inc` 内の shard queue を **head + tail** で管理:

```
typedef struct {
    _Atomic(hz4_seg_t*) head;
    _Atomic(hz4_seg_t*) tail;
} hz4_segq_t;
```

注意:
- **empty のとき head==NULL, tail==NULL** を保証
- `tail` は **best‑effort** で良いが、`head` は正であること

---

## 3) O(1) push_list の実装

### 3.1 API 変更

```
static inline void hz4_segq_push_list(hz4_segq_t* q, hz4_seg_t* list_head, hz4_seg_t* list_tail);
```

### 3.2 実装案（擬似コード）

```
if (!list_head) return;
list_tail->qnext = NULL;

for (;;) {
    hz4_seg_t* head = atomic_load(&q->head);
    if (!head) {
        // empty → 直接セット
        if (atomic_compare_exchange_weak(&q->head, &head, list_head)) {
            atomic_store(&q->tail, list_tail);
            break;
        }
        continue;
    }

    // non-empty → tail に接続
    hz4_seg_t* tail = atomic_load(&q->tail);
    if (!tail) {
        // fallback: tail 不整合なら head を基点に復旧
        tail = head;
        while (tail->qnext) tail = tail->qnext;
        atomic_store(&q->tail, tail);
    }

    if (atomic_compare_exchange_weak(&tail->qnext, &null, list_head)) {
        atomic_store(&q->tail, list_tail);
        break;
    }
}
```

注意:
- `tail` の不整合は **復旧**して継続  
- `head` は CAS で正を守る

---

## 4) Pop 側の調整

- `pop` で empty になったら **tail も NULL に戻す**
- `pop` が最後の要素を返すときに **head/tail を同時に空へ**

---

## 5) A/B と検証

### A/B フラグ
- `HZ4_SQ_O1_PUSH=1` (新)

### 検証項目
- `hz4_collect` の self% が **顕著に低下**すること
- T=16/R=90 の ops/s 改善
- queue 破損なし (Fail‑Fast)

---

## 6) 期待される成果

- `hz4_collect` 内の tail 走査消失
- branch miss / L1 miss の低下
- T=16 のスケール改善（hz4/hz3 gap 縮小）

