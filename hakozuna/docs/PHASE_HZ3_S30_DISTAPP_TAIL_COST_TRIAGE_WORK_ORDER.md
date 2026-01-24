# S30: dist=app gap の “tail cost” 分解（次に削る箱を確定する）

目的:
- S29 で dist=app の gap が最大（hz3 が tcmalloc 比で ~-8%）と確定した。
- ただし dist=app は **tail が 5%** でも 1op あたりのコストが高いと全体を支配しうる。
- ここでは **“どの tail レンジが一番高コストか”**を RUNS=30 SSOT で確定し、S31 の最適化ターゲットを 1本化する。

前提:
- `bench_random_mixed_malloc_dist` の `app` 分布は固定（80/15/5）:
  - 16–256: 80%
  - 257–2048: 15%
  - 2049–max: 5%
  - 実装: `hakozuna/bench/bench_random_mixed_malloc_dist.c`
- allocator挙動は compile-time `-D` のみ（envノブ禁止）。
- SSOT は `RUNS=30` を正とする。

---

## 0) Build（hz3 lane 既定）

```bash
cd hakmem
make -C hakozuna/hz3 clean all_ldpreload
make -C hakozuna bench_malloc_dist
```

以降のベンチは、必ず同じビルドのまま `SKIP_BUILD=1` で回す。

---

## 1) baseline 固定（dist=app）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

出力（例）:
- `/tmp/hz3_ssot_${git}_.../distapp_*.log`

---

## 2) tail-only の “レンジ別” SSOT（100% tail）

目的:
- 「app の 5% tail」がどのレンジで一番遅いかを、tail 100% で直接観測する。

共通:
- `min=16 max=32768`（bench引数は脚を揃える）
- `trimix` を使い、a/b を 0% にして c を 100% にする

### 2-1) 2049–4095（sub4k）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 0 257 2048 0 2049 4095 100" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 2-2) 4096–8191（sub8k）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 0 257 2048 0 4096 8191 100" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 2-3) 8192–16384（8–16k）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 0 257 2048 0 8192 16384 100" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 2-4) 16385–32768（16–32k）

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 0 257 2048 0 16385 32768 100" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

判定:
- この 4 本のうち「hz3 vs tcmalloc gap が最も大きい」レンジを **tail主犯**とする。

---

## 3) “app の形のまま” tailだけ差し替える（80/15/5 を固定）

目的:
- tail-only で遅いレンジが、実際に dist=app 全体をどれだけ落としているかを確認する。
- 「tail-only は遅いが 5% では支配しない」ケースを排除する。

共通:
- a/b は app 同等（80/15）
- c（tail）は 5 に固定し、レンジだけ差し替える

### 3-1) tail=2049–4095

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 2049 4095 5" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3-2) tail=4096–8191

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 4096 8191 5" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3-3) tail=8192–16384

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 8192 16384 5" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3-4) tail=16385–32768

```bash
RUNS=30 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 16385 32768 5" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

判定:
- baseline（`app`）と比べて「hz3 の throughput を一番落とす tail レンジ」が **dist=app 主犯**。

---

## 4) 出口（S30 完了条件）

S30 は “最適化” ではなく “次に削る箱を確定” フェーズ。

完了条件:
- 2章（tail-only 4本）+ 3章（app形 4本）の SSOT（RUNS=30）ログが揃っている
- 「主犯レンジ」を 1行で言える

次（S31候補）:
- 主犯が 2049–4095: **sub4kを “丸め回避” ではなく “供給の amortize（span carve）”**で再設計
- 主犯が 4096–8191: sub8k を同様に（少数クラス + span carve）
- 主犯が 16–32k: batch / central / segment 供給を優先（S26 系）

