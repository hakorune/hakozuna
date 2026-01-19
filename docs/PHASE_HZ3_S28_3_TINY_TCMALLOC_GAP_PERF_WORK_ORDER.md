# S28-3: tiny（16–256）残差の切り分け（hz3 vs tcmalloc / SSOT+perf）

目的:
- S28-2C（local bins split）が GO した後も、tiny / dist=app / uniform で tcmalloc に **~6–9%** 残差が出るケースがある。
- 次の“箱”を選ぶために、残差が
  - `malloc` 側（bin pop / size→sc / init check）
  - `free` 側（PTAG32 lookup / dst/bin push / local/remote分岐）
  - slow/event 側（refill/flush の固定費）
  のどれに寄っているかを **perfで定量化**する。

前提:
- allocator挙動は compile-time `-D` のみ（envノブ禁止）。
- ただし **ベンチの分布や比較対象ライブラリのパス**（`BENCH_EXTRA_ARGS`, `TCMALLOC_SO` など）は env でOK（SSOTログに残る）。
- 比較のたびに SSOTログ（`/tmp/hz3_ssot_*`）を残す（推測禁止）。

参照:
- S28 Work Order: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md`
- S28-2C（GO）: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`
- SSOT runner: `hakmem/hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## 0) 比較条件（固定）

### ビルド（hz3 lane のデフォルト）

`make -C hakmem/hakozuna/hz3 all_ldpreload` の既定（`HZ3_LDPRELOAD_DEFS`）に以下が含まれていること:
- `-DHZ3_LOCAL_BINS_SPLIT=1`
- `-DHZ3_PTAG_DSTBIN_ENABLE=1`
- `-DHZ3_PTAG_DSTBIN_FASTLOOKUP=1`
- `-DHZ3_REALLOC_PTAG32=1`
- `-DHZ3_USABLE_SIZE_PTAG32=1`

確認先:
- `hakmem/hakozuna/hz3/Makefile`

### ベンチ（tiny 100% / dist=app / uniform）

- tiny 100%（trimix 16–256のみ 100%）
  - `BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist`
  - `BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0"`
- dist=app
  - `BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist`
  - `BENCH_EXTRA_ARGS="0x12345678 app"`
- uniform（従来）
  - `BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args`
  - `BENCH_EXTRA_ARGS=""`

### 比較対象ライブラリ

環境によって `libtcmalloc.so` の種類が変わるので、パスは必ず SSOT header に残す。

例:
```bash
export TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so
export MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so
```

---

## 1) SSOT（RUNS=30, seed固定）で残差を確定

```bash
cd hakmem
make -C hakozuna bench_malloc_args bench_malloc_dist

RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_args \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

チェック:
- 各 `*.log` の先頭 `[BENCH-HEADER]` に `preload=` と `lib=`（tcmallocのパス）が出ていること。

---

## 2) perf stat（tiny 100%）で “命令 vs レイテンシ” を割る

狙い:
- tinyの残差が `instructions` 差なのか、同命令数でも `cycles` が高い（依存/TLB/I$）なのかを確定。

### 2-1) まず最小セット（RUNS=1〜3）

```bash
cd hakmem
make -C hakozuna bench_malloc_dist
make -C hakozuna/hz3 all_ldpreload

BENCH=./hakozuna/out/bench_random_mixed_malloc_dist
ARGS="20000000 400 16 256 0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0"

perf stat -o /tmp/perf_hz3_tiny.txt -e \
cycles,instructions,branches,branch-misses,\
L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses,iTLB-load-misses \
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so $BENCH $ARGS

perf stat -o /tmp/perf_tcmalloc_tiny.txt -e \
cycles,instructions,branches,branch-misses,\
L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses,iTLB-load-misses \
env -u LD_PRELOAD LD_PRELOAD="$TCMALLOC_SO" $BENCH $ARGS
```

見たい方向:
- hz3 の `instructions` が多い → “命令削減/フラット化” が必要（箱の形の問題）
- hz3 の `instructions` は同等だが `cycles` が多い → “TLB/I$ or 依存チェーン” が主因（配置/局所性/ヘルパ境界）

---

## 3) perf record（tiny 100%）で “どこが支配か” を確定

```bash
perf record -o /tmp/perf_hz3_tiny.data -g --call-graph dwarf -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so $BENCH $ARGS

