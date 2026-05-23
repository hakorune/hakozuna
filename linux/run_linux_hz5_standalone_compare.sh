#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
RUNS=3
THREADS=1
ITERS=1000000
SIZE=""
ALIGN=""
CASES="4096:8192,8192:8192,65536:8192"
ALLOCATORS="hz5,system,hz3,hz4,mimalloc,tcmalloc"
OUTDIR="${ROOT_DIR}/private/raw-results/linux/hz5_standalone_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0
SKIP_PREPARE_ALLOCATORS=0
BUILD_HZ3_HZ4=0
HZ5_LOCAL2P=0
HZ5_LOCAL2P_FAST=0
HZ5_P11_SPEED_CORE=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_hz5_standalone_compare.sh [options]

Options:
  --arch <arch>              override detected arch (default: auto)
  --runs N                   runs per allocator
  --threads N                worker threads
  --iters N                  iterations per thread
  --cases LIST               comma-separated size:align list
                             (default: 4096:8192,8192:8192,65536:8192)
  --size N                   single allocation size; requires --align
  --align N                  single allocation alignment; requires --size
  --allocators LIST          comma-separated allocator list
                             (default: hz5,system,hz3,hz4,mimalloc,tcmalloc)
  --outdir DIR               output directory
  --skip-build               skip HZ5 standalone build
  --skip-prepare-allocators  skip mimalloc/tcmalloc cache prep
  --build-hz3-hz4            rebuild HZ3/HZ4 preload libraries before running
  --hz5-local2p              build HZ5 with --linux-local2p before comparing
  --hz5-local2p-fast         build HZ5 with --linux-local2p-fast
  --hz5-p11-speed-core       build HZ5 with --linux-p11-speed-core before comparing
  --help                     show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    --iters) ITERS="$2"; shift 2 ;;
    --cases) CASES="$2"; shift 2 ;;
    --size) SIZE="$2"; shift 2 ;;
    --align) ALIGN="$2"; shift 2 ;;
    --allocators) ALLOCATORS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-prepare-allocators) SKIP_PREPARE_ALLOCATORS=1; shift ;;
    --build-hz3-hz4) BUILD_HZ3_HZ4=1; shift ;;
    --hz5-local2p) HZ5_LOCAL2P=1; shift ;;
    --hz5-local2p-fast) HZ5_LOCAL2P=1; HZ5_LOCAL2P_FAST=1; shift ;;
    --hz5-p11-speed-core) HZ5_P11_SPEED_CORE=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

if [[ -n "$SIZE" || -n "$ALIGN" ]]; then
  if [[ -z "$SIZE" || -z "$ALIGN" ]]; then
    echo "--size and --align must be provided together" >&2
    exit 2
  fi
  CASES="${SIZE}:${ALIGN}"
fi

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  hz5_build_args=(--arch "$ARCH")
  if [[ "$HZ5_LOCAL2P_FAST" -eq 1 ]]; then
    hz5_build_args+=(--linux-local2p-fast)
  elif [[ "$HZ5_LOCAL2P" -eq 1 ]]; then
    hz5_build_args+=(--linux-local2p)
  fi
  if [[ "$HZ5_P11_SPEED_CORE" -eq 1 ]]; then
    hz5_build_args+=(--linux-p11-speed-core)
  fi
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" "${hz5_build_args[@]}"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH"
fi

