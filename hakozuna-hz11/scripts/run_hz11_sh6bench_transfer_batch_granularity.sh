#!/usr/bin/env bash
# HZ11Sh6benchTransferBatchGranularity-L1: compare transfer batch widths.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_sh6bench_transfer_batch_granularity_${STAMP}}"
RUNS="${RUNS:-3}"
BUILD="${BUILD:-1}"
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
  /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 || true)"
sh6bench_bin="$(find_first_executable "${SH6BENCH_BIN:-}" \
  /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/sh6bench || true)"

if [[ -z "${sh6bench_bin}" ]]; then
  echo "sh6bench binary missing; set SH6BENCH_BIN" >&2
  exit 1
fi

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" \
    preload-span-transfer-thread-exit-cap \
    preload-span-transfer-thread-exit-cap-batch32 \
    preload-span-transfer-thread-exit-cap-batch64 \
    preload-span-transfer-thread-exit-cap-batch128 >/dev/null
fi

samples="${OUTDIR}/samples.csv"
central="${OUTDIR}/central_classes.csv"
summary="${OUTDIR}/summary.md"
printf 'allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,xfer_hit,xfer_miss,xfer_insert,xfer_spill,central_hit,central_miss,central_insert,span_create,log,stderr_log,stdout_log,note\n' >"${samples}"
printf 'allocator,run,class,count,high_water,max_overflow_need,insert,remove,overflow\n' >"${central}"

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
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,' \
      "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}" "${14}" "${15}"
    csv_escape "${16}"; printf ','; csv_escape "${17}"; printf ','
    csv_escape "${18}"; printf ','; csv_escape "${19}"; printf '\n'
  } >>"${samples}"
}

parse_central_classes() {
  local allocator="$1" run="$2" err="$3" out="$4"
  awk -v a="${allocator}" -v r="${run}" '
    /hz11_central_class / {
      delete v
      for (i = 1; i <= NF; ++i) {
        split($i, p, "=")
        v[p[1]] = p[2]
      }
      printf "\"%s\",%s,%s,%s,%s,%s,%s,%s,%s\n",
        a, r, v["class"]+0, v["count"]+0, v["high_water"]+0,
        v["max_overflow_need"]+0, v["insert"]+0, v["remove"]+0,
        v["overflow"]+0
    }' "${err}" "${out}" >>"${central}" 2>/dev/null || true
}

run_sampled() {
  local allocator="$1" run="$2" lib="$3"
  local log="${OUTDIR}/sh6bench_${allocator}_${run}.log"
  local err="${OUTDIR}/sh6bench_${allocator}_${run}.err"
  local out="${OUTDIR}/sh6bench_${allocator}_${run}.out"
  local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
  local status=OK rc=0 max_rss=NA cur_rss=NA last_rss="" sample hwm pid start now wall
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  [[ -n "${lib}" ]] && cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1 HZ11_DUMP_CURRENT_SPAN_POOL=1)
  printf '%s\n%s\n%s\n%s\n' "${SH6_CALL_COUNT}" "${SH6_MIN_BLOCK}" \
    "${SH6_MAX_BLOCK}" "${SH6_THREADS}" >"${input}"
  cmd+=("${sh6bench_bin}" "${input}")
  : >"${log}"; : >"${err}"; : >"${out}"
  printf '[cmd]' >"${log}"; printf ' %q' "${cmd[@]}" >>"${log}"; printf '\n' >>"${log}"
  start="$(date +%s%N)"
  "${cmd[@]}" >"${out}" 2>"${err}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_kb "${pid}" VmRSS || true)"
    hwm="$(read_kb "${pid}" VmHWM || true)"
    [[ -n "${sample}" ]] && { last_rss="${sample}"; cur_rss="${sample}"; }
    [[ -n "${hwm}" && ( "${max_rss}" == NA || "${hwm}" -gt "${max_rss}" ) ]] && max_rss="${hwm}"
    now="$(date +%s%N)"
    if awk -v s="${start}" -v n="${now}" 'BEGIN{exit !(((n-s)/1000000000)>30)}'; then
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
  local xfer_hit xfer_miss xfer_insert xfer_spill central_hit central_miss central_insert span_create
  xfer_hit="$(field_from_stats xfer_hit "${err}" "${out}")"
  xfer_miss="$(field_from_stats xfer_miss "${err}" "${out}")"
  xfer_insert="$(field_from_stats xfer_insert "${err}" "${out}")"
  xfer_spill="$(field_from_stats xfer_spill "${err}" "${out}")"
  central_hit="$(field_from_stats central_hit "${err}" "${out}")"
  central_miss="$(field_from_stats central_miss "${err}" "${out}")"
  central_insert="$(field_from_stats central_insert "${err}" "${out}")"
  span_create="$(field_from_stats span_create "${err}" "${out}")"
  write_sample "${allocator}" "${run}" "${status}" "${rc}" "${wall}" "${max_rss}" \
    "${cur_rss}" "${xfer_hit}" "${xfer_miss}" "${xfer_insert}" "${xfer_spill}" \
    "${central_hit}" "${central_miss}" "${central_insert}" "${span_create}" \
    "${log}" "${err}" "${out}" ""
  parse_central_classes "${allocator}" "${run}" "${err}" "${out}"
}

