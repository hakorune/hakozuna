# HZ4 Stage5-1: Remote-lite v1a2（R=90専用 lane 方針）

目的:
- MT remote の **R=90**（remote free 多）で、remote 経路の固定費を下げる候補として Remote-lite v1a2 を保持する。
- **R=50 を落とさない**（既定 lane を汚さない）。

## 何をしているか（Remote-lite v1a2）

- inbox / rbuf をバイパスし、remote free を
  - `page.remote_head[shard]`（page の MPSC remote list）
  - `pageq_notify()`（または `segq_notify()`）
 へ **直接 push + notify** する。
- 実装位置（境界）:
  - free 側: `hakozuna/hz4/core/hz4_remote_flush.inc` の `hz4_remote_free_keyed()` / `hz4_remote_free()`

notify mode:
- **transition-only notify**（empty->non-empty のときだけ pageq_notify）

制約:
- `HZ4_PAGE_META_SEPARATE=1` が必須（`HZ4_REMOTE_LITE` が ON の場合は compile-time `#error`）。
- CPH（CentralPageHeap）管理ページは PageQ と混線させない（`meta->cph_queued!=0` なら legacy inbox 経路へフォールバック）。

## SSOT 結果（2026-02-03）

Remote-lite v1a2（transition-only）A/B（RUNS=11, timeout=60, taskset=0-15, ITERS=1e6）:
- R=50: **退行の可能性あり**（既定 lane では NO-GO）
- R=90: **改善傾向**（R=90専用 lane でのみ採用）

結論:
- **既定 lane では NO-GO**（R=50 が落ちるため）。
- **R=90専用 lane（あるいは R=90専用ベンチ）でのみ** opt-in 候補として保持。

ログ（例）:
- `/tmp/rl_base_r50.txt` `/tmp/rl_opt_r50.txt` `/tmp/rl_base_r90.txt` `/tmp/rl_opt_r90.txt`

## 運用ルール（lane 方針）

1. `HZ4_REMOTE_LITE` は既定 OFF（本線 lane には入れない）
2. R=90 のみで検証する “専用 build” を作る（lane を増やすなら **R90専用 lane** として分離）
3. ベンチ実行は **必ず timeout 付き**（hang/取り違え防止）

### 例: R=90 専用ビルド（opt-in）

```sh
cd /mnt/workdisk/public_share/hakmem

common='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_REMOTE_FLUSH_THRESHOLD=64 -DHZ4_TCACHE_PREFETCH_LOCALITY=2'

make -C hakozuna/hz4 clean libhakozuna_hz4.so HZ4_DEFS_EXTRA="${common} -DHZ4_REMOTE_LITE=1"

env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so taskset -c 0-15 \
  timeout 60 ./bench/bin/bench_random_mixed_mt_remote_malloc 16 1000000 400 16 2048 90 65536
```

## 次の手

- R=50 退行が残る限り、**R=90専用 lane** でのみ扱う。
- 単一 lane 両立を狙う場合は pressure gate 等の別箱で制御する。
