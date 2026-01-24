# S145-A: CentralPop Local Cache Box - INCONCLUSIVE

**Date**: 2026-01-17
**Status**: INCONCLUSIVE (variance 過大で判定不能、既定 OFF で研究箱扱い)
**Flag**: `HZ3_S145_CENTRAL_LOCAL_CACHE` (default=0), `HZ3_S145_CACHE_BATCH` (default=16)

---

## 目的

`hz3_small_v2_alloc_slow()` の central pop path で発生する 77.98% atomic exchange hotspot を削減する。

### Bottleneck (perf annotate, T=8 R=50%)

| Location | Overhead | Instruction |
|----------|----------|-------------|
| `hz3_small_v2_alloc_slow` line c268 | **77.98%** | `xchg %rdx,(%r9)` |
| Total alloc_slow | 6.23% | (self: 5.74%) |

**根本原因**: S142 lock-free central は `atomic_exchange` で毎回 central bin から 1 要素を pop。refill 頻度が低くても（S74 stats: 18 calls/epoch）、alloc_slow 実行中は exchange が支配的。

---

## 実装概要

### TLS Central Cache 戦略

**Refill Hierarchy** (before → after):
```
Before: xfer → owner_stash → central (atomic xchg)
                               ↑ 77.98% hotspot

After:  xfer → owner_stash → [TLS cache] → central (atomic xchg)
                               ↓ hit         ↓ grab batch=16
                              return 1       store 15, return 1
```

**Cache Structure**:
- Per-thread, per-sizeclass cache (128 SCs)
- Simple linked list: `head` pointer + `count` field
- TLS footprint: ~1.25KB (128 × 10 bytes)

### 主要変更

1. **Flag definition** (`hz3_config_small_v2.h`):
   - `HZ3_S145_CENTRAL_LOCAL_CACHE` (default=0)
   - `HZ3_S145_CACHE_BATCH` (default=16)
2. **TLS fields** (`hz3_types.h`):
   - `s145_cache_head[HZ3_SMALL_NUM_SC]` (linked list heads)
   - `s145_cache_count[HZ3_SMALL_NUM_SC]` (item counts)
3. **Cache pop logic** (`hz3_small_v2_central.inc`):
   - Shard collision check FIRST (flush cache if collision)
   - Fast path: TLS cache hit
   - Slow path: batch from central, cache remainder
4. **Refill integration** (`hz3_small_v2_refill.inc`):
   - Replace `hz3_small_v2_central_pop_batch` with `hz3_s145_central_cache_pop`
5. **Thread exit cleanup** (`hz3_tcache.c`):
   - Flush cached items back to central in destructor

### Design Choices

1. **Shard collision check FIRST**: Before cache hit, prevents using stale cache
2. **Collision flushes cache**: Returns cached items to central, then fallback
3. **Uses `hz3_obj_get_next/set_next`**: Consistent with codebase helpers
4. **Always cache remainder**: Even if `got < batch` (fixes memory loss bug)
5. **No eviction**: Cache drains naturally on pop (demand-driven)

---

## A/B 条件

### Build Variants

```bash
# Baseline (S145=OFF)
make -C hakozuna/hz3 clean all_ldpreload_scale
cp hakozuna/hz3/libhakozuna_hz3_scale.so /tmp/s145_off.so

# Treatment (S145=ON, batch=16)
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S145_CENTRAL_LOCAL_CACHE=1'
cp hakozuna/hz3/libhakozuna_hz3_scale.so /tmp/s145_on.so
```

### Benchmark Suite (RUNS=10)

```bash
# R=50%
LD_PRELOAD=/tmp/s145_*.so ./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 50

# R=90%
LD_PRELOAD=/tmp/s145_*.so ./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 90

# R=0%
LD_PRELOAD=/tmp/s145_*.so ./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 0
```

---

## 結果 (RUNS=10)

### Summary

