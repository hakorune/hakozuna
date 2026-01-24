# HZ4 Phase 6: Bench Compare Work Order (hz4 vs hz3)

目的:
- hz4 (small/mid/large) と hz3 scale を **同条件**で比較
- LD_PRELOAD 経路で route の実動と性能を確認
- 結果を SSOT 形式で保存

前提:
- hz4 Phase6-1 (Mid/Large + realloc/calloc) 完了
- hz3 scale lane を使用

---

## 1) Build

```
make -C hakozuna/hz4 clean all
make -C hakozuna/hz3 clean all_ldpreload_scale
```

出力:
- hz4: `hakozuna/hz4/libhakozuna_hz4.so`
- hz3: `libhakozuna_hz3_scale.so`

---

## 2) ログ保存先

```
TS=$(date +%Y%m%d_%H%M%S)
OUT=/tmp/hz4_phase6_compare_${TS}
mkdir -p "$OUT"
```

---

## 3) SSOT ベンチ（hz4 vs hz3）

### 3.1 MT remote (R=90, small range 16–2048)

```
ITERS=2000000
WS=400
MIN=16
MAX=2048
R=90
RING=65536

for T in 4 8 16; do
  echo "[HZ4] T=$T" | tee -a "$OUT/mt_remote_small.log"
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a "$OUT/mt_remote_small.log"

  echo "[HZ3] T=$T" | tee -a "$OUT/mt_remote_small.log"
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a "$OUT/mt_remote_small.log"
done
```

### 3.2 MT remote (R=90, full range 16–65536)

```
ITERS=1000000
WS=400
MIN=16
MAX=65536
R=90
RING=65536

for T in 4 8 16; do
  echo "[HZ4] T=$T" | tee -a "$OUT/mt_remote_full.log"
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a "$OUT/mt_remote_full.log"

  echo "[HZ3] T=$T" | tee -a "$OUT/mt_remote_full.log"
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a "$OUT/mt_remote_full.log"
done
```

### 3.3 ST mixed (small range)

```
ITERS=20000000
WS=400
MIN=16
MAX=2048

echo "[HZ4] ST small" | tee -a "$OUT/st_small.log"
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_args $ITERS $WS $MIN $MAX \
  | tee -a "$OUT/st_small.log"

echo "[HZ3] ST small" | tee -a "$OUT/st_small.log"
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_args $ITERS $WS $MIN $MAX \
  | tee -a "$OUT/st_small.log"
```

### 3.4 ST dist_app (full range)

```
ITERS=20000000
WS=400
MIN=16
MAX=65536

echo "[HZ4] ST dist_app" | tee -a "$OUT/st_dist.log"
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX \
  | tee -a "$OUT/st_dist.log"

echo "[HZ3] ST dist_app" | tee -a "$OUT/st_dist.log"
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX \
  | tee -a "$OUT/st_dist.log"
```

---

## 4) 記録フォーマット（SSOT）

```
T=<threads> R=90
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
```

併記:
- hz4: route fail/abort が無いこと、overflow/fallback 0 を確認
- hz3: overflow/fallback の有無

---

## 6) 比較サマリ（SSOT, 2026-01-23）

### MT remote (T=16, R=90, 16–2048)

```
hz4 runs: 41.5M / 41.1M / 40.1M (median ~41M)
hz3 runs: 70.1M / 75.8M / 94.4M (median ~76M)
ratio (hz4/hz3): ~0.54
```

### perf（self%）

hz3 (T=16 R=90, 108.6M ops/s):
- `bench_thread` 30.01%
- `hz3_owner_stash_pop_batch` 10.62%
- `hz3_free` 6.69%
- `hz3_remote_stash_flush_budget_impl` 6.63%
- `hz3_malloc` 4.67%

hz4 (IE=0, ~41M ops/s):
- `hz4_collect` 8.39%
- `bench_thread` 6.69%
- `__tls_get_addr` 6.52%
- `hz4_malloc` 4.08%
- `hz4_remote_flush` 3.20%

### TLS 初期化最適化（IE=1）

```
Throughput: ~41M → ~43M
__tls_get_addr: 6.52% → 消滅
hz4_tls_get: 0.92% → 0.33%
```

hz4 perf (IE=1, self%):
- `bench_thread` 11.48%
- `hz4_collect` 9.28%
- `hz4_malloc` 3.67%
- `hz4_remote_flush` 3.54%

---

## 5) 注意

- hz4 の Mid/Large は **64KB base magic 判定**。サイズ境界は `hz4_mid_max_size()` を基準。
- ベンチは LD_PRELOAD 経路で **realloc/calloc も通る**前提。
- ログは `/tmp/hz4_phase6_compare_<ts>/` に保存して SSOT 化する。
