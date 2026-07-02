#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_profile_local_gate}"
export RUNS="${RUNS:-3}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-60000}"

export ROWS="${ROWS:-fixed64_local0,fixed48_local0,medium_local0,main_local0,guard_local0}"
export VARIANTS="${VARIANTS:-baseline,ownerpage_ownerfast_bits,staticlocal_shadow}"

make -C "${ROOT}" \
  bench-release \
  bench-release-hz9ownerpagepool-ownerfast-bits \
  bench-release-hz9staticlocalpage-shadow >/dev/null

export HZ9_CANDIDATE_SKIP_BUILD=1
"${ROOT}/scripts/run_hz9_candidate_gate.sh" "${STAMP}"

{
  echo
  echo "## Profile-Local Read"
  echo
  echo '```text'
  echo "purpose:"
  echo "  profile/local evidence only"
  echo "  not a mixed-row default promotion gate"
  echo
  echo "default rows:"
  echo "  ${ROWS}"
  echo
  echo "default variants:"
  echo "  ${VARIANTS}"
  echo '```'
} >>"${OUTDIR}/summary.md"

echo "[hz9-profile-local-gate] summary=${OUTDIR}/summary.md"
