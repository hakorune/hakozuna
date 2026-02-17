# HZ4 Phase16: ST/Dist Route FastBox Work Order

## 目的（固定）

`/tmp/hz4_fullbench_safe_20260214_002029` の基準では、`hz4` は MT で優位だが ST/dist が大きく劣後している。

- 強いレーン（維持対象）
  - `mt_main`: `61.621M`（vs `tcmalloc 54.142M`）
  - `mt_guard`: `102.616M`
  - `mt_cross64`: `53.994M`
- 弱いレーン（最優先）
  - `args(st)`: `14.314M`（vs `tcmalloc 82.097M`）
  - `dist(st)`: `5.956M`（vs `tcmalloc 80.991M`）
  - `dist_profiles`: `app 6.073M`, `trimix_smallhot 10.505M`, `trimix_mediumhot 4.299M`

Phase16 は **ST/dist の固定費削減**を先にやる。MT の勝ちレーンは守る。

---

## 箱の分割（Box Theory）

### Box 1: `STRouteFastBox`（free ルート短縮）

- 役割:
  - ST lane (`HZ4_TLS_SINGLE=1`) 限定で `hz4_small_free_with_page_tls()` の routing 固定費を削減する。
- 境界（1箇所）:
  - `hakozuna/hz4/src/hz4_tcache_free.inc`
  - `hz4_small_free_with_page_tls()` 冒頭の route 判定のみ。
- 内容:
  - `owner_tid` decode / remote gate 更新 / remote 分岐を ST 時はスキップ。
  - `sc` は page/meta から直接読む。
  - `HZ4_FAILFAST=1` 時は owner mismatch を強制検出。
- 新規ノブ（opt-in で開始）:
  - `HZ4_ST_ROUTE_FASTBOX`（default `0`）

### Box 2: `STRefillDirectBox`（small refill 境界短縮）

- 役割:
  - ST lane の refill slow path を短くし、`dist` 系の underflow コストを下げる。
- 境界（1箇所）:
  - `hakozuna/hz4/src/hz4_tcache_malloc.inc` の `hz4_small_malloc()`
  - `hakozuna/hz4/src/hz4_tcache_refill_*.inc` へ入る直前。
- 内容:
  - ST 時は `hz4_refill(...)` へ落ちる前に direct refill（既存 `HZ4_ST_SMALL_REFILL_DIRECT` 経路）を優先。
  - direct refill の対象を `sc` 条件で限定できるようにする（例: `sc<=64`）。
- 新規ノブ:
  - `HZ4_ST_REFILL_DIRECT_BOX`（default `0`）
  - `HZ4_ST_REFILL_DIRECT_SC_MAX`（default `HZ4_SC_MAX-1`）

### Box 3: `STModePresetBox`（ベンチ lane の定義固定）

- 役割:
  - `scripts/run_bench_suite_compare.sh` の ST 既定を、archived knob なしの安全 preset に固定。
- 境界（1箇所）:
  - `scripts/run_bench_suite_compare.sh` の `HZ4_MODE=st` preset 生成部のみ。
- 内容:
  - archived な `HZ4_ST_MID_ACTIVE_RUN*` は preset から除外。
  - Phase16 の ST knobs のみ構成する。

---

## 非目標（今回はやらない）

- `HZ4_MID_ACTIVE_RUN*` 復活（archive を再利用しない）
- MT remote の新規アルゴリズム追加
- RSSReturn/Decommit 系の再チューニング

---

## 実装ステップ

1. `core/hz4_config_tcache.h` に Phase16 knob を追加（default OFF）
2. `hz4_tcache_free.inc` に `STRouteFastBox` 分岐を追加
3. `hz4_tcache_malloc.inc` に `STRefillDirectBox` 分岐を追加
4. `run_bench_suite_compare.sh` の ST preset を archived-free へ更新
5. one-shot counters（`HZ4_ST_STATS_B1`）を追加して機構の通電確認

---

## 観測（最小）

- 追加カウンター（atexit one-shot, default OFF）
  - `st_free_fast_path`
  - `st_free_fallback_path`
  - `st_refill_direct_calls`
  - `st_refill_direct_hit`
  - `st_refill_to_slow`
- 出力形式:
  - `[HZ4_ST_STATS_B1] ...` 1回のみ

