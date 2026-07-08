#!/usr/bin/env bash
# HZ11FineclassPythonAllocCurrentRSS-L1: diagnostics only.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_fineclass_python_alloc_rss_${STAMP}}"
RUNS="${RUNS:-10}"
BUILD="${BUILD:-1}"
PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
SAMPLE_SLEEP="${SAMPLE_SLEEP:-0.005}"

mkdir -p "${OUTDIR}"

find_first_existing() {
  local path
  for path in "$@"; do
    [[ -n "${path}" && -f "${path}" ]] && { printf '%s\n' "${path}"; return 0; }
  done
  return 1
}

tcmalloc_lib="$(find_first_existing "${TCMALLOC_SO:-}" "${TCMALLOC_LIB:-}" \
  /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 || true)"
if [[ -z "${tcmalloc_lib}" ]]; then
  echo "tcmalloc library missing; set TCMALLOC_SO" >&2
  exit 1
fi

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" \
    preload-span-transfer-thread-exit-cap-batch32 \
    preload-span-transfer-thread-exit-cap-batch32-live-diag \
    preload-span-transfer-thread-exit-cap-batch32-fineclass \
    preload-span-transfer-thread-exit-cap-batch32-fineclass-live-diag >/dev/null
fi

samples="${OUTDIR}/samples.csv"
central="${OUTDIR}/central_classes.csv"
source="${OUTDIR}/class_source.csv"
live="${OUTDIR}/live_footprint.csv"
summary="${OUTDIR}/summary.md"
printf 'allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,rss_gap_kb,xfer_hit,xfer_insert,xfer_spill,central_hit,central_insert,span_create,log,stderr_log,stdout_log,note\n' >"${samples}"
printf 'allocator,run,class,count,high_water,max_overflow_need,insert,remove,overflow\n' >"${central}"
printf 'allocator,run,class,span_create,span_reuse,current_span_exhaust,current_span_replace,transfer_refill_hit,transfer_refill_miss,central_refill_hit,central_refill_miss,arena_carve,live_current_span,created_high_water,sweep_count,sweep_scanned,meta_lock\n' >"${source}"
printf 'allocator,run,class,slot,alloc,free,live,live_high,bytes_live,bytes_high,req_bytes_live,req_bytes_high,req_bytes_alloc\n' >"${live}"

csv_escape() {
  local s="${1//$'\n'/ }"; s="${s//\"/\"\"}"; printf '"%s"' "${s}"
}

read_kb() {
  awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true
}

field_from_stats() {
  local key="$1"; shift
  awk -v key="${key}" '/hz11_shim_exit_stats/ {
    for (i=1; i<=NF; ++i) { split($i,a,"="); if (a[1] == key) v=a[2] }
  } END { print v+0 }' "$@" 2>/dev/null || echo 0
}

write_sample() {
  {
    csv_escape "$1"; printf ',%s,' "$2"; csv_escape "$3"; printf ','
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,' \
      "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}" "${14}"
    csv_escape "${15}"; printf ','; csv_escape "${16}"; printf ','
    csv_escape "${17}"; printf ','; csv_escape "${18}"; printf '\n'
  } >>"${samples}"
}

parse_central() {
  local allocator="$1" run="$2" err="$3" out="$4"
  awk -v a="${allocator}" -v r="${run}" '
    /hz11_central_class / {
      delete v
      for (i = 1; i <= NF; ++i) { split($i, p, "="); v[p[1]] = p[2] }
      printf "\"%s\",%s,%s,%s,%s,%s,%s,%s,%s\n",
        a, r, v["class"]+0, v["count"]+0, v["high_water"]+0,
        v["max_overflow_need"]+0, v["insert"]+0, v["remove"]+0,
        v["overflow"]+0
    }' "${err}" "${out}" >>"${central}" 2>/dev/null || true
}

parse_source() {
  local allocator="$1" run="$2" err="$3" out="$4"
  awk -v a="${allocator}" -v r="${run}" '
    /hz11_span_source_class/ {
      delete v
      for (i = 1; i <= NF; ++i) { split($i, p, "="); v[p[1]] = p[2] }
      printf "\"%s\",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        a, r, v["class"]+0, v["span_create"]+0, v["span_reuse"]+0,
        v["current_span_exhaust"]+0, v["current_span_replace"]+0,
        v["transfer_refill_hit"]+0, v["transfer_refill_miss"]+0,
        v["central_refill_hit"]+0, v["central_refill_miss"]+0,
        v["arena_carve"]+0, v["live_current_span"]+0,
        v["created_high_water"]+0, v["sweep_count"]+0,
        v["sweep_scanned"]+0, v["meta_lock"]+0
    }' "${err}" "${out}" >>"${source}" 2>/dev/null || true
}

