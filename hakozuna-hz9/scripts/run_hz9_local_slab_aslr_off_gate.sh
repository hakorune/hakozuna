#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_local_slab_aslr_off_gate}"
BIN="${ROOT}/h8_bench_hz9localslabrouteboundary"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-100000000}"
CLASS_ID="${CLASS_ID:-5}"
TOUCH="${TOUCH:-1}"
MODES="${MODES:-routeleafcompact routeleaftrim routeleaftight publicentry publicentrynonlifo publicentrynosync publicentrynosyncnonlifo}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9localslabrouteboundary >/dev/null

ASLR_STATUS="on"
RUN_PREFIX=()
if command -v setarch >/dev/null 2>&1; then
  ARCH="$(uname -m)"
  if setarch "${ARCH}" -R true >/dev/null 2>&1; then
    RUN_PREFIX=(setarch "${ARCH}" -R)
    ASLR_STATUS="off"
  fi
fi

LOG="${OUTDIR}/runs.log"
SUMMARY="${OUTDIR}/summary.md"
: >"${LOG}"

for mode in ${MODES}; do
  for _ in $(seq 1 "${RUNS}"); do
    MODE="${mode}" CLASS_ID="${CLASS_ID}" ITERS="${ITERS}" TOUCH="${TOUCH}" \
      "${RUN_PREFIX[@]}" "${BIN}" | tee -a "${LOG}"
  done
done

python3 - "${LOG}" "${SUMMARY}" "${ASLR_STATUS}" <<'PY'
import pathlib
import re
import statistics
import sys

log = pathlib.Path(sys.argv[1])
summary = pathlib.Path(sys.argv[2])
aslr = sys.argv[3]
rows = {}
for line in log.read_text().splitlines():
    m_mode = re.search(r"mode=([^ ]+)", line)
    m_ops = re.search(r"ops_per_s=([0-9.]+)", line)
    if not (m_mode and m_ops):
        continue
    rows.setdefault(m_mode.group(1), []).append(float(m_ops.group(1)) / 1e6)

lines = [
    "# HZ9 Local Slab ASLR-Off Gate",
    "",
    "```text",
    f"aslr: {aslr}",
    f"log: {log}",
    "```",
    "",
    "| mode | n | median Mops/s | min | max |",
    "|---|---:|---:|---:|---:|",
]
for mode, vals in sorted(rows.items()):
    lines.append(
        f"| {mode} | {len(vals)} | {statistics.median(vals):.3f} | "
        f"{min(vals):.3f} | {max(vals):.3f} |"
    )
summary.write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-local-slab-aslr-off-gate] logs=${OUTDIR}"
