# HZ4 RSS Phase 1 (Option A): Decommit‑Safe Page 設計

目的:
- mimalloc の「空ページ即返却」級の RSS 低下を **安全に** 実現する。
- **decommit しても内部リンクが壊れない** page 設計に移行する。

非目的:
- hz3 本線や既定経路の置換。
- hot path の複雑化（境界 2 箇所に集約する）。

---

## Box Theory
- **PageMetaBox** を新設し、page の“真の状態”を **segment header 側に分離**する。
- **RemoteFlushBox** と **CollectBox** の境界だけを触る。
- decommit/commit は **OS Box** に閉じる。

---

## 1) 設計の要点（要約）

### なぜ必要？
現状は free list が **オブジェクト本体の next** に依存しているため、
`madvise(DONTNEED)` でページ内容が消えると **freelist が壊れる**。

### どうする？
**page メタを page から分離**して保持する：

- page 内メタ（`hz4_page_t`）を **薄く**する
- 代わりに **segment header に page meta 配列**を持つ
- remote_head / queued / sc / owner / used_count / capacity / decommit 状態は **meta 側**
- decommit 対象ページは **meta だけ残して OS に返す**

> 要するに、**「ページ内容は捨てても復元できる」**状態を作る。

---

## 2) 新しい構造（案）

### 2.1 Page Meta（segment header 側）
```
typedef struct hz4_page_meta {
  _Atomic(void*) remote_head[HZ4_REMOTE_SHARDS];
  _Atomic(uint64_t) remote_mask;
  _Atomic(uint8_t) queued;
  uint16_t owner_tid;
  uint16_t sc;
  uint16_t used_count;
  uint16_t capacity;
  uint8_t  decommitted;   // 0/1
  uint8_t  _pad;
  // optional: free bitmap / free head index
} hz4_page_meta_t;
```

### 2.2 Segment に meta 配列を持つ
```
struct hz4_seg {
  ...
  hz4_page_meta_t page_meta[HZ4_PAGES_PER_SEG];
  ...
};
```

**注意**: page_idx=0 は予約済みなので meta[0] は使用禁止。

---

## 3) Boundary での挙動

### RemoteFlushBox（free 側）
- `page->remote_head[]` ではなく **`meta->remote_head[]`** を更新。
- `hz4_segq_notify(seg, page_idx)` で pending を通知（現行維持）。

### CollectBox（malloc 側）
1) pending segment を drain
2) drain 後に **empty 判定**:
   - `used_count == 0`
   - `remote_head[] == NULL`
   - `queued == 0`
3) 空なら **decommit 候補**にする（OS Boxへ）

---

## 4) decommit の条件（暫定）

**安全優先ルール**:
- empty 判定は **CollectBox でのみ**行う
- remote_head が非空なら decommit 禁止
- `used_count==0` でも **meta が未初期化なら禁止**
- decommit 実行後は `meta->decommitted=1`

**A/B フラグ案**:
- `HZ4_PAGE_META_SEPARATE=1`（page meta 分離）
- `HZ4_PAGE_DECOMMIT=1`（decommit ON/OFF）

---

## 5) 再利用時の復元（commit/rebuild）

### commit フロー
1) `meta->decommitted==1` を検出
2) OS Box で `madvise(MADV_DONTNEED)` の逆方向を実行  
   （実際には “touch” でも良い）
3) page の free list を **meta から再構築**

### free list 再構築の基本
**アウトライン**
- page の object 配列を再走査
- `obj->next` を組み立て直す
- `meta->used_count` が 0 なら全スロット free

---

## 6) 実装ステップ（順番）

### Step 0: Phase0 観測（OS stats）
- `hakozuna/hz4/docs/HZ4_RSS_PHASE0_OS_STATS_WORK_ORDER.md` を通す
- seg_acq の増え方を基準にする

### Step 1: PageMetaBox を追加
1) `hz4_page_meta_t` を `hz4_seg.h` に追加
2) `hz4_seg_init_pages()` で meta を初期化
3) `hz4_page_from_ptr()` から page_idx を取って meta を引く helper 追加

### Step 2: RemoteFlush / Collect の参照先を meta に切替
- `remote_head` / `remote_mask` / `queued` / `sc` / `owner_tid` は meta 経由に統一

### Step 3: used_count 更新
- `malloc` で `used_count++`
- `free` で `used_count--`

### Step 4: empty 判定 → decommit（CollectBox のみ）
- `used_count==0` かつ `remote_head==NULL` を条件に decommit
- `HZ4_PAGE_DECOMMIT` でガード

### Step 5: 再利用時の rebuild
- `meta->decommitted` を見て rebuild
- rebuild 後に `meta->decommitted=0`

---

## 7) A/B・Fail‑Fast

### A/B
- `HZ4_PAGE_META_SEPARATE=1` を opt‑in
- `HZ4_PAGE_DECOMMIT=1` を opt‑in

### Fail‑Fast
- `decommitted==1` の page に対して **直接 free list を触ったら即 FAIL**
- `page_idx==0` での decommit は即 FAIL

---

## 8) Bench / SSOT

```
make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_OS_STATS=1'
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

評価:
- Peak RSS の低下
- ops/s の低下が 5% 以内なら GO
- seg_acq / seg_rel で “返せているか” を確認

---

## 9) 期待される効果
- **RSS が seg_acq に比例しない**状態を作れる
- mimalloc の purge 的な挙動に近づく
- hz4 の “event‑driven collect” と整合する

---

## 10) 注意事項
- metadata を page から剥がす設計は破壊力が大きい。**段階導入**が必須。
- hot path に入れず、境界（CollectBox / RemoteFlushBox）から積む。

