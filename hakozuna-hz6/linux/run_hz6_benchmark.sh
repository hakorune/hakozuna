#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="auto"
RUNS=5
OUTDIR="${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_benchmark_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0

LOCAL_ITERS=1000000
REMOTE_ITERS=200000
REUSE_ITERS=100000

LOCAL_SIZES="8192,65536"
REMOTE_SIZES="131072"
REUSE_SIZES="131072"

LOCAL_PROFILES="strict,speed,rss,remote"
REMOTE_PROFILES="speed,rss,remote"
REUSE_PROFILES="speed,rss,remote"

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_hz6_benchmark.sh [options]

Options:
  --arch <arch>           override detected arch (default: auto)
  --runs N                runs per mode/profile/size (default: 5)
  --outdir DIR            output directory
  --skip-build            reuse existing benchmark binary
  --local-iters N         iterations for local mode (default: 1000000)
  --remote-iters N        iterations for remote mode (default: 200000)
  --reuse-iters N         iterations for reuse mode (default: 100000)
  --local-sizes LIST      comma-separated local sizes (default: 8192,65536)
  --remote-sizes LIST     comma-separated remote sizes (default: 131072)
  --reuse-sizes LIST      comma-separated reuse sizes (default: 131072)
  --local-profiles LIST   comma-separated local profiles
  --remote-profiles LIST  comma-separated remote profiles
  --reuse-profiles LIST   comma-separated reuse profiles
  --help                  show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --local-iters) LOCAL_ITERS="$2"; shift 2 ;;
    --remote-iters) REMOTE_ITERS="$2"; shift 2 ;;
    --reuse-iters) REUSE_ITERS="$2"; shift 2 ;;
    --local-sizes) LOCAL_SIZES="$2"; shift 2 ;;
    --remote-sizes) REMOTE_SIZES="$2"; shift 2 ;;
    --reuse-sizes) REUSE_SIZES="$2"; shift 2 ;;
    --local-profiles) LOCAL_PROFILES="$2"; shift 2 ;;
    --remote-profiles) REMOTE_PROFILES="$2"; shift 2 ;;
    --reuse-profiles) REUSE_PROFILES="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_benchmark.sh" --arch "$ARCH"
fi

BENCH_BIN="${HZ6_BENCH_BIN:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_benchmark/hz6_allocator_bench}"
if [[ ! -x "$BENCH_BIN" ]]; then
  echo "[ERR] missing benchmark binary: $BENCH_BIN" >&2
  exit 3
fi

timestamp() {
  date +%Y%m%d_%H%M%S
}

split_list() {
  local raw="$1"
  local -n out_ref="$2"
  IFS=',' read -r -a out_ref <<< "$raw"
}

split_list "$LOCAL_SIZES" local_sizes
split_list "$REMOTE_SIZES" remote_sizes
split_list "$REUSE_SIZES" reuse_sizes
split_list "$LOCAL_PROFILES" local_profiles
split_list "$REMOTE_PROFILES" remote_profiles
split_list "$REUSE_PROFILES" reuse_profiles

{
  echo "[HZ6_BENCHMARK]"
  echo "arch=${ARCH}"
  echo "git_sha=$(git -C "${ROOT_DIR}/hakozuna-hz6" rev-parse --short HEAD 2>/dev/null || echo nogit)"
  echo "uname=$(uname -a)"
  echo "runs=${RUNS}"
  echo "local_iters=${LOCAL_ITERS}"
  echo "remote_iters=${REMOTE_ITERS}"
  echo "reuse_iters=${REUSE_ITERS}"
  echo "local_sizes=${LOCAL_SIZES}"
  echo "remote_sizes=${REMOTE_SIZES}"
  echo "reuse_sizes=${REUSE_SIZES}"
  echo "local_profiles=${LOCAL_PROFILES}"
  echo "remote_profiles=${REMOTE_PROFILES}"
  echo "reuse_profiles=${REUSE_PROFILES}"
  echo "bench_bin=${BENCH_BIN}"
  echo ""
} | tee "${OUTDIR}/README.log"

printf 'mode\tprofile\trun\tstatus\titers\tsize\tops\tops_s\tru_maxrss_kb\treuse_hits\tlog\n' > "${OUTDIR}/results.tsv"

run_case() {
  local mode="$1"
  local profile="$2"
  local run="$3"
  local iters="$4"
  local size="$5"
  local log="${OUTDIR}/${mode}_${profile}_s${size}_r${run}.log"
  local timefile="${OUTDIR}/${mode}_${profile}_s${size}_r${run}.time"
  local status=0

  /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
    "$BENCH_BIN" "$mode" "$profile" "$iters" "$size" \
    > "$log" 2>&1 || status=$?

  local summary_line ops ops_s reuse_hits rss
  summary_line="$(awk '/^allocator=/{line=$0} END{print line}' "$log")"
  ops="$(printf '%s\n' "$summary_line" | awk '{for (i=1; i<=NF; ++i) if ($i ~ /^ops=/) {sub(/^ops=/, "", $i); print $i; exit}}')"
  ops_s="$(printf '%s\n' "$summary_line" | awk '{for (i=1; i<=NF; ++i) if ($i ~ /^ops\/s=/) {sub(/^ops\/s=/, "", $i); print $i; exit}}')"
  reuse_hits="$(printf '%s\n' "$summary_line" | awk '{for (i=1; i<=NF; ++i) if ($i ~ /^reuse_hits=/) {sub(/^reuse_hits=/, "", $i); print $i; exit}}')"
  rss="$(awk -F= '/ru_maxrss_kb=/{print $2}' "$timefile" 2>/dev/null | tail -n 1)"

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$mode" "$profile" "$run" "$status" "$iters" "$size" \
    "${ops:-NA}" "${ops_s:-NA}" "${rss:-NA}" "${reuse_hits:-NA}" "$log" \
    >> "${OUTDIR}/results.tsv"
}

for run in $(seq 1 "$RUNS"); do
  for size in "${local_sizes[@]}"; do
    for profile in "${local_profiles[@]}"; do
      run_case local "$profile" "$run" "$LOCAL_ITERS" "$size"
    done
  done
  for size in "${remote_sizes[@]}"; do
    for profile in "${remote_profiles[@]}"; do
      run_case remote "$profile" "$run" "$REMOTE_ITERS" "$size"
    done
  done
  for size in "${reuse_sizes[@]}"; do
    for profile in "${reuse_profiles[@]}"; do
      run_case reuse "$profile" "$run" "$REUSE_ITERS" "$size"
    done
  done
done

echo "[DONE] ${OUTDIR}"
