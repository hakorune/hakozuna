#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0
ENABLE_STATS="${ENABLE_STATS:-1}"
STATS_VALUE="${STATS_VALUE:-1}"
ENABLE_DIAGNOSTICS="${ENABLE_DIAGNOSTICS:-0}"
VARIANTS="${VARIANTS:-}"
INCLUDE_TINY="${INCLUDE_TINY:-0}"
ROWS_CSV="${ROWS:-focused}"

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_flags.sh"
source "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_midpage_payload_trim_ab_variant_flags.inc"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_midpage_payload_trim_ab.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 3)
  --iters N         iterations per run (default: 200000)
  --ws N            working set (default: 4096)
  --outdir DIR      output directory
  --skip-bench      reuse existing benchmark binary
  --stats           enable HZ6_PRELOAD_STATS during runs (default)
  --no-stats        disable HZ6_PRELOAD_STATS for speed/RSS ranking
  --stats-value VAL value for HZ6_PRELOAD_STATS when stats are enabled
  --diagnostics     build with HZ6_DIAGNOSTIC_PROBES=1
  --no-diagnostics  build without diagnostic probe counters (default)
  --variants LIST   comma-separated variants to run
                   small_boundary_trusted is an alias for
                   small_boundary_trusted_owner
  --rows LIST       row groups to run: focused,fixed (default: focused)
  --include-tiny    also run the 16..256 tiny guard row
  --help            show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --ws)
      WS="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-bench)
      SKIP_BENCH_BUILD=1
      shift
      ;;
    --stats)
      ENABLE_STATS=1
      shift
      ;;
    --no-stats)
      ENABLE_STATS=0
      shift
      ;;
    --stats-value)
      STATS_VALUE="$2"
      shift 2
      ;;
    --diagnostics)
      ENABLE_DIAGNOSTICS=1
      shift
      ;;
    --no-diagnostics)
      ENABLE_DIAGNOSTICS=0
      shift
      ;;
    --variants)
      VARIANTS="$2"
      shift 2
      ;;
    --rows)
      ROWS_CSV="$2"
      shift 2
      ;;
    --include-tiny)
      INCLUDE_TINY=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

apply_small_boundary_trusted_toy_map8192_flags() {
  local flags_name="$1"
  hz6_preload_replace_define "$flags_name" HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
  hz6_preload_replace_define "$flags_name" HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
  hz6_preload_replace_define "$flags_name" HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
  hz6_preload_replace_define "$flags_name" HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
  hz6_preload_replace_define "$flags_name" HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
  hz6_preload_replace_define "$flags_name" HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 8192
}

variant_flags() {
  local variant="$1"
  local flags=()
  hz6_preload_effective_selected_cflags flags 1
  hz6_preload_preserve_phase_counters_when_observing flags \
    "$ENABLE_STATS" "$ENABLE_DIAGNOSTICS"
  if [[ "$ENABLE_DIAGNOSTICS" -ne 0 ]]; then
    flags+=("-DHZ6_DIAGNOSTIC_PROBES=1")
    flags+=("-DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1")
  fi
  apply_midpage_payload_trim_variant_flags "$variant" flags
  hz6_preload_join_flags "${flags[@]}"
}

build_variant() {
  local variant="$1"
  local out_dir="${OUTDIR}/build/${variant}"
  OUT_DIR="$out_dir" \
  HZ6_PRELOAD_DEFAULT_CFLAGS="$(variant_flags "$variant")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${variant}_build.log" 2>&1
}

