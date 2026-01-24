# S138: Memory-First Lane (SmallMaxSize=1024)

## 概要

S138は**メモリ節約レーン**として固定化されたビルドバリアントです。

**目的**: RSS優先最適化（throughput二次的）
**効果**: RSS -43.7%削減（-6～7MB）、throughput -1.6%（許容範囲内）

## 設計

### Core Change

```c
HZ3_SMALL_MAX_SIZE: 2048 → 1024
HZ3_SMALL_NUM_SC:   128 → 64   // SC 0-127 → SC 0-63
```

### Size Class再編成

| 範囲 | Baseline (2048) | S138 (1024) |
|------|-----------------|-------------|
| 16-1024B | Small (SC 0-63) | Small (SC 0-63) |
| 1025-2048B | **Small (SC 64-127)** | **Sub4k (SC 0: 2304B bin)** ← 変更 |
| 2049-4095B | Sub4k (4 bins) | Sub4k (4 bins) |
| 4096-32768B | Medium (8 bins) | Medium (8 bins) |

**重要**: `HZ3_SUB4K_ENABLE=1`が必須（これがないと1025-4095BがMedium 4096Bに丸められる）

## ビルド方法

### Memory-First Lane（S138）

```bash
make -C hakozuna/hz3 all_ldpreload_scale_s138_1024
# Output: libhakozuna_hz3_scale_s138_1024.so
```

### Performance-First Lane（Baseline、デフォルト）

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale
# Output: libhakozuna_hz3_scale.so
```

**注意**: デフォルトはperformance-first（2048B max）を維持

## 測定結果（2026-01-17）

### Test Configuration

- Benchmark: `bench_random_mixed_malloc_dist`
- Parameters: iters=20M, ws=400, min=16, max=32768, dist=app
- RSS Sampling: 50ms間隔、6 samples
- Baseline: `libhakozuna_hz3_scale.so` (90K, 2048B max)
- Treat: `libhakozuna_hz3_scale_s138_1024.so` (90K, 1024B max)

### RSS Results

|      | Baseline | S138 (1024) | 削減量 | 削減率 |
|------|----------|-------------|--------|--------|
| **Mean** | 13675 KB | 7701 KB | **-5974 KB** | **-43.7%** |
| **P50** | 16000 KB | 9088 KB | -6912 KB | -43.2% |
| **P95** | 16128 KB | 9088 KB | -7040 KB | -43.6% |
| **Max** | 16128 KB | 9088 KB | -7040 KB | -43.6% |

### Throughput Impact

- Baseline: 70.93M ops/s
- S138: 69.78M ops/s
- 差: **-1.15M ops/s (-1.6%)**（許容範囲≤15%内）

### Lane Gap (workload別, 2026-01-17)

| Workload | Perf-First (2048) | Mem-First (1024) | Delta |
|---|---:|---:|---:|
| dist_app (20M) | 70.05M | 69.30M | **-1.1%** |
| mixed uniform (20M) | 74.57M | 69.80M | **-6.4%** |

解釈:
- dist_app では 1025–2048B の頻度が低く、gap は小さい。
- mixed uniform では 1025–2048B の頻度が高く、**sub4k 経由のオーバーヘッドが顕在化**。

### GO Criteria (All Met ✓)

1. **RSS mean削減 ≥5%**: 実測 **-43.7%** ✓✓✓ (目標の8.7倍達成)
2. **RSS p95削減 ≥5%**: 実測 **-43.6%** ✓✓✓ (目標の8.7倍達成)
3. **Throughput低下 ≤15%**: 実測 **-1.6%** ✓ (余裕あり)

## 動作原理

### S135-1C観測結果（背景）

```
[HZ3_S135_FULL_SC] top_sc=127,85,90,91,92
                   total_waste=10160,3984,3504,3408,3312
                   avg_tail=2032,1328,1168,1136,1104
