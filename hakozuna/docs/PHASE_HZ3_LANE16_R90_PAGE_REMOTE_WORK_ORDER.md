# PHASE HZ3: Lane16R90 PageRemote（研究レーン）指示書

目的:
- T=16/R=90 のみで mimalloc 追撃を試す **研究レーン**を追加する。
- 本線は壊さず、境界を **remote dispatch と refill** の 2 箇所に固定する。
- S121（pageq/scan）の NO-GO を避け、**page-local + bitmap hint** だけ使う。

非目的:
- 本線 hz3 の既定経路を書き換えない
- S121 PageQ を復活させない

---

## 1) 追加フラグ（config）

`hakozuna/hz3/include/hz3_config_scale.h`

```c
#ifndef HZ3_LANE_T16_R90_PAGE_REMOTE
#define HZ3_LANE_T16_R90_PAGE_REMOTE 0
#endif

// Page-local remote head を使うためのヘッダ拡張
#ifndef HZ3_PAGE_REMOTE_HEADER
#define HZ3_PAGE_REMOTE_HEADER 0
#endif

// bitmap hint と bucketize の最小構成
#ifndef HZ3_PAGE_REMOTE_SHARDS
#define HZ3_PAGE_REMOTE_SHARDS 2
#endif
#ifndef HZ3_PAGE_REMOTE_BUCKET
#define HZ3_PAGE_REMOTE_BUCKET 32
#endif
```

ルール:
- `HZ3_LANE_T16_R90_PAGE_REMOTE=1` のときだけ有効化。
- **S121 は OFF のまま**（pageq/scan を使わない）。

---

## 2) Page Header 拡張（S121 を使わずに remote_head を持つ）

`hakozuna/hz3/src/small_v2/hz3_small_v2_types.inc`

S121 の `remote_head` 相当を **新フラグ `HZ3_PAGE_REMOTE_HEADER`** で別枠に出す。

方針:
- `HZ3_PAGE_REMOTE_HEADER=1` のとき **remote_head だけ追加**
- S121 用の `remote_state/remote_count/page_qnext` は追加しない

（例）
```c
#if HZ3_S121_PAGE_LOCAL_REMOTE
  // 既存（触らない）
#elif HZ3_PAGE_REMOTE_HEADER
  _Atomic(void*) remote_head;  // MPSC
  uint64_t pad;                // 16B境界維持
#else
  uint64_t reserved;
#endif
```

`hz3_small_v2_page_alloc.inc` で:
```c
#if HZ3_PAGE_REMOTE_HEADER
  atomic_init(&ph->remote_head, NULL);
#endif
```

---

## 3) Segment Bitmap Hint（分散ヒント）

`Hz3SegMeta` に remote_pending bitmap を追加:
- 256 pages / seg → 4x u64

```c
_Atomic(uint64_t) remote_pending[4];
```

set/clear API:
- `hz3_page_remote_set(seg_meta, page_idx)`
- `hz3_page_remote_test_and_clear(seg_meta, page_idx)`

**順序ルール**:
1) remote_head に push
2) bitmap を set（false negative を防ぐ）

---

## 4) 新箱の実装（1ファイルに隔離）

新規: `hakozuna/hz3/src/lane16/hz3_lane16_page_remote.inc`

### 4.1 注入（dispatch 側）
```c
void hz3_lane16_push_remote_list_small(uint8_t owner, int sc,
                                       void* head, void* tail, uint32_t n);
```

設計:
- 小さな **bucketize (32)** だけ行う
- overflow は **owner_stash に fallback**
- remote_head への push → bitmap set

### 4.2 回収（refill 側）
```c
uint32_t hz3_lane16_pop_batch_small(uint8_t owner, int sc,
                                    void** out, uint32_t want);
```

設計:
- owner thread の small segment リストを **round‑robin** で回す
- bitmap で **remote ありページだけ** drain
- 足りなければ **既存 owner_stash / central にフォールバック**

---

## 5) 境界の差し替え（2 箇所だけ）

### 5.1 remote dispatch
対象: `hakozuna/hz3/src/hz3_tcache_remote.c`

small dispatch のみ分岐:
```c
#if HZ3_LANE_T16_R90_PAGE_REMOTE
  hz3_lane16_push_remote_list_small(dst, sc, head, tail, n);
  return;
#endif
```