run_once() {
  local variant="$1"
  local row="$2"
  local min_size="$3"
  local max_size="$4"
  local run_id="$5"
  local so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  local log="${OUTDIR}/${row}/${run_id}_${variant}.log"
  local -a extra_env=()
  case "$variant" in
    selected_scavenge_before_rss)
      extra_env+=(HZ_BENCH_SCAVENGE_BEFORE_RSS=all)
      ;;
    selected_malloc_trim_before_rss)
      extra_env+=(HZ_BENCH_SCAVENGE_BEFORE_RSS=malloc_trim)
      ;;
    small_boundary_trusted_toy_map8192_scavenge_before_rss)
      extra_env+=(HZ_BENCH_SCAVENGE_BEFORE_RSS=all)
      ;;
    small_boundary_trusted_toy_map8192_malloc_trim_before_rss)
      extra_env+=(HZ_BENCH_SCAVENGE_BEFORE_RSS=malloc_trim)
      ;;
  esac
  mkdir -p "${OUTDIR}/${row}"
  if [[ "$ENABLE_STATS" -ne 0 ]]; then
    env "${extra_env[@]}" HZ6_PRELOAD_STATS="$STATS_VALUE" LD_PRELOAD="$so" \
      "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
      > "$log" 2>&1
  else
    env "${extra_env[@]}" LD_PRELOAD="$so" \
      "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
      > "$log" 2>&1
  fi
}

if [[ "$SKIP_BENCH_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }

default_variants=(selected run512k run256k run224k run192k run128k run320k run384k run448k run768k run1024k run1536k run1792k run1920k run1984k run2048k run2112k run2176k run2240k run2304k run2560k run3072k run4096k)
if [[ -n "$VARIANTS" ]]; then
  IFS=',' read -r -a variants <<< "$VARIANTS"
else
  variants=("${default_variants[@]}")
fi

mkdir -p "$OUTDIR"
{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "bench=${BENCH}"
  echo "stats=${ENABLE_STATS}"
  echo "stats_value=${STATS_VALUE}"
  echo "diagnostics=${ENABLE_DIAGNOSTICS}"
  echo "variants=${variants[*]}"
  echo "rows=${ROWS_CSV}"
  echo "include_tiny=${INCLUDE_TINY}"
} > "${OUTDIR}/README.log"

for variant in "${variants[@]}"; do
  build_variant "$variant"
done

rows=()
if [[ "$INCLUDE_TINY" -ne 0 ]]; then
  rows+=("16_256 16 256")
fi
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    focused)
      rows+=(
        "16_4096 16 4096"
        "1024_4096 1024 4096"
        "4096_16384 4096 16384"
      )
      ;;
    fixed)
      rows+=(
        "fixed_4k 4096 4096"
        "fixed_8k 8192 8192"
        "fixed_16k 16384 16384"
      )
      ;;
    *)
      echo "unknown row group: ${row_group}" >&2
      exit 2
      ;;
  esac
done

for row_spec in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_spec"
  for variant in "${variants[@]}"; do
    for ((run = 1; run <= RUNS; ++run)); do
      run_once "$variant" "$row" "$min_size" "$max_size" "$run"
    done
  done
done

python3 - <<'PY' "$OUTDIR" "${variants[*]}" "${rows[*]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
variants = sys.argv[2].split()
rows_blob = sys.argv[3].split()
rows = [rows_blob[i] for i in range(0, len(rows_blob), 3)]
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
current_re = re.compile(r"current_kb=([0-9]+)")
scavenge_re = re.compile(r"scavenge_released=([0-9]+)")
kv_re = re.compile(r"([A-Za-z0-9_]+)=([0-9]+)")

def parse_stats(text):
    out = {}
    for line in text.splitlines():
        if (
            line.startswith("[HZ6_PRELOAD_STATS]")
            or line.startswith("[HZ6_PRELOAD_FRONT_DETAIL]")
            or line.startswith("[HZ6_PRELOAD_PHASE_STATS]")
            or line.startswith("[HZ6_PRELOAD_HOOK_DETAIL]")
            or line.startswith("[HZ6_PRELOAD_RUNMETA_DETAIL]")
            or line.startswith("[HZ6_PRELOAD_MIDPAGE_CLASS_DETAIL]")
            or line.startswith("[HZ6_PRELOAD_PAGE_KIND_DETAIL]")
            or line.startswith("[HZ6_PRELOAD_MEMORY_ATTR]")
            or line.startswith("[HZ6_PRELOAD_SIZE_STATS]")
        ):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
    return out