parse_live() {
  local allocator="$1" run="$2" err="$3" out="$4"
  awk -v a="${allocator}" -v r="${run}" '
    /hz11_live_footprint_class/ {
      delete v
      for (i = 1; i <= NF; ++i) { split($i, p, "="); v[p[1]] = p[2] }
      printf "\"%s\",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        a, r, v["class"]+0, v["slot"]+0, v["alloc"]+0, v["free"]+0,
        v["live"]+0, v["live_high"]+0, v["bytes_live"]+0,
        v["bytes_high"]+0, v["req_bytes_live"]+0,
        v["req_bytes_high"]+0, v["req_bytes_alloc"]+0
    }' "${err}" "${out}" >>"${live}" 2>/dev/null || true
}

run_python_alloc() {
  local allocator="$1" run="$2" lib="$3" diag="$4"
  local log="${OUTDIR}/python_alloc_${allocator}_${run}.log"
  local err="${OUTDIR}/python_alloc_${allocator}_${run}.err"
  local out="${OUTDIR}/python_alloc_${allocator}_${run}.out"
  local status=OK rc=0 max_rss=NA cur_rss=NA last_rss="" sample hwm pid start now wall gap
  local -a cmd=(env PYTHONMALLOC=malloc)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  case "${diag}" in
    stats) cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1 HZ11_DUMP_CURRENT_SPAN_POOL=1) ;;
    live) cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1 HZ11_DUMP_CURRENT_SPAN_POOL=1 HZ11_DUMP_SPAN_SOURCE=1 HZ11_DUMP_LIVE_FOOTPRINT=1) ;;
  esac
  cmd+=(/usr/bin/python3 - "${PYTHON_LOOPS}")
  : >"${log}"; : >"${err}"; : >"${out}"
  printf '[cmd]' >"${log}"; printf ' %q' "${cmd[@]}" >>"${log}"; printf '\n' >>"${log}"
  start="$(date +%s%N)"
  "${cmd[@]}" >"${out}" 2>"${err}" <<'PY' &
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
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_kb "${pid}" VmRSS || true)"
    hwm="$(read_kb "${pid}" VmHWM || true)"
    [[ -n "${sample}" ]] && { last_rss="${sample}"; cur_rss="${sample}"; }
    [[ -n "${hwm}" && ( "${max_rss}" == NA || "${hwm}" -gt "${max_rss}" ) ]] && max_rss="${hwm}"
    now="$(date +%s%N)"
    if awk -v s="${start}" -v n="${now}" 'BEGIN{exit !(((n-s)/1000000000)>60)}'; then
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
  [[ -z "${last_rss}" ]] && cur_rss=NA
  [[ "${max_rss}" == NA && "${cur_rss}" != NA ]] && max_rss="${cur_rss}"
  gap="$(awk -v a="${max_rss}" -v b="${cur_rss}" 'BEGIN{if(a=="NA"||b=="NA")print "NA"; else print a-b}')"
  cat "${out}" "${err}" >>"${log}" || true
  local xfer_hit xfer_insert xfer_spill central_hit central_insert span_create
  xfer_hit="$(field_from_stats xfer_hit "${err}" "${out}")"
  xfer_insert="$(field_from_stats xfer_insert "${err}" "${out}")"
  xfer_spill="$(field_from_stats xfer_spill "${err}" "${out}")"
  central_hit="$(field_from_stats central_hit "${err}" "${out}")"
  central_insert="$(field_from_stats central_insert "${err}" "${out}")"
  span_create="$(field_from_stats span_create "${err}" "${out}")"
  write_sample "${allocator}" "${run}" "${status}" "${rc}" "${wall}" "${max_rss}" \
    "${cur_rss}" "${gap}" "${xfer_hit}" "${xfer_insert}" "${xfer_spill}" \
    "${central_hit}" "${central_insert}" "${span_create}" \
    "${log}" "${err}" "${out}" ""
  parse_central "${allocator}" "${run}" "${err}" "${out}"
  parse_source "${allocator}" "${run}" "${err}" "${out}"
  parse_live "${allocator}" "${run}" "${err}" "${out}"
}

for run in $(seq 1 "${RUNS}"); do
  run_python_alloc tcmalloc "${run}" "${tcmalloc_lib}" none
  run_python_alloc hz11-thread-exit-cap-batch32 "${run}" \
    "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32.so" stats
  run_python_alloc hz11-thread-exit-cap-batch32-fineclass "${run}" \
    "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so" stats
  run_python_alloc hz11-thread-exit-cap-batch32-live-diag "${run}" \
    "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32_live_diag.so" live
  run_python_alloc hz11-thread-exit-cap-batch32-fineclass-live-diag "${run}" \
    "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32_fineclass_live_diag.so" live
done

python3 - "${samples}" "${central}" "${source}" "${live}" "${summary}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
samples=list(csv.DictReader(open(sys.argv[1], newline="")))
central=list(csv.DictReader(open(sys.argv[2], newline="")))
source=list(csv.DictReader(open(sys.argv[3], newline="")))
live=list(csv.DictReader(open(sys.argv[4], newline="")))
sg=defaultdict(list); cg=defaultdict(list); so=defaultdict(list); lv=defaultdict(list)
for r in samples: sg[r["allocator"]].append(r)
for r in central: cg[(r["allocator"], r["class"])].append(r)
for r in source: so[(r["allocator"], r["class"])].append(r)
for r in live: lv[(r["allocator"], r["class"])].append(r)
def n(v): return None if v in ("","NA","-") else float(v)
def vals(items,k): return [n(r[k]) for r in items if r["status"]=="OK" and n(r[k]) is not None]
def med(items,k):
    v=vals(items,k)
    return statistics.median(v) if v else None
