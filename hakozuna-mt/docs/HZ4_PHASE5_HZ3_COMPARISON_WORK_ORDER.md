# HZ4 Phase 5: HZ3 Comparison Work Order

目的:
- hz4 (pageq + carry) を **hz3 baseline** と同条件で比較する
- ワークロードを揃え、T/R の差分を SSOT で記録する

前提:
- hz4 Phase 4 完了（carry=GO, pageq=GO）
- hz3 baseline は scale lane を使用

---

## 1) ビルド

### hz4 bench
```
make -C hakozuna/hz4 clean test
make -C hakozuna/hz4 bench CFLAGS='-O3 -DHZ4_STATS=1'
```

### hz3 baseline (scale)
```
make -C hakozuna/hz3 clean all_ldpreload_scale
```

---

## 2) ベンチ条件（SSOT）

### hz4 bench (alloc/free-like)
```
./hakozuna/hz4/out/bench_hz4 4 1000000 90 0
./hakozuna/hz4/out/bench_hz4 8 1000000 90 0
./hakozuna/hz4/out/bench_hz4 16 1000000 90 0
```

注記:
- `segs=0` は **threads × HZ4_BENCH_SEGS_PER_THREAD** の自動割当
- 低Tの探索コスト偏りを避けるため、比較では auto を推奨

### hz3 baseline (LD_PRELOAD, MT remote)
```
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 4 1000000 400 16 2048 90 65536

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 1000000 400 16 2048 90 65536

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 1000000 400 16 2048 90 65536
```

---

## 3) 記録フォーマット（SSOT）

```
T=<threads> R=90
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
```

併記:
- hz4 stats: segs_popped / segs_requeued / scan_fallback
- hz3 stats: (あれば) HZ3 stats dump

---

## 4) 注意

- hz4 は allocator 本体ではないため **絶対値比較は参考**。
- **傾向（T依存、R依存）** を重視する。
- hz4 が低Tで負けても、高Tで伸びるなら設計方向は正しい。

---

## 5) 結果サマリ（2026-01-22）

R=90, RUNS=10 median:

```
T=4  hz4=30.98M  hz3=60.87M  delta=-49%
T=8  hz4=37.63M  hz3=63.38M  delta=-41%
T=16 hz4=86.44M  hz3=71.59M  delta=+21%
```

スケーリング:

```
hz4: T=4→8 +21%, T=8→16 +130%
hz3: T=4→8 +4%,  T=8→16 +13%
```

結論:
- **低T帯は hz3 が優勢**（owner_stash の O(1) pop が効く）
- **高T帯 (T=16) は hz4 が逆転**（pageq + carry がスケール）
- hz4 は「スケーラブル設計」として有望
