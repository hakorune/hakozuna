# PHASE_HZ3_S55-3B: DelayedMediumSubreleaseBox（purge delay / RSS効率改善）

背景:
- S55-3（即時 subrelease）は **madvise_bytes が大きくても RSS があまり落ちない**（例: 783MB返却→RSS -57MB 程度）。
- 原因は “返した medium run がすぐ再利用される” ことで、DONTNEED→page fault の往復になり、RSS効率が悪い。

結論:
- **即時に捨てない**。freeになった run を “倉庫（候補キュー）” に積み、**一定 delay を越えても still-free の run だけ** DONTNEED する。
- hot path は触らず、event-only（epoch境界）のみ。

---

## 0) ゴール（GO/NO-GO）

GO:
- `mstress` mean RSS が **-10% 以上**（vs baseline）
- 速度退行 **-2%以内**（SSOT ±2%）
- `madvise_calls` が過剰に増えない（例: 数万/秒級にならない）

NO-GO:
- RSS が -5% 未満しか動かないのに速度が落ちる
- data corruption / crash

---

## 1) 箱割り（Box Theory）

### S55-3B の責務
- “subrelease すべき候補”を **溜める（queue）**
- “一定 delay 後も still-free”なら **OSに返す（discard）**

### 境界（1箇所）
- `hz3_retention_repay_epoch()` の中だけ（event-only）
  - schedule（候補登録）も drain（実madvise）もここだけ
  - hot path は触らない

---

## 2) データ構造（最小）

### TLS ring（固定長、thread-local）

1エントリは “medium run（4KB..32KB）” を表す:
- `seg_base`（1MiB align）
- `page_idx`（0..255）
- `pages`（1..8）
- `stamp`（epochカウンタ値）

リング長は power-of-two:
- `HZ3_S55_3B_QUEUE_SIZE=256` → 256 entries / thread（TLS）

実装メモ:
- head/tail は **32-bit monotonic counter**（wrapで “full判定が壊れる” のを防ぐ）。

オーバーフロー時:
- “新規を捨てる”か“最古を捨てる”を選ぶ（まずは **新規drop**が簡単）
- dropカウンタを one-shot で出す

---

## 3) アルゴリズム（概要）

### 3.1 schedule（候補登録）

S55-3（既存）の「central_pop→segment_free_run」までは維持し、
`madvise` を **その場では呼ばず**、queueに積む:

1. `obj = hz3_central_pop(my_shard, sc)`
2. `hz3_segment_free_run(meta, page_idx, pages)`
3. `enqueue(seg_base, page_idx, pages, stamp_now)`

ここは “候補を集める”だけ。

### 3.2 drain（遅延後に捨てる）

epoch境界で、queue先頭から `delay_epochs` を満たすものだけ処理:

条件:
- `now_stamp - entry.stamp >= HZ3_S55_3B_DELAY_EPOCHS`

still-free検査（必須）:
- `hz3_segmap_get(seg_base)` が生きている
- `free_bits` が entry 範囲で **全て1**（freeのまま）であること

OKなら:
- `madvise(run_addr, run_bytes, MADV_DONTNEED)`

NGなら（再利用された）:
- madviseせず捨てる（= “すぐ使われるrun”は subrelease しない）

syscall上限:
- 1epochあたり `HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS` を流用（または別ノブ）

---

## 4) フラグ（compile-time）

最低限:
- `HZ3_S55_3_MEDIUM_SUBRELEASE=1`

S55-3B 追加ノブ（案）:
- `HZ3_S55_3B_DELAYED_PURGE=1`（遅延方式ON。OFFなら既存の即時方式）
- `HZ3_S55_3B_DELAY_EPOCHS=8`（最初は 8 推奨）
- `HZ3_S55_3B_QUEUE_SIZE=256`（256 entries/thread）

既存ノブ（併用）:
- `HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES`（schedule側の回収上限）
- `HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS`（drain側のsyscall上限）
- `HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT`（追加間引き）
- `HZ3_S55_3_REQUIRE_GEN_DELTA0=0/1`（S55-3/3B の実行ゲート。default=1）

---

## 5) 可視化（one-shot）

atexit で 1回だけ:

`[HZ3_S55_3B_DELAY] schedule_calls=... scheduled_pages=... dropped=... drained_calls=... madvise_calls=... madvise_bytes=... reused_skips=...`

“RSSが落ちない”時に、原因（delayが短い/長い、再利用率が高い、queueが溢れている）を即判定できる形にする。

---

## 6) 測定（推奨）

1) 発火確認:
- `HZ3_S55_REPAY_EPOCH_INTERVAL=1`
- `HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT=1`
- `HZ3_S55_3B_DELAY_EPOCHS=8`

2) RSS:
- `measure_mem_timeseries.sh` で OFF/ON 比較（mean/p95/max）

3) 速度:
- SSOT（small/medium/mixed） ±2% を確認

---

## 7) チューニング順序（最小）

1. `DELAY_EPOCHS` を上げる（8→16）
  - “すぐ使われるrun”が多い場合、上げると効率が上がる
2. `BUDGET_PAGES` を上げる（256→512）
3. `MAX_CALLS` を下げる（32→16）
  - syscallが支配になったら抑える

---

## 8) 現状（観測）

S55-3B の “hot run skip” 検出が動作している例:

- `enqueued=48789`
- `processed=19933`
- `reused=12229`（reused rate 61.3%）
- `madvised=7704`（38.7%）

→ `reused>0` なので “delay後に再利用されたrunを捨てる” が成立している（箱の中身は正しい）。
RSSが動かない場合は、実行ゲート（`HZ3_S55_3_REQUIRE_GEN_DELTA0` / 水位）や `DELAY_EPOCHS` の再調整が主因になりやすい。

---

## 9) Step2結果（2026-01-06, mstress/larson 系）

結論:
- **NO-GO（steady-state RSS目的）**。
  - constant high-load（`mstress` / `larson`）では RSS が下がらない、または増えるケースがあり、目標（mean RSS -10%）に届かない。
  - “burst→idle（アイドル窓）” があるワークロードでのみ再評価の余地あり。

成立したこと（技術）:
- `reused > 0` が出ており、**hot run 検出（遅延後に再利用された run を捨てる）**自体は成立している。

Step2で潰した根本バグ（必須fix）:
- TLS ring の head/tail が 16-bit wrap して full/age 判定が壊れる問題
  - **head/tail を 32-bit monotonic counter に変更**（wrap前提を排除）
- TLS epoch による “time axis split”（各スレッドの stamp/age が噛み合わない）
  - **stamp/age を global epoch に統一**（delay の基準を共有）

次の判断:
- 省メモリ（RSS）目的の本線としてはここで打ち切り（NO-GO）。
- 研究として残す場合は “burst→idle” を評価できる **同一プロセス長寿命ハーネス**（sleep区間あり）を先に用意し、そこでのみGO/NO-GO判定する。
