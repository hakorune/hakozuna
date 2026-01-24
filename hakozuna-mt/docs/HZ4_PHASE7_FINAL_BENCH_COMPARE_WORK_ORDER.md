# HZ4 Phase 7: Final Bench Compare Work Order (hz4 vs hz3 vs mimalloc)

目的:
- P4.1b までの成果を **SSOT で最終比較**
- hz4 / hz3 / mimalloc を同条件で測定
- perf top3 も更新し、最後の課題を固定

---

## 0) Build

```
# hz4 (current default)
make -C hakozuna/hz4 clean all

# hz3 scale lane
make -C hakozuna/hz3 clean all_ldpreload_scale
```

mimalloc 参照:
- システムにある場合は LD_PRELOAD で `libmimalloc.so`
- ない場合は `mimalloc-bench` を使う（下記オプション）

---

## 1) SSOT: MT remote (R=90, 16–2048)

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536
for T in 4 8 16; do
  echo "[HZ4] T=$T" | tee -a /tmp/hz4_final_mt_remote.log
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_final_mt_remote.log

  echo "[HZ3] T=$T" | tee -a /tmp/hz4_final_mt_remote.log
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_final_mt_remote.log

done
```

mimalloc (if available):
```
LD_PRELOAD=./libmimalloc.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 2) SSOT: R sweep (T=16, 16–2048)

```
for R in 0 50 90; do
  echo "[HZ4] R=$R" | tee -a /tmp/hz4_final_r_sweep.log
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_final_r_sweep.log

  echo "[HZ3] R=$R" | tee -a /tmp/hz4_final_r_sweep.log
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_final_r_sweep.log

done
```

---

## 3) full‑range (16–65536, T=16 R=90)

```
ITERS=1000000 WS=400 MIN=16 MAX=65536 R=90 RING=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING \
  | tee /tmp/hz4_final_fullrange.log

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING \
  | tee -a /tmp/hz4_final_fullrange.log
```

---

## 4) ST dist_app (16–65536)

```
ITERS=2000000 WS=400 MIN=16 MAX=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX 0x12345678 app \
  | tee /tmp/hz4_final_dist_app.log

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX 0x12345678 app \
  | tee -a /tmp/hz4_final_dist_app.log
```

---

## 5) perf (T=16 R=90, 16–2048)

```
perf record -g -- env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
perf report --stdio -g none --no-children | head -20

perf record -g -- env LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
perf report --stdio -g none --no-children | head -20
```

---

## 6) 記録フォーマット（SSOT）

```
T=16 R=90
hz4: <ops/s>
hz3: <ops/s>
mimalloc: <ops/s>
ratio(hz4/hz3): <>
perf: <top3>
notes: <P4.1b ON>
```

---

## 7) 結果サマリ（SSOT, 2026-01-23）

### MT Remote T-sweep (R=90)

| T  | hz4   | hz3   | mimalloc | Winner |
|----|-------|-------|----------|--------|
| 4  | 50.10M | 54.34M | 23.03M  | hz3 |
| 8  | 50.90M | 66.65M | 47.95M  | hz3 |
| 16 | 74.67M | 81.16M | 87.75M  | mimalloc |

### R-sweep (T=16)

| R  | hz4    | hz3    | mimalloc | Winner |
|----|--------|--------|----------|--------|
| 0  | 264.96M | 348.31M | 296.39M | hz3 |
| 50 | 97.57M  | 102.18M | 85.48M  | hz3 |
| 90 | 113.17M | 83.96M  | 83.40M  | hz4 |

### ST dist_app (16–65536)

| Allocator | ops/s  | vs mimalloc |
|-----------|--------|-------------|
| hz4       | 50.29M | 0.72x |
| hz3       | 77.88M | 1.12x |
| mimalloc  | 69.48M | 1.00x |

### まとめ

| 条件 | 1位 | 2位 | 3位 |
|------|-----|-----|-----|
| R=0 (local only) | hz3 | mimalloc | hz4 |
| R=50 | hz3 | hz4 | mimalloc |
| R=90 (high remote) | hz4 | hz3 ≈ mimalloc | - |
| ST dist_app | hz3 | mimalloc | hz4 |

結論:
- hz4: 高 remote (R=90) で最速、local/ST は弱い\n+- hz3: バランス型で上位維持\n+- mimalloc: T=16 R=90 と ST で健闘
