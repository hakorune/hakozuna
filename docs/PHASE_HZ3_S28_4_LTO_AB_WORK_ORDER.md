# S28-4: LTO（-flto）A/B（tiny 残差の “命令数側” を詰める）

目的:
- S28-2C で hz3 は改善したが、tiny / dist=app / uniform で tcmalloc に **~6%** 残差が残るケースがある。
- 直近の perf 観測で「命令数 + IPC（依存/フロント）で半々」という示唆があるため、まず **命令数側**を低リスクで詰める。
- allocator の箱を増やさず、hot path の分岐/ロードも増やさない（ビルド最適化だけで寄せる）。

前提:
- allocator挙動の切替は compile-time `-D` のみ（envノブ禁止）。
- LTO は “挙動” ではなく “ビルド最適化” なので、`make` のビルド変数で A/B する。
- 判定は **RUNS=30 + seed固定**の SSOT を正とし、必要なら `perf stat` で補強する。
- `run_bench_hz3_ssot.sh` は既定で **自動リビルド**するため、手動ビルドをそのまま計測したい場合は `SKIP_BUILD=1` を使う（または `HZ3_MAKE_ARGS` で `make` 変数を渡す）。

参照:
- S28-2C（GO）: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`
- S28-3（SSOT+perf手順）: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S28_3_TINY_TCMALLOC_GAP_PERF_WORK_ORDER.md`
- SSOT runner: `hakmem/hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## 0) 実装（ビルドA/Bの入口を作る）

hz3 のみ（`hakozuna/hz3`）で LTO を A/B できるようにする:

- `make -C hakmem/hakozuna/hz3 all_ldpreload HZ3_LTO=0`（baseline）
- `make -C hakmem/hakozuna/hz3 all_ldpreload HZ3_LTO=1`（LTO）

期待する挙動:
- `HZ3_LTO=1` のとき、compile/link 両方に `-flto` が入る
- それ以外の最適化（`-ftls-model=initial-exec -fno-plt` 等）は現状維持

---

## 1) SSOT（RUNS=30, seed固定）で A/B

### tiny 100%（trimix 16–256）

```bash
cd hakmem
# baseline（スクリプト側でビルドする。手動ビルドは不要）
RUNS=30 ITERS=20000000 WS=400 \
  HZ3_CLEAN=1 HZ3_MAKE_ARGS="HZ3_LTO=0" \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# LTO（`HZ3_MAKE_ARGS` で `make` 変数を渡す）
RUNS=30 ITERS=20000000 WS=400 \
  HZ3_CLEAN=1 HZ3_MAKE_ARGS="HZ3_LTO=1" \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### dist=app（16–32768）

```bash
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### uniform（16–32768）

```bash
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## 2) perf stat（任意、RUNS=1〜3）

LTO の狙いは “命令数側” なので、tiny 100% だけで方向を見る。

```bash
BENCH=./hakozuna/out/bench_random_mixed_malloc_dist
ARGS="20000000 400 16 256 0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0"

perf stat -o /tmp/perf_hz3_tiny_lto0.txt -e cycles,instructions,branches,branch-misses \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so $BENCH $ARGS

perf stat -o /tmp/perf_hz3_tiny_lto1.txt -e cycles,instructions,branches,branch-misses \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so $BENCH $ARGS
```

（※LTO0/LTO1で `libhakozuna_hz3_ldpreload.so` が上書きされるので、実行順に注意）

---

## 3) GO/NO-GO

GO:
- tiny 100% が **+2% 以上**改善（RUNS=30 median）
- dist=app / uniform の退行なし（±1%）

NO-GO:
- tiny が動かない（±1%）または退行
- dist=app / uniform が落ちる

NO-GO の場合:
- `hakmem/hakozuna/hz3/archive/research/s28_4_lto/README.md` に SSOTログと結論を固定
- `hakmem/hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記
