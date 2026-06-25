#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUT="${ROOT}/bench_results/${STAMP}_medium_residual_reaudit"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
PHASE_THREADS="${PHASE_THREADS:-2}"
PHASE_ITERS="${PHASE_ITERS:-4000}"

mkdir -p "${OUT}"

make -C "${ROOT}" bench bench-release >/dev/null

run_release() {
  local name="$1"
  shift
  "${ROOT}/h8_bench_release" --runs "${RUNS}" --threads "${THREADS}" \
    --iters "${ITERS}" "$@" > "${OUT}/${name}.txt"
}

run_phase() {
  local name="$1"
  shift
  "${ROOT}/h8_bench_release" --runs "${RUNS}" --threads "${PHASE_THREADS}" \
    --iters "${PHASE_ITERS}" "$@" > "${OUT}/${name}.txt"
}

run_debug() {
  local name="$1"
  shift
  "${ROOT}/h8_bench" --runs "${RUNS}" --threads "${THREADS}" \
    --iters "${ITERS}" "$@" > "${OUT}/${name}.txt"
}

run_release medium_local0 \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1

run_release medium_interleaved_remote50 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

run_release main_interleaved_remote90 \
  --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 \
  --live-window 4096

run_phase medium_phase_remote90 \
  --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0

run_debug debug_medium_local0 \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1 \
  --live-window 4096

run_debug debug_medium_interleaved_remote50 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 \
  --live-window 4096

python3 - "${OUT}" > "${OUT}/README.md" <<'PY'
import pathlib
import re
import sys

out = pathlib.Path(sys.argv[1])

def first(pattern, text, default=""):
    m = re.search(pattern, text)
    return m.group(1) if m else default

def fields(prefix, text):
    m = re.search(rf"^{prefix} (.+)$", text, re.MULTILINE)
    if not m:
        return {}
    result = {}
    for item in m.group(1).split():
        if "=" in item:
            k, v = item.split("=", 1)
            result[k] = v.strip(",")
    return result

print("# Medium Residual Reaudit")
print()
print("| Row | median ops/s | min ops/s | median faults | max faults | notes |")
print("|---|---:|---:|---:|---:|---|")
for path in sorted(out.glob("*.txt")):
    text = path.read_text()
    thr = re.search(r"throughput median=([0-9.]+).*?min=([0-9.]+)", text)
    faults = re.search(r"page_faults minor_median=([0-9.]+).*?minor_max=([0-9.]+)", text)
    stats = fields("medium_stats", text)
    residual = fields("medium_residual", text)
    l3 = first(r"^medium_retention_l3 (.+)$", text, "")
    note = []
    if stats.get("budget_reject", "0") != "0":
        note.append(f"budget_reject={stats['budget_reject']}")
    if stats.get("remote_collect_ms", "0.000") != "0.000":
        note.append(f"collect_ms={stats['remote_collect_ms']}")
    if residual.get("collect_slots_per_run", "0.000") != "0.000":
        note.append(f"slots/run={residual['collect_slots_per_run']}")
    if l3:
        note.append(l3)
    print(
        f"| {path.stem} | {thr.group(1) if thr else ''} | "
        f"{thr.group(2) if thr else ''} | "
        f"{faults.group(1) if faults else ''} | "
        f"{faults.group(2) if faults else ''} | "
        f"{'; '.join(note)} |"
    )

print()
print("Raw logs are stored beside this README.")
PY

echo "${OUT}"
