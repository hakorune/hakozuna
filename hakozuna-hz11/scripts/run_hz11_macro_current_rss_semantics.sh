#!/usr/bin/env bash
# HZ11MacroCurrentRSSGateSemantics-L1: RSS sampling diagnostics only.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
ROOT_DIR="${REPO_ROOT}"
source "${REPO_ROOT}/bench/lib/bench_common.sh"

STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_macro_current_rss_${STAMP}}"
RUNS="${RUNS:-10}"
BUILD="${BUILD:-1}"
SAMPLE_SLEEP="${SAMPLE_SLEEP:-0.005}"
PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
SH6_MIN_BLOCK="${SH6_MIN_BLOCK:-1}"
SH6_MAX_BLOCK="${SH6_MAX_BLOCK:-1000}"
SH6_THREADS="${SH6_THREADS:-4}"

mkdir -p "${OUTDIR}"

find_first_existing() {
  local path
  for path in "$@"; do
    [[ -n "${path}" && -f "${path}" ]] && { printf '%s\n' "${path}"; return 0; }
  done
  return 1
}

find_first_executable() {
  local path
  for path in "$@"; do
    [[ -n "${path}" && -x "${path}" ]] && { printf '%s\n' "${path}"; return 0; }
  done
  return 1
}

tcmalloc_lib="$(find_first_existing "${TCMALLOC_SO:-}" "${TCMALLOC_LIB:-}" \
  "$(bench_find_tcmalloc_library 2>/dev/null || true)" \
  /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 || true)"
sh6bench_bin="$(find_first_executable "${SH6BENCH_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/sh6bench || true)"

[[ -z "${tcmalloc_lib}" ]] && { echo "tcmalloc library missing; set TCMALLOC_SO" >&2; exit 1; }
[[ -z "${sh6bench_bin}" ]] && { echo "sh6bench binary missing; set SH6BENCH_BIN" >&2; exit 1; }

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" \
    preload-span-transfer-thread-exit-cap-batch32 \
    preload-span-transfer-thread-exit-cap-batch32-fine128 >/dev/null
fi

samples="${OUTDIR}/samples.csv"
trace="${OUTDIR}/rss_trace.csv"
summary="${OUTDIR}/summary.md"
printf 'workload,allocator,run,status,exit_code,wall_sec,max_rss_kb,last_rss_kb,sample_count,p50_rss_kb,p75_rss_kb,p95_rss_kb,current_max_ratio,log,stderr_log,stdout_log,note\n' >"${samples}"
printf 'workload,allocator,run,sample_idx,elapsed_ms,rss_kb,hwm_kb\n' >"${trace}"

python_alloc_py="${OUTDIR}/python_alloc_workload.py"
cat >"${python_alloc_py}" <<'PY'
import sys
loops = int(sys.argv[1])
bag = []
for r in range(loops):
    for i in range(20000):
        bag.append(bytearray((i % 4096) + 1))
    del bag[::3]
    bag = bag[-20000:]
print(len(bag))
PY

csv_escape() {
  local s="${1//$'\n'/ }"; s="${s//\"/\"\"}"; printf '"%s"' "${s}"
}

read_kb() {
  awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true
}

write_sample() {
  {
    csv_escape "$1"; printf ','; csv_escape "$2"; printf ',%s,' "$3"
    csv_escape "$4"; printf ',%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,' \
      "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}"
    csv_escape "${14}"; printf ','; csv_escape "${15}"; printf ','
    csv_escape "${16}"; printf ','; csv_escape "${17}"; printf '\n'
  } >>"${samples}"
}

percentile_from_file() {
  local file="$1" p="$2"
  awk -v p="${p}" '
    NF { v[++n] = $1 }
    END {
      if (n == 0) { print "NA"; exit }
      idx = int((n - 1) * p + 0.5) + 1
      if (idx < 1) idx = 1
      if (idx > n) idx = n
      printf "%d", v[idx]
    }' "${file}"
}

