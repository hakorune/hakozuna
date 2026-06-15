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
  case "$variant" in
    selected)
      ;;
    selected_scavenge_before_rss)
      ;;
    selected_malloc_trim_before_rss)
      ;;
    run512k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 524288
      ;;
    run256k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 262144
      ;;
    run224k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 229376
      ;;
    run192k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 196608
      ;;
    run128k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 131072
      ;;
    run320k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 327680
      ;;
    run384k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 393216
      ;;
    run448k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 458752
      ;;
    run768k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 786432
      ;;
    run1024k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1048576
      ;;
    run1536k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1572864
      ;;
    run1792k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1835008
      ;;
    run1920k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1966080
      ;;
    run1984k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2031616
      ;;
    run2048k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2097152
      ;;
    run2112k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2162688
      ;;
    run2176k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2228224
      ;;
    run2240k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2293760
      ;;
    cold_retire)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      ;;
    cold_retire_lw1)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 1
      ;;
    cold_retire_lw256)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 256
      ;;
    cold_retire_eager)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 65536
      ;;
    cold_retire_hw1024)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER 1024
      ;;
    cold_retire_hw3072)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER 3072
      ;;
    cold_retire_scan64)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL 64
      ;;
    cold_retire_max2)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL 2
      ;;
    cold_retire_max16)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL 16
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL 256
      ;;
    current_bias_fast)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1 1
      ;;
    current_bias_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 0
      ;;
    current_bias_2x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 2
      ;;
    current_bias_4x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 4
      ;;
    page_kind_selector_dryrun)
      hz6_preload_replace_define flags HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1 1
      ;;
    page_kind_selector_first)
      hz6_preload_replace_define flags HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1 1
      ;;
    phase_count_on)
      hz6_preload_replace_define flags HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 0
      ;;
    aligned_free_skip)
      hz6_preload_replace_define flags HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1 1
      ;;
    realloc_in_place_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_IN_PLACE_L1 0
      ;;
    realloc_boundary_slack)
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      ;;
    realloc_boundary_slack_4k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1 1
      ;;
    realloc_boundary_slack_8k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 1
      ;;
    phase_count_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 1
      ;;
    preload_fast_free)
      hz6_preload_replace_define flags HZ6_PRELOAD_FAST_FREE_L1 1
      ;;
    frontcache8192)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_BIN_CAPACITY 8192
      ;;
    storage_trim_c4_8192)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 8192
      ;;
    storage_trim_c4_8192_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 8192
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    storage_trim_c4_12288_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 12288
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    storage_trim_c4_16384_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 16384
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    boundary_inline)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1 0
      ;;
    boundary_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1 0
      ;;
    opt_o3)
      flags+=("-O3")
      ;;
    no_semantic_interposition)
      flags+=("-fno-semantic-interposition")
      ;;
    opt_o3_no_semantic_interposition)
      flags+=("-O3" "-fno-semantic-interposition")
      ;;
    boundary_min8k)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES 8192
      ;;
    boundary_min16k)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES 16384
      ;;
    midpage_direct_class)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1 1
      ;;
    midpage_boundary_fused)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 1
      ;;
    raw_frontcache_pop)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1 1
      ;;
    midpage_reuse_trusted_class)
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 1
      ;;
    midpage_reuse_trusted_class5)
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS 5
      ;;
    raw_frontcache_push)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      ;;
    raw_frontcache_push_class4)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS 4
      ;;
    raw_frontcache_push_class5)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS 5
      ;;
    direct_max5)
      hz6_preload_replace_define flags HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS 5
      ;;
    prefill_descriptor_out)
      hz6_preload_replace_define flags HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1 1
      ;;
    same_owner_fast)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      ;;
    same_owner_trusted)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      ;;
    same_owner_trusted_class4)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MAX_CLASS 4
      ;;
    same_owner_trusted_class5)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MAX_CLASS 5
      ;;
    preload_midpage_fast_free)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      ;;
    preload_midpage_fast_free_class4)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS 4
      ;;
    preload_midpage_fast_free_class5)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS 5
      ;;
    toy_free_fast)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1 1
      ;;
    toy_addr_envelope)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1 1
      ;;
    toy_preclassified_malloc)
      hz6_preload_replace_define flags HZ6_TOY_PRECLASSIFIED_MALLOC_L1 1
      ;;
    toy_direct_fast_reuse_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      ;;
    toy_trusted_default_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 0
      ;;
    preload_toy_direct_class)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      ;;
    preload_toy_direct_class_fast_reuse)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      ;;
    preload_boundary_trusted_owner)
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    preload_toy_fast_reuse_trusted_owner)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    small_boundary_target)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      ;;
    small_boundary_raw_push)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      ;;
    small_boundary_trusted_owner|small_boundary_trusted)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    small_boundary_raw_push_trusted_owner|small_boundary_fast)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    preload_toy_direct_class_max1024)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 1024
      ;;
    preload_toy_direct_class_max256)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 256
      ;;
    preload_toy_direct_class_max512)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 512
      ;;
    preload_toy_direct_class_fast_reuse_max256)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 256
      ;;
    preload_toy_direct_class_fast_reuse_max512)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 512
      ;;
    preload_toy_direct_class_max2048)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 2048
      ;;
    preload_toy_direct_class_max3072)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 3072
      ;;
    preload_toy_direct_class_min1025)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 1025
      ;;
    preload_toy_direct_class_fast_reuse_min1025)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 1025
      ;;
    preload_toy_direct_class_min2049)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 2049
      ;;
    preload_toy_direct_class_min3073)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 3073
      ;;
    runroute_dryrun_after_maps)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 0
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1 1
      ;;
    runroute_after_maps)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 0
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 1
      ;;
    toy_map64k)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 65536
      ;;
    toy_probe8)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT 8
      ;;
    toy_mask_index)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 1
      ;;
    toy_shift12_index)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 1
      ;;
    toy_shift12_mask)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 1
      ;;
    toy_prefill64)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 64
      ;;
    toy_prefill96)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 96
      ;;
    toy_prefill192)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 192
      ;;
    toy_prefill256)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 256
      ;;
    sourcerun)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      ;;
    sourcerun_sameclass)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1 1
      ;;
    sourcerun_reclaim)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1 1
      ;;
    run2304k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2359296
      ;;
    run2560k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2621440
      ;;
    run3072k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 3145728
      ;;
    run4096k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 4194304
      ;;
    *)
      echo "unknown variant: ${variant}" >&2
      exit 2
      ;;
  esac
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
print("| row | variant | median ops/s | median peak MiB | median current MiB | scavenge count/result | payload MiB | active source blocks | fail | source_alloc | realloc copy | realloc same-class | realloc cross-class | realloc toy->mid | realloc mid8->mid32 | toy direct eligible | toy direct eligible 1025..4096 | toy direct enter | toy direct enter 1025..4096 | mid32_alloc | mid32_prefill | mid32_filled | mid32_front_push | toy4 fast | toy4 hit | toy4 front | toy4 pop | toy4 activate | toy4 free attempt | toy4 free success | toy4 map reg | toy4 collision | toy4 free hit | toy4 free base % | toy4 free avg probe | toy4 free max probe | runroute attempt | runroute hit | runroute prechecked | runroute fallback | retire attempt | retire scan | retire candidates | retire blocks | retire desc | retire MiB | retire blocked | retire fail | pagekind probe | pagekind toy | pagekind mid | pagekind mixed | pagekind toy hit | pagekind mid hit | pagekind wrong toy->mid | pagekind wrong mid->toy |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
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
            f"{retire_fail} | {pagekind_probe} | {pagekind_toy} | "
            f"{pagekind_mid} | {pagekind_mixed} | {pagekind_toy_hit} | "
            f"{pagekind_mid_hit} | {pagekind_wrong_toy_mid} | "
            f"{pagekind_wrong_mid_toy} |"
        )
PY