| R | ON median (M ops/s) | OFF median (M ops/s) | Delta | Status |
|---|---------------------|----------------------|-------|--------|
| 0 | 490.5 | 472.0 | **+3.9%** | ✅ PASS |
| 50 | 195.9 | 179.3 | **+9.3%** | ⚠️ HIGH VARIANCE |
| 90 | 137.7 | 171.1 | **-19.5%** | ⚠️ HIGH VARIANCE |

### Raw Data Variance

| R | ON range (M ops/s) | OFF range (M ops/s) | Range ratio |
|---|-------------------|---------------------|-------------|
| 50 | 122 – 236 | 133 – 225 | 1.9x – 1.7x |
| 90 | 87 – 208 | 83 – 226 | 2.4x – 2.7x |

**所見**: R=50%/R=90% とも variance が極端に大きい（1.7x～2.7x）。median 比較は信頼度が低い。

### SSOT Regression Check

| Bench | Result | Status (±2%) |
|-------|--------|--------------|
| args 16-2048 | +0.18% | ✅ PASS |
| dist app | +1.14% | ✅ PASS |
| uniform | -0.55% | ✅ PASS |

**所見**: SSOT は全て ±2% 以内で PASS。

---

## 判定

### 基準

| Criterion | Threshold | Result | Status |
|-----------|-----------|--------|--------|
| R=0 regression | ±2% | **+3.9%** | ✅ PASS |
| R=50% improvement | >0% | +9.3% (variance大) | ⚠️ UNCERTAIN |
| R=90% regression | ±2% | -19.5% (variance大) | ⚠️ UNCERTAIN |
| SSOT delta | ±2% | all PASS | ✅ PASS |

### 判定: ⚠️ **INCONCLUSIVE**

**理由**:

1. **Variance 過大**: R=50%/R=90% で 1.7x～2.7x の範囲。10 runs でも収束しない。
2. **R=90% median 退行**: -19.5% は ±2% 閾値を超えるが、OFF/ON とも 83M～226M でばらつき、結論を出せない。
3. **R=0 は安定**: 唯一信頼できる結果で +3.9% 改善。
4. **SSOT PASS**: 退行なしで correctness は問題なし。

---

## 次のステップ

### 確定させるには

1. **低 variance 環境で再測定**:
   - Windows native (WSL2 より安定する可能性)
   - `taskset` で CPU pinning
   - background process 最小化
2. **RUNS=30 で再測定**: 10 runs で収束しないなら増やす
3. **別ワークロードで評価**: Redis 等の実ワークロードで確認

### 現状の扱い

- **既定 OFF** (`HZ3_S145_CENTRAL_LOCAL_CACHE=0`)
- **研究箱扱い**: opt-in で使用可能
- **R=0 限定なら GO**: 退行なし (+3.9%) で R=0 専用 lane は作成可能

---

## Opt-in Usage

```bash
# R=0 / R=50% ワークロード向け (R=90% は避ける)
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S145_CENTRAL_LOCAL_CACHE=1'

# バッチサイズ調整
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S145_CENTRAL_LOCAL_CACHE=1 -DHZ3_S145_CACHE_BATCH=8'
```

---

## Memory Footprint

```
TLS per thread:
- s145_cache_head:  128 SC × 8 bytes = 1024 bytes
- s145_cache_count: 128 SC × 2 bytes =  256 bytes
- Total: 1.25 KB / thread
```

---

## SSOT ログ

- **RUNS=10**: `/tmp/s145_runs10/`
  - `on_r0.txt`, `off_r0.txt`, `on_r0_median.txt`, `off_r0_median.txt`
  - `on_r50.txt`, `off_r50.txt`, `on_r50_median.txt`, `off_r50_median.txt`
  - `on_r90.txt`, `off_r90.txt`, `on_r90_median.txt`, `off_r90_median.txt`
- **SSOT**: `/tmp/s145_ssot_*.csv`

---

## 関連資料

- Plan file: `/home/tomoaki/.claude/plans/dreamy-jumping-quokka.md`
- CURRENT_TASK: `CURRENT_TASK.md` (2026-01-17 entry)
- BUILD flags index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- perf baseline: `/tmp/s145_on.perf`

---

**END**