perf report -i /tmp/perf_hz3_tiny.data --no-children
```

チェック観点:
- `hz3_small_v2_alloc` / `hz3_malloc` / `hz3_free` のどこが最大か
- `hz3_pagetag32_lookup_fast`（range check + tag load）に偏っているか
- `hz3_tcache_ensure_init` が残っていないか（tinyで毎回飛ぶなら設計バグ）

---

## 4) 次の一手候補（結果で分岐）

### A) instructions差が主因

- “箱の境界は人間、CPU境界は最適化で消す”を活かす:
  - LTO（`-flto`）A/B
  - hot helper を `static inline` に寄せる（callが残っていないか objdump確認）

### B) cycles差（TLB/I$）が主因

- init-only / cold-only の対策を優先（hot命令増を避ける）:
  - tables（PTAG32 / bins）を連続・小さく保つ（配置/align）
  - 可能なら I$ で hot/cold を分離（`__attribute__((cold))` の整理、ただし過去に NO-GO 例あり）

### C) slow/event が主因（slow率が増えている）

- S28-3の範囲外（供給箱）として別フェーズへ:
  - small v2 の page provider / central batch / bitmap探索の amortize

---

## 成果物

- SSOTログ: `/tmp/hz3_ssot_*`
- perfログ: `/tmp/perf_hz3_tiny.txt`, `/tmp/perf_tcmalloc_tiny.txt`, `/tmp/perf_hz3_tiny.data`
- 結果まとめをこの文書末尾に追記（ログパス付き）。

---

## 結果（2026-01-02）

### perf stat (tiny 16-256, dist=app)

| Metric | hz3 | tcmalloc | 差分 |
|--------|-----|----------|------|
| ops/s | 59.6M | 61.9M | -3.7% |
| insn/op | 170.5 | 165.6 | **+2.9%** |
| cycles/op | 61.9 | 58.2 | +6.4% |
| IPC | 2.76 | 2.85 | **-3.2%** |

### perf stat (dist=app 16-32K)

| Metric | hz3 | tcmalloc | 差分 |
|--------|-----|----------|------|
| ops/s | 50.5M | 53.8M | -6.1% |
| insn/op | 171.2 | 167.4 | **+2.3%** |
| IPC | 2.34 | 2.42 | **-3.3%** |

### perf record (hz3_free hotspots)

| Address | Instruction | Sample % | 説明 |
|---------|-------------|----------|------|
| 2d44 | `cmp %fs:...,%al` | 5.90% | dst == my_shard check (S28-2C) |
| 2d0a | `mov ...,%rdx` | 5.10% | PTAG32 base load |
| 2d98 | `cmp $0x7f,%ebx` | 2.27% | bin < 128 check (S28-2C) |
| 2d7b | `addw $0x1,...` | 3.85% | count++ |

### 比較 (hz3_free vs tcmalloc delete[])

| 項目 | hz3_free | tcmalloc | 差分 |
|------|----------|----------|------|
| total samples | 9.93% | 7.61% | +2.3% |
| TLS loads | 3+ | 1 | 依存長 |
| conditionals | 2 | 1 | 分岐増 |

### 結論

**残差の内訳:**
- 命令数増 (+2-3%): helper call、S28-2C の条件分岐
- IPC低下 (-3%): PTAG32 依存チェーン、TLS多段load

**主犯: hz3_free** - tcmalloc より 2.3% 多くサンプルされる。
S28-2C で追加した `dst == my_shard` と `bin < 128` チェックが free path を重くしている。

### 次の一手候補

| 案 | 期待効果 | 備考 |
|----|---------|------|
| LTO (-flto) | 命令削減 2-3% | helper inline化 |
| bin条件削除 | IPC改善 1% | tiny専用経路必要 |
| PTAG32 flat化 | 依存削減 2% | S18-2でNO-GO済み |

**推奨: LTO** - 命令数差が主因の半分を占めるため、まずビルド最適化を試す。

ログ:
- `/tmp/perf_hz3_tiny.txt`
- `/tmp/perf_tcmalloc_tiny.txt`
- `/tmp/perf_hz3_tiny.data`
- `/tmp/perf_tc_tiny.data`
- `/tmp/perf_hz3_distapp.txt`
- `/tmp/perf_tcmalloc_distapp.txt`

