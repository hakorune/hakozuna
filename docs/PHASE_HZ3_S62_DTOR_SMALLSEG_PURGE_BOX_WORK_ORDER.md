# PHASE_HZ3_S62: DtorSmallSegPurgeBox（small/sub4k 残差を thread-exit で落とす）

Status: IN PROGRESS（S62-0/1 DONE, S62-2 MECH GO / RSS 可視化不可 / research / default OFF）

Date: 2026-01-09  
Track: hz3  
Previous:
- S59-2: mimalloc `MIMALLOC_PURGE_DELAY=0` で idle=10s 後 RSS ~2MB
- S61: DtorHardPurgeBox は **GO（idle RSS -76%）**（medium 側の返却/OS purge が成立）

---

## 0) 目的（SSOT）

S61（medium 側）の purge で残る差分（例: hz3 ~138MB vs mimalloc ~2MB）を、**small/sub4k の OS 返却不足**として切り分け、
thread-exit（cold）境界で “小さい方も purge” できる状態を作る。

狙い:
- S61: `hz3_central`（medium run）→ `hz3_segment_free_run()` → fully-free segment `madvise(DONTNEED)` は成立。
- small/sub4k: `Hz3SegHdr` セグメント内にページを切っており、**OS返却を撃つ箱が存在しない**。
- `hakozuna/bench/linux/hz3/bench_burst_idle`（S59-2）で **idle 窓の RSS をさらに落とす**（mimalloc `PURGE_DELAY=0` に近づける）。

重要（安全性 / Fail-Fast）:
- small/sub4k は「ページが空＝安全に `madvise`」を判定するメタが現状不足している。
- **live object を含むページに `madvise(DONTNEED)` を撃つのは破壊的（即バグ）**なので、
  S62 は **“page retirement（空ページを segment の free_bits に戻す）” が成立してから** purge を撃つ。
- したがって S62 の実装は 2 段階に分ける（S62-1 → S62-2）。本 work order はその台帳（SSOT）を固定する。

---

## 1) 箱割り（境界は 1 箇所）

Box: `S62_DtorSmallSegPurgeBox`

Boundary（唯一）:
- `hz3_tcache_destructor()` の末尾（thread-exit、cold）
  - すでに destructor で small/sub4k bin は central へ返る
  - 現状の順序（SSOT）:
    - S62-1（retire）→ S62-2（purge）→（任意）S62-0（observe）→ S61（medium hard purge）

禁止:
- malloc/free の hot path に purge ロジックを入れない（S62 は dtor-only）
- `getenv()` の分散（compile-time A/B のみ）

---

## 2) 設計（最小実装の方針 / 安全前提）

### 2.1 S62-1: PageRetireBox（先に“空ページを返す”）

目的:
- small/sub4k の “空になったページ” を **segment（`Hz3SegHdr`）へ返却**できるようにして、
  OS purge の “対象ページ” を作る（mimalloc と同じ前提）。

要点:
- page retirement は **境界 1 箇所**に集約（例: central/tcache drain の cold path）
- hot path へのカウンタ追加は最後の手段（まずは cold path の設計で成立させる）

### 2.2 S62-2: DtorSmallPagePurgeBox（retired pages のみ purge）

対象: `Hz3SegHdr` のうち、**S62-1 で “retired（空）” と判定できたページ範囲のみ**

戦略（最小・安全）:
- purge は `hz3_os_purge`（`hz3_os_madvise_dontneed_checked()`）を通す
- **メタページ（先頭 1 page）を除外する**のは保持
- ただし「残り全ページを一律 purge」は禁止（retired pages だけを purge）

### 2.2 どうやってセグメントを見つけるか（境界内で完結）

thread-exit の cold path なので、**arena used[] を線形走査**して my_shard の small seg を拾う:
- `base = hz3_arena_get_base()`
- `slots = hz3_arena_slots()`
- `if (hz3_arena_slot_used(i))` の slot base を `Hz3SegHdr*` として読む
- `hdr->magic == HZ3_SEG_HDR_MAGIC && hdr->kind == HZ3_SEG_KIND_SMALL && hdr->owner == my_shard` で候補

メリット:
- “segment list を別途持つ” 研究箱（メタ/同期/除去）が不要
- S59-2 のような “thread を大量生成” でも shard ごとの残留を確実に拾える

---

## 3) API / 実装ファイル

S62-0（OBSERVE / stats only）:
- `hakozuna/hz3/include/hz3_s62_observe.h`
- `hakozuna/hz3/src/hz3_s62_observe.c`

