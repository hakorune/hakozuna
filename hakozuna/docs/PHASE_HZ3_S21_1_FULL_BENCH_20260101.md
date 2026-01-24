# S21-1: フルベンチ一括（SSOT-all）+ dist(app) 追加ラン

目的:
- 現状の hz3/hakozuna を “lane混線なし” で一括測定し、比較用ログを固定する。
- 追加で「実アプリ寄り分布（app）」も SSOT-HZ3 で測る。

## 1) SSOT-all（一括）

実行:
```bash
cd hakmem

RUNS=5 ITERS=20000000 WS=400 SKIP_BUILD=0 \
  DO_MIMALLOC_BENCH_SUBSET=1 \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/scripts/run_bench_ssot_all.sh
```

ログ:
- `/tmp/ssot_all_6cf087fc6_20260101_213835`

### Lane HZ3: SSOT（uniform）

| workload | system | hz3 | mimalloc | tcmalloc |
|----------|--------|-----|----------|----------|
| small    | 33.81M | 100.28M | 75.19M | 95.01M |
| medium   | 13.61M | 104.22M | 63.13M | 109.78M |
| mixed    | 15.16M | 99.49M | 62.49M | 106.90M |

### Larson big hygienic（4KB–32KB）

| threads | system | hz3 | hakozuna | mimalloc | tcmalloc |
|---------|--------|-----|----------|----------|----------|
| T=1     | 5.08M  | 21.96M | 10.42M | 14.76M | 21.59M |
| T=4     | 14.42M | 60.98M | 29.38M | 43.54M | 59.29M |
| T=8     | 22.18M | 86.76M | 41.85M | 64.63M | 79.32M |

### Lane 1: hakozuna compare（system_bench_random_mixed）

| allocator | median ops/s |
|----------|--------------|
| system   | 81.74M |
| hakozuna | 104.24M |
| mimalloc | 104.63M |
| tcmalloc | 111.83M |

### mimalloc-bench subset（cache/malloc-large）

cache-thrash (T=1): hz3 943.96 / tcmalloc 911.97

cache-thrash (T=16): hz3 47764.69 / tcmalloc 44148.02

cache-scratch (T=1): hz3 891.32 / tcmalloc 898.34

cache-scratch (T=16): hz3 49648.36 / tcmalloc 46136.45

malloc-large (T=1): hz3 1105.02 / tcmalloc 1359.78

## 2) dist(app)（実アプリ寄り分布）

ログ:
- `/tmp/hz3_ssot_6cf087fc6_20260101_215211`

詳細:
- `hakozuna/hz3/docs/PHASE_HZ3_S21_0_DIST_BENCH_SSOT_APP_VS_TCMALLOC.md`

## 3) mimalloc-bench 全体（r=1, n=1）追加ラン

対象:
- hz3-scale / tcmalloc / mimalloc（sys）

結果:

| Bench | hz3 | tc | mi | 勝者 | 備考 |
|-------|-----|----|----|------|------|
| cfrac | 3.38s | 3.40s | 3.37s | mi | ほぼ同等 |
| espresso | 3.66s | 3.60s | 3.65s | tc | ほぼ同等 |
| barnes | 2.75s | 2.77s | 2.76s | hz3 | ほぼ同等 |
| larson-sized | 9.05s | 9.26s | 10.05s | hz3 | hz3 +2.3% vs tc |
| mstressN | 2.04s | 1.32s | 1.35s | tc | hz3 -55% |
| rptestN | 4.52s | 2.87s | 2.54s | mi | hz3 -77% |
| gs | 0.93s | 0.92s | 0.96s | tc | ほぼ同等 |
| alloc-test1 | 2.92s | 2.74s | 2.83s | tc | hz3 -6.5% |
| alloc-testN | 6.55s | 5.86s | 5.86s | tc/mi | hz3 -12% |
| sh6benchN | 0.31s | 0.32s | 0.27s | mi | 同等 |
| sh8benchN | 0.92s | 6.54s | 0.74s | mi | hz3 +24% vs mi, tc 崩壊 |
| xmalloc-testN | 3.96s | 27.6s | 0.44s | mi | hz3 -89% vs mi, tc 崩壊 |
| cache-scratch1 | 1.03s | 1.03s | 1.02s | mi | 同等 |
| cache-scratchN | 0.29s | 0.28s | 0.29s | tc | 同等 |
| glibc-simple | 2.91s | 1.71s | 2.36s | tc | hz3 -70% |
| glibc-thread | 1.58s | 1.72s | 1.49s | mi | hz3 +8% vs tc |

要約:
- hz3 優勢: larson-sized, sh8benchN, glibc-thread
- hz3 劣勢: mstressN, rptestN, glibc-simple, xmalloc-testN, alloc-testN
- 退行が大きい mstressN / rptestN は MT + remote-heavy の可能性が高く、perf で要調査