if [[ "$BUILD_HZ3_HZ4" -eq 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  env_file="$(mktemp)"
  trap 'rm -f "$env_file"' EXIT
  "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" > "$env_file"
  # shellcheck disable=SC1090
  source "$env_file"
fi

HZ5_BENCH="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}/bench_hz5_standalone_aligned64k"
GENERIC_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"
HZ5_LIB="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}/libhakozuna_hz5_standalone.so"
HZ3_SO="${HZ3_SO:-${ROOT_DIR}/libhakozuna_hz3_scale.so}"
HZ4_SO="${HZ4_SO:-${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.so}"
find_first_allocator_so() {
  local pattern="$1"
  shift
  local dir
  for dir in "$@"; do
    [[ -d "$dir" ]] || continue
    find "$dir" -type f -name "$pattern" 2>/dev/null | sort | head -n 1
  done | head -n 1
}

MIMALLOC_SO="${MIMALLOC_SO:-$(find_first_allocator_so 'libmimalloc.so*' \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libmimalloc2.0")}"
TCMALLOC_SO="${TCMALLOC_SO:-$(find_first_allocator_so 'libtcmalloc_minimal.so*' \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libtcmalloc-minimal4" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libtcmalloc-minimal4t64" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libgoogle-perftools4" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libgoogle-perftools4t64")}"

require_file() {
  local label="$1"
  local path="$2"
  if [[ -z "$path" || ! -f "$path" ]]; then
    echo "[ERR] missing ${label}: ${path:-unset}" >&2
    return 2
  fi
}

IFS=',' read -r -a allocator_list <<< "$ALLOCATORS"
IFS=',' read -r -a case_list <<< "$CASES"

for alloc in "${allocator_list[@]}"; do
  case "$alloc" in
    hz5) require_file hz5 "$HZ5_BENCH"; require_file hz5-lib "$HZ5_LIB" ;;
    system) require_file system-bench "$GENERIC_BENCH" ;;
    hz3) require_file hz3 "$HZ3_SO"; require_file hz3-bench "$GENERIC_BENCH" ;;
    hz4) require_file hz4 "$HZ4_SO"; require_file hz4-bench "$GENERIC_BENCH" ;;
    mimalloc) require_file mimalloc "$MIMALLOC_SO"; require_file mimalloc-bench "$GENERIC_BENCH" ;;
    tcmalloc) require_file tcmalloc "$TCMALLOC_SO"; require_file tcmalloc-bench "$GENERIC_BENCH" ;;
    *) echo "[ERR] unknown allocator: $alloc" >&2; exit 2 ;;
  esac
done

{
  echo "[HZ5_STANDALONE_COMPARE]"
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "threads=${THREADS}"
  echo "iters=${ITERS}"
  echo "cases=${CASES}"
  echo "allocators=${ALLOCATORS}"
  echo "build_hz3_hz4=${BUILD_HZ3_HZ4}"
  echo "hz5_local2p=${HZ5_LOCAL2P}"
  echo "hz5_local2p_fast=${HZ5_LOCAL2P_FAST}"
  echo "hz5_p11_speed_core=${HZ5_P11_SPEED_CORE}"
  echo "hz5_lib=${HZ5_LIB}"
  echo "hz5_bench=${HZ5_BENCH}"
  echo "generic_bench=${GENERIC_BENCH}"
  echo "hz3=${HZ3_SO}"
  echo "hz4=${HZ4_SO}"
  echo "mimalloc=${MIMALLOC_SO:-missing}"
  echo "tcmalloc=${TCMALLOC_SO:-missing}"
  echo ""
} | tee "${OUTDIR}/README.log"

printf 'case\talloc\trun\tstatus\tthreads\titers\tsize\talign\tops_s\tru_maxrss_kb\tlog\n' > "${OUTDIR}/results.tsv"

