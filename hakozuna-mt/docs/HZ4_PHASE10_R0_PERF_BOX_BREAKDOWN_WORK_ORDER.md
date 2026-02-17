# HZ4 Phase 10: R=0 Perf “箱分解” 指示書（差分の責務を確定）

目的: hz3(p5b) に対して残っている **R=0 の差分**を、推測ではなく **perf で箱単位に確定**する。

箱理論（Box Theory）
- **箱**: `PerfBreakdownBox`（観測専用・コードは触らない）
- **境界1箇所**: `bench_random_mixed_mt_remote_malloc (R=0)` の実行だけを SSOT とする
- **戻せる**: ベンチ条件とビルド条件を固定し、ログを `/tmp` に保存
- **見える化**: `perf stat` と `perf report` のみ（常時ログ禁止）
- **Fail-Fast**: 失敗/abort はそのまま露出（握りつぶさない）

---

## 0) 前提（SSOT固定）

- repo: `/mnt/workdisk/public_share/hakmem`
- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 既定条件（R=0）:
  - `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`
- lane: **perf lane**（RSS lane ではない）
  - `HZ4_PAGE_META_SEPARATE=0`
  - `HZ4_PAGE_DECOMMIT=0`

---

## 1) ビルド（perf lane 固定）

```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
```

確認（任意）:
```sh
./hakozuna/hz4/out/hz4_basic_test
```

---

## 2) “同一条件の素振り” を 3 回（速度の揺れ確認）

ログ置き場:
```sh
OUT=/tmp/hz4_phase10_r0_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
```

CPU pinning（推奨: T=16 なので 16 core に固定。環境に合わせて変更）:
```sh
PIN="taskset -c 0-15"
```

素振り（3回）:
```sh
for i in 1 2 3; do
  env -i PATH="$PATH" HOME="$HOME" \
    LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
    $PIN ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 | tee "$OUT/run_${i}.txt"
done
```

---

## 3) perf stat（R=0: 反復 5 回）

狙い:
- **instructions/cycle（IPC）**
- **L1 miss / cache miss**
- **branch miss**

```sh
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  $PIN perf stat -r 5 \
    -e cycles,instructions,branches,branch-misses,cache-misses,LLC-load-misses,L1-dcache-load-misses \
    -- ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
  2> "$OUT/perf_stat_r0.txt"
```

比較用（system malloc / R=0）:
```sh
env -i PATH="$PATH" HOME="$HOME" \
  $PIN perf stat -r 5 \
    -e cycles,instructions,branches,branch-misses,cache-misses,LLC-load-misses,L1-dcache-load-misses \
    -- ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
  2> "$OUT/perf_stat_r0_sys.txt"
```

---

## 4) perf record（R=0: callgraph）

```sh
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  $PIN perf record -g -F 999 -o "$OUT/perf_r0.data" -- \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
  >/dev/null
```

レポート（上位だけ保存）:
```sh
perf report -i "$OUT/perf_r0.data" --stdio --no-children | head -n 80 | tee "$OUT/perf_report_r0_top.txt"
```

“箱単位” の見るべき関数（上位に居るか確認）:
- `hz4_small_malloc`
- `hz4_refill*` / `hz4_collect*`
- `hz4_populate_page*`
- `hz4_tcache_push` / `hz4_tcache_pop`
- `hz4_remote_flush*`（R=0 では基本 0 に寄るはず）
- `hz4_mid_*` / `hz4_large_*`（R=0 の mixed で出ていたら要注意）

必要ならピンポイント annotate:
```sh
perf annotate -i "$OUT/perf_r0.data" --stdio --symbol=hz4_small_malloc | head -n 120 | tee "$OUT/annotate_small_malloc.txt"
```

---

## 5) 差分の “仮説→A/B” を 1 箱に絞る（次フェーズの入口）

Phase 10 の成果物は次の 2 つ:
1) `perf_report_r0_top.txt` の top 関数（self%）を **箱に割り当て**
2) “差分の責務箱” を 1つだけ選び、次の A/B を設計する

選び方（例）:
- `hz4_populate_page` が重い → **PopulateBox**（ただし Phase 8B は NO-GO だったので別案が必要）
- `hz4_refill/collect` が重い → **RefillBox/CollectBox**（budget/条件分岐/配列走査の最小化）
- `hz4_tcache_pop/push` が重い → **TcacheBox**（データ配置/分岐/カウンタ）

---

## 6) 安全弁（確認用）

- R=0 の次に、R=90 を 1 回だけ流して “壊してない” だけ確認:
```sh
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  $PIN ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
    16 2000000 400 16 2048 90 65536 | tee "$OUT/sanity_r90.txt"
```

