#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_direct_slab_debug_probe}"
ROWS="${ROWS:-medium_local0,main_local0,medium_r50,main_r90}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-20000}"
BIN="${ROOT}/h8_bench_hz9mediumslabpage_direct_use_proof"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9mediumslabpage-direct-use-proof >/dev/null

row_args() {
  case "$1" in
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    main_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1"
      ;;
    *)
      echo "unknown row: $1" >&2
      return 1
      ;;
  esac
}

{
  echo "# HZ9 Direct Slab Debug Probe"
  echo
  echo "| row | ops/s | key counters |"
  echo "|---|---:|---|"
} >"${OUTDIR}/summary.md"

IFS=',' read -r -a row_list <<< "${ROWS}"
for row in "${row_list[@]}"; do
  log="${OUTDIR}/${row}.log"
  # shellcheck disable=SC2086
  "${BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" \
    $(row_args "${row}") >"${log}" 2>&1
  ops="$(awk '/^run=/ { split($2, a, "="); print a[2]; exit }' "${log}")"
  slab="$(grep '^h9_slab_' "${log}" | tr '\n' ' ' | sed 's/[[:space:]]*$//')"
  printf '| %s | %s | `%s` |\n' "${row}" "${ops:-0}" "${slab}" \
    >>"${OUTDIR}/summary.md"
done

cat "${OUTDIR}/summary.md"
echo "[hz9-direct-slab-debug-probe] logs=${OUTDIR}"
