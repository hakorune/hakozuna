# S29: dist=app gap refresh + perf 固定（次の最適化の入口）

目的:
- いまの hz3 lane 既定プロファイル（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）を **RUNS=30 で再固定**し、
  tcmalloc との差（dist=app / tiny / uniform）を “再現できる形” で残す。
- 次の箱（最適化）に入る前に、**どのレンジが主犯か**と **ホットスポット**を perf で SSOT と一緒に保存する。

前提:
- allocator の挙動切替は compile-time `-D` のみ（env ノブ禁止）。
- 比較用 .so のパスやベンチの分布指定は env OK（SSOTログに残るので）。
- 以降の GO/NO-GO は基本的に **RUNS=30** を優先する。

---

## 0) 今日の “正” プロファイル（確認）

hz3 lane の既定:
- `make -C hakozuna/hz3 all_ldpreload`（Makefileの `HZ3_LDPRELOAD_DEFS` が付く）

重要:
- `hakozuna/hz3/include/hz3_config.h` は安全デフォルト（多くは `0`）。
- hz3 lane での既定ONは `hakozuna/hz3/Makefile` 側（`HZ3_LDPRELOAD_DEFS`）で決める。

---

## 1) SSOT（RUNS=30）: dist=app / uniform / tiny-trimix を固定する

### 1-1) Build

```bash
cd hakmem
make -C hakozuna/hz3 clean all_ldpreload
make -C hakozuna bench_malloc_dist
```

### 1-2) Run（dist=app）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 1-3) Run（uniform: 16–32768）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 1-4) Run（tiny-trimix: 16–256 を 100%）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

成果物:
- `/tmp/hz3_ssot_${git}_YYYYmmdd_HHMMSS/`（スクリプトが自動作成）
- `*_hz3.log`, `*_tcmalloc.log`, `*_mimalloc.log`（存在するものだけ）

GO条件:
- 3本とも完走し、ログ先頭の `[BENCH-HEADER]` に git/so/args が残っていること。

---

## 2) triage（dist=appの主犯レンジ確認：RUNS=30）

狙い:
- gap が残っている場合、主犯レンジを **最小本数で確定**する（例: 16–256 / 257–2048 / 2049–4095 / 4096–16384 / 16385–32768）。

例（tiny / mid / tail の切り方）:

```bash
# tiny
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# tail=2049..32768 だけ
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 2048 0 2049 32768 100" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## 3) perf（mixedのホットスポット固定）

狙い:
- “どこが重いか” を推測しないで、次の箱の入口にする。

### 3-1) perf stat（最小）

```bash
perf stat -o /tmp/hz3_perf_stat_distapp.txt -e \
  instructions,cycles,branches,branch-misses, \
  L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses \
  -- \
  env LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app
```

同条件で tcmalloc も取る（存在する場合）:
- `LD_PRELOAD=${TCMALLOC_SO}` を同様に実行して `/tmp/hz3_perf_stat_distapp_tcmalloc.txt` に保存。

### 3-2) perf record（必要なら）

```bash
perf record -o /tmp/hz3_perf_record_distapp.data -g -- \
  env LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app
```

成果物:
- `/tmp/hz3_perf_stat_distapp*.txt`
- `/tmp/hz3_perf_record_distapp*.data`

---

## 4) 出口（S29の完了条件）

S29 は “最適化” ではなく “次の最適化の入口を固定する” フェーズ。

完了条件:
- SSOT 3本（dist=app / uniform / tiny-trimix）が RUNS=30 で固定ログ化されている
- perf stat（最低1本）が `/tmp` に保存されている
- 「主犯レンジ（どこから削るべきか）」が 1行で言える状態

次（S30候補）:
- 主犯が tiny: `hz3_malloc` 側の TLS/offset/分岐形状（S28-3を拡張）
- 主犯が tail: refill/central/segment供給（S26/S25系列）
- 主犯が mixed 固定費: `hz3_free` の PTAG32 lookup / local split の形状（S17/S18系列）

