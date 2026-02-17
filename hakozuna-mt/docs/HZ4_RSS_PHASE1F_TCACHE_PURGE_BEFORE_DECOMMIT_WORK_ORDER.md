# HZ4 RSS Phase 1F: TCache Purge → Safe Decommit（Option A）

目的:
- Phase 1E で判明した「**decommit が tcache intrusive list を壊す**」問題を解消する。
- `used_count==0` を検出できたページを **安全に decommit** できるようにする。

非目的:
- hot path の常時コスト増（普段は OFF / A/B）。
- scan ベースの常時回収（Phase 2 以降に分離）。

---

## Box Theory
- 追加するのは **TCachePurgeBox**（owner-thread only）のみ。
- decommit/commit は **OS Box** に閉じる（既存の `hz4_os_page_decommit/commit`）。
- 変換点は 1 箇所:
  - `used_count==0` を検出したときだけ `purge → decommit` を実行する。

---

## 0) 現状の問題（Phase 1E）

- `used_count==0` を local free 側で検出して、その場で `madvise(DONTNEED)` すると、
  tcache に積まれている free object の `obj->next` が **decommit で消える**。
- 結果として次の `tcache_pop()` が `obj->next` を読めず、SIGABRT/整合性違反になる。

---

## 1) 方針

`used_count==0` のページを decommit する前に、
そのページ由来の free object を **tcache から unlink**（参照を断つ）してから decommit する。

```
used_count==0
  ├─ (1) tcache から page の obj を purge/unlink
  ├─ (2) pageq_notify（任意: 観測/統一）
  └─ (3) hz4_page_try_decommit(page, meta)
```

期待される挙動:
- local-only (R=0) でも `page_decommit>0` が出る
- R>0 でも安全に動作（クラッシュしない）

---

## 2) 追加フラグ（opt-in）

- `HZ4_PAGE_META_SEPARATE=1`
- `HZ4_PAGE_DECOMMIT=1`
- `HZ4_LOCAL_FREE_DECOMMIT=1`（local free の場で decommit まで走らせる）
- **新設**: `HZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1`（Phase 1F の本体）

※ `HZ4_TCACHE_PURGE_BEFORE_DECOMMIT=0` がデフォルト（安全策は opt-in）。

---

## 3) 実装（TCachePurgeBox）

### 3.1 API

```c
// owner thread only
static inline uint32_t hz4_tcache_purge_page_for_sc(hz4_tls_t* tls, uint8_t sc, hz4_page_t* page);
```

### 3.2 対象

- `tls->bins[sc]`（or `atls->bins[sc]`）
- もし存在するなら `tls->inbox_stash[sc]`（splice refill の stash）

### 3.3 アルゴリズム（単純な O(n) unlink）

```
prev = NULL
cur  = bin->head
while (cur):
  next = obj_next(cur)
  if (page_from_ptr(cur) == target_page):
    unlink cur
    dec count (if enabled)
  else:
    prev = cur
  cur = next
```

ポイント:
- `used_count==0` イベントの時だけ走るので、O(n) は許容（RSS モード専用）。
- purge した object は「free」なので、参照を落としても leak ではない（再利用は refill で回復）。

---

## 4) 変更点（統合）

local free（owner）で:

1. `hz4_page_used_dec(page, 1)`
2. `if (used==0)`:
   - `hz4_tcache_purge_page_for_sc(tls, sc, page)` を実行（Phase 1F）
   - `hz4_page_try_decommit(page, meta)` を実行（Phase 1E と同じ）

---

## 5) SSOT（Phase 1F 検証）

ビルド（例）:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_LOCAL_FREE_DECOMMIT=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1'
```

実行（R=0 / R=50）:
```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 50 65536
```

期待:
- R=50 が **落ちない**（SIGABRT が消える）
- `page_decommit > 0` が出る

---

## 6) 記録フォーマット

```
R=0  ops/s=... RSS=...KB  page_decommit=... page_commit=...
R=50 ops/s=... RSS=...KB  page_decommit=... page_commit=...
```

---

## 7) 既知の落とし穴（Phase 1F の結論）

- delay/hysteresis が無いと local-only (R=0) で `decommit≈commit` のスラッシングになり得る。
- Phase 2 では **deadline を持つ DecommitQueueBox** を入れて、decommit 回数を抑制する。
