# PHASE_HZ3_S62-1b: Sub4kRunRetireBox（sub4k 2-page run の retirement）

Status: DONE（GO / research / default OFF）

Date: 2026-01-09  
Track: hz3  
Previous:
- S62-1（small_v2 page retire）: GO（free_bits へ戻せることを確認）
- S62-2（small_v2 purge）: MECH GO（RSS 可視化不可）

---

## 0) 目的

sub4k は **2-page run** を使うため、page 単位の retire では安全に判断できない。
S62-1b は **run 単位**で “完全に空いた run” を検出して free_bits に戻す。

境界（1箇所）:
- `hz3_tcache_destructor()` の cold path（thread-exit）
- hot path は **run allocation 時の run_start マークのみ**

---

## 1) 安全性（Fail-Fast）

- **run の開始ページが特定できない場合は retire 禁止**
- sub4k run は **2-page fixed**。run_start と run_start+1 の両方が empty の時だけ retire
- `Hz3SegHdr` の `magic/kind/owner` を確認してから free_bits を触る

---

## 2) 設計の要点（run_start bitmap）

### 2.1 追加メタ

`Hz3SegHdr` に sub4k 用の run_start bitmap を追加:

```
uint64_t sub4k_run_start_bits[HZ3_BITMAP_WORDS];
```

意味:
- bit=1: その page_idx が sub4k run の開始ページ
- run は **2 pages** 固定（start と start+1 が対象）

### 2.2 run_start の設定（hot path ではない）

`hz3_sub4k_alloc_run()` で run 確保直後に:
- `sub4k_run_start_bits[start_page] = 1`

### 2.3 retire 時の run_start 判定

object の page_idx から:
- `if (run_start_bits[page_idx]) run_start = page_idx`
- `else if (page_idx > 0 && run_start_bits[page_idx - 1]) run_start = page_idx - 1`
- それ以外 → **判定不可**でスキップ（安全優先）

---

## 3) 実装フロー（cold path）

対象: sub4k central bins（my_shard のみ）

1. `hz3_sub4k_central_pop_batch_ext()` で batch ずつ全 drain
2. obj を run_base（run_start）ごとに集計
3. `run_count == run_capacity(sc)` なら retire
   - `hz3_bitmap_mark_free(hdr->free_bits, run_start, 2)`
   - `hdr->free_pages += 2`
   - `sub4k_run_start_bits[run_start] = 0`
4. retire できない run は central に push back

run capacity:
```
size_t run_capacity = (HZ3_PAGE_SIZE * 2u) / hz3_sub4k_sc_to_size(sc);
```

---

## 4) 追加 API（sub4k central）

`hakozuna/hz3/include/hz3_sub4k.h` に追加:

```
uint32_t hz3_sub4k_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want);
void hz3_sub4k_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n);
```

`hz3_sub4k.c` で static central の wrapper を公開。

---

## 5) Flag（戻せる）

`hakozuna/hz3/include/hz3_config.h`:

```
#ifndef HZ3_S62_SUB4K_RETIRE
#define HZ3_S62_SUB4K_RETIRE 0
#endif
```

default OFF（research）

---

## 6) 接続点（destructor）

```
#if HZ3_S62_SUB4K_RETIRE
    hz3_s62_sub4k_retire();
#endif
```

順序（SSOT）:
- S62-1 (small_v2 retire)
- S62-1b (sub4k retire)
- S62-2 (purge)
- S62-0 (observe)
- S61 (medium hard purge)

---

## 7) 変更ファイル一覧

| ファイル                                 | 内容                              |
|------------------------------------------|-----------------------------------|
| hakozuna/hz3/include/hz3_seg_hdr.h       | sub4k_run_start_bits 追加         |
| hakozuna/hz3/src/hz3_sub4k.c             | run_start mark + central wrapper  |
| hakozuna/hz3/include/hz3_sub4k.h         | pop_batch/push_list API           |
| hakozuna/hz3/include/hz3_s62_sub4k.h     | 新規（API）                       |
| hakozuna/hz3/src/hz3_s62_sub4k.c         | 新規（retire 実装）               |
| hakozuna/hz3/include/hz3_config.h        | HZ3_S62_SUB4K_RETIRE              |
| hakozuna/hz3/src/hz3_tcache.c            | dtor に接続                       |

---

## 8) GO / NO-GO

GO:
- `pages_retired > 0`（sub4k run が retire される）
- `HZ3_S62_OBSERVE` の free_pages が増える
- crash/abort なし

NO-GO:
- retire 0（run_start 判定が機能していない）
- retire 後に `free` 経路が壊れる / segv

---

## 9) 次の分岐

- S62-2 RSS 再評価（sub4k retire を含めた purge 面積の増加を確認）
- 効かなければ S63（TagMap RSS 常駐削減）へ

---

## 10) 結果ログ（2026-01-09）

```
[HZ3_S62_SUB4K_RETIRE] dtor_calls=1 pages_retired=442 objs_processed=463
[HZ3_S62_RETIRE] dtor_calls=1 pages_retired=204 objs_processed=695
[HZ3_S62_PURGE] dtor_calls=1 ... pages_purged=765
```

メモ:
- `HZ3_SUB4K_ENABLE=1` の single-thread 条件で retire 動作を確認
- threads>=2 でハング（sub4k allocator 自体の課題。S62-1b とは切り分け済み）
