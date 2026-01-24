# PHASE_HZ3_S55-2: RetentionPolicyBox（FROZEN）— OpenDebtGate（segment “開き過ぎ”抑制）

目的:
- S55-1 OBSERVE で `arena_used_bytes (= used_slots * HZ3_SEG_SIZE)` が **97–99% を支配**した。
- large cache（S53）ではなく **segment/arena 側の“開き過ぎ / 返し不足”**が主因候補なので、**segment を開く頻度を下げる**。
- hot path 固定費ゼロ（event-only の境界のみ）。
- 既存のエンジン（S49/S47/S45）を壊さず、**制御（いつ強めるか）だけ**を追加する。

注意:
- `arena_used_bytes` は RSS（物理）ではなく **segment/slot 使用量 proxy**。ただし “segment を開きすぎ” を抑える指標としては有効。

---

## 実測（2026-01-06）: RSS目的では NO-GO（ただし“制御箱”としては成立）

結論（短く）:
- **level 遷移（L0→L1→L2）は成立**（MBレンジ watermarks で確認）
- しかし `mstress` の mean RSS は概ね **-0〜-1% 程度**で、目標（-10%）に届かない → **RSS目的では NO-GO**

観測:
- 64GiB arena の比率 watermarks（25%/35%）だと、実 workload（数百MB）が届かず **一度も発火しない**。
  - scale lane ではまず **MB絶対値**（例: 256/384/64MiB）で発火確認をすること。
- S55-2B（epoch repay）を追加しても `mstress` では `gen_delta==0` が継続し、返却が進まない（full-free segment が作れない）。

示唆:
- “開く頻度を下げる（Admission Control）”だけでは steady-state の RSS が動かない。
- 次フェーズ（S55-3）は “full-free segment が作れない断片化” を前提に、返却単位を変える（page subrelease / out-of-band freelist など）必要がある可能性が高い。

---

## 0) 前提（現行 scale lane）

- `HZ3_SEG_SIZE=0x100000`（1MiB / `HZ3_PAGES_PER_SEG=256`）
- `HZ3_ARENA_SIZE=0x1000000000ULL`（64GiB）
- `HZ3_NUM_SHARDS=32`
- S49（contig-aware packing）、S47（quarantine/avoid）、S45（mem_budget）は既に存在。

---

## 1) 仕組み（最小・エレガント）

### 結論: Admission Control + “Open Debt（借金）”

- **Open Debt**: 「最近どれだけ segment を“開いたか”」を shard ごとに持つ。
  - 新規 segment を開いたら `debt++`
  - segment を返せたら `debt--`（下限0）
- **Watermark**: `arena_used_slots`（or bytes）でレベル判定（0/1/2）。
  - L0: 通常
  - L1（SOFT）: “開き過ぎ抑制”ON（S49 tries を増やす）
  - L2（HARD）: **開く直前に 1回だけ返済**（S47/S45 を budget 付きで 1回だけ踏む）

重要:
- “禁止”ではなく “優先順位を変更” するだけ。最後は必ず開く（alloc failed を増やさない）。

---

## 2) 境界（Box Theory: 1箇所 + 1箇所）

### 境界A（定期/軽い）: `hz3_epoch_force()` の中

役割:
- `arena_used_slots` を読む
- watermark 判定して `level` を更新（O(1)）
- パラメータは **書き換えるだけ**（ここで重い reclaim をしない）

### 境界B（開く直前/重いのはここだけ）: `hz3_slow_alloc_from_segment()` の “新規 segment を開く直前”

役割:
- S49 を “もう少し強く” 試す（tries boost）
- HARD なら **S47/S45 を最大 1回だけ** 試す（返済）
- それでもダメなら開く（`hz3_new_segment()`）
- 開いたら `debt++`

---

## 3) 実装方針（差分を小さく）

### 3.1 新しい箱（推奨）

新規:
- `hakozuna/hz3/include/hz3_retention_policy.h`
- `hakozuna/hz3/src/hz3_retention_policy.c`

API（最小）:
- `hz3_retention_tick_epoch()`（epoch境界で level 更新）
- `hz3_retention_level_get()`（L0/L1/L2 を返す）
- `hz3_retention_debt_on_open(owner)` / `hz3_retention_debt_on_free(owner)`（debt更新）
- `hz3_retention_before_open(owner, sc, pages)`（HARD時の “1回だけ返済” をやる/やらない判定）

### 3.2 使う既存エンジン（壊さない）

- “再利用”側: S49（`hz3_pack_try_alloc()`、`tries` を可変）
- “返却”側: S47（`hz3_s47_compact_hard(owner)`）
- “保険/補助”: S45（`hz3_mem_budget_reclaim_medium_runs()` の小 budget 版があるなら 1回）

---

## 4) パラメータ（FROZEN 初期値案）

### Watermark（比率 or 絶対値）

初期は比率でもよいが、scale lane は `HZ3_ARENA_SIZE` が大きいので **絶対値（MB）指定を推奨**。
（比率のままだと、実 workload が数百MBの場合に level が一度も上がらない）

