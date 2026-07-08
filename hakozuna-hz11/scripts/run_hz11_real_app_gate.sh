#!/usr/bin/env bash
# HZ11LaneFullEvidenceGate-L1: run an arbitrary REAL-APP command under LD_PRELOAD of
# each allocator lane (+ tcmalloc/jemalloc/glibc), measure wall + max/current RSS +
# HZ11 counters (xfer_insert, xfer_spill, refill_xfer, span_create, cached_bytes),
# RUNS=N, tabulate. Reuses the run_sampled RSS-sampling + field_from_stats pattern.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUNS=3
TIMEOUT=120
WORKLOAD_NAME=""
WORKLOAD_CMD=""
ALLOCATORS=""
OUTDIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --workload-name) WORKLOAD_NAME="$2"; shift 2 ;;
    --workload-cmd)  WORKLOAD_CMD="$2";  shift 2 ;;
    --allocators)    ALLOCATORS="$2";    shift 2 ;;
    --runs)          RUNS="$2";          shift 2 ;;
    --timeout)       TIMEOUT="$2";       shift 2 ;;
    --outdir)        OUTDIR="$2";        shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

[[ -n "${WORKLOAD_CMD}" && -n "${ALLOCATORS}" ]] || { echo "need --workload-cmd and --allocators" >&2; exit 2; }
[[ -n "${WORKLOAD_NAME}" ]] || WORKLOAD_NAME="realapp"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUTDIR="${OUTDIR:-${ROOT}/../bench_results/hz11_real_app_${WORKLOAD_NAME}_${STAMP}}"
mkdir -p "${OUTDIR}"

TCMALLOC_SO="${TCMALLOC_SO:-/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4}"
JEMALLOC_SO="${JEMALLOC_SO:-/usr/lib/x86_64-linux-gnu/libjemalloc.so.2}"

resolve_lib() {
  case "$1" in
    glibc) printf '' ;;
    tcmalloc) printf '%s' "${TCMALLOC_SO}" ;;
    jemalloc) printf '%s' "${JEMALLOC_SO}" ;;
    hz11-span-transfer) printf '%s/libhz11_span_transfer.so' "${ROOT}" ;;
    hz11-thread-exit-cap-batch32-fine128) printf '%s/libhz11_span_transfer_thread_exit_cap_batch32_fine128.so' "${ROOT}" ;;
    hz11-thread-exit-cap-batch32-fine128-cachecap768-bytes) printf '%s/libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap768_bytes.so' "${ROOT}" ;;
    hz11-thread-exit-cap-batch32-fine128-cachecap1024-bytes) printf '%s/libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap1024_bytes.so' "${ROOT}" ;;
    *) printf '' ;;
  esac
}

read_kb() { awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true; }

collect_tree_pids() {
  local pid="$1" child
  printf '%s\n' "${pid}"
  for child in $(pgrep -P "${pid}" 2>/dev/null || true); do
    collect_tree_pids "${child}"
  done
}

read_tree_kb() {
  local root="$1" key="$2" total=0 pid value
  while IFS= read -r pid; do
    value="$(read_kb "${pid}" "${key}" || true)"
    [[ -n "${value}" ]] && total=$((total + value))
  done < <(collect_tree_pids "${root}")
  printf '%s\n' "${total}"
}

field_from_stats() {
  local key="$1"; shift
  # Take the FIRST hz11_shim_exit_stats line: the workload process exits (and dumps)
  # before the bash -c wrapper, so its dump is first. (The wrapper also loads the shim
  # under LD_PRELOAD and dumps last -- we must not use that one.)
  awk -v key="${key}" '
    /hz11_shim_exit_stats/ && !done {
      for (i=1; i<=NF; ++i) { split($i,a,"="); if (a[1] == key) v=a[2] }
      done=1
    } END { print v+0 }' "$@" 2>/dev/null || echo 0
}

samples="${OUTDIR}/samples.csv"
printf 'workload,allocator,run,status,wall_sec,max_rss_kb,current_rss_kb,xfer_insert,xfer_spill,refill_xfer,span_create,cached_bytes\n' >"${samples}"

