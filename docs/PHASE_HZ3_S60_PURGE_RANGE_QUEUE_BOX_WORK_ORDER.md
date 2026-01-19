# PHASE_HZ3_S60: PurgeRangeQueueBox（S58 reclaim 直結の madvise）

Status: DONE（メカニズムOK / S59-2 idle RSS 目的では NO-GO）

Date: 2026-01-09  
Track: hz3  
Previous:
- S58 TCache Decay Box: SPEED GO（`tcache→central→segment` 通電）
- S59-2: mimalloc `MIMALLOC_PURGE_DELAY=0` で idle=10s でも RSS が大きく落ちる
- S57-B2: NO-GO（pack pool 走査では coverage が極小）

---

## 結果（NO-GO）

S59-2 基準ベンチ（`bench_burst_idle_v2`）での結論:
- `madvise(DONTNEED)` は発火する（メカニズムはOK）
- しかし **idle 後 RSS が動かない**（mimalloc `MIMALLOC_PURGE_DELAY=0` のような “圧縮” にならない）

例（代表値）:
- hz3（S58=1 + S60=1）: idle 後 RSS ~565MB（不変）
- mimalloc（reference）: `MIMALLOC_PURGE_DELAY=0` で RSS ~2.3MB まで低下

S60 stats（例）:
- `[HZ3_S60_PURGE] queued=512 dropped=0 madvise_calls=256 madvised_pages=256 skipped_arena=0 budget_dropped=256`
  - `queued` は **range 件数**（= queue された回数）
  - `madvised_pages` は **ページ数**（4KB/page）

主因（SSOT）:
- S60 の `madvise` 対象面積は **S58 reclaim が segment に戻せた分だけ**（= `reclaim_pages` に束縛）
- デフォルト予算では面積が小さすぎ、RSS の主成分に届かない（差が見えない）
- さらに epoch 中に撃つと、その後の割当で re-fault されやすい（idle 窓に寄せないと RSS が落ちにくい）

## 0) 目的（SSOT）

mimalloc の `MIMALLOC_PURGE_DELAY=0` 相当の “coverage” を hz3 に作る。

結論（現状のSSOT）:
- hz3 は S58 で **segment に free pages を戻せる**が、OS へ返す `madvise` が無いので RSS が落ちない。
- segment 走査（S57-B2）は **候補が取れず** NO-GO。

S60 は方針を変える:
- “探す” のではなく、S58 reclaim が `hz3_segment_free_run()` で **実際に戻した run/page 範囲**をその場で捕捉し、
- epoch 境界で budgeted に `madvise(DONTNEED)` を当てる（hot path 0コスト、coverage 最大化）。

---

## 1) 箱割り（境界は 1 箇所）

Box: `S60_PurgeRangeQueueBox`

Boundary（唯一）:
- `hz3_epoch_force()` 末尾（event-only）
  - 入口側（S58 reclaim）で “返却した範囲” を queue に積む
  - 境界関数 `hz3_s60_purge_tick()` で queue を drain し、OS へ返す（madvise）

禁止:
- malloc/free の hot path に purge ロジックを入れない
- `getenv()` 分散（compile-time A/B のみ）
- segment 全走査（S57-B2 の NO-GO を繰り返さない）

---

## 2) 設計（SSOT と不変条件）

### 2.1 何を purge するか

S58 reclaim で次を満たした範囲のみを purge 対象にする:

1. `obj` を `hz3_central_pop(my_shard, sc)` で central から外した
2. `hz3_segment_free_run(meta, page_idx, pages)` で segment に “free” として戻した
3. 範囲はページアライン（`addr = seg_base + page_idx*PAGE_SIZE`, `len = pages*PAGE_SIZE`）

→ この範囲は intrusive freelist の next を破壊しない（run/page 単位で扱うため）。

### 2.2 なぜ queue が必要か

- S57-B2 は pack pool 限定で coverage が取れず NO-GO。
- S60 は “返した量そのもの” を coverage として使うため、`madvised_pages` を **S58 reclaim_pages に追随**させられる（ただし S58 自体が budgeted）。

---

## 3) データ構造（malloc禁止 / bounded）

### 3.1 TLS queue（推奨）

S58 reclaim は TLS（`t_hz3_cache`）文脈で走るため、range queue も TLS で完結させる:

- 固定長 ring（compile-time）
- 要素: `{ seg_base, page_idx, pages }`（page-aligned）
- push は O(1)、overflow は drop（統計だけ増やす）

### 3.2 budget（必須）

- `HZ3_S60_BUDGET_PAGES`（epoch あたりの総 madvise ページ上限）
- `HZ3_S60_BUDGET_CALLS`（syscall storm 防止）

---

## 4) API（箱の境界）

新規:
- `hakozuna/hz3/include/hz3_purge_range_queue.h`
- `hakozuna/hz3/src/hz3_purge_range_queue.c`

実装 API（SSOT）:

