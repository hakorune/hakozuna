#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

ARCH="x86_64"
RUNS=10
THREADS=2
WS=100
SLOTS=65536
TIMEOUT_SEC=60
OUTDIR=""
SKIP_BUILD=0
DO_ATTRIB=1

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_hz5_midfront_observe.sh [options]

Options:
  --runs N          runs per candidate/case (default: 10)
  --threads N       benchmark threads (default: 2)
  --ws N            working set (default: 100)
  --slots N         ring slots (default: 65536)
  --timeout-sec N   timeout per run (default: 60)
  --outdir DIR      output directory
  --skip-build      reuse existing candidate builds
  --no-attrib       skip separate HZ5_PRELOAD_STATS attribution smoke

Outputs:
  raw.tsv           performance runs, no HZ5_PRELOAD_STATS
  summary.tsv       medians and failure counts
  attrib.tsv        separate stats smoke with HZ5_PRELOAD_STATS=1
  meta.txt          run configuration

Default candidates:
  rb16              broad MidFront default candidate
  allgate           mid-heavy candidate: drain-all + empty-gated exchange
  drainmask         pending-mask candidate/control
  globalrecycle     global recycle control

Important:
  Performance runs intentionally do not set HZ5_PRELOAD_STATS, so preload
  atomic counters are not mixed into ops/s measurements.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="${2:-}"; shift 2 ;;
    --threads) THREADS="${2:-}"; shift 2 ;;
    --ws) WS="${2:-}"; shift 2 ;;
    --slots) SLOTS="${2:-}"; shift 2 ;;
    --timeout-sec) TIMEOUT_SEC="${2:-}"; shift 2 ;;
    --outdir) OUTDIR="${2:-}"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --no-attrib) DO_ATTRIB=0; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

BENCH="/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc"
if [[ ! -x "${BENCH}" ]]; then
  echo "bench not executable: ${BENCH}" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUTDIR="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/midfront_observe_${TS}}"
mkdir -p "${OUTDIR}"

declare -A CAND_FLAGS
CAND_FLAGS[rb16]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16"
CAND_FLAGS[takefirst]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-take-first"
CAND_FLAGS[drainall]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-all-on-miss"
CAND_FLAGS[allgate]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-all-on-miss --linux-midfront-drain-empty-gated"
CAND_FLAGS[drainmask]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-mask-on-miss"
CAND_FLAGS[maskhitstop]="--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-mask-hit-stop"
CAND_FLAGS[globalrecycle]="--linux-midfront-owner-fast-state --linux-midfront-remote-global-recycle"

declare -a CANDIDATES=(rb16 allgate drainmask globalrecycle)

build_candidate() {
  local cand="$1"
  local out="hakozuna-hz5/out/linux/${ARCH}-hz5-midfront-observe-${cand}"
  # shellcheck disable=SC2086
  ./linux/build_linux_hz5_standalone.sh \
    ${CAND_FLAGS[$cand]} \
    --linux-local2p-speed-linkflags \
    --out-dir "${out}" >/dev/null
}

if [[ "${SKIP_BUILD}" == "0" ]]; then
  for cand in "${CANDIDATES[@]}"; do
    echo "[build] ${cand}"
    build_candidate "${cand}"
  done
fi

{
  echo "root=${ROOT_DIR}"
  echo "commit=$(git rev-parse HEAD)"
  echo "dirty=$([[ -n "$(git status --porcelain --untracked-files=all)" ]] && echo 1 || echo 0)"
  echo "bench=${BENCH}"
  echo "runs=${RUNS}"
  echo "threads=${THREADS}"
  echo "ws=${WS}"
  echo "slots=${SLOTS}"
  echo "timeout_sec=${TIMEOUT_SEC}"
  echo "performance_stats=HZ5_PRELOAD_STATS_UNSET"
  echo "diagnostic_stats_build=HZ5_DIAGNOSTIC_STATS_0"
  echo "midfront_source_batch_count=64"
  echo "midfront_map_bits=21"
  for cand in "${CANDIDATES[@]}"; do
    echo "candidate_${cand}_flags=${CAND_FLAGS[$cand]}"
  done
} > "${OUTDIR}/meta.txt"

cat > "${OUTDIR}/cases.tsv" <<'EOF'
case	min	max	remote_pct	iters
small_r90	16	2048	90	100000
mid_r0	2049	32768	0	100000
mid_r50	2049	32768	50	100000
mid_r90	2049	32768	90	100000
hi64_r90	32769	65536	90	100000
main_r0	16	65536	0	100000
main_r50	16	65536	50	100000
main_r90	16	65536	90	100000
cross128_r90	16	131072	90	60000
fixed4k_r0	4096	4096	0	100000
EOF