### 5.2 small_v2_refill
対象: `hakozuna/hz3/src/small_v2/hz3_small_v2_refill.inc`

```c
#if HZ3_LANE_T16_R90_PAGE_REMOTE
  got_stash = hz3_lane16_pop_batch_small(t_hz3_cache.my_shard, sc,
                                         batch + got, refill_batch - got);
#else
  got_stash = hz3_owner_stash_pop_batch(...)
#endif
```

---

## 6) small segment リストの登録

`hz3_small_v2_page_alloc.inc` で new seg を取得したタイミングで:
- `t_hz3_cache.lane16_small_seg_ids[]` に登録
- cap 付き（例: 1024）
- 参照は **arena_idx だけ**（seg_meta* を保持しない）

---

## 7) A/B レーン

ビルド例:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_LANE_T16_R90_PAGE_REMOTE=1 -DHZ3_PAGE_REMOTE_HEADER=1'
```

計測:
```sh
LD_PRELOAD=./hakozuna/hz3/out/libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 20000000 400 16 2048 90 65536
```

---

## 8) 失敗時の扱い

- 研究レーンは **archive/research** に隔離
- `HZ3_LANE_T16_R90_PAGE_REMOTE` は **既定OFF**
- 本線は汚さない

---

## 9) 成功条件（暫定）

- T=16/R=90 で mimalloc に **-1%以内**  
  かつ mixed/R=50 で **-1%以内**

---

## メモ

S121 PageQ は NO-GO なので **再導入しない**。
本レーンは “page-local + bitmap hint” の最小構成で試す。

---

## 10) 2026-01-22 更新（CAS バグ修正 + A/B 結果）

### 修正内容
対象: `hz3_lane16_page_push_list()`（`hakozuna/hz3/src/small_v2/hz3_small_v2_lane16.inc`）

**問題**: CAS 失敗時に `tail->next` が不整合になり、リスト循環でハング。

**修正**:
- `old_head` を acquire で読み込み
- `tail->next = old_head` を **更新済みの old_head に合わせて毎回再設定**
- `atomic_compare_exchange_strong_explicit()` に変更

```c
void* old_head = atomic_load_explicit(slot, memory_order_acquire);
for (;;) {
    hz3_obj_set_next(tail, old_head);
    if (atomic_compare_exchange_strong_explicit(
            slot, &old_head, head, memory_order_release, memory_order_acquire)) {
        break;
    }
}
```

### A/B 結果（T=16, R=90, RUNS=5）

```
Baseline (OFF): 69.5M ops/s  (range 67.3–75.9)
Lane16   (ON):  76.1M ops/s  (range 74.2–85.9)
改善: +9.6%
```

**判定**: CAS バグ修正で安定動作し、T=16/R=90 で明確な改善を確認。

---

## 11) 2026-01-22 更新（深層バグ発見 - 循環リスト問題）

### 問題の再発見

CAS バグ修正後もハングが継続。詳細調査の結果、より深い設計問題を発見。

**根本原因**: Remote Stash Ring に **stale entry** が残る問題。

シナリオ:
1. Thread A が object X を free → A の ring に entry 追加
2. Thread B が Lane16 から X を drain → allocation path へ
3. App が X を allocate/use/free → B の ring に entry 追加
4. **A の ring にはまだ X の stale entry が残っている**
5. A と B の両方が ring flush → X が Lane16 に 2 回 push される
6. `tail->next = old_head` で **X が既に old_head chain に存在** → 循環リスト作成
7. drain 時に無限ループ → ハング

### 検証結果

```
T=1, R=90:  OK (81.5M ops/s) - 単一スレッドでは race なし
T=2, R=90:  HANG
T=16, R=50: HANG
T=16, R=0:  OK (296M ops/s) - remote free なしでは問題なし
```

### 試行した修正（すべて不十分）

1. **Spinlock**: `HZ3_LANE16_REMOTE_LOCK=1`
   - push/drain を排他制御
   - 結果: 同一オブジェクトの **複数回 push** は防げない → NG

2. **Simple cycle detection**: `old_head == tail || old_head == head`
   - head/tail が old_head の先頭にいる場合のみ検出
   - 結果: tail が old_head chain の **中間にいる場合** は検出できない → NG

3. **Walk-based cycle detection**: `hz3_lane16_in_list(tail, old_head, 128)`
   - old_head を 128 ノードまで walk して tail を探す
   - 結果: Lock 下で O(n) walk が必要、副作用でページ混在を引き起こす → NG

4. **Cycle-breaking drain**: capacity 上限で walk を打ち切り
   - 循環リスト検出時に walk を停止、remainder を drop
   - 結果: 根本解決にならず、T>1 で依然ハング → NG

### 根本的な設計問題（一次仮説）

Lane16 の page-local list 設計は、**同一オブジェクトが複数回 push される可能性** を考慮していない。

Remote Stash Ring は SPSC だが、**drain された object が再 free されて別 thread の ring に入る** シナリオでは、複数 ring が同一 object を持つ。

### 修正オプション（将来の検討）

1. **Object generation/epoch**: 各 object に generation counter を追加
   - Ring entry に generation を記録、push 時に検証
   - 欠点: object metadata への変更が必要

2. **Ring entry invalidation**: drain 時に ring entry を無効化
   - 欠点: どの ring に entry があるか不明（cross-thread）

3. **Atomic CMPXCHG with tag**: 48-bit pointer + 16-bit tag を pack
   - ABA 問題の標準解
   - 欠点: アーキテクチャ依存

### 現在の判定（暫定）

**Lane16 は暫定 NO-GO**（T>1, R>0 でハング）

- `HZ3_LANE_T16_R90_PAGE_REMOTE=0` を維持（既定 OFF）
- 研究箱として archive、本線には影響なし
- Baseline (Lane16 OFF) で T=16/R=90 は 87.7M ops/s を維持

---

## 12) 2026-01-22 更新（入力リスト契約の見直し / 設計修正案）

### 背景

Remote dispatch → Lane16 の入口 `hz3_lane16_push_remote_list_small()` は **`head/tail/n` を受け取る**が、
現在の実装は `while (cur)` で **`next` を信じて walk する**。

この設計だと **n==1 のときに `cur->next` がユーザデータ由来のゴミを指す**可能性があり、
bucketize の途中で **同一ページ内循環**や **誤った重複 push** を誘発しうる。
`HZ3_LANE16_REMOTE_LOCK=1` でも循環が止まらないケースと整合する。

### 設計修正（最小差分 / 境界は保持）

#### A) 「n本だけ処理」に固定（next を信用しない）

**入口リストは `(head, tail, n)` を唯一の真実**とし、`next` は **i+1<n のときだけ**読む。
各ノードは **処理開始時に `next=NULL` で切断**し、壊れた連結を残さない。

擬似コード（方針）:
```c
void hz3_lane16_push_remote_list_small(uint8_t owner, int sc,
                                       void* head, void* tail, uint32_t n) {
    (void)owner; (void)tail;
    if (!head || n == 0) return;

    void* cur = head;
    for (uint32_t i = 0; i < n; i++) {
        void* next = (i + 1 < n) ? hz3_obj_get_next(cur) : NULL;
        hz3_obj_set_next(cur, NULL); // 入口で必ず切断
        // dup_mark -> bucketize の順序を守る（B 参照）
        ...
        cur = next;
    }
}
```

**狙い**: n==1 で `next` を読まない / 破損リストを追わない。

#### B) dup_guard の順序を先にする

`dup_mark` を **bucket へ入れる前に必ず実行**。
重複だった場合は **bucket を汚さずスキップ**する。

**狙い**: dup 検出後に bucket へ残留する “ゴミ next” を排除する。

#### C) n==1 専用パス（超低リスク）

`if (n == 1) { hz3_lane16_push_one(ph, head); return; }`

**狙い**: もっとも頻度の高い n==1 で `next` を一切読まない。

### 期待効果

- 循環リストの根絶（`seen > cap` の failfast 解消）
- 入口の “list contract” を厳格化して **remote ring 側の stale entry 影響を最小化**
- `REMOTE_LOCK` の有無に依存せず **構造的に安全**にする

### 影響範囲

- **境界はそのまま**（dispatch と refill の 2 箇所のみ）
- 変更は `hz3_lane16_push_remote_list_small()` の内部処理に限定
- 本線経路は変更なし

### 検証手順（最短）

1) `HZ3_LANE16_FAILFAST=1` を維持して T=2/R=90 を実行  
   - `consume:walk` / `remainder_walk` が消えることを確認
2) 次に T=16/R=50/90 でハングが消えるか確認  
3) OK なら failfast を切り、A/B を再実施