run_sampled() {
  local workload="$1" allocator="$2" run="$3" timeout="$4"; shift 4
  local -a cmd=("$@")
  local log="${OUTDIR}/${workload}_${allocator}_${run}.log"
  local err="${OUTDIR}/${workload}_${allocator}_${run}.err"
  local out="${OUTDIR}/${workload}_${allocator}_${run}.out"
  local rss_tmp="${OUTDIR}/${workload}_${allocator}_${run}.rss.tmp"
  local status=OK rc=0 max_rss=NA last_rss=NA sample hwm pid start now wall idx=0
  : >"${log}"; : >"${err}"; : >"${out}"; : >"${rss_tmp}"
  printf '[cmd]' >"${log}"; printf ' %q' "${cmd[@]}" >>"${log}"; printf '\n' >>"${log}"
  start="$(date +%s%N)"
  "${cmd[@]}" >"${out}" 2>"${err}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_kb "${pid}" VmRSS || true)"
    hwm="$(read_kb "${pid}" VmHWM || true)"
    now="$(date +%s%N)"
    if [[ -n "${sample}" ]]; then
      last_rss="${sample}"
      printf '%s\n' "${sample}" >>"${rss_tmp}"
      printf '%s,%s,%s,%s,%s,%s,%s\n' "${workload}" "${allocator}" "${run}" \
        "${idx}" "$(awk -v s="${start}" -v n="${now}" 'BEGIN{printf "%.3f",(n-s)/1000000}')" \
        "${sample}" "${hwm:-}" >>"${trace}"
      idx=$((idx + 1))
    fi
    [[ -n "${hwm}" && ( "${max_rss}" == NA || "${hwm}" -gt "${max_rss}" ) ]] && max_rss="${hwm}"
    if awk -v s="${start}" -v n="${now}" -v t="${timeout}" 'BEGIN{exit !(((n-s)/1000000000)>t)}'; then
      status=TIMEOUT
      kill -KILL "${pid}" 2>/dev/null || true
      break
    fi
    sleep "${SAMPLE_SLEEP}"
  done
  set +e; wait "${pid}" 2>/dev/null; rc=$?; set -e
  now="$(date +%s%N)"
  wall="$(awk -v s="${start}" -v e="${now}" 'BEGIN{printf "%.3f",(e-s)/1000000000}')"
  [[ "${status}" != TIMEOUT && "${rc}" -ne 0 ]] && status=FAIL
  [[ "${max_rss}" == NA && "${last_rss}" != NA ]] && max_rss="${last_rss}"
  sort -n "${rss_tmp}" -o "${rss_tmp}"
  local count p50 p75 p95 ratio
  count="$(wc -l <"${rss_tmp}" | tr -d ' ')"
  p50="$(percentile_from_file "${rss_tmp}" 0.50)"
  p75="$(percentile_from_file "${rss_tmp}" 0.75)"
  p95="$(percentile_from_file "${rss_tmp}" 0.95)"
  ratio="$(awk -v c="${last_rss}" -v m="${max_rss}" 'BEGIN{if(c=="NA"||m=="NA"||m==0)print "NA"; else printf "%.3f", c/m}')"
  cat "${out}" "${err}" >>"${log}" || true
  write_sample "${workload}" "${allocator}" "${run}" "${status}" "${rc}" "${wall}" \
    "${max_rss}" "${last_rss}" "${count}" "${p50}" "${p75}" "${p95}" "${ratio}" \
    "${log}" "${err}" "${out}" "post-exit /proc RSS unavailable after process exits"
}

run_python_alloc() {
  local allocator="$1" run="$2" lib="$3"
  local -a cmd=(env PYTHONMALLOC=malloc)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  cmd+=(/usr/bin/python3 "${python_alloc_py}" "${PYTHON_LOOPS}")
  run_sampled python_alloc "${allocator}" "${run}" 60 "${cmd[@]}"
}

run_sh6bench() {
  local allocator="$1" run="$2" lib="$3"
  local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
  printf '%s\n%s\n%s\n%s\n' "${SH6_CALL_COUNT}" "${SH6_MIN_BLOCK}" \
    "${SH6_MAX_BLOCK}" "${SH6_THREADS}" >"${input}"
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  cmd+=("${sh6bench_bin}" "${input}")
  run_sampled sh6bench "${allocator}" "${run}" 60 "${cmd[@]}"
}

