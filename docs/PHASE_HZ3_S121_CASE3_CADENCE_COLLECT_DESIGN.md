# S121 Case 3: mimalloc-style Cadence Collect 設計

## Status: 設計完了 / 実装待ち

## 背景

ChatGPT Pro 相談結果（2025-01-14）を元に、Case 3 の設計をまとめる。

### 現状
- S121-C: 83M free/sec
- mimalloc: 113M free/sec
- Gap: 36% behind

---

## mimalloc の "cadence collect" とは

mimalloc の核心：**高頻度の fast path に条件分岐や通知処理を足さず、一定回数ごとに必ず踏む slow path（generic routine）に回収/重い処理を寄せる**

具体的には：

1. **local_free を別に持つ** → fast path の free list が固定回数の alloc で必ず枯れる → generic routine が定期的に呼ばれる（temporal cadence）
2. **cross-thread free** は page ごとの `thread_free` list に **CAS だけで push**
3. **owner 側** は slow path で `atomic_swap` でまとめて回収して append（batching）
4. **full page 対策** として `thread_free` ポインタ下位ビットで状態（NORMAL/DELAYED/DELAYING）をエンコード

hz3 への翻訳：「pageq みたいな即時通知をやめて、**owner が slow path の一定 cadence で回収**する」

---

## 設計質問への回答

### 1) Scan Strategy: owner が「remote 付き page」をどう発見するか

#### A: per-owner page list（第一候補）

**強み:**
- remote-free fast path から **通知を完全に消せる**（= `remote_head` CAS のみ）
- スキャン範囲が「その owner のページ」だけ
- 実装が読みやすい

**弱み:**
- ページが再利用・解放・unmap されうる設計だと、リストに残ったポインタが危険
- → hz3 は「ページメタは常駐で unmap しない」なら問題は小さい

**実装の工夫:**
- **linked list より "chunked vector + カーソル"** を推奨（S121-D の linked-list 管理が致命傷だった教訓）
- 世代番号（generation）や tag でポインタ妥当性を検証

---

#### B: page_tag32 スキャン（第二候補）

**強み:**
- tag テーブルが常駐メタデータなら unmap/reuse に強い
- 連続メモリ走査で CPU が素直に速い

**弱み:**
- 何も工夫しないと **総ページ数に比例** してコスト爆発
- owner/sc で filter しても全体を舐めるなら辛い

**必須の前提:**
- **budget-limited + カーソル** で 1回あたりの走査を上限化
- 可能なら tag に "remote あり" ヒントを持たせる

---

#### C: owner bitmap（A/B が重い場合の勝ち筋）

