# PHASE_HZ3_S64: Mimalloc-like Purge (Epoch Retire + Purge Delay)

Status: IMPLEMENTED / EVAL RUN (retire/purge observed, RSS unchanged)

Date: 2026-01-09  
Track: hz3  
Previous:
- S62: thread-exit retire/purge works (scale lane default ON)
- S63: sub4k MT triage (no hang reproduced under targeted runs)
Implemented:
- `hz3_s64_retire_scan.{h,c}` / `hz3_s64_purge_delay.{h,c}`
- `hz3_s64_tcache_dump.{h,c}` (S64-1)
- `hz3_config_rss_memory.h`（flags）
- `hz3_knobs.c` / `hz3_types.h`（knobs 追加）
- `hz3_epoch.c`（tick 接続）

---

## 0) 目的（SSOT）

thread-exit 以外でも **steady-state で RSS を落とす**ために、
small_v2 の retire/purge を **epoch 境界で少しずつ実行**する。

狙い:
- mimalloc の purge_delay に近い挙動を hz3 に導入
- hot path 0、境界は epoch 1 箇所
- 学習層で delay/budget を調整しやすい設計

---

## 1) 箱割り（境界 1 箇所）

Box A: **S64_TCacheDumpBox**  
Box B: **S64_RetireScanBox**  
Box C: **S64_PurgeDelayBox**

Boundary（唯一）:
- `hz3_epoch_force()` / epoch tick の末尾で呼ぶ
  - 順序: `S64_TCacheDumpBox -> S64_RetireScanBox -> S64_PurgeDelayBox`

禁止:
- hot path に retire/purge を入れない
- 分散 getenv（knobs 経由のみ）

---

## 2) S64_TCacheDumpBox（epoch で tcache を中央へ戻す）

目的:
- tcache → central の流量を epoch で作り、retire/purge の入力を確保する
- hot path 0、境界は epoch のみ

予算:
- `HZ3_S64_TDUMP_BUDGET_OBJS`

備考:
- SoA/AoS 両対応
- bin count は `HZ3_BIN_LAZY_COUNT` に配慮

---

## 3) S64_RetireScanBox（epoch で retire を作る）

目的:
- small_v2 central の free object を **budgeted に走査**して空ページを retire
- S62-1 の “retire 判定” を epoch に持ち込む

方針（最小・安全）:
- `hz3_small_v2_central_pop_batch()` で batch 取得（my_shard のみ）
- page 単位で count 集計 → count==capacity なら retire
- retire できない分は central に戻す

Retire 1 箇所化:
- `hz3_small_v2_retire_page(hdr, page_idx)` のような helper に集約
- S62-1（dtor）と S64-A（epoch）で共有

予算:
- `HZ3_S64_RETIRE_BUDGET_OBJS` で batch 合計を制限

---

## 4) S64_PurgeDelayBox（delay 付き purge）

目的:
- retire 済みページを **delay 経過後に** `madvise(DONTNEED)` で返却
- 再フォールトの揺れを減らす

データ構造（per-shard ring）:
```
typedef struct {
  void*    seg_base;
  uint16_t page_idx;
  uint16_t pages;
  uint32_t retire_epoch;
} Hz3S64PurgeEntry;
```

ルール:
- S64_RetireScanBox で retire したページを queue に入れる
- epoch tick で `now - retire_epoch >= DELAY_EPOCHS` のみ purge
- purge は `hz3_os_madvise_dontneed_checked()` 経由

予算:
- `HZ3_S64_PURGE_BUDGET_PAGES`
- `HZ3_S64_PURGE_MAX_CALLS`

---

## 5) Compile-time Flags（戻せる）

`hakozuna/hz3/include/hz3_config.h`