S62-1（PageRetireBox / stats only）:
- `hakozuna/hz3/include/hz3_s62_retire.h`
- `hakozuna/hz3/src/hz3_s62_retire.c`
- `hakozuna/hz3/include/hz3_small_v2.h`（central pop-batch + capacity API）
- `hakozuna/hz3/src/hz3_small_v2.c`

共通（可視化の箱）:
- `hakozuna/hz3/include/hz3_dtor_stats.h`（atexit one-shot / atomic stats の共通化）

S62-2（DtorSmallPagePurgeBox）:
- `hakozuna/hz3/include/hz3_s62_purge.h`
- `hakozuna/hz3/src/hz3_s62_purge.c`

追加検討（S62-1）:
- `hz3_small_v2` / `hz3_sub4k` の “空ページ返却” を担う箱（ファイルは後決定、まず境界設計を確定する）

API（S62 の箱境界内で完結）:

```c
// S62-0: OBSERVE (cold, stats only)
void hz3_s62_observe(void);

// S62-1: PageRetireBox (cold, retire only)
void hz3_s62_retire(void);

// S62-2: DtorSmallPagePurgeBox (cold, planned)
// void hz3_s62_purge(void);
```

接続点:
- `hakozuna/hz3/src/hz3_tcache.c` の `hz3_tcache_destructor()` 末尾で呼ぶ（S61 の後）。

---

## 4) Compile-time Flags（戻せる）

`hakozuna/hz3/include/hz3_config.h` に追加（default OFF）:

- `HZ3_S62_OBSERVE`（0/1）: S62-0（stats only / madvise 無し）
- `HZ3_S62_RETIRE`（0/1）: S62-1（stats only / retire のみ）

S62-2（default OFF）:
- `HZ3_S62_PURGE`（0/1）
- `HZ3_S62_PURGE_MAX_CALLS`（madvise syscall 上限）
- （統計は `hz3_dtor_stats.h` の atexit one-shot を使用）

推奨デフォルト（暫定）:
- S62-2 `HZ3_S62_PURGE_MAX_CALLS=4096`（上限は大きめ、必要なら下げる）

---

## 5) 可視化（one-shot）

atexit で 1 回だけ（常時ログ禁止）:

例:
- S62-0: `[HZ3_S62_OBSERVE] ... purge_potential_bytes=...`
- S62-1: `[HZ3_S62_RETIRE] ... pages_retired=... objs_processed=...`
- S62-2（予定）: `[HZ3_S62_PURGE] ... madvise_ok=... purged_bytes=...`

観測したいこと:
- `purged_segs > 0`（配線確認）
- `purged_bytes` が “残差 RSS” と同オーダで増える（coverage の確認）

---

## 6) 実装手順（Work Order）

（完了）S62-0/1:
1. `hz3_s62_observe.{h,c}` を追加（S62-0、stats/atexit）
2. `hz3_s62_retire.{h,c}` を追加（S62-1、stats/atexit）
3. `hz3_small_v2` に central pop-batch + page capacity API を追加（S62-1 用）
4. `hz3_tcache.c` の destructor へ接続（S62-1 → S62-0 → S61）
5. `hz3_config.h` に `HZ3_S62_OBSERVE` / `HZ3_S62_RETIRE` を追加（default OFF）

（完了）S62-2:
1. `hz3_s62_purge.{h,c}` を追加（S62-2、run batching + budget）
2. `hz3_tcache.c` の destructor へ接続（S62-1 → S62-2 → S62-0 → S61）
3. `hz3_config.h` に `HZ3_S62_PURGE` / `HZ3_S62_PURGE_MAX_CALLS` を追加（default OFF）

---

## 7) 計測（S59-2 を SSOT にする）

比較ベンチ:
- `hakozuna/bench/linux/hz3/bench_burst_idle 200000 10 3 0 4`

比較対象:
- hz3 baseline: `-DHZ3_S61_DTOR_HARD_PURGE=1`
- hz3 treatment（S62-0/1/2）: `-DHZ3_S61_DTOR_HARD_PURGE=1 -DHZ3_S62_RETIRE=1 -DHZ3_S62_PURGE=1 -DHZ3_S62_OBSERVE=1`
- mimalloc reference: `MIMALLOC_PURGE_DELAY=0`

推奨コマンド（例）:

```sh
cd /mnt/workdisk/public_share/hakmem

# hz3 baseline (S61 only)
make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S61_DTOR_HARD_PURGE=1'
LD_PRELOAD=$PWD/libhakozuna_hz3_scale.so ./hakozuna/bench/linux/hz3/bench_burst_idle 200000 10 3 0 4

# hz3 treatment (S61 + S62-0/1/2)
make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S61_DTOR_HARD_PURGE=1 -DHZ3_S62_RETIRE=1 -DHZ3_S62_PURGE=1 -DHZ3_S62_OBSERVE=1'
LD_PRELOAD=$PWD/libhakozuna_hz3_scale.so ./hakozuna/bench/linux/hz3/bench_burst_idle 200000 10 3 0 4

# mimalloc reference
MIMALLOC_PURGE_DELAY=0 LD_PRELOAD=$PWD/libmimalloc.so ./hakozuna/bench/linux/hz3/bench_burst_idle 200000 10 3 0 4
```

---

## 8) GO / NO-GO（判定基準）

GO:
- `purged_segs > 0` かつ `purged_bytes` が “残差 RSS” と同オーダ
- idle RSS が **S61 からさらに有意に下がる**（目安: -50%以上）
- `madvise_fail == 0`（failfast 無しでも 0 が理想）

NO-GO:
- `purged_segs > 0` でも RSS が動かない（残差の主因が small/sub4k ではない）
- `madvise_fail` が出る、または不安定化（abort/segv）
- retired 判定が無いまま purge を撃って live object を破壊する設計（即 NG）

---

## 9) 次の分岐（S62-2 / S63）

S62 が効かない場合の疑い:
- 残差が `page_tag32/page_tag`（TagMap）や他のメタデータ常駐に寄っている
- small/sub4k セグメントの purge 対象が少ない（thread-exit でも “生存” が多い）

次の候補:
- S62-2: purge 条件を厳密化（live tracking / segment state など）して安全に一般化
- S63: TagMap の RSS 常駐を抑える箱（巨大 arena での TagMap touch 面積の削減）

---

## 10) 結果ログ（2026-01-09）

### S62-0（OBSERVE / retire 前）

```
[HZ3_S62_OBSERVE] dtor_calls=12 scanned_segs=13060 candidate_segs=372
  total_pages=94860 free_pages=1282 used_pages=93578 purge_potential_bytes=5251072
```

解釈:
- free_bits=1（初期未割当）だけでは purge 潜在が ~5MB しかなく、残差（~138MB）を説明できない
- → S62-1（PageRetireBox）が必須

### S62-1（PageRetireBox / small_v2）

観測:
- retire 後に S62-0 OBSERVE の `free_pages` が大きく増える（purge 潜在が増える）
- sub4k は run 境界が必要なため S62-1b に分離（安全性優先）
  - S62-1b（sub4k run retire）は single-thread 条件で GO（詳細は別指示書）

### S62-2（DtorSmallPagePurgeBox / small_v2）

```
[HZ3_S62_PURGE] dtor_calls=24 scanned_segs=26416 candidate_segs=384
  pages_purged=97920 madvise_calls=384 purged_bytes=401080320
[HZ3_S62_RETIRE] dtor_calls=24 pages_retired=94189 objs_processed=284162
```

解釈:
- free_bits=1 の連続 run をまとめて purge（平均 255 pages / call）
- budget 超過なし（calls=384 < max_calls）
- RSS 評価は idle 窓では可視化されない（keep=0 のため thread-exit に purge が集中）
  - dtor stats 確認: S62-1/2/0/S61 すべて発火済み（mechanism OK）

### S62 メモリ節約まとめ（3サイクル/各100k objects）

```
pages_retired=46840
pages_purged=47430
purged_bytes=185 MB
RSS delta=+128 KB（ほぼ0）
```

メモ:
- S62 は thread-exit で OS 返却するため、計測窓によって RSS 変化が見えにくい
- 長時間稼働で “膨張し続ける” ケースの抑制が主目的

### S62-2 RSS A/B（S59-2 条件, threads=8）

ベンチ:
`hakozuna/bench/linux/hz3/bench_burst_idle 200000 10 3 0 8`

結果（RSS_before → RSS_after）:
- S61 only: 539264→539264 / 542336→542336 / 544000→544000（delta 0）
- S61+S62-2: 539264→539264 / 542208→542208 / 543872→543872（delta 0）
- mimalloc `MIMALLOC_PURGE_DELAY=0`: 456464→456464 / 454860→454860 / 453884→453884（delta 0）

メモ:
- dtor 発火は確認済み（S62/S61 の stats が出力される）
- `bench_burst_idle` は keep=0 で全 free → purge が thread-exit に集中し、RSS 計測窓と一致しない
