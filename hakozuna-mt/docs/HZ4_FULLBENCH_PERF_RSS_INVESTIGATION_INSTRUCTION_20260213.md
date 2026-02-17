# HZ4 Full Bench + Perf + RSS 調査指示書（他AI向け / 2026-02-13 rev2）

目的:
- `hz3 / hz4 / mimalloc / tcmalloc` を同一条件で比較し、`ops/s`・`RSS`・`perf` を再現可能な形で採取する。
- 以前の取り違え（.so パス、LD_PRELOAD 検証漏れ、perf 記録コマンドミス）を防ぐ。

ワンコマンド安全ランナー:
- `scripts/run_bench_full_safe.sh`
  - 順次実行（suite → mt main/guard/cross128 → perf → RSS時系列）
  - `RESUME=1` で途中再開
  - `cross128` が落ちた場合は `cross128_safe` へ自動フォールバック（既定ON）
  - `MT_CROSS128_MODE=auto`（既定）: `MemAvailable` と `SwapUsed` を見て危険時は `cross128_safe` を先に実行
    - 強制: `MT_CROSS128_MODE=full|safe|skip`
  - `cross64` レーンを追加（`MT_CROSS64_ENABLED=1` 既定ON）
  - `dist_profiles`（`app/bimodal/trimix`）を追加採取（既定ON）
  - `mimalloc-bench subset`（`cache-thrash/cache-scratch/malloc-large`）を追加採取（既定ON）
  - 任意の実アプリを `APP_CMD` で採取可能（既定OFF）
  - 実行例:
    ```bash
    OUT_BASE=/tmp/hz4_fullbench_safe_$(date +%Y%m%d_%H%M%S) \
    ./scripts/run_bench_full_safe.sh
    ```
  - 追加例（実アプリコマンド）:
    ```bash
    OUT_BASE=/tmp/hz4_fullbench_safe_$(date +%Y%m%d_%H%M%S) \
    DO_APP_CMD=1 APP_NAME=redis APP_RUNS=3 \
    APP_CMD='redis-benchmark -n 200000 -c 16 -q' \
    ./scripts/run_bench_full_safe.sh
    ```

前提:
- repo root: `/mnt/workdisk/public_share/hakmem`
- CPU pin: `taskset -c 0-15`
- date: 2026-02-13 以降

---

## 0) Fail-Fast

- `segv/assert/abort`・`timeout`・`LD_PRELOAD verify fail` が出たケースは、そこで止めてログ保全。
- すべての提出物に `.so` の `sha1` を添付。
- `.so` のパスは必ず明示指定（ツールの古いデフォルト値に依存しない）。

---

## 1) 事前準備（固定パス + ハッシュ）

```bash
cd /mnt/workdisk/public_share/hakmem

make -C hakozuna bench_mt_remote_malloc bench_malloc_args bench_malloc_dist
make -C hakozuna/hz3 clean all_ldpreload_scale
make -C hakozuna/hz4 clean all

HZ3_SO=./libhakozuna_hz3_scale.so
HZ4_SO=./hakozuna/hz4/libhakozuna_hz4.so
MI_SO=./allocators/mimalloc/libmimalloc.so
TC_SO=./allocators/tcmalloc/libtcmalloc_minimal.so

ls -l "$HZ3_SO" "$HZ4_SO" "$MI_SO" "$TC_SO"

OUT_BASE="${OUT_BASE:-/tmp/hz4_fullbench_$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$OUT_BASE"
sha1sum "$HZ3_SO" "$HZ4_SO" "$MI_SO" "$TC_SO" | tee "$OUT_BASE/dso_sha1.txt"
```

---

## 2) 全ベンチ比較（suite 一括）

`scripts/run_bench_suite_compare.sh` で `args/dist/mt(r50/r90)` を 1 回で取得。

```bash
cd /mnt/workdisk/public_share/hakmem

HZ3_SO=./libhakozuna_hz3_scale.so
MI_SO=./allocators/mimalloc/libmimalloc.so
TC_SO=./allocators/tcmalloc/libtcmalloc_minimal.so
OUT_BASE="${OUT_BASE:-/tmp/hz4_fullbench_$(date +%Y%m%d_%H%M%S)}"

RUNS=11 \
DO_RSS=1 \
DO_PERF_STAT=1 \
ALLOW_FAIL=0 \
TIMEOUT_SEC=120 \
RSS_TIMEOUT_SEC=120 \
RUN_ARGS=1 RUN_DIST=1 RUN_MT_R50=1 RUN_MT_R90=1 \
HZ4_MODE=default \
HZ3_SO="$HZ3_SO" \
MIMALLOC_SO="$MI_SO" \
TCMALLOC_SO="$TC_SO" \
OUTDIR="$OUT_BASE/suite" \
./scripts/run_bench_suite_compare.sh
```

成果物:
- `$OUT_BASE/suite/SUMMARY.tsv`
- `$OUT_BASE/suite/perf_stat_*.tsv`
- `$OUT_BASE/suite/*.rss.log`
- `$OUT_BASE/suite/preload_verify_*/ssot_header.txt`

---

## 3) MT remote 3レーン（main / guard / cross128）

`ssot_mt_remote_matrix.sh` は古いデフォルト DSO パスを持つので、必ず `--*-so` を渡す。