run_case() {
  local alloc="$1"
  local run="$2"
  local size="$3"
  local align="$4"
  local case_name="s${size}_a${align}"
  local log="${OUTDIR}/${case_name}_${run}_${alloc}.log"
  local timefile="${OUTDIR}/${case_name}_${run}_${alloc}.time"
  local status=0
  {
    echo "[CASE] alloc=${alloc}"
    echo "[CASE] run=${run}"
    echo "[CASE] threads=${THREADS} iters=${ITERS} size=${size} align=${align}"
    echo ""
  } | tee "$log"
  case "$alloc" in
    hz5)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$HZ5_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    system)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$GENERIC_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    hz3)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        env LD_PRELOAD="$HZ3_SO" \
        "$GENERIC_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    hz4)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        env LD_PRELOAD="$HZ4_SO" \
        "$GENERIC_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    mimalloc)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        env LD_PRELOAD="$MIMALLOC_SO" \
        "$GENERIC_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    tcmalloc)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        env LD_PRELOAD="$TCMALLOC_SO" \
        "$GENERIC_BENCH" "$THREADS" "$ITERS" "$size" "$align" \
        >> "$log" 2>&1 || status=$?
      ;;
    *)
      echo "unknown alloc: $alloc" >&2
      return 2
      ;;
  esac
  local ops_s rss
  ops_s="$(awk -F'ops/s=' '/ops\/s=/{value=$2} END{gsub(/[[:space:]].*/, "", value); print value}' "$log")"
  rss="$(awk -F= '/ru_maxrss_kb=/{print $2}' "$timefile" 2>/dev/null | tail -n 1)"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$case_name" "$alloc" "$run" "$status" "$THREADS" "$ITERS" "$size" \
    "$align" "${ops_s:-NA}" "${rss:-NA}" "$log" \
    >> "${OUTDIR}/results.tsv"
}

for case_spec in "${case_list[@]}"; do
  if [[ "$case_spec" != *:* ]]; then
    echo "[ERR] invalid case spec: ${case_spec}; expected size:align" >&2
    exit 2
  fi
  case_size="${case_spec%%:*}"
  case_align="${case_spec##*:}"
  for run in $(seq 1 "$RUNS"); do
    for alloc in "${allocator_list[@]}"; do
      run_case "$alloc" "$run" "$case_size" "$case_align"
    done
  done
done

awk -F'\t' '
  NR == 1 { next }
  $4 != 0 || $9 == "NA" || $9 == "" { next }
  {
    key = $1 "\t" $2 "\t" $5 "\t" $6 "\t" $7 "\t" $8
    values[key, ++count[key]] = $9 + 0
    rss_values[key, count[key]] = $10 + 0
  }
  END {
    print "case\talloc\tthreads\titers\tsize\talign\truns\tmedian_ops_s\tmedian_ru_maxrss_kb"
    for (key in count) {
      n = count[key]
      for (i = 1; i <= n; ++i) {
        sorted[i] = values[key, i]
        rss_sorted[i] = rss_values[key, i]
      }
      for (i = 1; i <= n; ++i) {
        for (j = i + 1; j <= n; ++j) {
          if (sorted[j] < sorted[i]) {
            tmp = sorted[i]
            sorted[i] = sorted[j]
            sorted[j] = tmp
          }
          if (rss_sorted[j] < rss_sorted[i]) {
            tmp = rss_sorted[i]
            rss_sorted[i] = rss_sorted[j]
            rss_sorted[j] = tmp
          }
        }
      }
      if (n % 2 == 1) {
        median = sorted[(n + 1) / 2]
        rss_median = rss_sorted[(n + 1) / 2]
      } else {
        median = (sorted[n / 2] + sorted[n / 2 + 1]) / 2
        rss_median = (rss_sorted[n / 2] + rss_sorted[n / 2 + 1]) / 2
      }
      print key "\t" n "\t" median "\t" rss_median
      for (i = 1; i <= n; ++i) {
        delete sorted[i]
        delete rss_sorted[i]
      }
    }
  }
' "${OUTDIR}/results.tsv" > "${OUTDIR}/summary.unsorted.tsv"

{
  head -n 1 "${OUTDIR}/summary.unsorted.tsv"
  tail -n +2 "${OUTDIR}/summary.unsorted.tsv" | sort
} > "${OUTDIR}/summary.tsv"

echo "[SUMMARY] ${OUTDIR}/summary.tsv"
echo "[DONE] logs saved to ${OUTDIR}"
