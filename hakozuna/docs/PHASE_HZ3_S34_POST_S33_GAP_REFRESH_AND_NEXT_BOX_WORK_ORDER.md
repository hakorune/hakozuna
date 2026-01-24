# S34: Post-S33 gap refresh（dist=app / uniform / tiny-only）＋次の箱を決める

Status: TODO

目的:
- S33（GO）で `hz3_free` の local bin range check（`bin < 128`）を削除し、dist=app が大きく改善した。
- 次の最適化を “推測” で選ばず、**S33後の hz3↔tcmalloc gap を SSOT と perf で固定**してから次の箱へ進む。

参照:
- S33（GO）: `hakozuna/hz3/docs/PHASE_HZ3_S33_FREE_LOCAL_BIN_RANGE_CHECK_REMOVAL_WORK_ORDER.md`
- S31（perf hotspot 入口）: `hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md`
- hz3 build/flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- SSOT lane: `hakozuna/docs/SSOT_THREE_LANES.md`

---

## 前提（SSOT/箱理論）

- allocator の挙動切替は compile-time `-D`（env ノブ禁止）。
- ベンチ導線（RUNS/ITERS/WS/BENCH_BIN 等）はスクリプト用 env のみ許可。
- 比較は同一 lane 内のみ。
- 主判定は **RUNS=30**（RUNS=10 はノイズが出やすい）。

---

## Step 1: “S33後の到達点” を SSOT で固定（RUNS=30）

### 1A) dist=app（実アプリ寄り分布, 16–32768）

```bash
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna bench_malloc_dist
make -C hakozuna/hz3 clean all_ldpreload

RUNS=30 ITERS=20000000 WS=400 \
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
BENCH_EXTRA_ARGS="app" \
TCMALLOC_SO=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

期待:
- `system/hz3/tcmalloc` の small/medium/mixed が `/tmp/hz3_ssot_${git}_${timestamp}` に保存される。

### 1B) uniform（ストレス系, 16–32768）

```bash
cd /mnt/workdisk/public_share/hakmem
RUNS=30 ITERS=20000000 WS=400 \
TCMALLOC_SO=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 1C) tiny-only（dist=app の “主戦場” 切り出し, 16–256）

`bench_random_mixed_malloc_args` を直接叩く（SSOT runner でもよいが、ここは 1ケース固定で十分）。

```bash
cd /mnt/workdisk/public_share/hakmem
for _ in $(seq 1 30); do
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
    ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
done | tee /tmp/hz3_tiny_only_hz3_$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD).log

for _ in $(seq 1 30); do
  env -u LD_PRELOAD LD_PRELOAD=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so \
    ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 256
done | tee /tmp/hz3_tiny_only_tcmalloc_$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD).log
```

---

## Step 2: perf で “次の主犯” を確定（dist=app mixed のみ）

`perf` は 1回の方向確認（RUNS=1相当）でも十分。

```bash
cd /mnt/workdisk/public_share/hakmem
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  perf stat -e cycles,instructions,branches,branch-misses \
    ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 app \
  2>&1 | tee /tmp/hz3_perf_stat_dist_app_hz3_$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD).log

env -u LD_PRELOAD LD_PRELOAD=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so \
  perf stat -e cycles,instructions,branches,branch-misses \
    ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 app \
  2>&1 | tee /tmp/hz3_perf_stat_dist_app_tcmalloc_$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD).log
```

必要なら `perf record`（重いので最後）:
- `perf record -g -- ./...` → `perf report`

---

## Step 3: 次の箱の選び方（結果で分岐）

観測結果で “次の箱” を 1つだけ選ぶ（当てずっぽう禁止）。

### A) `hz3_malloc` が重い（allocator比率が高い / insnが多い）

候補:
- S35: “bin pop の固定費（count-- / spills）” を削る（ただし S28-2B は NO-GO なので慎重に）。
- S35: “size→bin 入口” を table 化（S28-A は NO-GO、別形で）。

### B) `hz3_free` が重い（PTAG32 lookup / dst compare / local push）

候補:
- S35: “PTAG32 lookup をさらに薄くする” 方向（ロード回数を増やさない範囲で）。
- S35: “local/remote 比率が高い MT” 専用で別戦略（ただし default では ST を守る）。

### C) dist=app の gap は消えて uniform だけ残る

候補:
- “ストレス系ベンチの改善”が目的か再確認（趣味最強なら全部勝つが、優先順位を決める）。

---

## GO/NO-GO（S34は “測定フェーズ”）

S34 は最適化フェーズではなく、**次の最適化の入口を固定**するフェーズ。

完了条件:
- dist=app / uniform / tiny-only の RUNS=30 ログ（hz3 + tcmalloc）が揃っている
- perf stat ログ（hz3 + tcmalloc）が揃っている
- 次に触る “箱” が 1つに絞れている