```bash
cd /mnt/workdisk/public_share/hakmem

HZ3_SO=./libhakozuna_hz3_scale.so
HZ4_SO=./hakozuna/hz4/libhakozuna_hz4.so
MI_SO=./allocators/mimalloc/libmimalloc.so
TC_SO=./allocators/tcmalloc/libtcmalloc_minimal.so
OUT_BASE="${OUT_BASE:-/tmp/hz4_fullbench_$(date +%Y%m%d_%H%M%S)}"

run_lane() {
  local out="$1"
  local args="$2"
  hakozuna/scripts/ssot_mt_remote_matrix.sh \
    --runs 10 --rss-runs 3 \
    --allocs hz3,hz4,mimalloc,tcmalloc \
    --hz3-so "$HZ3_SO" \
    --hz4-so "$HZ4_SO" \
    --mimalloc-so "$MI_SO" \
    --tcmalloc-so "$TC_SO" \
    --outdir "$out" \
    --bench-args "$args"
}

run_lane "$OUT_BASE/mt_main"     "16 2000000 400 16 32768 90 65536"
run_lane "$OUT_BASE/mt_guard"    "16 2000000 400 16 2048 90 65536"
run_lane "$OUT_BASE/mt_cross128" "16 2000000 400 16 131072 90 65536"
```

成果物:
- `$OUT_BASE/mt_*/summary.md`
- `$OUT_BASE/mt_*/ops.tsv`
- `$OUT_BASE/mt_*/rss.tsv`
- `$OUT_BASE/mt_*/meta.txt`

---

## 4) perf record（4 allocator）

`perf record` は `-o` を `--` より前に置く。

```bash
cd /mnt/workdisk/public_share/hakmem

HZ3_SO=./libhakozuna_hz3_scale.so
HZ4_SO=./hakozuna/hz4/libhakozuna_hz4.so
MI_SO=./allocators/mimalloc/libmimalloc.so
TC_SO=./allocators/tcmalloc/libtcmalloc_minimal.so
OUT_BASE="${OUT_BASE:-/tmp/hz4_fullbench_$(date +%Y%m%d_%H%M%S)}"
OUT_PERF="$OUT_BASE/perf_record"
mkdir -p "$OUT_PERF"

BENCH=./bench/bin/bench_random_mixed_mt_remote_malloc
ARGS=(16 2000000 400 16 32768 90 65536)

run_perf() {
  local name="$1"
  local so="${2:-}"
  local data="$OUT_PERF/${name}.data"
  local rep="$OUT_PERF/${name}.report.txt"

  if [[ -z "$so" ]]; then
    taskset -c 0-15 perf record -F 999 -g -o "$data" -- "$BENCH" "${ARGS[@]}"
  else
    env LD_PRELOAD="$so" taskset -c 0-15 perf record -F 999 -g -o "$data" -- "$BENCH" "${ARGS[@]}"
  fi
  perf report --stdio --no-children -n -i "$data" > "$rep"
}

run_perf hz3 "$HZ3_SO"
run_perf hz4 "$HZ4_SO"
run_perf mimalloc "$MI_SO"
run_perf tcmalloc "$TC_SO"
```

---

## 5) RSS 時系列（main lane）

```bash
cd /mnt/workdisk/public_share/hakmem

HZ3_SO=./libhakozuna_hz3_scale.so
HZ4_SO=./hakozuna/hz4/libhakozuna_hz4.so
MI_SO=./allocators/mimalloc/libmimalloc.so
TC_SO=./allocators/tcmalloc/libtcmalloc_minimal.so
OUT_BASE="${OUT_BASE:-/tmp/hz4_fullbench_$(date +%Y%m%d_%H%M%S)}"
TS_DIR="$OUT_BASE/rss_timeseries"
mkdir -p "$TS_DIR"

BENCH=./bench/bin/bench_random_mixed_mt_remote_malloc
ARGS=(16 2000000 400 16 32768 90 65536)

run_ts() {
  local name="$1"
  local so="$2"
  local out_csv="$TS_DIR/${name}.csv"
  OUT="$out_csv" INTERVAL_MS=50 PSS_EVERY=20 \
    ./hakozuna/hz3/scripts/measure_mem_timeseries.sh \
    env LD_PRELOAD="$so" taskset -c 0-15 "$BENCH" "${ARGS[@]}"
}

run_ts hz3 "$HZ3_SO"
run_ts hz4 "$HZ4_SO"
run_ts mimalloc "$MI_SO"
run_ts tcmalloc "$TC_SO"
```

---

## 6) 30分以上停滞したときの採取（hang triage）

```bash
cd /mnt/workdisk/public_share/hakmem
OUT_HANG="$OUT_BASE/hang_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUT_HANG"
pidof bench_random_mixed_mt_remote_malloc | tee "$OUT_HANG/pid.txt"
ps -L -p "$(cat "$OUT_HANG/pid.txt")" -o pid,tid,pcpu,stat,wchan:32,comm > "$OUT_HANG/ps_L.txt"
cat /proc/"$(cat "$OUT_HANG/pid.txt")"/maps > "$OUT_HANG/maps.txt"
timeout 5 perf top -H -p "$(cat "$OUT_HANG/pid.txt")" --stdio > "$OUT_HANG/perf_top.txt" || true
```

---

## 7) 提出物（最低限）

- `dso_sha1.txt`
- `suite/SUMMARY.tsv`
- `mt_main/summary.md`, `mt_guard/summary.md`, `mt_cross128/summary.md`
- `perf_record/*.report.txt`
- `rss_timeseries/*.csv`
- ハングがあれば `hang_*/ps_L.txt` と `hang_*/perf_top.txt`
