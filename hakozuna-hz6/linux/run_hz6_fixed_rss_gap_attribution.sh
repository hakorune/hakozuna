#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
ITERS="${ITERS:-120000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-fixed_mid}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_fixed_rss_gap_attribution_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_fixed_rss_gap_attribution.sh [options]

Options:
  --arch ARCH     target arch (default: x86_64)
  --iters N       iterations per row (default: 120000)
  --ws N          working set (default: 4096)
  --rows CSV      row groups: focused,fixed_mid,large_span (default: fixed_mid)
  --outdir DIR    output directory
  --skip-builds   reuse existing diagnostic preload DSOs and benchmark
  --help          show this message

This runner compares diagnostic memory attribution for selected HZ6 and the
explicit fixed-boundary external-meta-off profile. It is diagnostic-only and
does not compare external allocators.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --iters) ITERS="$2"; shift 2 ;;
    --ws) WS="$2"; shift 2 ;;
    --rows) ROWS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-builds) SKIP_BUILDS=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

mkdir -p "$OUTDIR"

SELECTED_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_diag/libhakozuna_hz6_preload.so"
EXTERNAL_META_OFF_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_diag"
EXTERNAL_META_OFF_SO="${EXTERNAL_META_OFF_DIR}/libhakozuna_hz6_preload.so"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_diag.sh"
  OUT_DIR="$EXTERNAL_META_OFF_DIR" \
  HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
  HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-} -DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_target.sh"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

[[ -f "$SELECTED_SO" ]] || { echo "missing selected diag DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$EXTERNAL_META_OFF_SO" ]] || { echo "missing external-meta-off diag DSO: $EXTERNAL_META_OFF_SO" >&2; exit 2; }

{
  echo "arch=${ARCH}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "rows=${ROWS_CSV}"
  echo "selected_so=${SELECTED_SO}"
  echo "external_meta_off_so=${EXTERNAL_META_OFF_SO}"
} > "${OUTDIR}/README.log"

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh" \
  --arch "$ARCH" \
  --iters "$ITERS" \
  --ws "$WS" \
  --rows "$ROWS_CSV" \
  --outdir "${OUTDIR}/selected_diag" \
  --preload-so "$SELECTED_SO" \
  --label selected_diag \
  --skip-build

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh" \
  --arch "$ARCH" \
  --iters "$ITERS" \
  --ws "$WS" \
  --rows "$ROWS_CSV" \
  --outdir "${OUTDIR}/external_meta_off_diag" \
  --preload-so "$EXTERNAL_META_OFF_SO" \
  --label external_meta_off_diag \
  --skip-build

{
  echo "# HZ6 Fixed RSS Gap Attribution"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "## Selected Diagnostic"
  echo
  cat "${OUTDIR}/selected_diag/summary.md"
  echo
  echo "## External Meta-Off Diagnostic"
  echo
  cat "${OUTDIR}/external_meta_off_diag/summary.md"
  echo
  echo "## Read Rules"
  echo
  echo '```text'
  echo "Use this to attribute HZ6 fixed_4k/8k residual RSS after meta-off."
  echo "It is diagnostic-only; profile/default promotion still needs production guards."
  echo '```'
} > "${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"
