#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
THREADS="${THREADS:-4}"
COUNT="${COUNT:-4096}"
SIZE="${SIZE:-128}"
RUNS="${RUNS:-3}"
VARIANTS_CSV="${VARIANTS:-selected,direct,direct_stats}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_preload_phase_reuse_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILD=0

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_phase_reuse.sh [options]

Options:
  --threads N      foreign free threads (default: 4)
  --count N        phase object count (default: 4096)
  --size N         allocation size (default: 128)
  --runs N         repeat count (default: 3)
  --variants CSV   selected,direct,direct_stats (default: all)
  --outdir DIR     output directory
  --skip-build     reuse existing target and preload DSOs
  --help           show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --threads) THREADS="$2"; shift 2 ;;
    --count) COUNT="$2"; shift 2 ;;
    --size) SIZE="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --variants) VARIANTS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

mkdir -p "$OUTDIR"
TARGET="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_phase_reuse"

variants=()
IFS=',' read -r -a raw_variants <<< "$VARIANTS_CSV"
for variant in "${raw_variants[@]}"; do
  case "$variant" in
    selected|direct|direct_stats) variants+=("$variant") ;;
    "") ;;
    *) echo "unknown variant: $variant" >&2; exit 2 ;;
  esac
done

variant_requested() {
  local needle="$1"
  local variant
  for variant in "${variants[@]}"; do
    [[ "$variant" == "$needle" ]] && return 0
  done
  return 1
}

build_preload_variant() {
  local name="$1"
  shift
  local flags=()
  hz6_preload_effective_selected_cflags flags 1
  while [[ $# -gt 0 ]]; do
    hz6_preload_replace_define flags "$1" "$2"
    shift 2
  done
  OUT_DIR="${OUTDIR}/build/${name}" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags[@]}")" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${name}_build.log" 2>&1
}

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${HZ6_DIR}/linux/build_hz6_preload_phase_reuse_target.sh" \
    > "${OUTDIR}/target_build.log" 2>&1
  variant_requested selected && build_preload_variant selected
  if variant_requested direct || variant_requested direct_stats; then
    build_preload_variant direct \
      HZ6_REMOTE_PENDING_INBOX_CORE_L1 1 \
      HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1 \
      HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1 \
      HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
  fi
  if variant_requested direct_stats; then
    build_preload_variant direct_stats \
      HZ6_DIAGNOSTIC_PROBES 1 \
      HZ6_REMOTE_PENDING_INBOX_CORE_L1 1 \
      HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1 \
      HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1 \
      HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_L1 1 \
      HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 1 \
      HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
  fi
fi

[[ -x "$TARGET" ]] || { echo "missing target: $TARGET" >&2; exit 3; }

printf 'variant\trun\tstatus\tops_s\treuse_hits\tlog\n' \
  > "${OUTDIR}/results.tsv"

run_variant() {
  local variant="$1"
  local so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing preload: $so" >&2; exit 4; }
  for run in $(seq 1 "$RUNS"); do
    local log="${OUTDIR}/${variant}_r${run}.log"
    local status=0
    env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$so" \
      "$TARGET" "$THREADS" "$COUNT" "$SIZE" \
      > "$log" 2>&1 || status=$?
    local summary ops_s reuse_hits
    summary="$(awk '/^phase_reuse /{line=$0} END{print line}' "$log")"
    ops_s="$(printf '%s\n' "$summary" | awk '{for (i=1; i<=NF; ++i) if ($i ~ /^ops\/s=/) {sub(/^ops\/s=/, "", $i); print $i; exit}}')"
    reuse_hits="$(printf '%s\n' "$summary" | awk '{for (i=1; i<=NF; ++i) if ($i ~ /^reuse_hits=/) {sub(/^reuse_hits=/, "", $i); print $i; exit}}')"
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$variant" "$run" "$status" "${ops_s:-NA}" "${reuse_hits:-NA}" "$log" \
      >> "${OUTDIR}/results.tsv"
  done
}

for variant in "${variants[@]}"; do
  run_variant "$variant"
done

cat "${OUTDIR}/results.tsv"
echo "[DONE] ${OUTDIR}"
