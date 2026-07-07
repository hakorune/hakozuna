#!/usr/bin/env bash
# HZ11MacroFailureAttribution-L1:
# Attribute macro aborts to central stack class/cap pressure. This script builds
# diagnostic sibling preload libraries; it does not change the default transfer
# lane or allocator policy.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"

STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_macro_failure_attr_${STAMP}}"
RUNS="${RUNS:-3}"
ROWS="${ROWS:-python_alloc,mstress}"
CAPS="${CAPS:-4096,8192,16384,32768,65536}"
BUILD="${BUILD:-1}"

PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
MSTRESS_THREADS="${MSTRESS_THREADS:-8}"
MSTRESS_SCALE="${MSTRESS_SCALE:-80}"
MSTRESS_ITER="${MSTRESS_ITER:-3}"
LARSON_SECONDS="${LARSON_SECONDS:-2}"
LARSON_MIN="${LARSON_MIN:-8}"
LARSON_MAX="${LARSON_MAX:-128}"
LARSON_CHUNKS="${LARSON_CHUNKS:-128}"
LARSON_ROUNDS="${LARSON_ROUNDS:-1}"
LARSON_SEED="${LARSON_SEED:-12345}"
LARSON_THREADS="${LARSON_THREADS:-4}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
SH6_MIN_BLOCK="${SH6_MIN_BLOCK:-1}"
SH6_MAX_BLOCK="${SH6_MAX_BLOCK:-1000}"
SH6_THREADS="${SH6_THREADS:-4}"

usage() {
  cat <<'EOF'
Usage:
  hakozuna-hz11/scripts/run_hz11_macro_failure_attribution.sh [options]

Options:
  --runs N       samples per row/cap (default 3)
  --rows LIST    comma-separated rows (default python_alloc,mstress)
  --caps LIST    comma-separated HZ11_CENTRAL_CAP values
                 (default 4096,8192,16384,32768,65536)
  --outdir DIR   output directory
  --skip-build   reuse existing libraries
  --help         show this message

Rows:
  python_alloc
  mstress
  larson
  sh6bench
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --rows) ROWS="$2"; shift 2 ;;
    --caps) CAPS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) BUILD=0; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

mkdir -p "${OUTDIR}"

