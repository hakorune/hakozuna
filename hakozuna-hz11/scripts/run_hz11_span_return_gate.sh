#!/usr/bin/env bash
# HZ11CentralFreeListSpanReturn-L1 macro gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_span_return_${STAMP}}"
RUNS="${RUNS:-3}"
ROWS="${ROWS:-python_alloc,mstress,larson,sh6bench,xmalloc_test}"
BUILD="${BUILD:-1}"

PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
MSTRESS_THREADS="${MSTRESS_THREADS:-8}"
MSTRESS_SCALE="${MSTRESS_SCALE:-80}"
MSTRESS_ITER="${MSTRESS_ITER:-3}"
LARSON_SECONDS="${LARSON_SECONDS:-2}"
LARSON_THREADS="${LARSON_THREADS:-4}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
SH6_THREADS="${SH6_THREADS:-4}"
XMALLOC_WORKERS="${XMALLOC_WORKERS:-4}"
XMALLOC_SECONDS="${XMALLOC_SECONDS:-2}"

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
  /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 || true)"
mstress_bin="$(find_first_executable "${MSTRESS_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/mstress || true)"
larson_bin="$(find_first_executable "${LARSON_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/larson_system \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson || true)"
sh6bench_bin="$(find_first_executable "${SH6BENCH_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/sh6bench || true)"
xmalloc_bin="$(find_first_executable "${XMALLOC_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/xmalloc-test || true)"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-transfer preload-span-return >/dev/null
fi

csv="${OUTDIR}/samples.csv"
summary_md="${OUTDIR}/summary.md"
printf 'workload,allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,span_return,span_reuse,central_objects,xfer_hit,xfer_insert,log,stderr_log,stdout_log,note\n' >"${csv}"

{
  echo "[HZ11_SPAN_RETURN] ts=${STAMP}"
  echo "[HZ11_SPAN_RETURN] runs=${RUNS}"
  echo "[HZ11_SPAN_RETURN] rows=${ROWS}"
  echo "[HZ11_SPAN_RETURN] tcmalloc=${tcmalloc_lib:-SKIP}"
  echo "[HZ11_SPAN_RETURN] transfer=${ROOT}/libhz11_span_transfer.so"
  echo "[HZ11_SPAN_RETURN] span_return=${ROOT}/libhz11_span_return.so"
} >"${OUTDIR}/README.log"

csv_escape() {
  local s="${1//$'\n'/ }"
  s="${s//\"/\"\"}"
  printf '"%s"' "${s}"
}

append_sample() {
  {
    csv_escape "$1"; printf ','; csv_escape "$2"; printf ','
    printf '%s,' "$3"; csv_escape "$4"; printf ','
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,' "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}"
    csv_escape "${14}"; printf ','; csv_escape "${15}"; printf ','; csv_escape "${16}"; printf ','
    csv_escape "${17}"; printf '\n'
  } >>"${csv}"
}

read_kb() {
  awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true
}