def q(items,k,p):
    v=sorted(vals(items,k))
    if not v: return None
    return v[int(round((len(v)-1)*p))]
def rng(items,k):
    v=vals(items,k)
    return (min(v), max(v)) if v else (None, None)
def total(items,k): return sum(int(float(r[k])) for r in items if r.get(k,"") not in ("","NA","-"))
def maxv(items,k):
    v=[int(float(r[k])) for r in items if r.get(k,"") not in ("","NA","-")]
    return max(v) if v else 0
def fmt(v): return "NA" if v is None else f"{v:.3f}"
order=[
    "tcmalloc",
    "hz11-thread-exit-cap-batch32",
    "hz11-thread-exit-cap-batch32-fineclass",
    "hz11-thread-exit-cap-batch32-live-diag",
    "hz11-thread-exit-cap-batch32-fineclass-live-diag",
]
base=med(sg.get("tcmalloc",[]),"current_rss_kb")
with open(sys.argv[5],"w") as f:
    f.write("# HZ11 Fineclass Python Alloc Current RSS L1\n\n")
    f.write("Diagnostics only. Live-diagnostic rows include side-table overhead and are not RSS gate rows.\n\n")
    f.write("## RSS Samples\n\n")
    f.write("| allocator | OK runs | median wall sec | median max RSS KiB | max RSS min..max | median current RSS KiB | current RSS min..max | current/tcmalloc | median max-current gap KiB | xfer_insert | central_insert | span_create |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for alloc in order:
        items=sg.get(alloc,[])
        if not items: continue
        ok=sum(1 for r in items if r["status"]=="OK")
        max_lo,max_hi=rng(items,"max_rss_kb")
        cur_lo,cur_hi=rng(items,"current_rss_kb")
        cur=med(items,"current_rss_kb")
        f.write(f"| {alloc} | {ok} | {fmt(med(items,'wall_sec'))} | {fmt(med(items,'max_rss_kb'))} | {fmt(max_lo)}..{fmt(max_hi)} | {fmt(cur)} | {fmt(cur_lo)}..{fmt(cur_hi)} | {fmt(cur/base if cur and base else None)} | {fmt(med(items,'rss_gap_kb'))} | {total(items,'xfer_insert')} | {total(items,'central_insert')} | {total(items,'span_create')} |\n")
    f.write("\n## Central Classes\n\n")
    f.write("| allocator | class | final count total | high-water max | insert total | remove total | overflow total |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|\n")
    for alloc in order:
        rows=[(cls,items) for (a,cls),items in cg.items() if a==alloc]
        for cls,items in sorted(rows, key=lambda kv: -maxv(kv[1],"high_water")):
            if total(items,"insert")==0 and maxv(items,"high_water")==0:
                continue
            f.write(f"| {alloc} | {cls} | {total(items,'count')} | {maxv(items,'high_water')} | {total(items,'insert')} | {total(items,'remove')} | {total(items,'overflow')} |\n")
    f.write("\n## Source Classes\n\n")
    f.write("| allocator | class | span create | arena carve | span reuse | live current span | created high-water |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|\n")
    for alloc in order:
        rows=[(cls,items) for (a,cls),items in so.items() if a==alloc]
        for cls,items in sorted(rows, key=lambda kv: -total(kv[1],"span_create")):
            if total(items,"span_create")==0 and total(items,"arena_carve")==0:
                continue
            f.write(f"| {alloc} | {cls} | {total(items,'span_create')} | {total(items,'arena_carve')} | {total(items,'span_reuse')} | {total(items,'live_current_span')} | {maxv(items,'created_high_water')} |\n")
    f.write("\n## Live Diagnostic Totals\n\n")
    f.write("| allocator | requested bytes high | slot bytes high | request-to-slot waste | live objects high | alloc count |\n")
    f.write("|---|---:|---:|---:|---:|---:|\n")
    for alloc in ("hz11-thread-exit-cap-batch32-live-diag","hz11-thread-exit-cap-batch32-fineclass-live-diag"):
        req=slot=waste=live_high=allocs=0
        for (a,cls),items in lv.items():
            if a!=alloc: continue
            rh=maxv(items,"req_bytes_high")
            sh=maxv(items,"bytes_high")
            req += rh; slot += sh; waste += max(0, sh-rh)
            live_high += maxv(items,"live_high")
            allocs += total(items,"alloc")
        f.write(f"| {alloc} | {req} | {slot} | {waste} | {live_high} | {allocs} |\n")
    f.write("\nRaw samples: `samples.csv`; central: `central_classes.csv`; source: `class_source.csv`; live: `live_footprint.csv`\n")
PY

echo "[DONE] HZ11 fineclass python_alloc logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