```

**発見**: SC 127（2048B）が最大のtail waste contributor
- 5ページで10160B waste
- 平均2032B/page（4096-16-2048=2032）

### S138の効果

**SC 127削除**:
```c
HZ3_SMALL_NUM_SC = 1024 / 16 = 64  // SC 0-63のみ
// → SC 127は物理的に存在不可能
```

**1025-2048B範囲の移行**:
- 旧: Small SC 64-127（64クラス、16B刻み）
- 新: Sub4k SC 0（2304B固定bin）

**トレードオフ**:
- ✅ **Tail waste削減**: SC 127の2032B/page waste消失
- ⚠️ **内部断片化増加**: 1025B → 2304B = 1279B waste per allocation
- **結果**: Tail waste削減効果が圧倒的（内部断片化の影響軽微）

### 内部断片化が軽微だった理由（推測）

1. **アロケーション頻度**: dist=appで1025-2048B範囲の頻度が低い
2. **Sub4k効率**: 2304B binのパッキングが予想以上に良い
3. **Tail waste規模**: 2032B/pageが非常に大きかった（相対的に内部断片化は小さい）

## 使い分けガイド

### Memory-First Lane (S138) 推奨ケース

- **メモリ制約が厳しい環境**（組み込み、コンテナ、サーバレス）
- **RSS削減が最優先**（throughput -1.6%は許容可能）
- **Peak memory使用量を抑えたい**（16MB → 9MB）

### Performance-First Lane (Baseline) 推奨ケース

- **throughputが最優先**（-1.6%も避けたい）
- **メモリは十分にある**（RSS増加は問題なし）
- **デフォルト設定を維持したい**

## 実装ファイル

### Modified Files

1. **`hakozuna/hz3/include/hz3_config_small_v2.h`** (line 28-54)
   - `HZ3_SMALL_MAX_SIZE_OVERRIDE`フラグ定義
   - 範囲検証（512-2048、16倍数）

2. **`hakozuna/hz3/include/hz3_types.h`** (line 20-26)
   - `HZ3_SMALL_MAX_SIZE`条件付き定義

3. **`hakozuna/hz3/Makefile`** (line 225-232)
   - `all_ldpreload_scale_s138_1024`ターゲット
   - **CRITICAL**: `-DHZ3_SUB4K_ENABLE=1`含む

### Critical Dependencies

```c
#if HZ3_SMALL_MAX_SIZE_OVERRIDE != 0
  #define HZ3_SMALL_MAX_SIZE HZ3_SMALL_MAX_SIZE_OVERRIDE
#else
  #define HZ3_SMALL_MAX_SIZE 2048  // baseline default
#endif
```

**Makefileターゲット**:
```makefile
all_ldpreload_scale_s138_1024:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SMALL_MAX_SIZE_OVERRIDE=1024 -DHZ3_SUB4K_ENABLE=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(ROOT)/libhakozuna_hz3_scale_s138_1024.so
	@echo "S138 treat build (max=1024, sub4k enabled) -> libhakozuna_hz3_scale_s138_1024.so"