---

## A/B 手順（固定）

### 1) ST/dist 主要ゲート

```sh
HZ4_MODE=st RUN_MT_R50=0 RUN_MT_R90=0 RUNS=11 \
OUTDIR=/tmp/hz4_phase16_st_base \
./scripts/run_bench_suite_compare.sh

HZ4_MODE=st RUN_MT_R50=0 RUN_MT_R90=0 RUNS=11 \
HZ4_ST_DEFS_EXTRA='-DHZ4_ST_ROUTE_FASTBOX=1 -DHZ4_ST_REFILL_DIRECT_BOX=1' \
OUTDIR=/tmp/hz4_phase16_st_var \
./scripts/run_bench_suite_compare.sh
```

評価:
- `args(st)` と `dist(st)` の中央値差分

### 1.5) dist_profiles 補助ゲート

```sh
OUT_BASE=/tmp/hz4_phase16_distprofiles \
DO_SUITE=0 DO_MT=0 DO_MIMALLOC_SUBSET=0 DO_PERF=0 DO_TS=0 DO_DIST_PROFILES=1 \
./scripts/run_bench_full_safe.sh
```

評価:
- `app` / `trimix_smallhot` / `trimix_mediumhot` の中央値差分

### 2) MT 保持ゲート

```sh
OUT_BASE=/tmp/hz4_phase16_hold \
DO_DIST_PROFILES=0 DO_MIMALLOC_SUBSET=0 DO_PERF=0 DO_TS=0 \
./scripts/run_bench_full_safe.sh
```

評価:
- `mt_main >= 58.0M`
- `mt_guard >= 99.0M`
- `mt_cross64 >= 50.0M`
- `larson` quick `rc=0`

---

## GO / NO-GO 判定

- GO:
  - `args(st) >= +30%`
  - `dist(st) >= +50%`
  - `app` と `trimix_mediumhot` がともに正改善
  - MT 保持ゲートを満たす
- NO-GO:
  - ST 改善が弱い（`args +15%`未満かつ`dist +25%`未満）
  - MT 主要3レーンのいずれかが `-3%`より悪化
  - crash/abort/assert 発生

## 実行ロック（2026-02-14）

- A/B artifact:
  - base: `/tmp/hz4_phase16_st_base_20260214_011855`
  - var: `/tmp/hz4_phase16_st_var_20260214_011904`
- RUNS=9 screen:
  - `args(st)`: `14.940M -> 33.453M`（`+123.92%`）
  - `dist(st)`: `6.176M -> 46.803M`（`+657.83%`）
- dist profiles (RUNS=5 medians):
  - artifact: `/tmp/hz4_phase16_dist_profiles_ab_20260214_011943.tsv`
  - `app`: `6.275M -> 52.576M`（`+737.90%`）
  - `trimix_smallhot`: `10.806M -> 57.787M`（`+434.77%`）
  - `trimix_mediumhot`: `4.411M -> 41.653M`（`+844.24%`）
- one-shot mechanism lock:
  - `[HZ4_ST_STATS_B1] free_fast_path=7795 free_fallback_path=0 refill_direct_calls=65 refill_direct_hit=65 refill_to_slow=63`
  - log: `/tmp/hz4_phase16_stats_run.log`
- decision:
  - Phase16 screen GO（ST/dist lane）。
  - global default は変更せず、ST lane preset で採用。

---

## リスクと Fail-Fast

- リスク:
  - ST 固定最適化の混入で MT 経路を汚す
- 対策:
  - `HZ4_TLS_SINGLE` 条件 + `HZ4_ST_*` knob で明確に分離
  - default OFF で導入し、A/B 後に default 化判定
  - `HZ4_FAILFAST` で owner 整合性を即 abort

---

## 成果物

- A/B ログ:
  - `/tmp/hz4_phase16_st_base`
  - `/tmp/hz4_phase16_st_var`
  - `/tmp/hz4_phase16_hold`
- 更新ドキュメント:
  - `CURRENT_TASK.md`
  - `hakozuna/hz4/docs/HZ4_GO_BOXES.md`
  - `hakozuna/hz4/docs/HZ4_PHASE16_ST_DIST_ROUTE_FASTBOX_WORK_ORDER.md`
