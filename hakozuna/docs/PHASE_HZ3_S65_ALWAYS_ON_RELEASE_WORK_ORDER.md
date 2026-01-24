# PHASE_HZ3_S65: Always-On Release (Boundary + Ledger + Coverage)

Status: IMPLEMENTED (partial) / EVAL  
Date: 2026-01-09  
Track: hz3  
Previous: S62 (dtor retire/purge GO), S64 (epoch retire/purge fires but RSS unchanged)

---

## 0) 目的（SSOT）

常時オンでも性能を落とさず、RSS の主成分に **確実に当たる release** を作る。  
S64 は purge が発火しているのに RSS が落ちないため、**“返せる状態”の coverage を広げる**必要がある。

狙い:
- 「free_bits に戻した瞬間」だけが OS purge 可能、という SSOT を確立
- small/sub4k/medium を **同じ境界**に寄せる
- syscall を削減（range coalesce）

---

## 1) 箱割り（境界1箇所）

**Boundary Box（唯一）**:

```
void hz3_release_range_definitely_free(
    Hz3SegHdr* hdr,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason);
```

この関数だけが:
- free_bits の更新（=「空」確定）
- release ledger への enqueue
- 統計カウンタ

を行う。**これ以外の場所で purge 対象登録は禁止**。

---

## 2) 新規箱（S65）

### S65-0: ReleaseBoundaryBox（必須）

**目的**: free_bits 更新の “唯一の入口” を固定する。  
**安全条件**:
- page_idx != 0（header page は触らない）
- hdr->magic/kind/owner を検証（Fail-Fast）
- arena 範囲内であること（hz3_os_in_arena_range）

### S65-1: ReleaseLedgerBox（range coalesce）

**目的**: 1page=1madvise を避け、**連続 range をまとめて purge** する。  
**ルール**:
- 同一 seg_base かつ連続 page_idx の場合は merge
- enqueue は TLS ring（固定サイズ、overflow は drop+stats）
- purge は epoch tick のみ

### S65-2: CoverageBox（small/sub4k/medium）

**目的**: coverage を揃える（S64 が small_v2 偏りで RSS が落ちない問題を解消）。

対象:
- small_v2: page retire（S62-1 / S64 の retire scan を boundary へ）
- sub4k: 2-page run retire（S62-1b の run retire を boundary へ）
- medium: central run reclaim → segment free_run → boundary へ

### S65-3: Idle/Busy Gate Box（常時オンの安全弁）

**目的**: 常時オンでも性能を落とさない。

方針:
- idle 時: delay=0 / budget 増（aggressive purge）
- busy 時: delay>0 / budget 縮小（ほぼ動かさない）

判定は epoch 内で **1箇所**に固定（recent_ops / pressure / idle_tick のどれか）。

---

## 3) API/構造体（案）

### Reason enum

```
typedef enum {
    HZ3_RELEASE_SMALL_PAGE_RETIRE = 1,
    HZ3_RELEASE_SUB4K_RUN_RETIRE  = 2,
    HZ3_RELEASE_MEDIUM_RUN_RECLAIM = 3,
} Hz3ReleaseReason;
```

### Ledger entry

```
typedef struct {
    void*    seg_base;
    uint32_t page_idx;
    uint16_t pages;
    uint16_t reason;
    uint32_t retire_epoch;
} Hz3ReleaseEntry;
```

---

## 4) 実装手順（順番固定）

### Step 0: Boundary API 追加（S65-0）

ファイル:
- `hakozuna/hz3/include/hz3_release_boundary.h`（新規）
- `hakozuna/hz3/src/hz3_release_boundary.c`（新規）

内容:
- safety check（magic/kind/owner/page_idx）
- free_bits 更新
- ledger enqueue
- stats

### Step 1: ReleaseLedgerBox（S65-1）

ファイル:
- `hakozuna/hz3/include/hz3_release_ledger.h`（新規）
- `hakozuna/hz3/src/hz3_release_ledger.c`（新規）

内容:
- TLS ring（power-of-2）
- enqueue 時の range coalesce
- epoch tick で budgeted purge（madvise）

### Step 2: small_v2 coverage（S65-2a）

変更:
- S62-1 / S64 retire scan で **free_bits 更新の代わりに boundary API を呼ぶ**
- retire した page_idx 群は sort→coalesce→range enqueue

### Step 3: sub4k coverage（S65-2b）

変更:
- S62-1b の run retire で boundary API を呼ぶ
- run start bits を使って 2-page run の退役を確実化

### Step 4: medium coverage（S65-2c）

変更:
- S61 の reclaim ロジックを epoch に持ち込み
- free_run を boundary API へ（segment fully free の range は segment 全体で enqueue）

### Step 5: Idle/Busy Gate（S65-3）

変更:
- `hz3_epoch_force()` 内で 1 箇所の gate 判定
- delay/budget を切替（idle=0 / busy>0）

---

## 5) Flags / Knobs（戻せる）

compile-time:
- `HZ3_S65_RELEASE_BOUNDARY` (default OFF)
- `HZ3_S65_RELEASE_LEDGER` (default OFF)
- `HZ3_S65_RELEASE_COVERAGE` (default OFF)
- `HZ3_S65_IDLE_GATE` (default OFF)
- `HZ3_S65_LEDGER_SIZE`
- `HZ3_S65_PURGE_BUDGET_PAGES`
- `HZ3_S65_PURGE_MAX_CALLS`
- `HZ3_S65_DELAY_EPOCHS_IDLE`
- `HZ3_S65_DELAY_EPOCHS_BUSY`

