# PHASE_HZ3_S61: DtorHardPurgeBox（thread-exit で “PURGE_DELAY=0 相当” を作る）

Status: DONE（GO / research / default OFF）

Date: 2026-01-09  
Track: hz3  
Previous:
- S59-2: mimalloc `MIMALLOC_PURGE_DELAY=0` で idle=10s 後 RSS ~2.3MB
- S60: PurgeRangeQueueBox は `madvise` 発火は成立したが、S59-2 の idle RSS 目的では NO-GO

---

## Results（GO）

S59-2 基準ベンチ（`bench_burst_idle_v2 200000 10 3 0 4`）で GO:

| Condition | cycle 1 RSS | cycle 3 RSS | delta |
|---|---:|---:|---:|
| hz3 baseline (`HZ3_S61_DTOR_HARD_PURGE=0`) | 566MB | 1676MB | - |
| hz3 treatment (`HZ3_S61_DTOR_HARD_PURGE=1`) | 138MB | 393MB | -76% |
| mimalloc reference (`MIMALLOC_PURGE_DELAY=0`) | 2MB | 2MB | - |

S61 stats（atexit one-shot）:
`[HZ3_S61_DTOR_PURGE] dtor_calls=12 madvise_ok=1258 madvise_fail=0 purged_segs=1258 purged_pages=322048 drained_objs=320748`

結論:
- `madvise` の配線と safety は成立（fail=0）。
- fully-free segment 単位の `madvise(DONTNEED)` により、idle RSS を大きく削減できた。
- 残差（hz3 ~138MB vs mimalloc ~2MB）は small/sub4k 側の “page retirement / purge” が未実装のため（次フェーズ）。

## 0) 目的（SSOT）

`bench_burst_idle_v2` のような “burst → free → idle” で、hz3 も **idle 窓で RSS が落ちる**状態を作る。

狙い:
- mimalloc `MIMALLOC_PURGE_DELAY=0` は「free されたページを早期に `madvise`」できる。
- hz3 は free object を **tcache/central に保持**するため、OS 返却が起きない。
- **thread exit は hot path ではない**ため、ここに “強い purge” を閉じ込めて A/B しやすくする。

S61 の焦点:
- S60 のような “epoch 中の purge” ではなく、**free 波の直後（thread exit）**で purge を撃つ。

---

## 1) 箱割り（境界は 1 箇所）

Box: `S61_DtorHardPurgeBox`

Boundary（唯一）:
- `hz3_tcache_destructor()` の末尾（thread-exit、cold）
  - すでに destructor で「各 bin を central へ返す」ことは成立している
  - その直後に “central → segment 返却 + madvise” を実行して RSS を落とす

禁止:
- malloc/free の hot path に purge ロジックを入れない
- `getenv()` の分散（compile-time A/B のみ）
- 他 shard を触る（`hz3_segment_free_run()` は lock-less → my_shard only）

---

## 2) 設計（やること / やらないこと）

### 2.1 やること（medium run → fully-free segment を確実に落とす）

thread-exit 時に “my_shard の central” を drain して、run を segment に戻し、fully-free になった segment を `madvise(DONTNEED)`:

1. `run = hz3_central_pop(my_shard, sc)`（pop で intrusive next 依存を切る）
2. `hz3_segment_free_run(meta, page_idx, pages)`（segment の free_bits/free_pages を更新）
3. `meta->free_pages == HZ3_PAGES_PER_SEG` になったら `madvise(seg_base, HZ3_SEG_SIZE, MADV_DONTNEED)`（OS 返却）

※サイズクラスは **大きい順（sc=7→0）**に処理して syscall 数を減らす（S55-3 の教訓）。

### 2.2 やらないこと（S60 の NO-GO を繰り返さない）

- pack pool 走査（S57-B2 の coverage 問題を再導入しない）
- TLS ring queue に大量に積んで overflow する設計（S61 は thread-exit で “その場で purge”）

---

## 3) API / 実装ファイル

新規:
- `hakozuna/hz3/include/hz3_dtor_hard_purge.h`
- `hakozuna/hz3/src/hz3_dtor_hard_purge.c`

API（S61 の箱境界内で完結）:

```c
// Thread-exit only (cold). my_shard only.
void hz3_s61_dtor_hard_purge(void);
```

接続点:
- `hakozuna/hz3/src/hz3_tcache.c` の `hz3_tcache_destructor()` 末尾で呼ぶ。

---

## 4) Compile-time Flags（戻せる）

`hakozuna/hz3/include/hz3_config.h` に追加（default OFF）:

- `HZ3_S61_DTOR_HARD_PURGE`（0/1）
- `HZ3_S61_DTOR_MAX_PAGES`（1回の destructor で purge する最大ページ数）
- `HZ3_S61_DTOR_MAX_CALLS`（madvise syscall 上限）
- `HZ3_S61_DTOR_STATS`（atexit one-shot）
- `HZ3_S61_DTOR_FAILFAST`（madvise 失敗や境界違反で abort、research 用）

推奨デフォルト（暫定）:
- `HZ3_S61_DTOR_MAX_PAGES=65536`（256MB）
- `HZ3_S61_DTOR_MAX_CALLS=256`（segment 単位なので少数で十分）

---

## 5) 可視化（one-shot）

atexit で 1 回だけ（常時ログ禁止）:

例:
`[HZ3_S61_DTOR_PURGE] dtor_calls=... madvise_ok=... madvise_fail=... purged_segs=... purged_pages=... drained_objs=...`

観測したいこと:
- `purged_pages > 0`（動作確認）
- `purged_pages` が S59-2 の RSS 主成分に届くオーダ（少なくとも数万 pages〜）

---

## 6) 測定（GO/NO-GO）

SSOT:
- `bench_burst_idle_v2 200000 10 3 0 4`（S59-2 と同一条件）

比較:
- hz3 baseline: `HZ3_S61_DTOR_HARD_PURGE=0`
- hz3 treatment: `HZ3_S61_DTOR_HARD_PURGE=1`
- reference: mimalloc `MIMALLOC_PURGE_DELAY=0`

GO（暫定）:
- idle 後 RSS が baseline 比で **有意に下がる**（まず -50% 以上）
- `purged_pages` が十分大きい
- SSOT small/medium/mixed の throughput が ±2% 以内（thread-exit only なので理想は 0 影響）

NO-GO:
- `purged_pages > 0` でも RSS が動かない（即 re-fault / purge タイミング不一致）
- 不安定化（list破壊、abort、segv）

---

## 7) 実装手順（作業順）

1. `hz3_dtor_hard_purge.[ch]` を追加（my_shard drain + madvise、budget 付き）
2. `hz3_config.h` に S61 flags を追加（default OFF）
3. `hz3_tcache.c` の destructor 末尾に `hz3_s61_dtor_hard_purge()` を接続
4. `bench_burst_idle_v2`（S59-2）で A/B（hz3 baseline vs S61）
5. 結果を `CURRENT_TASK.md` と関連台帳（S59-2 / S60 / BUILD_FLAGS_INDEX）に反映

---

## 8) リスクと次の分岐

- `bench_burst_idle_v2` の RSS 主成分が small（<=2048）のページに偏っている場合、
  medium run だけ purge しても mimalloc の ~2MB には届かない可能性がある。
  - その場合は small/sub4k の “page retirement（out-of-band metadata）” が必要（別フェーズ）。
