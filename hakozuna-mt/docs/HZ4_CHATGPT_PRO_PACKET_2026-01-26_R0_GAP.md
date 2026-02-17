# HZ4 → ChatGPT Pro Packet（2026-01-26, R=0 gap）

目的: hz3(p5b) が hz4(perf lane) に対して **R=0 で +30.58%** 優勢の理由を、設計（箱）レベルで議論するための材料まとめ。

前提（SSOT）
- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 条件: `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`
- pinning: `taskset -c 0-15`
- 判定: **10runs median**

## 1) SSOT 10runs 結果（R=0）

logs:
- `/tmp/ssot_hz3_vs_hz4_r0_20260126_141510`

.so（固定）
- hz3(p5b): `/tmp/ssot_hz3_p5b.so`
  - sha1: `03c1a9ef1f75b482746eb6fa845bf14cf3ba66d7`
- hz4(perf): `/tmp/ssot_hz4_perf.so`
  - sha1: `9f518f32ba0c4970cab46e19405f0cdd57e6c8bb`

結果（median）
- hz3(p5b): **347.44M ops/s**（min 333.13M / max 367.50M）
- hz4(perf): **266.07M ops/s**（min 259.27M / max 273.70M）
- 差分: **hz3 +30.58%**

## 2) perf diff の成果物

logs:
- `/tmp/perf_hz3_vs_hz4_r0_20260126_151231/`
  - `hz3_top.txt`
  - `hz4_top.txt`

計測条件（重要）
- `-O2` + `-g` + `-fno-omit-frame-pointer`
- `perf record --call-graph fp`

注意
- `perf` の “%” は **各プロセス内の相対比率**なので、hz3/hz4 間で % を単純に差し引いて “主要因” と断定しない。
- 相談では「どの箱の構造差が 30% を生むか」の仮説を優先。

## 3) hz4 の R=0 hot path（対象箇所）

ファイル:
- `hakozuna/hz4/src/hz4_tcache.c`

主要関数:
- `hz4_small_malloc()`（`hz4_tls_get()` → `hz4_size_to_sc()` → `hz4_tcache_pop()` → `hz4_refill()`）
- `hz4_tcache_pop()` / `hz4_tcache_push()`
- `hz4_refill()`（`hz4_collect_list()` → `hz4_alloc_page()` → `hz4_populate_page()`）

TLS getter:
- `hakozuna/hz4/include/hz4_tls_init.h`（`HZ4_TLS_DIRECT=1`）

SizeClass:
- `hakozuna/hz4/include/hz4_sizeclass.h`（16B aligned, 128 classes）

## 4) 既知の NO-GO / GO

GO
- `HZ4_TLS_MERGE=1`（default ON）
- `HZ4_COLLECT_LIST=1`（default ON）

NO-GO（本線から削除してアーカイブ）
- Phase 8B slice refill
- Phase 8C TLS hot/cold split
- Phase 9 RouteBox head64
- Phase 11 Tcache bulk push

## 5) 相談したい問い（箱理論）

R=0 の 30% gap を詰めるために、hz4 の “哲学（page header中心）” を壊さずに取れる次の一手は何か？

候補の方向性（例）
- SizeClassBox: 128 bins のランダムアクセスが L1 を汚している可能性（bin clustering / 2-level bins / hotset cache）
- MallocBox: `hz4_size_to_sc` / `hz4_tcache_pop` の分岐・データ配置の再設計（ただし TLS hot/cold split は NO-GO）
- Workload-aware: SSOT の size 分布に対して “bin locality” を作る（last-sc cache 等）

