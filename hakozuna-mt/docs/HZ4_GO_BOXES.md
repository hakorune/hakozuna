# HZ4 GO Boxes（採用/推奨 台帳）

このドキュメントは、`hz4` で **運用上の“正”**として扱う default/lane を一覧化します。

- NO-GO（アーカイブ）: `hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md`
- 現在の採用状態（SSOT）: `CURRENT_TASK.md`
- 短縮ステータス: `hakozuna/hz4/docs/HZ4_STATUS_SHORT.md`
- Phase22 NO-GO まとめ: `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_SUMMARY.md`
- フラグ詳細（索引）: `hakozuna/docs/BUILD_FLAGS_INDEX.md`
- 実務向け knob 早見表: `hakozuna/hz4/docs/HZ4_KNOB_QUICK_REFERENCE.md`

最終同期: 2026-02-16（B70 default反映, `CURRENT_TASK.md` 準拠）

## 2026-02-14 Baseline Lock（fullbench safe）

- Source: `/tmp/hz4_fullbench_safe_20260214_002029/REPORT.md`
- Strength (locked):
  - `mt_main`: `61.621M`（vs `tcmalloc 54.142M`）
  - `mt_guard`: `102.616M`（vs `hz3 95.811M`）
  - `mt_cross64`: `53.994M`（vs `tcmalloc 22.955M`）
  - RSS peak: `529,760KB`（4 allocator 中で最小）
- Weakness (locked):
  - `args(st)`: `14.314M`（vs `tcmalloc 82.097M`）
  - `dist(st)`: `5.956M`（vs `tcmalloc 80.991M`）
  - `dist_profiles`: `app 6.073M`, `trimix_mediumhot 4.299M`
  - `mt_cross128_safe`: `7.746M`（vs `mimalloc 43.620M`）
- Phase16 ST lane A/B（2026-02-14, RUNS=9 screen）:
  - `args(st)`: `14.940M -> 33.453M`（`+123.92%`）
  - `dist(st)`: `6.176M -> 46.803M`（`+657.83%`）
  - knobs: `HZ4_ST_ROUTE_FASTBOX=1`, `HZ4_ST_REFILL_DIRECT_BOX=1`, `HZ4_ST_REFILL_DIRECT_SC_MAX=64`
  - artifacts: `/tmp/hz4_phase16_st_base_20260214_011855`, `/tmp/hz4_phase16_st_var_20260214_011904`
- Next attack order (fixed):
  1. ST/dist route cost reduction（Phase16 ST/dist Route FastBox）
  2. large-path uplift for `cross128_safe` / `malloc-large`
  3. MT lanes (`main/guard/cross64`) の維持

## 判定の用語

- **GO(default)**: 既定ビルドで有効（`hz4_config_*.h` の default ON）。
- **GO(lane)**: lane/preset として提供（用途依存で既定には入れない）。
- **GO(opt-in)**: 既定OFF。A/B 用に `-D...=1` で有効化。

---

## GO(default knobs)

| Knob | Default | 意図 | Source |
|---|---:|---|---|
| `HZ4_REMOTE_FLUSH_UNROLL` | `1` | remote flush の固定費削減 | `hakozuna/hz4/core/hz4_config_remote.h` |
| `HZ4_RBUF_KEY` | `1` | remote flush で owner/sc の再decodeを削減 | `hakozuna/hz4/core/hz4_config_remote.h` |
| `HZ4_LARGE_LOCK_SHARD_LAYER` | `1` | large front cache の lock-shard 層を有効化 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_LOCK_SHARD_SHARDS` | `8` | lock-shard 層の shard 数 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_LOCK_SHARD_SLOTS` | `2` | lock-shard 層の slot 数 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_LOCK_SHARD_MIN_PAGES` | `2` | lock-shard 適用下限（128KB帯） | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_LOCK_SHARD_MAX_PAGES` | `4` | lock-shard 適用上限（256KB帯まで） | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_PAGEBIN_LOCKLESS_BOX` | `1` | 64/128KB帯の lockless page-bin 前段（cross128底上げ） | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES` | `2` | page-bin 適用上限（64/128KB帯） | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS` | `4` | page-bin 1ページ帯あたり slot 数 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_S219_LARGE_SPAN_HIBAND` | `1` | 64/128KB帯の span retain を強化 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_S219_LARGE_SPAN_PAGES` | `2` | S219 の対象 pages | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_S219_LARGE_SPAN_SLOTS` | `32` | S219 の span slot 数 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE` | `2` | S219 時の lock-shard steal 幅 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_S220_LARGE_MMAP_NOALIGN` | `1` | large acquire を plain mmap 化（trim munmap削減） | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_TLS_DIRECT` | `1` | `hz4_tls_get()` を direct TLS inline 経路で実行 | `hakozuna/hz4/core/hz4_config_core.h` |
| `HZ4_ST_FREE_USEDDEC_RELAXED` | `1` | small local free の useddec 固定費を削減（B33） | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_PAGE_SUPPLY_RESV_BOX` | `1` | mid page create の seg lock をページ予約で償却（B70） | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES` | `8` | B70 の予約ページ数（lock取得あたり） | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX` | `auto(1/0)` | gate-offかつremote未発生時のlocal free counter更新を間引き（B40） | `hakozuna/hz4/core/hz4_config_remote.h` |
| `HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT` | `6` | B40のサンプリング間隔（1/64） | `hakozuna/hz4/core/hz4_config_remote.h` |