既定（比率）:
- `HZ3_S55_WM_SOFT_BYTES = HZ3_ARENA_SIZE / 4`（25%）
- `HZ3_S55_WM_HARD_BYTES = HZ3_ARENA_SIZE * 35 / 100`（35%）
- `HZ3_S55_WM_HYST_BYTES = HZ3_ARENA_SIZE / 50`（2%）

scale lane 推奨（例）:
- `HZ3_S55_WM_SOFT_BYTES = 268435456ULL`（256MiB）
- `HZ3_S55_WM_HARD_BYTES = 402653184ULL`（384MiB）
- `HZ3_S55_WM_HYST_BYTES = 67108864ULL`（64MiB）

注意（Makefile経由の `-D`）:
- `-DNAME=(256ULL<<20)` のように `()` や `<<` を含む値は、shell/make の都合で壊れる場合がある。
- まずは上のように **整数バイト値**（`...ULL`）を推奨。

実装は bytes でも slots でも可（slots の方が安い）:
- `used_slots = hz3_arena_used_slots()`
- `used_bytes = used_slots * HZ3_SEG_SIZE`

### Pack tries boost（開く直前だけ）

- L0: `tries=HZ3_S49_PACK_TRIES_SOFT`（現行のまま）
- L1: `tries=HZ3_S55_PACK_TRIES_L1`（default 16）
- L2: `tries=HZ3_S55_PACK_TRIES_L2`（default 64）

### Debt limit（目安）

- L1: `HZ3_S55_DEBT_LIMIT_L1`（default 8）
- L2: `HZ3_S55_DEBT_LIMIT_L2`（default 2）

### 返済（repay）回数

- L2 の “開く直前” で **最大 1回だけ**
- 進捗が出た（`freed_segments>0` / `arena_free_gen` が進んだ）なら即終了
- 進捗ゼロならその alloc は諦めて開く（暴走防止）

---

## 5) ログ/可視化（one-shot）

追加するなら最小:
- `[HZ3_S55_FROZEN] level=... used_slots=... debt=...`（level遷移時に 1回だけ）
- `[HZ3_S55_FROZEN] repay=1 freed_segments=...`（HARDで返済を踏んだ時に 1回だけ）

常時ログは禁止。SSOTはこの 1行だけ。

---

## 6) 実装箇所（具体）

### 6.1 epoch 側

- `hakozuna/hz3/src/hz3_epoch.c`
  - `hz3_epoch_force()` の末尾付近（knobs/learn の後でも可）で `hz3_retention_tick_epoch()` を呼ぶ。

### 6.2 segment open 直前

- `hakozuna/hz3/src/hz3_tcache.c`
  - `hz3_slow_alloc_from_segment(int sc)` の
    - `if (!meta || meta->free_pages < pages) { ... hz3_new_segment(...) ... }`
    の直前に “tries boost / repay” を差し込む（event-only）。
  - ここは **hot ではない**が頻度は高いので、重い操作は **HARD時だけ最大1回**に制限する。

### 6.3 debt 更新

- `hakozuna/hz3/src/hz3_segment.c`
  - `hz3_new_segment(owner)` の成功後に `hz3_retention_debt_on_open(owner)` を呼ぶ（統一境界）。
- segment が返された瞬間（例: `hz3_segment_free()` / `hz3_arena_free()` の直後）に
  - `hz3_retention_debt_on_free(owner)` を呼ぶ（owner は meta->owner から取得）。

---

## 7) A/B（compile-timeのみ）

最低限:
- `HZ3_S55_RETENTION_FROZEN=0/1`（default 0）

切り分け用（必要なら）:
- `HZ3_S55_OPEN_DEBT_GATE=0/1`
- `HZ3_S55_PACK_BOOST=0/1`
- `HZ3_S55_HARD_REPAY=0/1`

ビルド例:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S55_RETENTION_FROZEN=1'
```

---

## 8) 検証（GO/NO-GO）

### 観測
- S55-1 OBSERVE を一緒に有効化して `arena_used_bytes` の上限/傾向が落ちるか確認。

### ベンチ
- `mstress`（mean RSS を tcmalloc に寄せる）
- `bench_random_mixed_mt_remote_malloc`（T=32, R=50）
- SSOT（small/medium/mixed）

推奨スクリプト:
- `hakozuna/hz3/scripts/run_bench_hz3_rss_compare.sh`
- `hakozuna/hz3/scripts/measure_mem_timeseries.sh`

GO（まずの目標）:
- `mstress` mean RSS が -10% 以上（vs hz3 baseline）、速度 -2% 以内。
- SSOT ±2% 以内。
- alloc_failed は増やさない（増えるなら即OFF）。

---

## 9) 他 allocator との違い（なぜこの箱が要る？）

- 多くの allocator（tcmalloc/jemalloc/mimalloc）は、span/segment の “開き過ぎ” を抑えるために
  - decay / subrelease（時間/カウンタで少しずつ返す）
  - active/retired span 管理（新規を開く前に既存から取る）
  - page heap の再利用優先（admission control）
  を持つ。
- hz3 は PTAG / arena 前提で高速だが、現状 “開き過ぎ制御” が薄いため、`used_slots` が増えやすい。
  → S55-2 は hot を汚さずにその差分を埋めるための **制御箱**。
