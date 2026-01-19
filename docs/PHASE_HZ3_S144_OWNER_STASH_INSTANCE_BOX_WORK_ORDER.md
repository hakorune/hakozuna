# S144: OwnerStash InstanceBox (N-way Partitioning) - NO-GO

**Date**: 2026-01-17
**Status**: NO-GO (既定OFF、opt-in専用フラグとして保持)
**Flag**: `HZ3_OWNER_STASH_INSTANCES` (default=1, power-of-2: 1/2/4/8/16)

---

## 目的

OwnerStash の push path における CAS 競合を削減し、MT remote ワークロード（T≥8, R=50%〜90%）の throughput を向上させる。

### Bottleneck (perf 測定, T=8 R=50%)

| Function | Overhead | Hot Spot |
|----------|----------|----------|
| `hz3_owner_stash_push_one` | 9.30% | 98.09% in CAS retry loop |
| `hz3_owner_stash_pop_batch` | 12.11% | 78.43% in linked list walk |
| Cache miss rate | 51.25% | - |

**根本原因**: 複数スレッドが同一の `[owner][sc]` bin に push → CAS 競合

---

## 実装概要

### N-way Partitioning 戦略

- **Push**: Hash-based instance selection (load balancing)
  - `hash(seed, owner, sc) & (INSTANCES-1)` で bin を選択
  - 各スレッドは `owner_stash_seed`（unique per thread）を使用
- **Pop**: Round-robin instance drain (fairness)
  - 1 instance のみを pop（overhead 最小化）
  - `owner_stash_rr` cursor で次回は別 instance を試行
- **Power-of-2 制限**: Fast `& mask` selection
- **Backward compatible**: INSTANCES=1 で zero overhead

### 主要変更

1. **Flag definition** (`hz3_config_small_v2.h`):
   - `HZ3_OWNER_STASH_INSTANCES` (1/2/4/8/16、power-of-2)
   - Memory footprint check (≤16 で <2.5MB)
2. **TLS fields** (`hz3_types.h`):
   - `owner_stash_seed` (push hash用)
   - `owner_stash_rr` (pop round-robin cursor)
3. **Global array** (`hz3_owner_stash_globals.inc`):
   - 2D→3D array `[owner][sc][inst]`
   - selector functions: `hz3_s144_get_bin_push/pop`
4. **Push path** (`hz3_owner_stash_push.inc`):
   - hash-based bin selection
5. **Pop path** (`hz3_owner_stash_pop.inc`):
   - round-robin 1-instance-only (fair over time)
6. **TLS init** (`hz3_tcache.c`):
   - `owner_stash_seed = start` (unique thread counter)
   - `owner_stash_rr = 0`
7. **Auxiliary APIs** (`hz3_owner_stash_stats.inc`, `hz3_owner_stash_flush.inc`):
   - `count()` / `has_pending()` の instance 集約

---

## A/B 条件

### Build Variants

```bash
# Baseline (INSTANCES=1, default)
make -C hakozuna/hz3 clean all_ldpreload_scale

# INST=2
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_OWNER_STASH_INSTANCES=2'

# INST=4/8 も同様
```

### Benchmark Suite (RUNS=10)

1. **R=0 Regression Check** (single-thread baseline)
   - `bench_random_mixed_mt_remote 1 2500000 400 16 2048 0`
   - INST=1/2/4/8 で median 比較

2. **MT Remote Performance** (multi-thread high contention)
   - T=8/16/32, R=50%/90%
   - `bench_random_mixed_mt_remote <T> 2500000 400 16 2048 <R>`

3. **SSOT Small Bench** (real-world single-thread)
   - dist (app-like distribution)
   - args 16-2048 (uniform small sizes)

---

## 結果 (RUNS=10 Final Decision)

### 1. R=0 Regression Check

| INST | Median (M ops/s) | Delta | Status |
|------|------------------|-------|--------|
| 1 | 408.7 | baseline | baseline |
| 2 | 459.1 | **+12.4%** | ✅ PASS |
| 4 | 421.6 | +3.2% | ✅ PASS |
| 8 | 418.6 | +2.4% | ✅ PASS |

**所見**: 全 INST が R=0 で**改善**（RUNS=3 の退行はノイズだった）

---

### 2. MT Remote Performance (INST=2)

#### Summary

| INST | Wins/Total | Win Rate | Status (≥5/6) |
|------|------------|----------|---------------|
| 2 | 3/6 | 50% | ❌ FAIL (3/6) |
| 4 | 2/6 | 33% | ❌ FAIL (2/6) |
| 8 | 0/6 | 0% | ❌ FAIL (0/6) |

#### Detailed Results (INST=2 vs INST=1)

| Condition | INST=1 (M ops/s) | INST=2 (M ops/s) | Delta | Winner |
|-----------|------------------|------------------|-------|--------|
| T=8 R=50% | 169.9 | 194.4 | +14.4% | **INST=2** |
| T=16 R=50% | 217.6 | 239.6 | +10.1% | **INST=2** |
| T=32 R=50% | 256.1 | 277.9 | +8.5% | **INST=2** |
| T=8 R=90% | 166.4 | 149.1 | **-10.4%** | INST=1 |
| T=16 R=90% | 190.9 | 156.9 | **-17.8%** | INST=1 |
| T=32 R=90% | 184.3 | 171.4 | **-7.0%** | INST=1 |

**所見**:
- **R=50%**: INST=2 が全勝（+8.5%〜+14.4%）
- **R=90%**: INST=2 が全敗（-7.0%〜-17.8%）
- **Win rate 50%**: ≥5/6 基準（83%）に未達 → **FAIL**

