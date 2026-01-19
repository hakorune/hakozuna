# S37: post-S36 gap refresh（SSOT RUNS=30 + perf hotspot 再確認）

**Status: COMPLETE** ✅

## Results Summary (2026-01-02)

Git commit: 885a3ebba (S36: bin index zero-extension applied)

### SSOT Baseline Results

| Benchmark | Hz3 | TCMalloc | Gap | Config |
|-----------|-----|----------|-----|--------|
| dist=app (warm) | 51.33M ops/s | 54.09M ops/s | **-5.11%** | RUNS=60, warm phase (31-60) |
| uniform | 78.98M ops/s | 83.72M ops/s | **-6.00%** | RUNS=30 |
| tiny-only | 102.96M ops/s | 110.36M ops/s | **-6.71%** | RUNS=30 |

### Log Directories
- dist=app: `/tmp/hz3_ssot_885a3ebba_20260102_231301`
- uniform: `/tmp/hz3_ssot_885a3ebba_20260102_224649`
- tiny-only: `/tmp/s37_tiny_hz3_20260102_231004.log`, `/tmp/s37_tiny_tcmalloc_20260102_231004.log`

### Analysis
- Hz3 shows competitive performance (5-7% gap) across all distributions
- dist=app warm phase shows best relative performance (-5.11%)
- tiny-only shows largest gap (-6.71%), indicating optimization opportunity in small fast path
- Hz3 demonstrates good stability (3.86% CV on dist=app warm phase)

Comprehensive report: `/mnt/workdisk/public_share/hakmem/s37_baseline_ssot_report.txt`

---

目的:
- S36（bin index sign-extension 排除）後の到達点を SSOT で固定し、hz3 vs tcmalloc の残差と「次に削る固定費」を再確定する。

前提:
- SSOT runner: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
  - `SKIP_BUILD=0` の場合、スクリプトが hz3 をリビルドする。
  - A/B の build 変数は `HZ3_LDPRELOAD_DEFS` / `HZ3_MAKE_ARGS` で渡す。

参照:
- S36: `hakozuna/hz3/docs/PHASE_HZ3_S36_BIN_INDEX_ZEROEXT_WORK_ORDER.md`
- SSOT index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## Step 1: ビルド・スモーク

```bash
cd hakmem
make -C hakozuna/hz3 clean all_ldpreload
LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/true
```

---

## Step 2: SSOT（RUNS=30, seed固定）

### 2.1 dist=app（比較の主戦場）

```bash
make -C hakozuna bench_malloc_dist
TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 2.2 uniform（16–32768）

```bash
TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 2.3 tiny-only（dist bench を tiny 100% にする）

```bash
make -C hakozuna bench_malloc_dist
TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

期待:
- `/tmp/hz3_ssot_*` が作られ、各 `*.log` の先頭 `[BENCH-HEADER]` に
  `hz3_ldpreload_defs=...` と `hz3_make_args=...` が記録されていること。

---

## Step 3: まとめ（GO/NO-GO ではなく “現状固定”）

このフェーズは「次の箱選定」が目的なので GO/NO-GO は置かない。

必要な成果物:
- dist=app / uniform / tiny-only の SSOTログパス
- hz3 vs tcmalloc の gap（%）

---

## Step 4: perf（任意）

dist=app の 1回だけで方向を見る（RUNS=1相当でOK）。

```bash
perf stat -e cycles,instructions,branches,branch-misses \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app
```

見るポイント:
- S36 後も `hz3_free` の fixed-cost（dst compare / PTAG decode / bin push）が支配的か
- `hz3_malloc` の fixed-cost（TLS base / count-- / head update）が支配的か