find_first_existing() {
  local path
  for path in "$@"; do
    if [[ -n "${path}" && -f "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_first_executable() {
  local path
  for path in "$@"; do
    if [[ -n "${path}" && -x "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_tcmalloc_lib() {
  find_first_existing \
    "${TCMALLOC_SO:-}" \
    "${TCMALLOC_LIB:-}" \
    /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/local/lib/libtcmalloc_minimal.so.4 \
    /usr/lib/libtcmalloc_minimal.so.4
}

find_mstress_bin() {
  find_first_executable \
    "${MSTRESS_BIN:-}" \
    "${MIMALLOC_BENCH_DIR:-}/mstress" \
    /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/mstress \
    /mnt/workdisk/public_share/hakmem/mimalloc-bench/build/bench/mstress
}

find_larson_bin() {
  find_first_executable \
    "${LARSON_BIN:-}" \
    /mnt/workdisk/public_share/hakmem/larson_system \
    /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson
}

find_mimalloc_bench_bin() {
  local name="$1"
  find_first_executable \
    "${MIMALLOC_BENCH_DIR:-}/${name}" \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/${name}" \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/build/bench/${name}"
}

tcmalloc_lib="$(find_tcmalloc_lib || true)"
mstress_bin="$(find_mstress_bin || true)"
larson_bin="$(find_larson_bin || true)"
sh6bench_bin="$(find_mimalloc_bench_bin sh6bench || true)"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-transfer-diag >/dev/null
  IFS=',' read -r -a cap_builds <<<"${CAPS}"
  for cap in "${cap_builds[@]}"; do
    rm -f "${ROOT}/libhz11_span_transfer_cap.so"
    make -C "${ROOT}" preload-span-transfer-cap HZ11_CENTRAL_CAP_AB="${cap}" >/dev/null
    cp "${ROOT}/libhz11_span_transfer_cap.so" \
      "${OUTDIR}/libhz11_span_transfer_cap_${cap}.so"
  done
fi

csv="${OUTDIR}/samples.csv"
summary_md="${OUTDIR}/summary.md"
readme_log="${OUTDIR}/README.log"
printf 'workload,allocator,central_cap,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,overflow_class,overflow_needed,max_high_water,log,stderr_log,stdout_log,note\n' >"${csv}"

{
  echo "[HZ11_MACRO_FAILURE_ATTR] ts=${STAMP}"
  echo "[HZ11_MACRO_FAILURE_ATTR] root=${REPO_ROOT}"
  echo "[HZ11_MACRO_FAILURE_ATTR] hz11_root=${ROOT}"
  echo "[HZ11_MACRO_FAILURE_ATTR] runs=${RUNS}"
  echo "[HZ11_MACRO_FAILURE_ATTR] rows=${ROWS}"
  echo "[HZ11_MACRO_FAILURE_ATTR] caps=${CAPS}"
  echo "[HZ11_MACRO_FAILURE_ATTR] tcmalloc=${tcmalloc_lib:-SKIP}"
  echo "[HZ11_MACRO_FAILURE_ATTR] mstress=${mstress_bin:-SKIP}"
  echo "[HZ11_MACRO_FAILURE_ATTR] larson=${larson_bin:-SKIP}"
  echo "[HZ11_MACRO_FAILURE_ATTR] sh6bench=${sh6bench_bin:-SKIP}"
  echo "[HZ11_MACRO_FAILURE_ATTR] git_sha=$(git -C "${REPO_ROOT}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
} >"${readme_log}"

csv_escape() {
  local s="${1//$'\n'/ }"
  s="${s//\"/\"\"}"
  printf '"%s"' "${s}"
}

append_sample() {
  local workload="$1" allocator="$2" cap="$3" run="$4" status="$5" rc="$6"
  local wall="$7" max_rss="$8" current_rss="$9" overflow_class="${10}"
  local overflow_needed="${11}" max_high_water="${12}" log_file="${13}"
  local err_file="${14}" out_file="${15}" note="${16}"
  {
    csv_escape "${workload}"; printf ','
    csv_escape "${allocator}"; printf ','
    printf '%s,%s,' "${cap}" "${run}"
    csv_escape "${status}"; printf ','
    printf '%s,%s,%s,%s,%s,%s,%s,' "${rc}" "${wall}" "${max_rss}" \
      "${current_rss}" "${overflow_class}" "${overflow_needed}" "${max_high_water}"
    csv_escape "${log_file}"; printf ','
    csv_escape "${err_file}"; printf ','
    csv_escape "${out_file}"; printf ','
    csv_escape "${note}"; printf '\n'
  } >>"${csv}"
}

read_proc_status_kb() {
  local pid="$1" key="$2"
  awk -v key="${key}:" '$1 == key { print $2; exit }' "/proc/${pid}/status" 2>/dev/null || true
}

run_sampled() {
  local workload="$1" allocator="$2" cap="$3" run="$4" timeout_sec="$5"
  shift 5
  local safe_alloc="${allocator//[^A-Za-z0-9_.-]/_}"
  local log_file="${OUTDIR}/${workload}_${safe_alloc}_${cap}_${run}.log"
  local err_file="${OUTDIR}/${workload}_${safe_alloc}_${cap}_${run}.err"
  local out_file="${OUTDIR}/${workload}_${safe_alloc}_${cap}_${run}.out"
  local status="OK" rc=0 max_rss="NA" current_rss="NA" last_rss=""
  local start_ns now_ns wall hwm sample pid

  : >"${log_file}"
  : >"${err_file}"
  : >"${out_file}"
  printf '[cmd]' >"${log_file}"
  printf ' %q' "$@" >>"${log_file}"
  printf '\n' >>"${log_file}"

  start_ns="$(date +%s%N)"
  "$@" >"${out_file}" 2>"${err_file}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_proc_status_kb "${pid}" VmRSS || true)"
    hwm="$(read_proc_status_kb "${pid}" VmHWM || true)"
    if [[ -n "${sample}" ]]; then
      last_rss="${sample}"
      current_rss="${sample}"
    fi
    if [[ -n "${hwm}" && ( "${max_rss}" == "NA" || "${hwm}" -gt "${max_rss}" ) ]]; then
      max_rss="${hwm}"
    fi
    now_ns="$(date +%s%N)"
    if awk -v s="${start_ns}" -v n="${now_ns}" -v t="${timeout_sec}" 'BEGIN { exit !(((n-s)/1000000000) > t) }'; then
      status="TIMEOUT"
      kill -KILL "${pid}" 2>/dev/null || true
      break
    fi
    sleep 0.02
  done
  set +e
  wait "${pid}" 2>/dev/null
  rc=$?
  set -e
  now_ns="$(date +%s%N)"
  wall="$(awk -v s="${start_ns}" -v e="${now_ns}" 'BEGIN { printf "%.3f", (e - s) / 1000000000 }')"
  if [[ "${status}" != "TIMEOUT" && "${rc}" -ne 0 ]]; then
    status="FAIL"
  fi
  if [[ -z "${last_rss}" ]]; then
    current_rss="NA"
  fi
  if [[ "${max_rss}" == "NA" && "${current_rss}" != "NA" ]]; then
    max_rss="${current_rss}"
  fi
  cat "${out_file}" >>"${log_file}" || true
  cat "${err_file}" >>"${log_file}" || true

  local overflow_class overflow_needed max_high_water
  overflow_class="$(awk '/hz11_central_overflow/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="class") v=a[2]}} END{if (v=="") print "NA"; else print v}' "${err_file}")"
  overflow_needed="$(awk '/hz11_central_overflow/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="needed") v=a[2]}} END{if (v=="") print "NA"; else print v}' "${err_file}")"
  max_high_water="$(awk '/hz11_central_class / {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="high_water" && a[2] > v) v=a[2]}} END{if (v=="") print "NA"; else print v+0}' "${err_file}")"

  append_sample "${workload}" "${allocator}" "${cap}" "${run}" "${status}" "${rc}" \
    "${wall}" "${max_rss}" "${current_rss}" "${overflow_class}" \
    "${overflow_needed}" "${max_high_water}" "${log_file}" "${err_file}" \
    "${out_file}" ""
}

run_skip() {
  append_sample "$1" "$2" "$3" "$4" "SKIP" "-" "NA" "NA" "NA" \
    "NA" "NA" "NA" "" "" "" "$5"
}

run_python_alloc() {
  local allocator="$1" cap="$2" run="$3" lib="$4"
  local code='import sys
loops = int(sys.argv[1])
bag = []
for r in range(loops):
    for i in range(20000):
        bag.append(bytearray((i % 4096) + 1))
    del bag[::3]
    bag = bag[-20000:]
print(len(bag))'
  local -a cmd=(env PYTHONMALLOC=malloc)
  if [[ -n "${lib}" ]]; then
    cmd+=(LD_PRELOAD="${lib}" HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1)
  fi
  cmd+=(/usr/bin/python3 -c "${code}" "${PYTHON_LOOPS}")
  run_sampled python_alloc "${allocator}" "${cap}" "${run}" 60 "${cmd[@]}"
}

run_mstress() {
  local allocator="$1" cap="$2" run="$3" lib="$4"
  if [[ -z "${mstress_bin}" ]]; then
    run_skip mstress "${allocator}" "${cap}" "${run}" "mstress binary not found"
    return 0
  fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then
    cmd+=(LD_PRELOAD="${lib}" HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1)
  fi
  cmd+=("${mstress_bin}" "${MSTRESS_THREADS}" "${MSTRESS_SCALE}" "${MSTRESS_ITER}")
  run_sampled mstress "${allocator}" "${cap}" "${run}" 30 "${cmd[@]}"
}

run_larson() {
  local allocator="$1" cap="$2" run="$3" lib="$4"
  if [[ -z "${larson_bin}" ]]; then
    run_skip larson "${allocator}" "${cap}" "${run}" "larson binary not found"
    return 0
  fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then
    cmd+=(LD_PRELOAD="${lib}" HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1)
  fi
  cmd+=("${larson_bin}" "${LARSON_SECONDS}" "${LARSON_MIN}" "${LARSON_MAX}" \
    "${LARSON_CHUNKS}" "${LARSON_ROUNDS}" "${LARSON_SEED}" "${LARSON_THREADS}")
  run_sampled larson "${allocator}" "${cap}" "${run}" 30 "${cmd[@]}"
}

run_sh6bench() {
  local allocator="$1" cap="$2" run="$3" lib="$4"
  if [[ -z "${sh6bench_bin}" ]]; then
    run_skip sh6bench "${allocator}" "${cap}" "${run}" "sh6bench binary not found"
    return 0
  fi
  local input="${OUTDIR}/sh6bench_${allocator}_${cap}_${run}.in"
  printf '%s\n%s\n%s\n%s\n' "${SH6_CALL_COUNT}" "${SH6_MIN_BLOCK}" \
    "${SH6_MAX_BLOCK}" "${SH6_THREADS}" >"${input}"
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then
    cmd+=(LD_PRELOAD="${lib}" HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1)
  fi
  cmd+=("${sh6bench_bin}" "${input}")
  run_sampled sh6bench "${allocator}" "${cap}" "${run}" 30 "${cmd[@]}"
}

IFS=',' read -r -a rows <<<"${ROWS}"
IFS=',' read -r -a caps <<<"${CAPS}"

run_row() {
  local row="$1" allocator="$2" cap="$3" run="$4" lib="$5"
  case "${row}" in
    python_alloc) run_python_alloc "${allocator}" "${cap}" "${run}" "${lib}" ;;
    mstress) run_mstress "${allocator}" "${cap}" "${run}" "${lib}" ;;
    larson) run_larson "${allocator}" "${cap}" "${run}" "${lib}" ;;
    sh6bench) run_sh6bench "${allocator}" "${cap}" "${run}" "${lib}" ;;
    *) run_skip "${row}" "${allocator}" "${cap}" "${run}" "unknown row" ;;
  esac
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    if [[ -n "${tcmalloc_lib}" ]]; then
      run_row "${row}" tcmalloc 0 "${run}" "${tcmalloc_lib}"
    else
      run_skip "${row}" tcmalloc 0 "${run}" "tcmalloc library not found"
    fi
    for cap in "${caps[@]}"; do
      lib="${OUTDIR}/libhz11_span_transfer_cap_${cap}.so"
      if [[ ! -f "${lib}" ]]; then
        run_skip "${row}" "hz11-transfer-cap" "${cap}" "${run}" "cap library not found"
        continue
      fi
      run_row "${row}" "hz11-transfer-cap" "${cap}" "${run}" "${lib}"
    done
  done
