# HZ4 Phase 6: P3 Inbox Full-Range Bench Work Order

目的:
- P3 inbox lane (default ON) の **full‑range (16–65536)** での安定性と性能を確認
- hz3 scale と **同条件比較**
- perf でボトルネックを再確認

---

## 0) 前提

- `HZ4_REMOTE_INBOX=1`（default ON）
- 必要なら `HZ4_REMOTE_INBOX=0` で A/B 可能
- hz3 は `all_ldpreload_scale`

---

## 1) Build

```
make -C hakozuna/hz4 clean all
make -C hakozuna/hz3 clean all_ldpreload_scale
```

---

## 2) SSOT ベンチ（full‑range）

### 2.1 MT remote (T=4/8/16, R=90, 16–65536)

```
ITERS=1000000
WS=400
MIN=16
MAX=65536
R=90
RING=65536

for T in 4 8 16; do
  echo "[HZ4] T=$T" | tee -a /tmp/hz4_p3_fullrange_mt_remote.log
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_p3_fullrange_mt_remote.log

  echo "[HZ3] T=$T" | tee -a /tmp/hz4_p3_fullrange_mt_remote.log
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc $T $ITERS $WS $MIN $MAX $R $RING \
    | tee -a /tmp/hz4_p3_fullrange_mt_remote.log

done
```

### 2.2 ST dist_app (16–65536)

```
ITERS=2000000
WS=400
MIN=16
MAX=65536

LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX 0x12345678 app \
  | tee /tmp/hz4_p3_fullrange_dist_st.log

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist $ITERS $WS $MIN $MAX 0x12345678 app \
  | tee -a /tmp/hz4_p3_fullrange_dist_st.log
```

---

## 3) perf (T=16/R=90, 16–2048)

```
perf record -g -- env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536

perf report --stdio -g none --no-children | head -20
```

同条件で hz3 も取得:

```
perf record -g -- env LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536

perf report --stdio -g none --no-children | head -20
```

---

## 4) 記録フォーマット（SSOT）

```
T=16 R=90
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
perf: <top5>
notes: <inbox ON/OFF>
```

---

## 5) 結果サマリ（SSOT, 2026-01-23）

### T 依存性（R=90, 16–2048）

| T  | hz4  | hz3  | hz4/hz3 |
|----|------|------|---------|
| 4  | 49.0M | 66.3M | -26% |
| 8  | 61.4M | 68.3M | -10% |
| 16 | 69.0M | 78.7M | -12% |

### R 依存性（T=16, 16–2048）

| R   | hz4   | hz3   | hz4/hz3 |
|-----|-------|-------|---------|
| 0%  | 256.7M | 361.5M | -29% |
| 50% | 89.4M  | 96.4M  | -7%  |
| 90% | 70.2M  | 75.4M  | -7%  |

### 追加観測

- **R=0 で差が大きい** → local 経路の最適化余地
- **R=50/90 で差が縮小** → inbox lane が効いている
- **full‑range (16–65536) で crash** → mid/large の問題を優先調査

---

## 5) 結果（SSOT, 2026-01-23）

### 5.1) T 依存性（R=90, 16-2048）

| T | hz4 (M ops/s) | hz3 (M ops/s) | hz4/hz3 |
|---|---------------|---------------|---------|
| 4 | 49.0 | 66.3 | 0.74 (-26%) |
| 8 | 61.4 | 68.3 | 0.90 (-10%) |
| 16 | 69.0 | 78.7 | 0.88 (-12%) |

### 5.2) R 依存性（T=16, 16-2048）

| R | hz4 (M ops/s) | hz3 (M ops/s) | hz4/hz3 |
|---|---------------|---------------|---------|
| 0% | 256.7 | 361.5 | 0.71 (-29%) |
| 50% | 89.4 | 96.4 | 0.93 (-7%) |
| 90% | 70.2 | 75.4 | 0.93 (-7%) |

### 5.3) Full-range (16-65536)

- ~~**hz4 crash**: T=16 R=90 で abort（mid/large 経路に問題あり）~~ **解消**
- hz3 は正常動作

**修正後の結果** (T=16 R=90 16-65536, iters=500000):

| Allocator | ops/s | 比率 |
|-----------|-------|------|
| hz4 | 1.66M | 0.94 (-6%) |
| hz3 | 1.76M | baseline |

- large_free の header 位置計算バグを修正（`HZ4_PHASE6_LARGE_FREE_FIX_WORK_ORDER.md` 参照）

### 5.4) 評価

- hz4 inbox ON は hz3 に対して **-7〜29%** の差
- R=0 (local-only) で差が大きい → local 経路の最適化余地あり
- R=50/90 で差が縮まる → inbox は効いている
- T が増えると差が縮まる傾向
- full-range は mid/large の安定性改善が必要

### 5.5) 次のアクション

1. mid/large crash の調査・修正
2. local 経路 (R=0) の最適化
3. hz3 との perf 比較で具体的なボトルネック特定
