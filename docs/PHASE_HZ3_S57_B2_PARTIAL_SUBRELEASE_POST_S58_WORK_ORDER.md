# PHASE_HZ3_S57-B2: PartialSubrelease（Segment madvise）Re-eval after S58

Status: ❌ NO-GO（coverage不足; pack pool scanではRSSが動かない）

Date: 2026-01-09  
Track: hz3  
Previous: S57-A/B NO-GO（tcache滞留 / re-fault）, S58 SPEED GO（tcache→central→segment通電）

---

## Results（S59-2条件 / idle=10s）

| Metric | Value | Verdict |
|---|---:|---|
| `madvise_calls` | 2 | mechanism works |
| `purged_pages` | 61（~250KB） | insufficient coverage |
| RSS（hz3 S58=1 + S57-B2=1） | ~565MB | no change |
| RSS（mimalloc `MIMALLOC_PURGE_DELAY=0`） | ~2.3MB | reference |

Root cause:
- S57-B2 は pack pool 内 segment を走査する設計だが、burst 後は segment の多くが full/active。
- pack pool に候補が残らず、purge 対象がほぼ得られない（coverage が極小）。

Conclusion:
- “segment 走査して free-range を探す”方針は、S58 後でも coverage が足りず NO-GO。
- 次は **S60 → S61**:
  - S60（PurgeRangeQueueBox）で `madvise` 発火の配線は成立したが、S59-2 の idle RSS 目的では NO-GO（台帳: `hakozuna/hz3/docs/PHASE_HZ3_S60_PURGE_RANGE_QUEUE_BOX_WORK_ORDER.md`）。
  - S61（thread-exit/idle 境界で hard purge）へピボット（指示書: `hakozuna/hz3/docs/PHASE_HZ3_S61_DTOR_HARD_PURGE_BOX_WORK_ORDER.md`）。

## 0) 目的（SSOT）

S58 で `tcache→central→segment` が通ったため、**segment に戻った free pages** に対して `madvise` を当てれば、
mimalloc の「PURGE_DELAY=0 相当（即 purge）」に近い RSS 圧縮が可能かを再評価する。

前提: S57-B は **master本線に戻さない**。必ず research branch/worktree で実施し、GO/NO-GO に応じて keep/archive を決める。

---

## 1) 比較で判明したこと（入口）

S59-2 で「比較可能な条件」が見つかった:

- mimalloc default: idle=10s でも RSS が落ちない
- mimalloc `PURGE_DELAY=0`: idle=10s で RSS が大きく落ちる
- tcmalloc default: idle=10s でも RSS が落ちない
- hz3 S58=1: reclaim は動くが `madvise` が無いので RSS は落ちない

結論: **hz3 で mimalloc(PURGE_DELAY=0) 相当の挙動を作るには、segment側の `madvise` 箱が必要**。

---

## 2) 箱割り（境界は 1 箇所）

Box: `S57-B2_SegmentPurgeBox`

- Boundary（唯一）: `hz3_epoch_force()` の末尾（event-only）
  - 先に S58 の reclaim を実行し、segment 側に free pages を作る
  - その後、candidate segment を走査して `madvise(MADV_DONTNEED)` を budgeted に実行

禁止:
- malloc/free hot path に purge ロジックを入れない
- `getenv()` 分散（compile-time A/B のみ）

---

## 3) 実装方針（復元は snapshot から / ただし B2 用に最小修正）

### 3.1 作業場所（必須）

master を汚さないため、worktree 推奨:

```sh
cd /mnt/workdisk/public_share/hakmem
git worktree add -b hz3_s57b2_revival ../hakmem_hz3_s57b2 HEAD
cd ../hakmem_hz3_s57b2
```

### 3.2 Snapshot 復元

S57-B snapshot を復元（まずはそのまま持ってくる）:

```sh
cp -a hakozuna/hz3/archive/research/s57_b_partial_subrelease/snapshot/hz3_segment_purge.c \
      hakozuna/hz3/src/hz3_segment_purge.c
cp -a hakozuna/hz3/archive/research/s57_b_partial_subrelease/snapshot/hz3_segment_purge.h \
      hakozuna/hz3/include/hz3_segment_purge.h
```