- `HZ3_S64_RETIRE_SCAN` (default OFF)
- `HZ3_S64_PURGE_DELAY` (default OFF)
- `HZ3_S64_TCACHE_DUMP` (default OFF)
- `HZ3_S64_RETIRE_BUDGET_OBJS`
- `HZ3_S64_TDUMP_BUDGET_OBJS`
- `HZ3_S64_PURGE_DELAY_EPOCHS`
- `HZ3_S64_PURGE_BUDGET_PAGES`
- `HZ3_S64_PURGE_MAX_CALLS`
- `HZ3_S64_PURGE_QUEUE_SIZE`
- `HZ3_S64_PURGE_IMMEDIATE_ON_ZERO`
- `HZ3_S64_STATS` (atexit one-shot)
- `HZ3_OS_PURGE_STATS` (atexit one-shot)

---

## 6) Knobs / Learning Layer 連携

knobs は `hz3_knobs` に集約:
- `s64_delay_epochs`
- `s64_retire_budget_objs`
- `s64_tdump_budget_objs`
- `s64_purge_budget_pages`

学習層（ACE/ELO/CAP）は **knobs の値だけ更新**する設計で相性が良い:
- FROZEN default（学習 OFF）
- LEARN の時のみ値を書き換え（hot path は読むだけ）

---

## 7) 実装ファイル

追加:
- `hakozuna/hz3/include/hz3_s64_retire_scan.h`
- `hakozuna/hz3/src/hz3_s64_retire_scan.c`
- `hakozuna/hz3/include/hz3_s64_purge_delay.h`
- `hakozuna/hz3/src/hz3_s64_purge_delay.c`

変更:
- `hz3_epoch.c`（epoch tick に接続）
- `hz3_small_v2.c/h`（retire helper 共有）
- `hz3_config.h`（flags）
- `hz3_knobs.c/h`（env snapshot）

---

## 8) 可視化（one-shot）

```
[HZ3_S64] retire_scanned=... pages_retired=... queue_len=...
  purged_pages=... purged_bytes=... budget_hits=...
```

常時ログ禁止。atexit の 1 回のみ。

---

## 9) 評価（S59-2 + keep_hold）

ベンチ:
```
OUT=/tmp/hz3_mem_keep20_hold1.csv INTERVAL_MS=50 \
  ./hakozuna/hz3/scripts/measure_mem_timeseries.sh \
  env LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 200000 10 3 20 4 256 4096 1
```

期待:
- keep_hold=1 でも idle で RSS が緩やかに下降
- S62 と同程度の速度維持

結果（2026-01-09, keep_hold=1 / idle_tick=100ms / immediate=ON）:
- `[HZ3_OS_PURGE] calls=50202 ok=50202 fail=0`
- `[HZ3_S64_PURGE_DELAY] purged_pages=49830 madvise_calls=49830 immediate=49830`
- RSS delta: 0（RSS 主成分の匿名 VMA が残留）
- smaps: 最大 RSS は arena 範囲内の匿名 RW（例: Size 656,384 KB / RSS 429,688 KB）
- arena: `base=... end=... size=64GB`, `used_slots=545/65536`（≈1.06GB使用中）
まとめ: purge は発火しているが **arena 内の使用セグメントが残るため RSS が落ちない**可能性が高い

---

## 10) GO / NO-GO

GO:
- purged_pages が steady-state で継続的に増える
- RSS が mimalloc `PURGE_DELAY=0` に近づく傾向
- SSOT 退行 ≤ 2%

NO-GO:
- purged_pages > 0 でも RSS が全く動かない
- hot path への影響が出る

---

## 11) 実装メモ（2026-01-09）

有効化例:
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S64_TCACHE_DUMP=1 -DHZ3_S64_RETIRE_SCAN=1 -DHZ3_S64_PURGE_DELAY=1 -DHZ3_S64_STATS=1'
```

評価は keep_hold=1 で実施する（S59-2 条件を踏襲）。
ベンチ拡張: `bench_burst_idle_v2` に `idle_tick_ms` / `dump_mode` / `S59_2_DUMP_PATH` を追加。