declare -A libs=(
  [tcmalloc]="${tcmalloc_lib}"
  [hz11-thread-exit-cap-batch32]="${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32.so"
  [hz11-thread-exit-cap-batch32-fine128]="${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32_fine128.so"
)
allocators=(tcmalloc hz11-thread-exit-cap-batch32 hz11-thread-exit-cap-batch32-fine128)

for run in $(seq 1 "${RUNS}"); do
  for alloc in "${allocators[@]}"; do
    echo "[RUN] workload=python_alloc allocator=${alloc} run=${run}"
    run_python_alloc "${alloc}" "${run}" "${libs[$alloc]}"
  done
  for alloc in "${allocators[@]}"; do
    echo "[RUN] workload=sh6bench allocator=${alloc} run=${run}"
    run_sh6bench "${alloc}" "${run}" "${libs[$alloc]}"
  done
done

python3 - "${samples}" "${trace}" "${summary}" "${RUNS}" "${SAMPLE_SLEEP}" <<'PY'
import csv, math, statistics, sys
from collections import defaultdict
samples=list(csv.DictReader(open(sys.argv[1], newline="")))
trace=list(csv.DictReader(open(sys.argv[2], newline="")))
dst=sys.argv[3]; runs=sys.argv[4]; sleep=sys.argv[5]
groups=defaultdict(list)
for r in samples:
    groups[(r["workload"], r["allocator"])].append(r)
def num(v):
    return None if v in ("","NA","-") else float(v)
def vals(items,k):
    return [num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
def med(items,k):
    v=vals(items,k); return statistics.median(v) if v else math.nan
def q(items,k,p):
    v=sorted(vals(items,k))
    if not v: return math.nan
    return v[int(round((len(v)-1)*p))]
def ratio(workload, alloc, base, key):
    a=med(groups[(workload,alloc)], key); b=med(groups[(workload,base)], key)
    return a/b if b and not math.isnan(a) and not math.isnan(b) else math.nan
def fmt(v):
    return "NA" if v is None or math.isnan(v) else f"{v:.3f}"
allocs=["tcmalloc","hz11-thread-exit-cap-batch32","hz11-thread-exit-cap-batch32-fine128"]
with open(dst,"w") as f:
    f.write("# HZ11 Macro Current RSS Gate Semantics L1\n\n")
    f.write(f"Diagnostics only. RUNS={runs}, SAMPLE_SLEEP={sleep}s.\n\n")
    f.write("## Samples\n\n")
    f.write("| workload | allocator | OK runs | wall med | max RSS med KiB | last RSS med KiB | p50 RSS med KiB | p75 RSS med KiB | p95 RSS med KiB | last/max med | last/tcmalloc | p95/tcmalloc | max/tcmalloc |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for w in ("python_alloc","sh6bench"):
        for a in allocs:
            items=groups[(w,a)]
            ok=sum(1 for r in items if r["status"]=="OK")
            f.write(f"| {w} | {a} | {ok} | {fmt(med(items,'wall_sec'))} | {fmt(med(items,'max_rss_kb'))} | {fmt(med(items,'last_rss_kb'))} | {fmt(med(items,'p50_rss_kb'))} | {fmt(med(items,'p75_rss_kb'))} | {fmt(med(items,'p95_rss_kb'))} | {fmt(med(items,'current_max_ratio'))} | {fmt(ratio(w,a,'tcmalloc','last_rss_kb'))} | {fmt(ratio(w,a,'tcmalloc','p95_rss_kb'))} | {fmt(ratio(w,a,'tcmalloc','max_rss_kb'))} |\n")
    f.write("\n## Per-Run Last/Max Ratios\n\n")
    f.write("| workload | allocator | run | max RSS KiB | last RSS KiB | p95 RSS KiB | last/max |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|\n")
    for r in samples:
        if r["status"] != "OK": continue
        f.write(f"| {r['workload']} | {r['allocator']} | {r['run']} | {r['max_rss_kb']} | {r['last_rss_kb']} | {r['p95_rss_kb']} | {r['current_max_ratio']} |\n")
    f.write("\nRaw samples: `samples.csv`; RSS trace: `rss_trace.csv`.\n")
PY

echo "[DONE] HZ11 current RSS semantics logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