run_sampled() {
  local workload="$1" allocator="$2" run="$3" timeout_sec="$4"
  shift 4
  local log="${OUTDIR}/${workload}_${allocator}_${run}.log"
  local err="${OUTDIR}/${workload}_${allocator}_${run}.err"
  local out="${OUTDIR}/${workload}_${allocator}_${run}.out"
  local status=OK rc=0 max_rss=NA cur_rss=NA last_rss="" hwm sample pid start now wall
  : >"${log}"; : >"${err}"; : >"${out}"
  printf '[cmd]' >"${log}"; printf ' %q' "$@" >>"${log}"; printf '\n' >>"${log}"
  start="$(date +%s%N)"
  "$@" >"${out}" 2>"${err}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_kb "${pid}" VmRSS || true)"
    hwm="$(read_kb "${pid}" VmHWM || true)"
    [[ -n "${sample}" ]] && { last_rss="${sample}"; cur_rss="${sample}"; }
    [[ -n "${hwm}" && ( "${max_rss}" == NA || "${hwm}" -gt "${max_rss}" ) ]] && max_rss="${hwm}"
    now="$(date +%s%N)"
    if awk -v s="${start}" -v n="${now}" -v t="${timeout_sec}" 'BEGIN{exit !(((n-s)/1000000000)>t)}'; then
      status=TIMEOUT
      kill -KILL "${pid}" 2>/dev/null || true
      break
    fi
    sleep 0.02
  done
  set +e; wait "${pid}" 2>/dev/null; rc=$?; set -e
  now="$(date +%s%N)"
  wall="$(awk -v s="${start}" -v e="${now}" 'BEGIN{printf "%.3f",(e-s)/1000000000}')"
  [[ "${status}" != TIMEOUT && "${rc}" -ne 0 ]] && status=FAIL
  [[ -z "${last_rss}" ]] && cur_rss=NA
  [[ "${max_rss}" == NA && "${cur_rss}" != NA ]] && max_rss="${cur_rss}"
  cat "${out}" "${err}" >>"${log}" || true
  local span_return span_reuse central_objects xfer_hit xfer_insert
  span_return="$(awk '/hz11_shim_exit_stats/ {for(i=1;i<=NF;i++){split($i,a,"="); if(a[1]=="span_return") v=a[2]}} END{print v+0}' "${err}" "${out}" 2>/dev/null || echo 0)"
  span_reuse="$(awk '/hz11_shim_exit_stats/ {for(i=1;i<=NF;i++){split($i,a,"="); if(a[1]=="span_reuse") v=a[2]}} END{print v+0}' "${err}" "${out}" 2>/dev/null || echo 0)"
  central_objects="$(awk '/hz11_shim_exit_stats/ {for(i=1;i<=NF;i++){split($i,a,"="); if(a[1]=="central_objects") v=a[2]}} END{print v+0}' "${err}" "${out}" 2>/dev/null || echo 0)"
  xfer_hit="$(awk '/hz11_shim_exit_stats/ {for(i=1;i<=NF;i++){split($i,a,"="); if(a[1]=="xfer_hit") v=a[2]}} END{print v+0}' "${err}" "${out}" 2>/dev/null || echo 0)"
  xfer_insert="$(awk '/hz11_shim_exit_stats/ {for(i=1;i<=NF;i++){split($i,a,"="); if(a[1]=="xfer_insert") v=a[2]}} END{print v+0}' "${err}" "${out}" 2>/dev/null || echo 0)"
  append_sample "${workload}" "${allocator}" "${run}" "${status}" "${rc}" "${wall}" "${max_rss}" "${cur_rss}" \
    "${span_return}" "${span_reuse}" "${central_objects}" "${xfer_hit}" "${xfer_insert}" "${log}" "${err}" "${out}" ""
}

run_skip() { append_sample "$1" "$2" "$3" SKIP - NA NA NA 0 0 0 0 0 "" "" "" "$4"; }

