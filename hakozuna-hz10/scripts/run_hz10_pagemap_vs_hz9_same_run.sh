#!/usr/bin/env bash
set -euo pipefail

# Runs HZ10's own pagemap-route bench, and -- only if HZ10_EXT_ROOT points at
# a real sibling HZ9 checkout with a prebuilt route-boundary bench
# binary -- also runs that in the same script invocation, merging both
# outputs into one combined log. This is the literal "same script run, same
# machine state" reading of bench/README.md's "compare same-run HZ8/HZ9/HZ10
# where possible": zero cost when HZ10_EXT_ROOT isn't configured, and it
# never conflates hz10_pagemap_route_bench.c's in-process hash-probe
# baseline with HZ9's real number.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/hz10_bench_common.sh"

STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz10_pagemap_vs_hz9_same_run}"
ITERS="${ITERS:-20000000}"
PAGES="${PAGES:-4096}"
RUNS="${RUNS:-1}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" bench-pagemap-route >/dev/null 2>&1 || {
  echo "[hz10-same-run] building hz10_pagemap_route_bench" >&2
  make -C "${ROOT}" bench-pagemap-route
}

log="${OUTDIR}/combined.log"
{
  echo "# HZ10 pagemap-route vs HZ9 route-boundary, same-run log"
  echo "# generated ${STAMP}"
  echo
  echo "## hz10 (pagemap + in-process hash-probe baseline)"
  ITERS="${ITERS}" PAGES="${PAGES}" RUNS="${RUNS}" \
    "${ROOT}/hz10_pagemap_route_bench"
} >"${log}"

hz9_bench=""
if hz9_bench="$(hz10_bench_find_hz9_route_boundary_bench)"; then
  {
    echo
    echo "## hz9 (${hz9_bench})"
    echo "# HZ10_EXT_ROOT=${HZ10_EXT_ROOT}"
    MODE="${HZ9_BENCH_MODE:-}" CLASS_ID="${HZ9_BENCH_CLASS_ID:-5}" \
      ITERS="${ITERS}" TOUCH="${HZ9_BENCH_TOUCH:-1}" "${hz9_bench}"
  } >>"${log}"
else
  {
    echo
    echo "## hz9: not run (HZ10_EXT_ROOT not configured or binary missing)"
    echo "# set HZ10_EXT_ROOT to a sibling HZ9 checkout with"
    echo "# h8_bench_hz9localslabrouteboundary already built, or set"
    echo "# HZ9_ROUTE_BOUNDARY_BENCH to an explicit binary path"
  } >>"${log}"
fi

cat "${log}"
echo "[hz10-same-run] wrote ${log}" >&2
