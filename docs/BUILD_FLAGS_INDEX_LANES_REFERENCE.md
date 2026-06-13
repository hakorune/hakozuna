## 3) SSOT-HZ3（hz3専用 lane）

用途:
- hz3 の small/medium/mixed を同一導線で SSOT 化する（混線防止）。

スクリプト:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

スクリプトの上書き変数（ベンチ導線用）:
- `RUNS` / `ITERS` / `WS` / `SKIP_BUILD`
- `HZ3_SO`（default: `./libhakozuna_hz3_ldpreload.so`）
- `BENCH_BIN`（default: `./hakozuna/out/bench_random_mixed_malloc_args`）
- `BENCH_EXTRA_ARGS`（任意。`BENCH_BIN` に追加引数を渡す。分布付きベンチなど）
- `MIMALLOC_SO` / `TCMALLOC_SO`（任意。存在する場合のみ追加で比較）
- `HZ3_LDPRELOAD_DEFS`（任意。`SKIP_BUILD=0` のとき `make -C hakozuna/hz3 all_ldpreload` に渡す）
- `HZ3_LDPRELOAD_DEFS_EXTRA`（任意。`SKIP_BUILD=0` のとき `HZ3_LDPRELOAD_DEFS` の後ろに追加で渡す。A/Bで “単一の -D だけ足したい” ときはこれを推奨）
- `HZ3_MAKE_ARGS`（任意。`SKIP_BUILD=0` のとき `make -C hakozuna/hz3 all_ldpreload` に追加で渡す。例: `HZ3_LTO=1`）

flood（malloc/free, 100/1000 threads）を追加で走らせる（任意）:
- `RUN_FLOOD=0/1`（1 のとき flood も実行）
- `FLOOD_BIN`（default: `./hakozuna/out/bench_malloc_flood_mt`）
- `FLOOD_THREADS`（default: `100 1000`）
- `FLOOD_SECONDS`（default: `10`）
- `FLOOD_SIZE`（default: `256`）
- `FLOOD_BATCH`（default: `1`）
- `FLOOD_TOUCH`（default: `1`）
- `FLOOD_TIMEOUT`（default: `30`）

`.env`:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh.env`（override-friendly）
  - ベンチの既定値を置く場所（allocator本体のノブには使わない）。

注意:
- `SKIP_BUILD=0`（既定）の場合、スクリプトは **毎回リビルド**する。
  - A/B が「手動ビルドの差分」に依存する場合は `SKIP_BUILD=1` を使う（または `HZ3_LDPRELOAD_DEFS` / `HZ3_MAKE_ARGS` を渡す）。
- ⚠️ `HZ3_LDPRELOAD_DEFS="-Dfoo=1"` のように “単一の -D” で上書きすると、Makefile既定の `-DHZ3_ENABLE=1` などが落ちて **hz3 が無効化され、測定が壊れる**。差分だけ足す場合は `HZ3_LDPRELOAD_DEFS_EXTRA` を使う。

メモリ観測（外部サンプリング、平均/p95も取る）:
```bash
OUT=/tmp/mem.csv INTERVAL_MS=50 PSS_EVERY=20 \
  ./hakozuna/hz3/scripts/measure_mem_timeseries.sh <cmd...>
```

実行例:
```bash
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S41 fast/scale の測定例（手動ビルドして `HZ3_SO` で切替）:
```bash
make -C hakozuna/hz3 all_ldpreload_fast all_ldpreload_scale

# fast lane
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_fast.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# scale lane
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S53-3 mem-mode 2プリセット（opt-in lanes）:
```bash
make -C hakozuna/hz3 all_ldpreload_scale_mem_mstress all_ldpreload_scale_mem_large

# mem_mstress（RSS重視）
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale_mem_mstress.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# mem_large（malloc-large速度重視）
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale_mem_large.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S54（OBSERVE）:
```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SEG_SCAVENGE_OBSERVE=1'
```

補足（分布付きベンチ）:
- `bench_random_mixed_malloc_args` は `min..max` の uniform random（ストレス寄り）なので、実アプリ寄り分布で測りたい場合は `bench_random_mixed_malloc_dist` を使う。
- build: `make -C hakozuna bench_malloc_dist`

例（実アプリ寄り: 80% 16–256 / 15% 257–2048 / 5% 2049–32768）:
```bash
make -C hakozuna bench_malloc_dist
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
BENCH_EXTRA_ARGS="app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

triage（dist/app mixed の詰め方）:
- `hakozuna/hz3/docs/PHASE_HZ3_S22_DIST_APP_MIXED_TRIAGE_WORK_ORDER.md`

## 3.x) Eco Mode（S202, research opt-in）

目的: CPU効率（ops/s per CPU%）を改善するための **adaptive batch**。既定は OFF。

- compile-time: `HZ3_ECO_MODE=0/1`（default: `0`）
  - 有効化した場合のみ eco code を含める（hot path の分岐増加を避けるため）。
- runtime: `HZ3_ECO_ENABLED=0/1`（default: `0`）
- threshold: `HZ3_ECO_RATE_THRESH`（default: `10000000` ops/sec）

動作:
- alloc_rate が閾値以上 → LARGE（`S74_REFILL_BURST=16`, `S118_REFILL_BATCH=64`）
- alloc_rate が閾値未満 → SMALL（`S74_REFILL_BURST=8`, `S118_REFILL_BATCH=32`）
- 判定は refill slow-path でのみ実施（1秒 or 1M ops のどちらか早い方）。

注意:
- 短時間ベンチでは warm-up が足りず SMALL のままになることがある。
- 詳細と結果: `hakozuna/hz3/docs/PHASE_HZ3_S202_ECO_MODE_ADAPTIVE_BATCH.md`

---

## 4) Makefile 既定プロファイル（SSOT到達点）

`make -C hakmem/hakozuna/hz3 all_ldpreload` の既定（`HZ3_LDPRELOAD_DEFS`）は、SSOT到達点として次を ON にしている:
- `HZ3_ENABLE=1`
- `HZ3_SHIM_FORWARD_ONLY=0`
- `HZ3_SMALL_V2_ENABLE=1`
- `HZ3_SEG_SELF_DESC_ENABLE=1`
- `HZ3_SMALL_V2_PTAG_ENABLE=1`（S12-4）
- `HZ3_PTAG_V1_ENABLE=1`（S12-5: 4KB–32KB も PTAG で統一 dispatch）
- `HZ3_PTAG_DSTBIN_ENABLE=1`（S17: free を dst/bin 直結にして命令数削減）
- `HZ3_PTAG_DSTBIN_FASTLOOKUP=1`（S18-1: range check + tag load の 1-shot 化）
- `HZ3_PTAG32_NOINRANGE=1`（S28-5: dist=app/uniform の固定費削減）
- `HZ3_BIN_COUNT_POLICY=1`（S38-2: count 型を U32 にして命令形/依存を改善）
- `HZ3_REALLOC_PTAG32=1`（S21: realloc を PTAG32-first に寄せる）
- `HZ3_USABLE_SIZE_PTAG32=1`（S21: usable_size を PTAG32-first に寄せる）
- `HZ3_LOCAL_BINS_SPLIT=1`（S28-2C: local を浅いTLS binsへ）

既定を変えたい場合は、`HZ3_LDPRELOAD_DEFS` を上書きする（`CFLAGS` ではなく）:
```bash
make -C hakmem/hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0'
```

差分だけ足す（推奨）:
```bash
make -C hakmem/hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_BIN_LAZY_COUNT=1'
```