run_one() {
  local alloc="$1" run="$2" lib="$3"
  local err="${OUTDIR}/${WORKLOAD_NAME}_${alloc}_${run}.err"
  local out="${OUTDIR}/${WORKLOAD_NAME}_${alloc}_${run}.out"
  local status=OK rc=0 max_rss=NA cur_rss=NA last="" pid start now wall
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  case "${alloc}" in
    hz11-*) cmd+=(HZ11_DUMP_STATS=1) ;;
  esac
  cmd+=(bash -c "${WORKLOAD_CMD}")
  : >"${err}"; : >"${out}"
  start="$(date +%s%N)"
  "${cmd[@]}" >"${out}" 2>"${err}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    local sample hwm
    sample="$(read_tree_kb "${pid}" VmRSS || true)"
    hwm="$(read_tree_kb "${pid}" VmHWM || true)"
    [[ -n "${sample}" && "${sample}" -gt 0 ]] && { last="${sample}"; cur_rss="${sample}"; }
    [[ -n "${hwm}" && "${hwm}" -gt 0 && ( "${max_rss}" == NA || "${hwm}" -gt "${max_rss}" ) ]] && max_rss="${hwm}"
    now="$(date +%s%N)"
    if awk -v s="${start}" -v n="${now}" -v t="${TIMEOUT}" 'BEGIN{exit !(((n-s)/1000000000)>t)}'; then
      status=TIMEOUT; kill -KILL "${pid}" 2>/dev/null || true; break
    fi
    sleep 0.02
  done
  set +e; wait "${pid}" 2>/dev/null; rc=$?; set -e
  now="$(date +%s%N)"
  wall="$(awk -v s="${start}" -v e="${now}" 'BEGIN{printf "%.3f",(e-s)/1000000000}')"
  [[ "${status}" != TIMEOUT && "${rc}" -ne 0 ]] && status=FAIL
  [[ -z "${last}" ]] && cur_rss=NA
  [[ "${max_rss}" == NA && "${cur_rss}" != NA ]] && max_rss="${cur_rss}"
  local xi xs rx sc cb
  xi="$(field_from_stats xfer_insert "${err}" "${out}")"
  xs="$(field_from_stats xfer_spill "${err}" "${out}")"
  rx="$(field_from_stats refill_xfer "${err}" "${out}")"
  sc="$(field_from_stats span_create "${err}" "${out}")"
  cb="$(field_from_stats cached_bytes "${err}" "${out}")"
  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "${WORKLOAD_NAME}" "${alloc}" "${run}" "${status}" \
    "${wall}" "${max_rss}" "${cur_rss}" "${xi}" "${xs}" "${rx}" "${sc}" "${cb}" >>"${samples}"
}

IFS=',' read -r -a allocs <<<"${ALLOCATORS}"
for run in $(seq 1 "${RUNS}"); do
  for alloc in "${allocs[@]}"; do
    lib="$(resolve_lib "${alloc}")"
    run_one "${alloc}" "${run}" "${lib}"
    echo "[RUN] workload=${WORKLOAD_NAME} alloc=${alloc} run=${run} done"
  done
done

summary="${OUTDIR}/summary.md"
python3 - "${samples}" "${summary}" "${WORKLOAD_NAME}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
rows=list(csv.DictReader(open(sys.argv[1], newline="")))
name=sys.argv[3]
sg=defaultdict(list)
for r in rows: sg[r["allocator"]].append(r)
def num(v): return None if v in ("","NA") else float(v)
def med(items,k):
    vs=[num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
    return statistics.median(vs) if vs else None
def fmt(v): return "NA" if v is None else f"{v:.3f}"
def tot(items,k): return sum(int(float(r[k])) for r in items if r.get(k,"") not in ("","NA"))
# base = first allocator in input order (caller puts tcmalloc first)
order=list(dict.fromkeys(r["allocator"] for r in rows))
base=order[0]
bw=med(sg.get(base,[]),"wall_sec")
with open(sys.argv[2],"w") as f:
    f.write(f"# HZ11 Real-App Gate: {name}\n\n")
    f.write(f"Runs: {len(sg.get(base,[]))}. Base/reference: `{base}`.\n\n")
    f.write("| Allocator | status | median wall sec | wall/base | median max RSS KiB | maxRSS/base | xfer_insert | xfer_spill | refill_xfer | span_create | cached_bytes |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for a in order:
        it=sg[a]; st={}
        for r in it: st[r["status"]]=st.get(r["status"],0)+1
        stat=" ".join(f"{k}:{st[k]}" for k in sorted(st))
        w=med(it,"wall_sec"); rss=med(it,"max_rss_kb")
        f.write(f"| {a} | {stat} | {fmt(w)} | {fmt(w/bw if w and bw else None)} | {fmt(rss)} | {fmt(rss/med(sg.get(base,[]),'max_rss_kb') if rss and med(sg.get(base,[]),'max_rss_kb') else None)} | {tot(it,'xfer_insert')} | {tot(it,'xfer_spill')} | {tot(it,'refill_xfer')} | {tot(it,'span_create')} | {tot(it,'cached_bytes')} |\n")
    f.write(f"\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] ${WORKLOAD_NAME}: ${summary}"