### 3.3 必要な接続（最低限）

S57-B2 を “通電” するために、以下を復元/追加する（すべて `#if HZ3_S57_PARTIAL_PURGE` ガード）:

1. `hakozuna/hz3/include/hz3_config.h`
   - `HZ3_S57_PARTIAL_PURGE=0` default OFF
   - ノブ（B2用の初期値は aggressive を推奨）:
     - `HZ3_S57_PURGE_DECAY_EPOCHS`（まず 0）
     - `HZ3_S57_PURGE_BUDGET_PAGES`（例: 4096 pages = 16MiB / epoch）
     - `HZ3_S57_PURGE_MIN_RANGE_PAGES`（まず 1）
     - `HZ3_S57_PURGE_STATS`（1）
2. `hakozuna/hz3/include/hz3_types.h`
   - `Hz3SegMeta` に `last_touch_epoch` と `purged_bits[]` を追加（S57-B2 用）
3. `hakozuna/hz3/src/hz3_segment.c`
   - `hz3_segment_init()` で `hz3_segment_purge_init_meta(meta)`
   - `hz3_segment_alloc_run/free_run()` で `hz3_segment_clear_purged_range()` を呼ぶ（slow path なので許容）
4. `hakozuna/hz3/src/hz3_epoch.c`
   - `hz3_epoch_force()` の末尾で `hz3_segment_purge_tick()` を呼ぶ
   - 呼び出し順: **S58 reclaim → S57-B2 purge**（この順がSSOT）
5. `hakozuna/hz3/src/hz3_segment_packing.c` / `hakozuna/hz3/include/hz3_segment_packing.h`
   - snapshot が `hz3_pack_iterate()` を使う場合、iterator API を復元（read-only traversal, my_shard only）

---

## 4) 計測（GO/NO-GO）

### 4.1 計測条件（S59-2 と同一）

- `bench_burst_idle_v2`（idle=10s, threads=4, keep_ratio=0 の “比較成立条件”）
- RSS timeseries は `measure_mem_timeseries.sh` で `/tmp/*.csv` に保存

### 4.2 比較対象（最低限）

| Name | 条件 |
|---|---|
| mimalloc default | baseline |
| mimalloc `PURGE_DELAY=0` | “圧縮できるケース”の基準 |
| hz3 `S58=1` | purge無し（現状） |
| hz3 `S58=1 + S57-B2=1` | 目的経路 |

※ tcmalloc は default で落ちないことが多いので、比較に入れるなら “RSSを落とす設定” を別途見つけた場合のみ。

### 4.3 hz3 build（A/B）

```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S58_TCACHE_DECAY=1 -DHZ3_S57_PARTIAL_PURGE=1'
```

初回は “メカニズム確認” のため aggressive 推奨（必要なら追加）:

```sh
HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S58_TCACHE_DECAY=1 -DHZ3_S57_PARTIAL_PURGE=1 \
  -DHZ3_S57_PURGE_DECAY_EPOCHS=0 -DHZ3_S57_PURGE_MIN_RANGE_PAGES=1 \
  -DHZ3_S57_PURGE_BUDGET_PAGES=4096'
```

### 4.4 成功条件（GO）

- `HZ3_S57B_STATS` で `purged_pages > 0` が継続的に出る（mechanism）
- mimalloc `PURGE_DELAY=0` と同条件で、idle 後に RSS が **有意に下がる**（少なくとも -50%以上を目安、最終は同等レベル）
- SSOT（small/medium/mixed）退行が **±2%以内**

NO-GO:
- `purged_pages > 0` でも RSS が動かない（re-fault 優勢）
- 速度退行 >2%（特に mixed）
- 不安定化（クラッシュ/破壊）

---

## 5) 後始末（箱理論）

GO:
- 研究箱として keep（default OFF）し、`docs/NO_GO_SUMMARY.md` ではなく別の “KEEP（research）” 台帳へ記録
- master へ入れる場合も **default OFF** のまま（A/B 前提）

NO-GO:
- 変更を再度 `archive/research/s57_b2_*` に隔離し、本線を汚さない（S57-B と同様）