---

### 3. SSOT Small Bench (INST=2 vs INST=1)

| Bench | INST=1 (M ops/s) | INST=2 (M ops/s) | Delta | Status (±2%) |
|-------|------------------|------------------|-------|--------------|
| dist | 135.8 | 138.5 | +1.9% | ✅ PASS |
| args 16-2048 | 148.2 | 144.0 | **-2.9%** | ❌ FAIL |

**所見**: args bench で一貫して -2.9%〜-3.2% 退行（RUNS=5/10 で再現）

---

## 判定

### 基準 (Revised)

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| R=0 regression | No regression | Improvement is positive |
| MT Remote wins | **≥5/6 wins** | Strong majority |
| SSOT delta | ±2% | Real-world noise tolerance |

### INST=2 Final Results

| Criterion | Result | Status |
|-----------|--------|--------|
| R=0 (no regression) | +12.4% | ✅ PASS |
| MT Remote (≥5/6 wins) | **3/6 wins** | ❌ **FAIL** |
| SSOT (±2%) | 1/2 | ⚠️ PARTIAL |

### 判定: ❌ **NO-GO**

**理由**:

1. **MT Remote win rate 50%**: Critical criterion (≥5/6) failed
2. **R=90% 全敗**: 高 remote 比率で一貫した退行（-7%〜-18%）
3. **SSOT args -2.9%**: Single-thread small alloc 退行
4. **Workload sensitivity**: R=50% では勝つが R=90% で負ける（trade-off が大きい）

---

## CPU Architecture Dependency (重要発見)

### Test Environment

- **CPU**: AMD Ryzen 9 9950X
  - 16 physical cores / 32 logical cores
  - **2 CCDs** (Chiplet Dies)
  - Each CCD: 8 cores + L3 cache domain

### Hypothesis

**INST=2 が INST=4 より速い理由**:
- INST=2 → 各 CCD に 1 instance = **完全分離** (no inter-CCD contention)
- INST=4 → 各 CCD で 2 instances = **CCD内競合残る**

**示唆**:
- INST の最適値は **CCD数/NUMA構成に依存**
- 2 CCDs 環境では INST=2 が最適
- 他の CPU (1 CCD, 4 CCDs, NUMA nodes) では最適値が異なる可能性

**結論**: **Runtime configurable** にする価値がある（環境変数 `HZ3_OWNER_STASH_INSTANCES`）

---

## Opt-in Usage (研究箱扱い)

S144 は既定 OFF だが、**特定ワークロード向けに opt-in 可能**:

### Build Example

```bash
# R=50% MT ワークロード向け (INST=2)
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_OWNER_STASH_INSTANCES=2'
cp ./hakozuna/hz3/libhakozuna_hz3_scale.so /tmp/hz3_s144_inst2.so

# 使用
LD_PRELOAD=/tmp/hz3_s144_inst2.so ./your_app
```

### 推奨条件

| Condition | Recommendation |
|-----------|----------------|
| Remote ratio | R=50% 前後（R=90% は避ける） |
| Thread count | T≥8 |
| CPU topology | CCD/NUMA 数に合わせて INST 調整 |
| Workload | MT remote dominant（args bench は諦める） |

### 注意事項

- ❌ **R=90% では使用禁止**（大幅退行）
- ❌ **Single-thread ワークロード**には不向き（-2.9% 退行）
- ⚠️ **Memory footprint**: INST=16 で 2.0MB 増加

---

## Memory Footprint

```
Hz3OwnerStashBin = 16 bytes (_Atomic ptr + _Atomic u32 + padding)
- INST=1:  63 shards × 128 SC × 1  inst × 16 = 129 KB
- INST=2:  63 shards × 128 SC × 2  inst × 16 = 258 KB
- INST=4:  63 shards × 128 SC × 4  inst × 16 = 516 KB
- INST=8:  63 shards × 128 SC × 8  inst × 16 = 1.0 MB
- INST=16: 63 shards × 128 SC × 16 inst × 16 = 2.0 MB
```

Limit ≤16 で BSS < 2.5 MB（server workloads で許容範囲）

---

## SSOT ログ

### RUNS=10 測定結果

- **R=0 regression**: `/tmp/s144_ab_full/regression_runs10.txt`
- **MT remote**: `/tmp/s144_ab_full/mt_remote_runs10.txt` (240 iterations)
- **SSOT small**: `/tmp/s144_ab_full/ssot_small_runs10.txt`
- **Final decision**: `/tmp/s144_ab_full/FINAL_DECISION_RUNS10.txt`
- **Summary**: `/tmp/s144_ab_full/SUMMARY_SO_FAR.md`

### Analysis Script

- `/tmp/s144_ab_full/final_analysis_all_runs10.py`

---

## 復活条件

S144 を scale lane 既定に採用するには:

1. **MT Remote win rate ≥5/6** を RUNS=10 で達成
2. **R=90% 退行を解決**（または R=90% を諦める lane split）
3. **SSOT args 退行を ±2% 以内**に収める

または:

- **Runtime configurable** にして、ワークロード/CPU に応じてユーザーが選択
- **Lane split**: `all_ldpreload_scale_r50_s144` (INST=2 default) を追加

---

## 関連資料

- Plan file: `/home/tomoaki/.claude/plans/dreamy-jumping-quokka.md`
- NO-GO summary: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- BUILD flags index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- CURRENT_TASK: `CURRENT_TASK.md` (2026-01-17 entry)

---

**END**
