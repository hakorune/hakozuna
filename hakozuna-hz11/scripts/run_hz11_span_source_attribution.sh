#!/usr/bin/env bash
# HZ11SpanSourceAttribution-L1: source/current-span attribution gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_span_source_attr_${STAMP}}"
RUNS="${RUNS:-3}"
ROWS="${ROWS:-larson,sh6bench,python_alloc,mstress}"
BUILD="${BUILD:-1}"

PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
MSTRESS_THREADS="${MSTRESS_THREADS:-8}"
MSTRESS_SCALE="${MSTRESS_SCALE:-80}"
MSTRESS_ITER="${MSTRESS_ITER:-3}"
LARSON_SECONDS="${LARSON_SECONDS:-2}"
LARSON_THREADS="${LARSON_THREADS:-4}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
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
  /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 || true)"
larson_bin="$(find_first_executable "${LARSON_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/larson_system \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson || true)"
sh6bench_bin="$(find_first_executable "${SH6BENCH_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/sh6bench || true)"
mstress_bin="$(find_first_executable "${MSTRESS_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/mstress || true)"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-source-diag preload-span-return-source-diag >/dev/null
fi

samples="${OUTDIR}/samples.csv"
classes="${OUTDIR}/class_source.csv"
summary="${OUTDIR}/summary.md"
printf 'workload,allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,log,stderr_log,stdout_log,note\n' >"${samples}"
printf 'workload,allocator,run,class,span_create,span_reuse,current_span_exhaust,current_span_replace,transfer_refill_hit,transfer_refill_miss,central_refill_hit,central_refill_miss,arena_carve,live_current_span,created_high_water,sweep_count,sweep_scanned,meta_lock\n' >"${classes}"

{
  echo "[HZ11_SPAN_SOURCE_ATTR] ts=${STAMP}"
  echo "[HZ11_SPAN_SOURCE_ATTR] rows=${ROWS}"
  echo "[HZ11_SPAN_SOURCE_ATTR] runs=${RUNS}"
  echo "[HZ11_SPAN_SOURCE_ATTR] tcmalloc=${tcmalloc_lib:-SKIP}"
} >"${OUTDIR}/README.log"

csv_escape() {
  local s="${1//$'\n'/ }"; s="${s//\"/\"\"}"; printf '"%s"' "${s}"
}

read_kb() {
  awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true
}

write_sample() {
  {
    csv_escape "$1"; printf ','; csv_escape "$2"; printf ','
    printf '%s,' "$3"; csv_escape "$4"; printf ','
    printf '%s,%s,%s,%s,' "$5" "$6" "$7" "$8"
    csv_escape "$9"; printf ','; csv_escape "${10}"; printf ','; csv_escape "${11}"; printf ','
    csv_escape "${12}"; printf '\n'
  } >>"${samples}"
}

parse_classes() {
  local workload="$1" allocator="$2" run="$3" err="$4" out="$5"
  awk -v w="${workload}" -v a="${allocator}" -v r="${run}" '
    /hz11_span_source_class/ {
      delete v
      for (i = 1; i <= NF; ++i) {
        split($i, p, "=")
        v[p[1]] = p[2]
      }
      printf "\"%s\",\"%s\",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        w, a, r, v["class"]+0, v["span_create"]+0, v["span_reuse"]+0,
        v["current_span_exhaust"]+0, v["current_span_replace"]+0,
        v["transfer_refill_hit"]+0, v["transfer_refill_miss"]+0,
        v["central_refill_hit"]+0, v["central_refill_miss"]+0,
        v["arena_carve"]+0, v["live_current_span"]+0,
        v["created_high_water"]+0, v["sweep_count"]+0,
        v["sweep_scanned"]+0, v["meta_lock"]+0
    }' "${err}" "${out}" >>"${classes}" 2>/dev/null || true
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
  write_sample "${workload}" "${allocator}" "${run}" "${status}" "${rc}" "${wall}" \
    "${max_rss}" "${cur_rss}" "${log}" "${err}" "${out}" ""
  parse_classes "${workload}" "${allocator}" "${run}" "${err}" "${out}"
}

run_skip() {
  write_sample "$1" "$2" "$3" SKIP - NA NA NA "" "" "" "$4"
}