注記:
- `HZ4_S219_LARGE_SPAN_HIBAND=1` のとき、`HZ4_LARGE_SPAN_CACHE=1` かつ `HZ4_LARGE_SPAN_CACHE_GATEBOX=0` が config 側で自動適用される。
- `HZ4_S220_LARGE_OWNER_RETURN` は NO-GO hard-archive のため default対象外。
- `HZ4_ST_FREE_USEDDEC_RELAXED` は `HZ4_PAGE_DECOMMIT` / `HZ4_SEG_RELEASE_EMPTY` / `HZ4_CENTRAL_PAGEHEAP` が有効な構成では自動で strict useddec にフォールバックする（`hz4_tcache_free.inc` 内 gate）。
- `HZ4_LOCAL_FREE_META_TCACHE_FIRST` は B38 NO-GO で hard-archive 済み（`HZ4_ALLOW_ARCHIVED_BOXES=1` がない限り有効化不可）。
- `HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX` は `HZ4_REMOTE_PAGE_RBUF=1` かつ `HZ4_REMOTE_PAGE_RBUF_GATEBOX=1` のときのみ有効（それ以外では実行パス非活性）。

---

## GO(lane)

| Lane | Build | Output | 目的 |
|---|---|---|---|
| `mt_default` | `HZ4_LANE=mt_default RUNS=11 ./scripts/run_bench_suite_compare.sh` | `OUTDIR/libhakozuna_hz4_default.so` | MT 既定レーン（main/guard/cross の基準） |
| `st_base` | `HZ4_LANE=st_base RUNS=11 ./scripts/run_bench_suite_compare.sh` | `OUTDIR/libhakozuna_hz4_st.so` | ST 基準レーン（Phase16 前の比較基準） |
| `st_fast` | `HZ4_LANE=st_fast RUNS=11 ./scripts/run_bench_suite_compare.sh` | `OUTDIR/libhakozuna_hz4_st.so` | ST 高速レーン（Phase16 Route/Refill FastBox） |
| `three-lane dev` | `./scripts/run_hz4_three_lanes_dev.sh` | `OUT_BASE/LANE_SUMMARY.tsv` | 3レーン一括A/B（開発用の入口） |

---

## GO(opt-in)

| Knob | Default | 目的 | 備考 |
|---|---:|---|---|
| `HZ4_MID_PREFETCHED_BIN_HEAD_BOX` | `0` | mid lock-path で last-hit page hint を利用して bin scan を短縮（B37） | screen/replay: `main +2.1〜+2.9%`, `guard -0.1〜-0.6%`, `cross128 -1.4〜-0.6%`。default gate未達のため opt-in 維持 |
| `HZ4_MID_PREFETCHED_BIN_HEAD_PREV_SCAN_MAX` | `2` | hint の prev 探索上限ステップ数 | B37 補助 knob |
| `HZ4_LARGE_HOT_CACHE_LAYER` | `0` | large fail rescue 前段の hot cache 実験 | S218-B 系（既定OFF） |
| `HZ4_LARGE_FAIL_RESCUE_BOX` | `0` | large miss streak 救済（rescue）実験 | B4 周辺のみ使用 |
| `HZ4_SMALL_FREE_META_DIRECT_BOX` | `0` | small free decode を meta直読みに切替（B35） | screen結果: guard/main 微負, crossのみ正で default NO-GO |
