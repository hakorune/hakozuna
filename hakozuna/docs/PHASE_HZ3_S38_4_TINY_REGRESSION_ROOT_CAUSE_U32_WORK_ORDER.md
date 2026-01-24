# S38-4: `HZ3_BIN_COUNT_U32=1` の tiny-only 退行の原因特定（指示書）

背景:
- S38-2: `HZ3_BIN_COUNT_U32=1` は uniform（medium/mixed）を改善する一方、tiny-only（16–256）が退行。
- S38-3: `HZ3_SMALL_BIN_NOCOUNT=1` で tiny hot path から `count++/--` を消しても改善せず → **count 操作は tiny の芯ではない**。

目的:
- `HZ3_BIN_COUNT_U32=1` を維持したまま、tiny-only 退行の「芯」（支配的 self% / 供給律速 / キャッシュ律速）を特定する。
- “推測”で触らず、**観測→1ノブ→A/B** の順で進める。

前提（計測の罠回避）:
- A/B は必ず `HZ3_LDPRELOAD_DEFS_EXTRA` で差分だけ渡す。
- 基本は `RUNS=30`（seed固定）。

---

## Step 1: tiny-only 退行を SSOT で再確認（U32=0 vs 1）

tiny-only（16–256）を **hz3 単独**で回す（RUNS=30）。

```bash
make -C hakozuna bench_malloc_args

# baseline: U32=0
make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=0"
for _ in $(seq 1 30); do
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
    ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
done | tee /tmp/hz3_s38_4_tinyonly_u32_0_runs30.txt

# treatment: U32=1
make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=1"
for _ in $(seq 1 30); do
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
    ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
done | tee /tmp/hz3_s38_4_tinyonly_u32_1_runs30.txt
```

判定:
- 退行が再現しないなら「ノイズ/条件差」疑い → 先にログヘッダ（git/hash/defs）整合を点検。

---

## Step 2: slow-rate を測る（供給律速かどうか）

狙い:
- tiny-only が **hot hit** ではなく **slow path**（central/page/segment）に律速されているなら、
  `count` 系を削っても効かないのは正常。

やること:
- `HZ3_TINY_STATS=1` を有効化して tiny slow/cntrl_hit/page_alloc を atexit dump で取得する（event-only）。

```bash
# U32=0 + TINY_STATS
make -C hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=0 -DHZ3_TINY_STATS=1"
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256 \
  2> /tmp/hz3_s38_4_tiny_stats_u32_0.txt >/tmp/hz3_s38_4_tiny_u32_0.out

# U32=1 + TINY_STATS
make -C hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=1 -DHZ3_TINY_STATS=1"
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256 \
  2> /tmp/hz3_s38_4_tiny_stats_u32_1.txt >/tmp/hz3_s38_4_tiny_u32_1.out
```

見るもの:
- `[hz3] tiny totals: slow=...` が十分小さい（hot hit 優勢）か？
- `central_hit_rate/page_alloc_rate` が高いなら供給律速 → 最適化対象は hot path ではない。

---

## Step 3: perf で差分の芯を掴む（U32=0 vs 1）

狙い:
- tiny-only の self% 差分トップを 1個に絞る（「何が増えたか」）。

```bash
# U32=0
make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=0"
sudo perf record -g -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
sudo perf report --stdio > /tmp/hz3_s38_4_perf_u32_0.txt

# U32=1
make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_U32=1"
sudo perf record -g -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
sudo perf report --stdio > /tmp/hz3_s38_4_perf_u32_1.txt
```

差分抽出（軽量）:
```bash
rg -n "hz3_|Hz3" /tmp/hz3_s38_4_perf_u32_0.txt | head -n 60
rg -n "hz3_|Hz3" /tmp/hz3_s38_4_perf_u32_1.txt | head -n 60
```

---

## Step 4: “次の一手” を 1つだけ選ぶ（Box 化して A/B）

S38-4 は「原因特定」がゴール。実装は次フェーズでやる。

観測結果に応じて分岐:

1) **slow-rate 高い（供給律速）**
   - 候補: refill batch / central push/pop / page alloc（small v2）を 1ノブで動かす。
   - 例: `HZ3_SMALL_V2_REFILL_BATCH` の A/B（箱: SmallV2 Supply Box）

2) **hot hit 優勢（hot path 律速）**
   - 候補: TLS bin アクセスの依存鎖削減（箱: TCache Addressing Box）
   - 例: `HZ3_TCACHE_BANK_ROW_CACHE` / `HZ3_LOCAL_BINS_SPLIT` の A/B

3) **perf の差分トップが “予想外”**
   - その 1 シンボルだけを対象に work order を切る（境界 1 箇所、A/B ガード、ログ固定）。

---

## 完了条件（S38-4 DONE）

- U32=0/1 の tiny-only 退行が再現し、かつ perf または tiny-stats で「支配的な原因」を 1つに特定できた。
- `/tmp/hz3_s38_4_*` ログが残っている。
- アーカイブ: `hakozuna/hz3/archive/research/s38_4_u32_tiny_regression_rootcause/README.md` に結果固定。