```

## リスク分析

### R0: HZ3_SUB4K_ENABLE=0の致命的問題（修正済み）

**問題**: Sub4k無効時、1025-4095BがMedium 4096Bに丸められる
- 1025B allocation → 4096B = **3071B waste**（3倍のRSS増加）

**修正**: Makefileに`-DHZ3_SUB4K_ENABLE=1`を必須追加

### R1: Sub4k Allocator過負荷

**リスク**: 1025-2048B allocationsがSub4k SC 0に集中、central lock競合

**測定結果**: dist_app では問題なし（-1.1%）、mixed uniform では -6.4% まで低下
→ mem-first lane は **メモリ優先用途に限定**するのが安全

### R2: 内部断片化増加（当初の懸念）

**リスク**: Sub4k固定サイズ（2304B）により内部断片化増加
- 1025B → 2304B = 1279B waste
- 2048B → 2304B = 256B waste

**測定結果**: Tail waste削減効果が圧倒的（RSS -43.7%）

## 次のステップ

### 完了事項 ✓

- [x] S138実装（3ファイル変更）
- [x] Compile tests（baseline/treat/invalid）
- [x] Functional tests（smoke + full benchmark）
- [x] Throughput測定（-1.6%確認）
- [x] RSS測定（-43.7%確認）
- [x] SC 127消失証明（compile-time定義確認）
- [x] Memory-First Lane文書化

### 今後の検討事項

1. **他のワークロードで検証**
   - Larson big/small
   - malloc-large
   - 実アプリケーション

2. **1536B override検討**
   - 1537-2048BのみSub4kへ（1025-1536Bは維持）
   - 内部断片化をさらに削減

3. **S135観測の修正**
   - S69/S135/S138の組み合わせでSEGFAULT発生
   - 原因調査と修正

## Health Check / Regression SSOT

### Purpose

Memory-First Lane（S138）の安定性と回帰チェック用の最小SSOTです。
新しい変更がS138に影響を与えないことを確認するため、定期的に実行します。

### Test Suite

**2本のみ**（dist_app + mixed、RSS + ops/s）:

1. **dist_app (20M iters)**
   - 用途: RSS削減効果の確認
   - 期待値: RSS mean 7-8MB、ops/s 68-71M

2. **mixed (20M iters)**
   - 用途: 汎用ワークロードでの回帰確認
   - 期待値: RSS安定、ops/s退行なし

### Execution

```bash
# Memory-First Lane (S138) ビルド
make -C hakozuna/hz3 all_ldpreload_scale_s138_1024

# 1. dist_app測定
LD_PRELOAD=./libhakozuna_hz3_scale_s138_1024.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app 2>&1 | \
  tee /tmp/s138_health_dist_app.log

# 2. mixed測定
LD_PRELOAD=./libhakozuna_hz3_scale_s138_1024.so \
  ./hakozuna/out/bench_random_mixed 20000000 400 16 32768 0x12345678 2>&1 | \
  tee /tmp/s138_health_mixed.log

# RSS測定（オプショナル、詳細確認時のみ）
/tmp/quick_rss.sh /tmp/s138_health_dist_app_rss.log \
  env LD_PRELOAD=./libhakozuna_hz3_scale_s138_1024.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app
```

### Baseline Values (2026-01-17)

| Workload | ops/s | RSS mean | RSS p95 | RSS max |
|----------|-------|----------|---------|---------|
| **dist_app** | 69.78M | 7701 KB | 9088 KB | 9088 KB |
| **mixed** | TBD | TBD | TBD | TBD |

**Regression Threshold**:
- ops/s: ≤-5%（性能退行）
- RSS mean: ≥+20%（メモリ効率悪化）
- RSS p95: ≥+20%（ピーク増加）

### Interpretation

**Pass条件**:
- dist_app ops/s: 66M以上（-5%以内）
- dist_app RSS mean: 9.2MB以下（+20%以内）
- mixed: クラッシュなし、極端な退行なし

**Fail時のアクション**:
1. Performance-First Lane (baseline) と比較
2. Sub4k allocatorのhotspot確認（perf）
3. S138特有の問題か、全体の問題かを切り分け

### Notes

- **軽量**: 2本のみ、各20M iters（~1分/本）
- **目的**: 回帰検出（詳細分析は別途）
- **頻度**: 新しいcommit/PRでS138に影響がありそうな場合
- **補足**: RUNS=1でOK（傾向確認のみ）

## References

- S135-1C Implementation: `hakozuna/hz3/docs/PHASE_HZ3_S135_FULL_SC_OBS.md` (仮)
- Sub4k Allocator: `hakozuna/hz3/src/hz3_sub4k.c`
- Size Class Mapping: `hakozuna/hz3/include/hz3_small.h`
- Build Flags Index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

**Date**: 2026-01-17
**Status**: GO（固定化完了）
**Lanes**: Performance-First (default) / Memory-First (S138)