print("# HZ6 MidPage Payload Trim A/B\n")
print(f"root: `{root}`\n")
readme = (root / "README.log").read_text(errors="replace")
stats_mode = readme.split("stats=", 1)[1].splitlines()[0] if "stats=" in readme else "unknown"
print(f"stats: `{stats_mode}`\n")
print("| row | variant | median ops/s | median peak MiB | median current MiB | scavenge count/result | payload MiB | active source blocks | fail | source_alloc | realloc copy | realloc same-class | realloc cross-class | realloc toy->mid | realloc mid8->mid32 | toy direct eligible | toy direct eligible 1025..4096 | toy direct enter | toy direct enter 1025..4096 | mid32_alloc | mid32_prefill | mid32_filled | mid32_front_push | toy4 fast | toy4 hit | toy4 front | toy4 pop | toy4 activate | toy4 free attempt | toy4 free success | toy4 map reg | toy4 collision | toy4 free hit | toy4 free base % | toy4 free avg probe | toy4 free max probe | runroute attempt | runroute hit | runroute prechecked | runroute fallback | retire attempt | retire scan | retire candidates | retire blocks | retire desc | retire MiB | retire blocked | retire fail | purge attempt | purge scan | purge candidates | purge blocks | purge MiB | purge blocked | purge fail | purge recommit | purge recommit fail | pagekind probe | pagekind toy | pagekind mid | pagekind mixed | pagekind toy hit | pagekind mid hit | pagekind wrong toy->mid | pagekind wrong mid->toy |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in variants:
        ops_values = []
        peak_values = []
        current_values = []
        scavenge_released = 0
        fail = 0
        source_alloc = 0
        realloc_copy = 0
        realloc_same_class = 0
        realloc_cross_class = 0
        realloc_toy_to_mid = 0
        realloc_mid8_to_mid32 = 0
        payload_mib = 0.0
        active_source_blocks = 0
        toy_direct_eligible = 0
        toy_direct_eligible_mid = 0
        toy_direct_enter = 0
        toy_direct_enter_mid = 0
        mid32_alloc = 0
        mid32_prefill = 0
        mid32_filled = 0
        mid32_front_push = 0
        toy4_fast = 0
        toy4_hit = 0
        toy4_front = 0
        toy4_pop = 0
        toy4_activate = 0
        toy4_free_attempt = 0
        toy4_free_success = 0
        toy4_map_reg = 0
        toy4_collision = 0
        toy4_free_hit = 0
        toy4_free_base = 0
        toy4_free_probe_total = 0
        toy4_free_probe_max = 0
        runroute_attempt = 0
        runroute_hit = 0
        runroute_prechecked = 0
        runroute_fallback = 0
        retire_attempt = 0
        retire_scan = 0
        retire_candidates = 0
        retire_blocks = 0
        retire_desc = 0
        retire_mib = 0.0
        retire_blocked = 0
        retire_fail = 0
        purge_attempt = 0
        purge_scan = 0
        purge_candidates = 0
        purge_blocks = 0
        purge_mib = 0.0
        purge_blocked = 0
        purge_fail = 0
        purge_recommit = 0
        purge_recommit_fail = 0
        pagekind_probe = 0
        pagekind_toy = 0
        pagekind_mid = 0
        pagekind_mixed = 0
        pagekind_toy_hit = 0
        pagekind_mid_hit = 0
        pagekind_wrong_toy_mid = 0
        pagekind_wrong_mid_toy = 0
        for log in sorted((root / row).glob(f"*_{variant}.log")):
            text = log.read_text(errors="replace")
            ops = ops_re.search(text)
            peak = peak_re.search(text)
            if ops:
                ops_values.append(float(ops.group(1)))
            if peak:
                peak_values.append(int(peak.group(1)) / 1024.0)
            current = current_re.search(text)
            if current:
                current_values.append(int(current.group(1)) / 1024.0)
            scavenge = scavenge_re.search(text)
            if scavenge:
                scavenge_released += int(scavenge.group(1))
            stats = parse_stats(text)
            fail += stats.get("alloc_fail", 0)
            fail += stats.get("route_register_fail", 0)
            fail += stats.get("source_block_exhausted", 0)
            fail += stats.get("malloc_real_fallback", 0)
            source_alloc += stats.get("source_alloc", 0)
            realloc_copy += stats.get("realloc_copy_calls", 0)
            realloc_same_class += stats.get("realloc_copy_same_class", 0)
            realloc_cross_class += stats.get("realloc_copy_cross_class", 0)
            realloc_toy_to_mid += stats.get(
                "realloc_copy_boundary_toy_to_midpage", 0
            )
            realloc_mid8_to_mid32 += stats.get(
                "realloc_copy_boundary_mid8_to_mid32", 0
            )
            payload_mib = max(
                payload_mib,
                stats.get("source_block_payload_bytes", 0) / (1024.0 * 1024.0),
            )
            active_source_blocks = max(
                active_source_blocks, stats.get("active_source_blocks", 0)
            )
            toy_direct_eligible += stats.get("malloc_toy_direct_class_eligible", 0)
            toy_direct_eligible_mid += stats.get(
                "malloc_toy_direct_class_eligible_1025_4096", 0
            )
            toy_direct_enter += stats.get("malloc_toy_direct_class_enter", 0)
            toy_direct_enter_mid += stats.get(
                "malloc_toy_direct_class_enter_1025_4096", 0
            )
            mid32_alloc += stats.get("midpage_32k_alloc_call", 0)
            mid32_prefill += stats.get("midpage_32k_prefill_run_call", 0)
            mid32_filled += stats.get("midpage_32k_prefill_run_filled", 0)
            mid32_front_push += stats.get("midpage_32k_frontcache_push", 0)
            toy4_fast += stats.get("toy_class4_malloc_fast_attempt", 0)
            toy4_hit += stats.get("toy_class4_malloc_fast_hit", 0)
            toy4_front += stats.get("toy_class4_malloc_front_dispatch", 0)
            toy4_pop += stats.get("toy_class4_malloc_frontcache_pop", 0)
            toy4_activate += stats.get("toy_class4_malloc_activate_success", 0)
            toy4_free_attempt += stats.get("toy_class4_free_cache_attempt", 0)
            toy4_free_success += stats.get("toy_class4_free_cache_success", 0)
            toy4_map_reg += stats.get("toy_class4_active_map_register", 0)
            toy4_collision += stats.get(
                "toy_class4_active_map_register_collision", 0
            )
            toy4_free_hit += stats.get("toy_class4_active_map_free_hit", 0)
            toy4_free_base += stats.get(
                "toy_class4_active_map_free_hit_base_slot", 0
            )
            toy4_free_probe_total += stats.get(
                "toy_class4_active_map_free_hit_probe_total", 0
            )
            toy4_free_probe_max = max(
                toy4_free_probe_max,
                stats.get("toy_class4_active_map_free_hit_probe_max", 0),
            )
            runroute_attempt += stats.get("free_source_run_route_attempt", 0)
            runroute_hit += stats.get("free_source_run_route_hit", 0)
            runroute_prechecked += stats.get(
                "free_source_run_route_prechecked", 0
            )
            runroute_fallback += stats.get("free_source_run_route_fallback", 0)
            retire_attempt += stats.get("midpage_32k_cold_retire_attempt", 0)
            retire_scan += stats.get("midpage_32k_cold_retire_scan_blocks", 0)
            retire_candidates += stats.get(
                "midpage_32k_cold_retire_candidate_blocks", 0
            )
            retire_blocks += stats.get("midpage_32k_cold_retire_retired_blocks", 0)
            retire_desc += stats.get(
                "midpage_32k_cold_retire_retired_descriptors", 0
            )
            retire_mib += stats.get("midpage_32k_cold_retire_retired_bytes", 0) / (
                1024.0 * 1024.0
            )
            retire_blocked += stats.get("midpage_32k_cold_retire_blocked", 0)
            retire_fail += stats.get(
                "midpage_32k_cold_retire_frontcache_remove_fail", 0
            )
            purge_attempt += stats.get("midpage_32k_cold_purge_attempt", 0)
            purge_scan += stats.get("midpage_32k_cold_purge_scan_blocks", 0)
            purge_candidates += stats.get(
                "midpage_32k_cold_purge_candidate_blocks", 0
            )
            purge_blocks += stats.get("midpage_32k_cold_purge_purged_blocks", 0)
            purge_mib += stats.get("midpage_32k_cold_purge_purged_bytes", 0) / (
                1024.0 * 1024.0
            )
            purge_blocked += stats.get("midpage_32k_cold_purge_blocked", 0)
            purge_fail += stats.get("midpage_32k_cold_purge_fail", 0)
            purge_recommit += stats.get("midpage_32k_cold_purge_recommit", 0)
            purge_recommit_fail += stats.get(
                "midpage_32k_cold_purge_recommit_fail", 0
            )
            pagekind_probe += stats.get("free_page_kind_selector_probe", 0)
            pagekind_toy += stats.get("free_page_kind_selector_toy", 0)
            pagekind_mid += stats.get("free_page_kind_selector_midpage", 0)
            pagekind_mixed += stats.get("free_page_kind_selector_mixed", 0)
            pagekind_toy_hit += stats.get("free_page_kind_selector_toy_hit", 0)
            pagekind_mid_hit += stats.get("free_page_kind_selector_midpage_hit", 0)
            pagekind_wrong_toy_mid += stats.get(
                "free_page_kind_selector_wrong_toy_page_mid_hit", 0
            )
            pagekind_wrong_mid_toy += stats.get(
                "free_page_kind_selector_wrong_midpage_page_toy_hit", 0
            )
        median_ops = statistics.median(ops_values) if ops_values else 0.0
        median_peak = statistics.median(peak_values) if peak_values else 0.0
        median_current = statistics.median(current_values) if current_values else 0.0
        toy4_free_base_pct = (
            (100.0 * toy4_free_base / toy4_free_hit) if toy4_free_hit else 0.0
        )
        toy4_free_avg_probe = (
            (toy4_free_probe_total / toy4_free_hit) if toy4_free_hit else 0.0
        )
        print(
            f"| `{row}` | `{variant}` | {median_ops:.3f} | {median_peak:.2f} | "
            f"{median_current:.2f} | {scavenge_released} | "
            f"{payload_mib:.2f} | {active_source_blocks} | {fail} | "
            f"{source_alloc} | {realloc_copy} | {realloc_same_class} | "
            f"{realloc_cross_class} | {realloc_toy_to_mid} | "
            f"{realloc_mid8_to_mid32} | {toy_direct_eligible} | "
            f"{toy_direct_eligible_mid} | {toy_direct_enter} | "
            f"{toy_direct_enter_mid} | {mid32_alloc} | {mid32_prefill} | "
            f"{mid32_filled} | {mid32_front_push} | {toy4_fast} | "
            f"{toy4_hit} | {toy4_front} | {toy4_pop} | {toy4_activate} | "
            f"{toy4_free_attempt} | {toy4_free_success} | {toy4_map_reg} | "
            f"{toy4_collision} | {toy4_free_hit} | {toy4_free_base_pct:.1f} | "
            f"{toy4_free_avg_probe:.2f} | {toy4_free_probe_max} | "
            f"{runroute_attempt} | {runroute_hit} | {runroute_prechecked} | "
            f"{runroute_fallback} | "
            f"{retire_attempt} | "
            f"{retire_scan} | {retire_candidates} | {retire_blocks} | "
            f"{retire_desc} | {retire_mib:.2f} | {retire_blocked} | "
            f"{retire_fail} | {purge_attempt} | {purge_scan} | "
            f"{purge_candidates} | {purge_blocks} | {purge_mib:.2f} | "
            f"{purge_blocked} | {purge_fail} | {purge_recommit} | "
            f"{purge_recommit_fail} | {pagekind_probe} | {pagekind_toy} | "
            f"{pagekind_mid} | {pagekind_mixed} | {pagekind_toy_hit} | "
            f"{pagekind_mid_hit} | {pagekind_wrong_toy_mid} | "
            f"{pagekind_wrong_mid_toy} |"
        )
PY