for run in $(seq 1 "${RUNS}"); do
  [[ -n "${tcmalloc_lib}" ]] && run_sampled tcmalloc "${run}" "${tcmalloc_lib}"
  run_sampled hz11-thread-exit-cap-batch16 "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap.so"
  run_sampled hz11-thread-exit-cap-batch32 "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32.so"
  run_sampled hz11-thread-exit-cap-batch64 "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch64.so"
  run_sampled hz11-thread-exit-cap-batch128 "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch128.so"
done

python3 - "${samples}" "${central}" "${summary}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
samples=list(csv.DictReader(open(sys.argv[1], newline="")))
central=list(csv.DictReader(open(sys.argv[2], newline="")))
sg=defaultdict(list)
cg=defaultdict(list)
for r in samples: sg[r["allocator"]].append(r)
for r in central: cg[r["allocator"]].append(r)
def num(v): return None if v in ("","NA","-") else float(v)
def med(items,k):
    vals=[num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
    return statistics.median(vals) if vals else None
def total(items,k): return sum(int(float(r[k])) for r in items if r.get(k,"") not in ("","NA","-"))
def maxv(items,k):
    vals=[int(float(r[k])) for r in items if r.get(k,"") not in ("","NA","-")]
    return max(vals) if vals else 0
def fmt(v): return "NA" if v is None else f"{v:.3f}"
def status(items):
    d=defaultdict(int)
    for r in items: d[r["status"]]+=1
    return " ".join(f"{k}:{d[k]}" for k in sorted(d))
base_wall=med(sg.get("tcmalloc",[]),"wall_sec")
base_rss=med(sg.get("tcmalloc",[]),"max_rss_kb")
order=[
    "tcmalloc",
    "hz11-thread-exit-cap-batch16",
    "hz11-thread-exit-cap-batch32",
    "hz11-thread-exit-cap-batch64",
    "hz11-thread-exit-cap-batch128",
]
with open(sys.argv[3],"w") as f:
    f.write("# HZ11 Sh6bench Transfer Batch Granularity L1\n\n")
    f.write(f"RUNS={len(next(iter(sg.values()), []))}\n\n")
    f.write("## Samples\n\n")
    f.write("| Allocator | status | median wall sec | wall/tcmalloc | median max RSS KiB | max RSS/tcmalloc | median current RSS KiB | xfer_hit | xfer_miss | xfer_insert | xfer_spill | central_hit | central_miss | central_insert | central_high_water_max | span_create |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for alloc in order:
        if alloc not in sg:
            continue
        items=sg[alloc]
        wall=med(items,"wall_sec"); rss=med(items,"max_rss_kb"); cur=med(items,"current_rss_kb")
        f.write(f"| {alloc} | {status(items)} | {fmt(wall)} | {fmt(wall/base_wall if wall and base_wall else None)} | {fmt(rss)} | {fmt(rss/base_rss if rss and base_rss else None)} | {fmt(cur)} | {total(items,'xfer_hit')} | {total(items,'xfer_miss')} | {total(items,'xfer_insert')} | {total(items,'xfer_spill')} | {total(items,'central_hit')} | {total(items,'central_miss')} | {total(items,'central_insert')} | {maxv(cg.get(alloc,[]),'high_water')} | {total(items,'span_create')} |\n")
    f.write("\n## Central Classes\n\n")
    f.write("| Allocator | class | final count total | high_water max | insert total | remove total | overflow total |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|\n")
    by_key=defaultdict(list)
    for r in central:
        by_key[(r["allocator"], r["class"])].append(r)
    for alloc in order:
        rows=[(cls, items) for (a, cls), items in by_key.items() if a == alloc]
        for cls, items in sorted(rows, key=lambda kv: -maxv(kv[1],"high_water")):
            if total(items,"insert") == 0 and maxv(items,"high_water") == 0:
                continue
            f.write(f"| {alloc} | {cls} | {total(items,'count')} | {maxv(items,'high_water')} | {total(items,'insert')} | {total(items,'remove')} | {total(items,'overflow')} |\n")
    f.write("\nRaw samples: `samples.csv`; raw central classes: `central_classes.csv`\n")
PY

echo "[DONE] HZ11 sh6bench batch-granularity logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