run_row() {
  local row="$1" allocator="$2" run="$3" lib="$4"
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  [[ "${allocator}" == hz11-* ]] && cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_SPAN_SOURCE=1)
  case "${row}" in
    larson)
      [[ -z "${larson_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "larson missing"; return; }
      cmd+=("${larson_bin}" "${LARSON_SECONDS}" 8 128 128 1 12345 "${LARSON_THREADS}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${cmd[@]}" ;;
    sh6bench)
      [[ -z "${sh6bench_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "sh6bench missing"; return; }
      local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
      printf '%s\n1\n1000\n%s\n' "${SH6_CALL_COUNT}" "${SH6_THREADS}" >"${input}"
      cmd+=("${sh6bench_bin}" "${input}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${cmd[@]}" ;;
    python_alloc)
      cmd+=(PYTHONMALLOC=malloc /usr/bin/python3 -c 'import sys
loops=int(sys.argv[1]); bag=[]
for r in range(loops):
  for i in range(20000): bag.append(bytearray((i % 4096)+1))
  del bag[::3]; bag=bag[-20000:]
print(len(bag))' "${PYTHON_LOOPS}")
      run_sampled "${row}" "${allocator}" "${run}" 60 "${cmd[@]}" ;;
    mstress)
      [[ -z "${mstress_bin}" ]] && { run_skip "${row}" "${allocator}" "${run}" "mstress missing"; return; }
      cmd+=("${mstress_bin}" "${MSTRESS_THREADS}" "${MSTRESS_SCALE}" "${MSTRESS_ITER}")
      run_sampled "${row}" "${allocator}" "${run}" 30 "${cmd[@]}" ;;
    *) run_skip "${row}" "${allocator}" "${run}" "unknown row" ;;
  esac
}

IFS=',' read -r -a rows <<<"${ROWS}"
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    [[ -n "${tcmalloc_lib}" ]] && run_row "${row}" tcmalloc "${run}" "${tcmalloc_lib}" || run_skip "${row}" tcmalloc "${run}" "tcmalloc missing"
    run_row "${row}" hz11-span-transfer "${run}" "${ROOT}/libhz11_span_source_diag.so"
    run_row "${row}" hz11-span-return "${run}" "${ROOT}/libhz11_span_return_source_diag.so"
  done
done

python3 - "${samples}" "${classes}" "${summary}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
samples=list(csv.DictReader(open(sys.argv[1], newline="")))
classes=list(csv.DictReader(open(sys.argv[2], newline="")))
sg=defaultdict(list)
cg=defaultdict(list)
for r in samples: sg[(r["workload"],r["allocator"])].append(r)
for r in classes: cg[(r["workload"],r["allocator"],r["class"])].append(r)
def num(v): return None if v in ("","NA","-") else float(v)
def med(items,k):
    vals=[num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
    return statistics.median(vals) if vals else None
def total(items,k): return sum(int(float(r[k])) for r in items if r.get(k,"") not in ("","NA","-"))
def fmt(v): return "NA" if v is None else f"{v:.3f}"
def status(items):
    d=defaultdict(int)
    for r in items: d[r["status"]]+=1
    return " ".join(f"{k}:{d[k]}" for k in sorted(d))
def class_totals(w,a):
    out=[]
    for (ww,aa,c),items in cg.items():
        if ww==w and aa==a:
            out.append((c, {k: total(items,k) for k in items[0].keys() if k not in ("workload","allocator","run","class")}))
    return out
with open(sys.argv[3],"w") as f:
    f.write("# HZ11 Span Source Attribution L1\n\n")
    f.write("## Samples\n\n")
    f.write("| Workload | Allocator | status | median wall sec | median max RSS KiB | median current RSS KiB |\n")
    f.write("|---|---|---:|---:|---:|---:|\n")
    for key in sorted(sg):
        items=sg[key]
        f.write(f"| {key[0]} | {key[1]} | {status(items)} | {fmt(med(items,'wall_sec'))} | {fmt(med(items,'max_rss_kb'))} | {fmt(med(items,'current_rss_kb'))} |\n")
    f.write("\n## Top Span Create Classes\n\n")
    f.write("| Workload | Allocator | class | span_create | arena_carve | span_reuse | reuse_rate | central_bypass_rate | live_current_span | sweep_scanned | meta_lock |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for w,a in sorted({(r["workload"],r["allocator"]) for r in classes}):
        rows=class_totals(w,a)
        rows.sort(key=lambda x: x[1].get("span_create",0), reverse=True)
        for c,v in rows[:5]:
            create=v.get("span_create",0); reuse=v.get("span_reuse",0)
            transfer_miss=v.get("transfer_refill_miss",0); central_miss=v.get("central_refill_miss",0)
            reuse_rate=reuse/(create+reuse) if create+reuse else 0
            bypass=central_miss/transfer_miss if transfer_miss else 0
            f.write(f"| {w} | {a} | {c} | {create} | {v.get('arena_carve',0)} | {reuse} | {reuse_rate:.3f} | {bypass:.3f} | {v.get('live_current_span',0)} | {v.get('sweep_scanned',0)} | {v.get('meta_lock',0)} |\n")
    f.write("\nRaw samples: `samples.csv`; raw class counters: `class_source.csv`\n")
PY

echo "[DONE] HZ11 span source attribution logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