done

python3 - "${csv}" "${summary_md}" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

csv_path, out_path = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(csv_path, newline="")))
groups = defaultdict(list)
for r in rows:
    groups[(r["workload"], r["allocator"], r["central_cap"])].append(r)

def num(v):
    if v in ("", "NA", "-"):
        return None
    return float(v)

def med(items, key):
    vals = [num(r[key]) for r in items if r["status"] == "OK" and num(r[key]) is not None]
    if not vals:
        return None
    return statistics.median(vals)

def fmt(v):
    if v is None:
        return "NA"
    return f"{v:.3f}"

def status_counts(items):
    counts = defaultdict(int)
    for r in items:
        counts[r["status"]] += 1
    return " ".join(f"{k}:{counts[k]}" for k in sorted(counts))

def max_int(items, key):
    vals = []
    for r in items:
        v = r[key]
        if v not in ("", "NA", "-"):
            vals.append(int(float(v)))
    return max(vals) if vals else None

with open(out_path, "w") as f:
    f.write("# HZ11 Macro Failure Attribution L1\n\n")
    f.write("Focus: central stack class/cap pressure for macro abort rows.\n\n")
    f.write("## Summary\n\n")
    f.write("| Workload | Allocator | cap | status runs | median wall sec | median max RSS KiB | median current RSS KiB | overflow class | overflow needed | max high-water |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for key in sorted(groups):
        items = groups[key]
        overflow_class = max_int(items, "overflow_class")
        overflow_needed = max_int(items, "overflow_needed")
        high = max_int(items, "max_high_water")
        f.write(
            f"| {key[0]} | {key[1]} | {key[2]} | {status_counts(items)} | "
            f"{fmt(med(items, 'wall_sec'))} | {fmt(med(items, 'max_rss_kb'))} | "
            f"{fmt(med(items, 'current_rss_kb'))} | "
            f"{overflow_class if overflow_class is not None else 'NA'} | "
            f"{overflow_needed if overflow_needed is not None else 'NA'} | "
            f"{high if high is not None else 'NA'} |\n"
        )
    f.write("\n## Cap Pass/Fail\n\n")
    f.write("| Workload | cap | transfer OK runs | tcmalloc max RSS KiB | transfer max RSS KiB | transfer/tcmalloc max RSS |\n")
    f.write("|---|---:|---:|---:|---:|---:|\n")
    workloads = sorted({r["workload"] for r in rows})
    caps = sorted({r["central_cap"] for r in rows if r["allocator"] == "hz11-transfer-cap"}, key=lambda x: int(x))
    for w in workloads:
        tc = groups.get((w, "tcmalloc", "0"), [])
        tc_rss = med(tc, "max_rss_kb")
        for cap in caps:
            items = groups.get((w, "hz11-transfer-cap", cap), [])
            ok = sum(1 for r in items if r["status"] == "OK")
            tr_rss = med(items, "max_rss_kb")
            ratio = tr_rss / tc_rss if tr_rss is not None and tc_rss else None
            f.write(f"| {w} | {cap} | {ok}/{len(items)} | {fmt(tc_rss)} | {fmt(tr_rss)} | {fmt(ratio)} |\n")
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] HZ11 macro failure attribution logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary_md}"
