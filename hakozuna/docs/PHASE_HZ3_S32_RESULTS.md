# S32: TLS init check / dst compare 削除 - 結果

Status: **S32-1 GO, S32-2 NO-GO**

Date: 2026-01-02

## 背景

S31 perf で dist=app gap (-7.69%) の主因が確定:
1. `hz3_malloc` の TLS init check: malloc overhead の ~20%
2. `hz3_free` の `dst == my_shard` compare: free overhead の ~8%

## S32-1: TLS init check を miss/slow 入口に押し込む

### 実装

- Flag: `HZ3_TCACHE_INIT_ON_MISS=1`
- malloc hot hit では `hz3_tcache_ensure_init()` を呼ばない
- slow path (`hz3_alloc_slow()`, `hz3_small_v2_alloc_slow()`) の入口で init
- free は init check を残す（S32-2 の安全のため）

### 結果 (RUNS=30, dist=app)

| 構成 | Median | vs Baseline |
|------|--------|-------------|
| Baseline | 43.55M | - |
| S32-1 | 44.66M | **+2.55%** |

**判定: GO**

---

## S32-2: dst compare を消す (row_off 方式)

注記:
- S32-2 は NO-GO のため、`HZ3_LOCAL_BINS_SPLIT_ROW_OFF` を含む実装は本線から撤去済み。
- 参照: `hakozuna/hz3/archive/research/s32_2_row_off/README.md`

### 実装

- Flag: `HZ3_LOCAL_BINS_SPLIT_ROW_OFF=1`
- `Hz3TCache` に `local_row[HZ3_BIN_TOTAL]` と `row_off[HZ3_NUM_SHARDS]` を追加
- init 時に row_off を設定:
  - `row_off[i] = offsetof(Hz3TCache, bank[i])` (全dst)
  - `row_off[my_shard] = offsetof(Hz3TCache, local_row)` (patch)
- free hot path: `Hz3Bin* row = (Hz3Bin*)((char*)&t_hz3_cache + row_off[dst]); hz3_bin_push(row + bin, ptr);`

### 結果 (RUNS=30, dist=app)

| 構成 | Median | vs Baseline |
|------|--------|-------------|
| Baseline | 43.55M | - |
| S32-1+2 | 42.47M | **-2.48%** |

**判定: NO-GO**

### 分析

row_off 方式は以下の理由で退行:

1. **分岐予測の効率**: dist=app では 80% が local free
   - `dst == my_shard` の分岐は分岐予測でほぼ完璧に当たる
   - 比較命令自体のコストはほぼゼロ

2. **row_off のオーバーヘッド**:
   - `row_off[dst]` のテーブル lookup (メモリ読み込み)
   - ポインタ演算 `(char*)&t_hz3_cache + offset`
   - これらが分岐予測の利点を上回るコスト

3. **結論**: 分岐予測が効く状況では、分岐を消しても得にならない

---

## 最終結論

| 項目 | 状態 | 効果 |
|------|------|------|
| S32-1 (TCACHE_INIT_ON_MISS) | **GO** | +2.55% |
| S32-2 (LOCAL_BINS_SPLIT_ROW_OFF) | **NO-GO** | -2.48% |

S32-1 を有効化することで、dist=app で +2.55% の改善が得られる。

---

## 変更ファイル

### S32-1 (有効化推奨)

- `hz3/include/hz3_config.h`: `HZ3_TCACHE_INIT_ON_MISS` フラグ追加
- `hz3/include/hz3_small_v2.h`: malloc側 ensure_init をガード
- `hz3/src/hz3_hot.c`: malloc側 ensure_init をガード
- `hz3/src/hz3_tcache.c`: slow path 入口に ensure_init_slow 追加
- `hz3/src/hz3_small_v2.c`: slow path 入口に ensure_init_slow 追加

### S32-2 (デフォルトOFF)

- この案は NO-GO のため、実装は本線から撤去し研究箱へ移した:
  - `hakozuna/hz3/archive/research/s32_2_row_off/README.md`

---

## ベンチログ

- Baseline: `/tmp/baseline_results.txt`
- S32-1: `/tmp/s32_1_results.txt`
- S32-1+2: 統計のみ保存

## 参照

- Work order: `hakozuna/hz3/docs/PHASE_HZ3_S32_TLS_INIT_AND_DST_ROW_OFF_WORK_ORDER.md`
- S31 perf 結果: `hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md`