```c
// Called by S58 reclaim after hz3_segment_free_run().
// Event-only, my_shard context. (TLS ring, no locks)
void hz3_s60_queue_range(void* seg_base, uint16_t page_idx, uint16_t pages);

// Called once per epoch at a single boundary (hz3_epoch_force tail).
// Drains queued ranges and applies madvise (budgeted).
void hz3_s60_purge_tick(void);
```

OS call は S60 の中に閉じ込める（madvise はここだけ）。

---

## 5) 差し込み点（最小）

### 5.1 S58 reclaim との接続（coverageの源泉）

`hakozuna/hz3/src/hz3_tcache_decay.c` の `hz3_s58_central_reclaim()`:

- `hz3_segment_free_run(meta, page_idx, pages);` の直後に、
  - `hz3_s60_queue_range(seg_base, page_idx, pages);` を追加

### 5.2 epoch 境界（唯一の purge 実行点）

`hakozuna/hz3/src/hz3_epoch.c`:

- `S58 reclaim` の後、`hz3_s60_purge_tick()` を 1 回だけ呼ぶ
- 順序を固定: **S58 reclaim → S60 purge**（これがSSOT）

---

## 6) Fail-Fast（安全）

### 6.1 my_shard 制約

- S58 reclaim と同様、`my_shard` の central から pop して segment に戻す前提。
- したがって S60 も “my_shard 文脈で queue された範囲のみ” を扱う。

### 6.2 shard collision（任意Fail-Fast）

shard oversubscription（複数 thread が同一 shard を共有）時は `hz3_segment_free_run()` 自体がデータ競合になり得る。
S60 は研究箱なので、必要なら opt-in の fail-fast を置く:

- `HZ3_S60_FAILFAST_ON_SHARD_COLLISION=1` のとき、
  - shard collision 検出フラグ（既存）を見て 1 回だけ abort する（事故の早期露出）。

---

## 7) 可視化（one-shot）

atexit で 1 回だけ:

`[HZ3_S60_PURGE] queued=... dropped=... madvise_calls=... madvised_pages=... skipped_arena=... budget_dropped=...`

常時ログは禁止。

---

## 8) A/B（compile-time）

`hakozuna/hz3/include/hz3_config.h` に追加（default OFF）:

- `HZ3_S60_PURGE_RANGE_QUEUE`（0/1）
- `HZ3_S60_BUDGET_PAGES`
- `HZ3_S60_BUDGET_CALLS`
- `HZ3_S60_STATS`
- `HZ3_S60_FAILFAST_ON_SHARD_COLLISION`（optional）

---

## 9) 測定（GO/NO-GO）

### 9.1 SSOT（比較成立条件）

基準（mimalloc “圧縮できるケース”）:
- `bench_burst_idle_v2`（idle=10s, threads=4, keep_ratio=0）
- mimalloc: `MIMALLOC_PURGE_DELAY=0`
  - 台帳: `hakozuna/hz3/docs/PHASE_HZ3_S59_2_IDLE_PURGE_COMPARISON.md`

### 9.2 比較対象

- hz3: `S58=1`（baseline）
- hz3: `S58=1 + S60=1`（treatment）
- mimalloc: `MIMALLOC_PURGE_DELAY=0`（reference）

### 9.3 GO 条件（暫定）

- `HZ3_S60_PURGE` の `madvised_pages` が **S58 reclaim_pages と同程度のオーダ**で出る（coverage）
- idle 後 RSS が **有意に下がる**（まず -50% 以上、最終は mimalloc `PURGE_DELAY=0` に近づく）
- SSOT small/medium/mixed が **±2%以内**

NO-GO:
- `madvised_pages > 0` でも RSS が動かない（instant re-fault 優勢）
- 速度退行 >2%
- 不安定化（クラッシュ/破壊）

---

## 10) 実装順序（作業手順）

1. `hz3_purge_range_queue.[ch]` 追加（queue + madvise + stats）
2. `hz3_config.h` に S60 flags 追加（default OFF）
3. `hz3_tcache_decay.c` の S58 reclaim に `hz3_s60_queue_range()` を接続
4. `hz3_epoch.c` の末尾に `hz3_s60_purge_tick()` を追加（S58後）
5. `bench_burst_idle_v2` + RSS時系列で A/B（S58 vs S58+S60）
6. GO/NO-GO 判定 → keep/archive 決定

---

## 11) Next（S61 へ）

S60 は `madvise` 発火の配線としては成立したが、S59-2 の “idle で RSS が落ちる” 目的では NO-GO。
次は境界を変える（idle/exit/collect のいずれかに寄せる）:

- free 波の後（idle 窓）で purge を撃つ（thread-exit / explicit collect）
- `madvise` 面積を増やすには **S58 reclaim の面積（central→segment 返却量）**を増やす必要がある

次の指示書:
- `hakozuna/hz3/docs/PHASE_HZ3_S61_DTOR_HARD_PURGE_BOX_WORK_ORDER.md`

追記（結果）:
- S61 は GO（idle RSS -76%）まで到達。S60 の “epoch 中 purge” ではなく、thread-exit 境界で効いた。
