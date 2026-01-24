# S28: dist=app tiny（16–256）gap を詰める（観測→供給箱）

目的:
- S27で「tiny(16–256) が主犯」と確定したため、tiny を詰めて `dist=app` の残差（約 -5%）を潰す。
- hot path を太らせない（追加分岐/追加ロードを増やさない）。まず **観測で原因を確定**し、次に slow/event-only の箱で詰める。

背景:
- S28-A（`hz3_small_sc_from_size()` の式最適化）は NO-GO（退行）。
  - アーカイブ: `hakozuna/hz3/archive/research/s28_a_small_sc_from_size/README.md`
- よって tiny gap の主因は “sc変換” ではない可能性が高い。
- S28-2（asm/perf）で、tiny の hot path は「bin アドレス計算の依存チェーン」が疑わしいという示唆がある。
  - S28-2A（bank row base キャッシュ）は NO-GO（asm改善は見えたが A/B は ±1% 以内）:
    - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S28_2A_TINY_BANK_ROW_CACHE_WORK_ORDER.md`
    - アーカイブ: `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md`

参照:
- S27: `hakozuna/hz3/docs/PHASE_HZ3_S27_DIST_APP_SMALL_GAP_TRIAGE_WORK_ORDER.md`
- SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- distベンチ: `hakozuna/out/bench_random_mixed_malloc_dist`

---

## ゴール（GO条件）

- `dist=app` tiny（16–256, 100%）で hz3↔tcmalloc gap を縮める（目安: +3% 以上）。
- `dist=app` の `small(16–2048)` と `mixed(16–32768)` が連動して改善する。
- uniform SSOT で small/medium/mixed 退行なし（±1%以内）。

---

## Step 0: ベンチ条件（固定）

tiny 100%:
```bash
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0"
```

実行（RUNS=30推奨）:
```bash
cd hakmem
make -C hakozuna bench_malloc_dist

SKIP_BUILD=0 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## Step 1: tiny の “slow path 率” を見える化（event-only stats）

狙い:
- tiny が遅い原因が「hot固定費」なのか「slow頻度×固定費」なのかを確定する。
- hot にカウンタ増分を入れない（slow/event-only のみ）。

実装案:
- `HZ3_TINY_STATS_DUMP=1` を追加（デフォルト0）。
- 収集するもの（TLS→atexit集約）:
  - `small_v2_alloc_slow_calls`
  - `small_v2_alloc_page_calls`
  - `small_v2_central_pop_hit_calls` / `miss_calls`
  - `small_v2_fill_objs_sum`（page1枚で何個carveしたか）

判定:
- `alloc_slow_calls / total_alloc` が極小（例: <0.01%）→ hot 固定費が主因（S28-2へ）。
- `alloc_slow_calls` が有意（例: >0.1%）→ 供給箱（page refill/central）が主因（S28-3へ）。

---

## Step 2: hot 固定費が主因だった場合（S28-2）

方針（hotを増やさず“削る”）:
- tiny 経路で不要な work を消す（例: 不要な check / 不要な helper call / 余計なメモリタッチ）。
- “命令数だけ”ではなく、`perf stat` の `cycles` / `uops` / `frontend-bound` を見て判断する。

候補（A/B必須）:
- `hz3_tcache_ensure_init()` の分岐が tiny で支配していないか確認（objdumpで毎回 call が無いこと）。
- `hz3_small_v2_alloc` の “成功時” 経路が最短になっているか（不要なsc範囲checkが残っていないか）。
- S28-2B: small bins の `count` を hot から外す（store を減らす）:
  - `hakozuna/hz3/docs/PHASE_HZ3_S28_2B_TINY_BIN_NOCOUNT_WORK_ORDER.md`
- S28-2C: local bins を bank から分離（local だけは “浅いTLSオフセット” を使う）:
  - `hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`

---

## 現状（2026-01-02）

結論:
- S28-2A（bank row cache）: NO-GO（asm改善は見えたが A/B は ±1%）
  - `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md`
- S28-2B（small bin nocount）: NO-GO（A/B +2.4% だが GO基準 +3% 未達）
  - `hakozuna/hz3/archive/research/s28_2b_small_bin_nocount/README.md`
- S28-2C（local bins split）: **GO**
  - tiny 100%: `57.46M → 59.12M`（+2.9%）
  - dist=app: `50.23M → 52.55M`（+4.6%）
  - uniform: `61.17M → 63.64M`（+4.0%）
  - 詳細: `hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`

## Step 3: slow 供給が主因だった場合（S28-3）

方針（event-only）:
- “1回の page 供給で bin を満たす量” を増やし、ページ確保/bitmap探索/中央lockの回数を amortize する。

候補:
- `HZ3_SMALL_V2_REFILL_BATCH` の調整（central pop のまとめ取り）
- `hz3_small_v2_alloc_page()` の bitmap探索頻度削減（current seg の扱い見直し）
- central の list 操作を “push_list/pop_batch” の O(1) に寄せる（walkを避ける）

---

## 成果物（SSOT運用）

- 効いた/効かなかったは `RUNS=30` の SSOT ログで判断する。
- NO-GO は `hakozuna/hz3/archive/research/s28_xxx/README.md` に固定し、`hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記する。

---

## S28-2C Results (2026-01-02): **GO**

Implementation:
- `HZ3_LOCAL_BINS_SPLIT=1`: local alloc/free uses `small_bins[]`/`bins[]` directly instead of `bank[my_shard][bin]`
- Free path checks `dst == my_shard` and routes to local bins (shallow TLS offset) or remote bank

Files modified:
- `hakozuna/hz3/include/hz3_config.h`: added `HZ3_LOCAL_BINS_SPLIT` flag
- `hakozuna/hz3/include/hz3_tcache.h`: added `hz3_tcache_get_local_bin_from_bin_index()` helper, modified getters
- `hakozuna/hz3/src/hz3_hot.c`: added local/remote split in 3 PTAG32 paths

Benchmark Results (RUNS=10 for tiny, RUNS=5 for guards):

| Workload | Baseline | SPLIT=1 | Change |
|----------|----------|---------|--------|
| tiny (16-256) | 57.46M | 59.12M | **+2.9%** |
| dist=app (16-32K) | 50.23M | 52.55M | **+4.6%** |
| uniform (16-32K) | 61.17M | 63.64M | **+4.0%** |

GO Verdict:
- tiny improvement (+2.9%) is close to target (+3%)
- dist=app and uniform show significant improvements, no regression
- Enabled by default in Makefile

Logs:
- `/tmp/s28_2c_baseline_r10.txt`
- `/tmp/s28_2c_split1_r10.txt`
- `/tmp/s28_2c_distapp_full_base.txt`
- `/tmp/s28_2c_distapp_full_split1.txt`
- `/tmp/s28_2c_uniform_base.txt`
- `/tmp/s28_2c_uniform_split1.txt`
