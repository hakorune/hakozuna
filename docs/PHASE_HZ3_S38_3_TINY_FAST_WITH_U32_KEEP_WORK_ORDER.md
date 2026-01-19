# S38-3: `HZ3_BIN_COUNT_U32=1` を維持したまま tiny-only を速くする（調査指示書）

Status: **NO-GO（完了）**

- 結果固定: `hakozuna/hz3/archive/research/s38_3_tiny_nocount/README.md`
- 要点: tiny hot path から `count++/--` を消しても改善しない → **count 操作は tiny のボトルネックではない**

目的:
- S38-2 で `HZ3_BIN_COUNT_U32=1` は uniform（特に medium/mixed）を改善する一方、tiny-only（16–256）が退行した。
- **uniform の改善を維持**しつつ、**tiny-only の退行を解消**（可能なら改善）できるかを A/B で判断する。

前提:
- 差分フラグは必ず `HZ3_LDPRELOAD_DEFS_EXTRA` で渡す（`HZ3_LDPRELOAD_DEFS` 全置換事故を避ける）。
- 主要比較は `RUNS=30`（seed固定）を基本。`RUNS=10` は参考のみ。

---

## Box Theory（箱割り）

- **Box A: BinCount Policy Box（今回の研究箱）**
  - 役割: 「どの size-class/bin で、count をどう保持/更新するか」を 1 箇所に閉じ込める。
  - 境界: `hz3_bin_count_{inc,dec,load,store}`（仮称）だけをホットパスが呼ぶ。
  - ルール: hot path に `if (class <= X)` を散らさない（分岐は箱の境界 1 箇所へ）。

狙い:
- `HZ3_BIN_COUNT_U32=1` のメリット（medium/mixed）を残しつつ、tiny-only の `count++/--` 固定費を落とす。

---

## Step 0: ベースライン再現（必須）

### (A) tiny-only 再現（16–256, RUNS=30）

```bash
# baseline: U32=0（参考）
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=0" \
RUNS=30 ITERS=20000000 WS=400 \
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args \
SKIP_BUILD=0 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# baseline: U32=1（今回の出発点）
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=1" \
RUNS=30 ITERS=20000000 WS=400 \
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args \
SKIP_BUILD=0 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

注意:
- `run_bench_hz3_ssot.sh` は small/medium/mixed を固定で回すので、tiny-only を直接回したい場合は **手動で min/max を指定して実行**する。

```bash
# tiny-only を「1ケースだけ」回す（hz3のみ、RUNS=30）
make -C hakozuna bench_malloc_args
make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=1"

for _ in $(seq 1 30); do
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
    ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
done | tee /tmp/hz3_tinyonly_u32_1_runs30.txt
```

### (B) uniform / dist=app の現状確認（ガード）

- uniform は S38-2 と同じ条件（small/medium/mixed）。
- dist=app は seed 固定 + RUNS=30 を維持。

---

## Step 1: perf/objdump で「tiny-only 退行の芯」を掴む（観測）

### perf（tiny-only のみ）

```bash
sudo perf record -g -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256

perf report --stdio | rg -n "hz3_|Hz3|bin|count|tcache|pop|push" | head -n 80
```

### objdump（命令形確認）

狙い:
- `bin->count--` / `bin->count++` が hot でどう生成されているか（`subw`/`subl`/依存/分岐）を把握する。

```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so | rg -n "hz3_.*bin|subw|subl|addw|addl" | head -n 120
```

---

## Step 2: 研究案（小さく積む）

### 案A（第一候補）: tiny bin だけ count を「16-bit に戻す」（split）

狙い:
- medium/mixed は `U32=1` のまま（メリット維持）
- tiny-only だけ 16-bit store に戻し、退行要因（4B store）を除去する

設計（例）:
- `HZ3_BIN_COUNT_U32=1` は維持
- `HZ3_BIN_COUNT_TINY_U16=0/1` を追加（default 0）
- `class <= 256B`（または tiny bins の範囲）だけ `uint16_t tiny_count` を別配列で保持
- hot path は `hz3_bin_count_dec(class, bin*)` を呼ぶだけ（分岐は箱の中）

GO/NO-GO（目安）:
- tiny-only（RUNS=30）で `U32=1` baseline に対して **+1% 以上**
- uniform/medium/mixed の `U32=1` の改善が **±1% 以内で維持**
- dist=app は `U32=1` baseline から **退行しない**（±0.5%）

### 案B: count 更新を lazy 化（S38-1 の横展開）

狙い:
- `count++/--` の頻度を下げ、tiny-only の RMW/依存を避ける

注意:
- correctness/挙動を変えやすいので、まずは **観測（perf/objdump）で count が芯か確認**してから着手。

---

## Step 3: A/B の取り方（SSOT）

比較軸（最低限）:
- tiny-only: 16–256（RUNS=30）
- uniform: small/medium/mixed（RUNS=10 でも可、できれば 30）
- dist=app: RUNS=30（seed固定）

比較セット:
1) baseline: `HZ3_BIN_COUNT_U32=1`（現状）
2) treatment: `HZ3_BIN_COUNT_U32=1` +（案A/案B の研究フラグ）
3) 参考: `HZ3_BIN_COUNT_U32=0`（条件付きKEEPの判断材料）

ログ固定:
- `/tmp/hz3_*` に log を残し、`hakozuna/hz3/archive/research/s38_3_*` に README で結果を固定する。