run_row() {
  local row="$1" allocator="$2" run="$3" lib="$4"
  local -a envp=(env)
  [[ -n "${lib}" ]] && envp+=(LD_PRELOAD="${lib}")
  [[ "${allocator}" == hz11-* ]] && envp+=(HZ11_DUMP_STATS=1)
  case "${row}" in
    python_alloc)
      envp+=(PYTHONMALLOC=malloc /usr/bin/python3 -c 'import sys
loops=int(sys.argv[1]); bag=[]
for r in range(loops):
  for i in range(20000): bag.append(bytearray((i % 4096)+1))
  del bag[::3]; bag=bag[-20000:]
print(len(bag))' "${PYTHON_LOOPS}")
      run_sampled "${row}" "${allocator}" "${run}" 60 "${envp[@]}" ;;
    mstress)
      [[ -z "${mstress_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "mstress missing"; return; }
      envp+=("${mstress_bin}" "${MSTRESS_THREADS}" "${MSTRESS_SCALE}" "${MSTRESS_ITER}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${envp[@]}" ;;
    larson)
      [[ -z "${larson_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "larson missing"; return; }
      envp+=("${larson_bin}" "${LARSON_SECONDS}" 8 128 128 1 12345 "${LARSON_THREADS}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${envp[@]}" ;;
    sh6bench)
      [[ -z "${sh6bench_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "sh6bench missing"; return; }
      local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
      printf '%s\n1\n1000\n%s\n' "${SH6_CALL_COUNT}" "${SH6_THREADS}" >"${input}"
      envp+=("${sh6bench_bin}" "${input}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${envp[@]}" ;;
    xmalloc_test)
      [[ -z "${xmalloc_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "xmalloc missing"; return; }
      envp+=("${xmalloc_bin}" -w "${XMALLOC_WORKERS}" -t "${XMALLOC_SECONDS}" -s -1)
      run_sampled "${row}" "${allocator}" "${run}" 30 "${envp[@]}" ;;
    *) run_skip "${row}" "${allocator}" "${run}" "unknown row" ;;
  esac
}

IFS=',' read -r -a rows <<<"${ROWS}"
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    [[ -n "${tcmalloc_lib}" ]] && run_row "${row}" tcmalloc "${run}" "${tcmalloc_lib}" || run_skip "${row}" tcmalloc "${run}" "tcmalloc missing"
    run_row "${row}" hz11-span-transfer "${run}" "${ROOT}/libhz11_span_transfer.so"
    run_row "${row}" hz11-span-return "${run}" "${ROOT}/libhz11_span_return.so"
  done
done

python3 - "${csv}" "${summary_md}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
rows=list(csv.DictReader(open(sys.argv[1], newline="")))
g=defaultdict(list)
for r in rows: g[(r["workload"],r["allocator"])].append(r)
def num(v):
    return None if v in ("","NA","-") else float(v)
def med(items,k):
    vals=[num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
    return statistics.median(vals) if vals else None
def total(items,k):
    return sum(num(r[k]) or 0 for r in items if r["status"]=="OK")
def fmt(v): return "NA" if v is None else f"{v:.3f}"
def status(items):
    d=defaultdict(int)
    for r in items: d[r["status"]]+=1
    return " ".join(f"{k}:{d[k]}" for k in sorted(d))
with open(sys.argv[2],"w") as f:
    f.write("# HZ11 Span Return Gate\n\n")
    f.write("| Workload | Allocator | status | median wall sec | median max RSS KiB | median current RSS KiB | span_return | span_reuse | central_objects | xfer_hit | xfer_insert |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for key in sorted(g):
        items=g[key]
        f.write(f"| {key[0]} | {key[1]} | {status(items)} | {fmt(med(items,'wall_sec'))} | {fmt(med(items,'max_rss_kb'))} | {fmt(med(items,'current_rss_kb'))} | {int(total(items,'span_return'))} | {int(total(items,'span_reuse'))} | {int(total(items,'central_objects'))} | {int(total(items,'xfer_hit'))} | {int(total(items,'xfer_insert'))} |\n")
    f.write("\n## Ratios\n\n")
    f.write("| Workload | return/transfer wall | return/transfer max RSS | return/tcmalloc max RSS |\n")
    f.write("|---|---:|---:|---:|\n")
    workloads=sorted({r["workload"] for r in rows})
    for w in workloads:
        ret=g.get((w,"hz11-span-return"),[])
        tr=g.get((w,"hz11-span-transfer"),[])
        tc=g.get((w,"tcmalloc"),[])
        rw, tw = med(ret,"wall_sec"), med(tr,"wall_sec")
        rr, trr, tcr = med(ret,"max_rss_kb"), med(tr,"max_rss_kb"), med(tc,"max_rss_kb")
        f.write(f"| {w} | {fmt(rw/tw if rw and tw else None)} | {fmt(rr/trr if rr and trr else None)} | {fmt(rr/tcr if rr and tcr else None)} |\n")
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] HZ11 span return logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary_md}"
