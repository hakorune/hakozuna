# S39: free path 命令数削減（hz_free / hz3_free）— Work Order

背景（Phase 97）:
- tcmalloc vs hakozuna の主因は **命令数差（+19%）**
- allocator 内の最大ホットが `hz_free`（~25%）と `hz3_free`（~18%）
- free が malloc より重い（目安 2.2x）

目的:
- free path の命令数を減らし、tcmalloc gap を詰める。
- “推測”で触らず、**perf → ターゲット1個 → A/B → rollback** を守る。

非目標:
- 正しさ（in-arena 判定や fallback）を壊す変更
- env ノブの追加（hz3 は compile-time `-D` のみ）

---

## Box Theory（箱割りと境界）

Box: **FreeHotBox**
- 役割: free hot path を “短い一本道” に固定する（range check + tag load + bin push まで）
- 境界: `free(ptr)` の入口から **fast path 決定**までを 1 箇所に集約
- ルール:
  - hot path に統計/ログ/複数の safety gate を散らさない（必要なら cold 側へ隔離）
  - A/B は compile-time フラグ 1 つだけ（rollback 即時）

---

## Step 0: 再現条件を固定（SSOT）

推奨 runner:
- hakozuna/hz3: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- 4者比較（同一バイナリ + LD_PRELOAD）: `scripts/run_bench_all_standard_swizzle.sh`

基本:
- `RUNS=30 ITERS=20000000 WS=400`（seed は runner 側に準拠）
- A/B は `HZ3_LDPRELOAD_DEFS_EXTRA` のみ（`HZ3_LDPRELOAD_DEFS` 全置換は禁止）

---

## Step 1: perf で “free の芯” を 1 個に絞る（target selection）

### 1A) hakozuna（hz_free）の top-self 確認

```bash
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD=./libhakozuna_ldpreload.so \
  perf record -g -- \
  ./system_bench_random_mixed 20000000 400 1

perf report --stdio > /tmp/hz_s39_perf_hakozuna.txt
rg -n "hz_free|hz_page|hz_dir_lookup|hz_page_index" /tmp/hz_s39_perf_hakozuna.txt | head -n 80
```

### 1B) hz3（hz3_free）の top-self 確認

```bash
make -C hakozuna/hz3 clean all_ldpreload
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  perf record -g -- \
  ./system_bench_random_mixed 20000000 400 1

perf report --stdio > /tmp/hz3_s39_perf_hz3.txt
rg -n "hz3_free|hz3_pagetag|hz3_tcache|hz3_inbox" /tmp/hz3_s39_perf_hz3.txt | head -n 80
```

完了条件:
- 「この 1 関数（または 1 ブロック）が free の最大 self%」を **1 個**に確定する。

---

## Step 2: A/B ノブ（1個）を決める

候補（例）:
1) **tag decode 効率化**（decode の lifetime を短く、無駄な load/store を除去）
2) **tcache push の分岐削減**（cap 超過/flush 判定の形を変える）
3) **remote 判定の early-exit 強化**（event-only の条件を cold 側へ隔離）

このフェーズは「まず 1 個だけ」:
- 例フラグ名: `HZ3_FREE_HOT_MINIMAL=0/1`（default 0）
- 実装範囲は `hz3_free` の箱境界 1 箇所（必要なら helper に分離して LTO で融合）

---

## Step 3: 実装（最小）→ A/B（RUNS=30）→ objdump/perf で裏取り

### 3A) A/B（hz3）

```bash
# baseline
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_FREE_HOT_MINIMAL=0" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# treatment
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_FREE_HOT_MINIMAL=1" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3B) objdump（命令形の確認）

```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so \
  | rg -n "hz3_free|branch|cmp|test|jne|je|callq" | head -n 120
```

### 3C) perf（self% が下がったか）

```bash
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  perf record -g -- \
  ./system_bench_random_mixed 20000000 400 1
perf report --stdio | rg -n "hz3_free" | head -n 40
```

GO/NO-GO（目安）:
- GO: SSOT（RUNS=30）で +1.0% 以上、かつ dist=app/uniform 退行なし
- NO-GO: 伸びない / 退行する / correctness リスクが増える

---

## Step 4: 結果固定（必須）

- `hakozuna/hz3/archive/research/s39_free_path_insn_reduction/README.md`
  - baseline/treatment の log パス
  - SSOT table（median/cv）
  - perf report の top-self（before/after）
  - rollback 手順（フラグ=0 で復帰）