echo -e "candidate\tcase\trun\tops_per_sec\talloc_failed\tstatus\tlog" \
  > "${OUTDIR}/raw.tsv"

run_case() {
  local cand="$1"
  local case_name="$2"
  local min_size="$3"
  local max_size="$4"
  local remote_pct="$5"
  local iters="$6"
  local run="$7"
  local so="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-midfront-observe-${cand}/libhakozuna_hz5_preload_full.so"
  local log="${OUTDIR}/${cand}_${case_name}_${run}.log"

  if [[ ! -f "${so}" ]]; then
    echo "missing preload library: ${so}" >&2
    exit 2
  fi

  set +e
  timeout "${TIMEOUT_SEC}" \
    env -i PATH="${PATH}" HOME="${HOME}" LD_PRELOAD="${so}" \
    "${BENCH}" "${THREADS}" "${iters}" "${WS}" \
    "${min_size}" "${max_size}" "${remote_pct}" "${SLOTS}" \
    > "${log}" 2>&1
  local rc=$?
  set -e

  local ops
  ops="$(grep -oE 'ops/s=[0-9]+([.][0-9]+)?' "${log}" | tail -1 | cut -d= -f2 || true)"
  local failed
  failed="$(grep -c 'alloc failed' "${log}" || true)"
  local status="ok"
  if [[ "${rc}" -eq 124 ]]; then
    status="timeout"
  elif [[ "${rc}" -ne 0 ]]; then
    status="rc${rc}"
  fi
  if [[ -z "${ops}" ]]; then
    ops="nan"
    status="${status}_no_ops"
  fi
  echo -e "${cand}\t${case_name}\t${run}\t${ops}\t${failed}\t${status}\t${log}" \
    | tee -a "${OUTDIR}/raw.tsv" >/dev/null
}

while IFS=$'\t' read -r case_name min_size max_size remote_pct iters; do
  [[ "${case_name}" == "case" ]] && continue
  for cand in "${CANDIDATES[@]}"; do
    for run in $(seq 1 "${RUNS}"); do
      run_case "${cand}" "${case_name}" "${min_size}" "${max_size}" \
        "${remote_pct}" "${iters}" "${run}"
    done
    echo "[case] ${case_name} ${cand}"
  done
done < "${OUTDIR}/cases.tsv"

python3 - <<'PY' "${OUTDIR}/raw.tsv" "${OUTDIR}/summary.tsv"
import math
import statistics
import sys
from collections import defaultdict

raw, out = sys.argv[1], sys.argv[2]
rows = []
with open(raw) as f:
    header = next(f)
    for line in f:
        cand, case, run, ops, failed, status, log = line.rstrip("\n").split("\t")
        val = float(ops) if ops != "nan" else math.nan
        rows.append((cand, case, val, int(failed), status))

groups = defaultdict(list)
for row in rows:
    groups[(row[0], row[1])].append(row)

with open(out, "w") as f:
    f.write("candidate\tcase\tmedian_ops\tmin_ops\tmax_ops\talloc_failed_runs\tbad_status_runs\n")
    for key in sorted(groups, key=lambda x: (x[1], x[0])):
        vals = [r[2] for r in groups[key] if not math.isnan(r[2])]
        failed_runs = sum(1 for r in groups[key] if r[3] > 0)
        bad = sum(1 for r in groups[key] if r[4] != "ok")
        if vals:
            f.write(
                f"{key[0]}\t{key[1]}\t{statistics.median(vals):.2f}\t"
                f"{min(vals):.2f}\t{max(vals):.2f}\t{failed_runs}\t{bad}\n"
            )
        else:
            f.write(f"{key[0]}\t{key[1]}\tnan\tnan\tnan\t{failed_runs}\t{bad}\n")
PY

if [[ "${DO_ATTRIB}" == "1" ]]; then
  echo -e "candidate\tcase\tstats_line\tlog" > "${OUTDIR}/attrib.tsv"
  for cand in "${CANDIDATES[@]}"; do
    so="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-midfront-observe-${cand}/libhakozuna_hz5_preload_full.so"
    log="${OUTDIR}/${cand}_attrib_main_r90.log"
    env -i PATH="${PATH}" HOME="${HOME}" HZ5_PRELOAD_STATS=1 LD_PRELOAD="${so}" \
      "${BENCH}" "${THREADS}" 10000 "${WS}" 16 65536 90 "${SLOTS}" \
      > "${log}" 2>&1 || true
    stats="$(grep 'HZ5_PRELOAD_FULL_STATS' "${log}" | tail -1 || true)"
    echo -e "${cand}\tmain_r90\t${stats}\t${log}" >> "${OUTDIR}/attrib.tsv"
  done
fi

echo "[out] ${OUTDIR}"
echo "[summary]"
cat "${OUTDIR}/summary.tsv"