**強み:**
- pageq の中央 head 競合を避けつつ、探索を O(#set bits) に
- contention 分散の思想は mimalloc が推奨

**弱み:**
- remote-free fast path に atomic RMW (OR) を足す
- ビット→ページのマッピングが複雑化しやすい

**設計のコツ:**
- segment 単位 bitmap（owner は自分の segment 群だけ走査）
- bit は "page が NULL→non-NULL になったときだけ立てる"

---

### 2) Scan Frequency: いつ collect を走らせるか

**推奨: 「alloc miss 毎 + budget-limited」の合成**

```
トリガ:
  pop_batch(owner, sc, want) が呼ばれた（= TLS が足りない）

やること:
  最大 K pages だけスキャン
  見つかった page の remote_head を atomic_exchange で drain

停止条件:
  - got >= want で即停止（最優先）
  - K pages で打ち切り（必ず上限化）

エスカレーション:
  "今回 0 個だった" が続く場合だけ K を一時的に増やす
```

mimalloc も slow path では sizeclass の pages を線形に辿って「最初に free がある page を見つけたら止まる」設計。

---

### 3) remote_state: pageq がなくなっても必要か？

**純 scan に振り切るなら remote_state は必須ではない**

理由: remote の有無は `remote_head != NULL` が真実

**ただし mimalloc 型拡張を入れるなら復活:**

mimalloc の thread_free ポインタ下位ビット:
- NORMAL: remote-free は page->remote_head に push（CAS only）
- DELAYED: 最初の1回だけ owner の delayed list に印を付ける
- その後は NORMAL に戻す（ページあたり最大1回）

**結論:**
- MVP では remote_state は消して良い
- "full ページ滞留" が見えた段階で mimalloc 型 state を入れる

---

### 4) Lost Wakeup: 通知なしで object が滞留しない保証

#### 4-A. 並行実行の正しさ

`remote_head` CAS + `atomic_exchange` drain の組み合わせで **消失はしない**（次回に回るだけ）

#### 4-B. 進捗保証（owner が動き続ける限り）

budget-limited スキャンを **カーソルで round-robin** すれば:
- P = 対象 page 数
- K = 1回の scan budget

**高々 ceil(P/K) 回の pop_batch で全 page を一巡**

#### 4-C. owner が止まる場合

mimalloc でも本質的に同じ問題。対策:
1. 明示的 collect API
2. thread exit 時の回収/abandon 機構
3. DELAYED（full page wake）だけ通知を残す

**Case 3 の MVP では 4-B で十分**（owner が動く限り回収される）

---

## 実装ロードマップ

### Phase 1: MVP（純 cadence scan）

```
変更点:
- pageq を完全に捨てる
- push は remote_head CAS のみ（remote_state/pageq 削除）
- pop_batch（TLS 枯れ）で budget-limited scan + exchange drain

目的:
- pageq/remote_state の hot overhead を消して、差分の天井を見る
```

**Scan 方式: Option A（per-owner page list）**

```c
// Hz3TCache に追加
typedef struct {
    void** pages;      // chunked vector of page pointers
    uint32_t count;    // current page count
    uint32_t capacity; // allocated capacity
    uint32_t cursor;   // round-robin scan cursor
} Hz3OwnerPageList;

Hz3OwnerPageList owner_pages[HZ3_SMALL_NUM_SC];
```

**Push path (simplified):**
```c
// remote_free(obj)
page = obj & PAGE_MASK;
// CAS prepend to page->remote_head (ONLY)
do {
    old_head = atomic_load(&page->remote_head);
    obj->next = old_head;
} while (!CAS(&page->remote_head, old_head, obj));
// NO notification - owner will discover via scan
```

**Pop path (cadence scan):**
```c
int hz3_owner_stash_pop_batch(owner, sc, out, want) {
    int got = 0;

    // 1. Drain TLS spill first (existing)
    got += drain_tls_spill(sc, out, want);
    if (got >= want) return got;

    // 2. Cadence scan: budget-limited round-robin
    Hz3OwnerPageList* list = &t_hz3_cache.owner_pages[sc];
    int budget = HZ3_S121_E_SCAN_BUDGET;  // e.g., 16 pages

    for (int i = 0; i < budget && got < want; i++) {
        if (list->count == 0) break;

        uint32_t idx = list->cursor % list->count;
        void* page = list->pages[idx];
        list->cursor++;

        // Check if page has remote objects
        if (atomic_load(&page->remote_head) != NULL) {
            // Drain with atomic_exchange
            void* objs = atomic_exchange(&page->remote_head, NULL);
            got += fill_out_and_spill(objs, out + got, want - got);
        }
    }

    return got;
}
```

---

### Phase 2: scan が重い場合

Option C（bitmap）を検討:
- pageq ほど中央化しない "分散した印"
- segment 単位 bitmap

---

### Phase 3: full ページ滞留対策

mimalloc 型 NORMAL/DELAYED を入れて "1ページにつき1回だけ起こす":
- full page への最初の remote-free で owner を起こす
- 2回目以降は CAS only

---

## GO/NO-GO 基準

| 条件 | GO | NO-GO |
|------|-----|-------|
| xmalloc-testN | +10% 以上 (>91M) | - |
| hz3 SSOT mixed | -3% 以内 | -5% 超退行 |
| sh8bench | -3% 以内 | -5% 超退行 |

---

## 参考文献

- [mimalloc paper (MSR-TR-2019-18)](https://www.microsoft.com/en-us/research/wp-content/uploads/2019/06/mimalloc-tr-v1.pdf)
- [mimalloc GitHub](https://github.com/microsoft/mimalloc)
- ChatGPT Pro consultation (2025-01-14)