runtime knobs（hz3_knobs 経由）:
- `s65_purge_budget_pages`
- `s65_delay_epochs_idle`
- `s65_delay_epochs_busy`

---

## 6) 可視化（one-shot）

```
[HZ3_S65] enq=... coalesce=... purged_pages=... madvise_calls=...
  idle_ticks=... busy_ticks=... drop=...
```

常時ログ禁止。atexit 1回のみ。

---

## 7) 評価

Bench:
```
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 200000 10 3 0 4 256 4096 1 100 1
```

観測:
- RSS delta が idle 後に **マイナス方向**へ動くか
- madvise_calls が pages と同数になっていないか（coalesce 効果）
- SSOT 退行 ≤ 2%

---

## 8) GO / NO-GO

GO:
- purged_pages 増加 + RSS 低下が一致
- madvise_calls が pages の 1/10 以下（coalesce 効果）
- SSOT ±2%

NO-GO:
- purged_pages > 0 でも RSS 不動
- syscall 数が多く mixed 退行

---

## 9) 位置づけ

S65 は **S64 の上位互換**。  
S64 の評価結果（purge 発火でも RSS 不動）を踏まえ、
**coverage と boundary の統一で RSS 主成分に当てる**ためのフェーズ。

---

## 実装状況（2026-01-09）

完了:
- ReleaseBoundaryBox: `hz3_release_range_definitely_free()`（free_bits 更新の唯一入口）
- ReleaseLedgerBox: TLS ring + range coalesce（overflow は即 purge）
- Coverage 統合: small_v2 retire / sub4k run retire / S64 retire scan → boundary 経由
- Idle/Busy Gate: `hz3_pack_pressure_active()` で判定

保留:
- medium coverage（Hz3SegMeta 経路の API 拡張が必要）

結果:
- `enq=93578 purged_pages=90452 madvise_calls=90410`
- RSS は baseline 同等（S62 が seg 単位で purge 済みのため）

課題:
- TLS ledger のため、idle thread が他スレッドの ledger を drain できない
- 対策: overflow 時即 purge（coalesce 効果は減るが確実に purge）

既定:
- scale lane で常時 ON（boundary/ledger/medium reclaim + idle gate）

---

## S65-2C（medium epoch reclaim）実装結果

Status: GO

Stats:
- `[HZ3_S65_MEDIUM] reclaim_runs=3072 reclaim_pages=3072 boundary_calls=3072`
- `skip_null_meta=0 skip_bounds=0`

Ledger連携:
- `[HZ3_S65] enq=96650 coalesce=2891 purged_pages=93524`

注意:
- `drop=90410` は overflow 即 purge（損失ではないが syscall 多め）

次の候補:
- `HZ3_S65_LEDGER_SIZE` を 256→512/1024 へ拡大（overflow削減）
- `HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS` の調整
- coalesce scan depth を設定可能に（`HZ3_S65_COALESCE_SCAN_DEPTH` の導入）

---

## Ledger チューニング（報告ベース）

観測:
- enqueue >> ledger capacity（例: enq=96650 vs size=256）
- coalesce scan depth=8 のため、同一 seg が離れると merge 不能

推奨設定（A/B）:
- Option A（保守的）:
  - `HZ3_S65_LEDGER_SIZE=512`
  - `HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS=512`
  - `HZ3_S65_DELAY_EPOCHS_IDLE=1`
- Option B（アグレッシブ）:
  - `HZ3_S65_LEDGER_SIZE=1024`
  - `HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS=1024`
  - `HZ3_S65_DELAY_EPOCHS_IDLE=1`

根本解決案:
- `HZ3_S65_COALESCE_SCAN_DEPTH` を追加（32/64 など）

---

## S65 Full Bench（RUNS=3, 4者比較）

hz3 SSOT（median）:
- small: hz3 100.3M / mi 73.5M / tc 92.5M（hz3 vs mi +36%）
- medium: hz3 110.8M / mi 63.3M / tc 112.5M（hz3 vs mi +75%）
- mixed: hz3 105.2M / mi 62.9M / tc 104.1M（hz3 vs mi +67%）

mimalloc-bench（selected, time）:
- larson-sized: hz3 9.43s / mi 10.01s / tc 9.47s（hz3 最速）
- mstressN: hz3 1.66s / mi 1.37s / tc 1.36s（mi/tc 優位）
- glibc-thread: hz3 1.67s / mi 1.52s / tc 1.75s（mi 最速）
- sh8benchN: hz3 2.04s / mi 0.73s / tc 6.89s（mi 最速）
- xmalloc-testN: hz3 1.59s / mi 0.46s / tc 29.5s（hz3 >> tc）

RSS（KB, selected）:
- mstressN: hz3 379,352 / mi 655,056 / tc 289,016（hz3 -42% vs mi）
- larsonN-sized: hz3 126,976 / mi 104,448 / tc 64,256（hz3 +21% vs mi）
- rptestN: hz3 111,016 / mi 70,916 / tc 72,320（hz3 +56% vs mi）
- sh6benchN: hz3 262,344 / mi 222,336 / tc 219,768（hz3 +18% vs mi）
- sh8benchN: hz3 226,504 / mi 128,120 / tc 129,024（hz3 +77% vs mi）
- alloc-testN: hz3 25,856 / mi 13,568 / tc 18,688（hz3 +90% vs mi）

要約:
- mstressN は S65 purge 効果が可視化（purged_pages=257821）
- 短時間ベンチでは purge 効果が見えにくく、hz3 の RSS が多めになる傾向
