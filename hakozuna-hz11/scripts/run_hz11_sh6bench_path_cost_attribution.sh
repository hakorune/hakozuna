#!/usr/bin/env bash
# HZ11Sh6benchPathCostAttribution-L1: sh6bench path/RSS attribution gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_sh6bench_path_cost_${STAMP}}"
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
  make -C "${ROOT}" preload-span-transfer \
    preload-span-transfer-thread-exit-cap \
    preload-span-transfer-thread-exit-cap-source-diag \
    preload-span-return-source-diag >/dev/null
fi

samples="${OUTDIR}/samples.csv"
classes="${OUTDIR}/class_source.csv"
summary="${OUTDIR}/summary.md"
printf 'allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,xfer_hit,xfer_miss,xfer_insert,xfer_spill,central_hit,central_miss,central_insert,span_create,log,stderr_log,stdout_log,note\n' >"${samples}"
printf 'allocator,run,class,span_create,span_reuse,current_span_exhaust,current_span_replace,transfer_refill_hit,transfer_refill_miss,central_refill_hit,central_refill_miss,arena_carve,live_current_span,created_high_water,sweep_count,sweep_scanned,meta_lock\n' >"${classes}"

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

parse_classes() {
  local allocator="$1" run="$2" err="$3" out="$4"
  awk -v a="${allocator}" -v r="${run}" '
    /hz11_span_source_class/ {
      delete v
      for (i = 1; i <= NF; ++i) {
        split($i, p, "=")
        v[p[1]] = p[2]
      }
      printf "\"%s\",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        a, r, v["class"]+0, v["span_create"]+0, v["span_reuse"]+0,
        v["current_span_exhaust"]+0, v["current_span_replace"]+0,
        v["transfer_refill_hit"]+0, v["transfer_refill_miss"]+0,
        v["central_refill_hit"]+0, v["central_refill_miss"]+0,
        v["arena_carve"]+0, v["live_current_span"]+0,
        v["created_high_water"]+0, v["sweep_count"]+0,
        v["sweep_scanned"]+0, v["meta_lock"]+0
    }' "${err}" "${out}" >>"${classes}" 2>/dev/null || true
}

run_sampled() {
  local allocator="$1" run="$2" lib="$3" diag="$4"
  local log="${OUTDIR}/sh6bench_${allocator}_${run}.log"
  local err="${OUTDIR}/sh6bench_${allocator}_${run}.err"
  local out="${OUTDIR}/sh6bench_${allocator}_${run}.out"
  local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
  local status=OK rc=0 max_rss=NA cur_rss=NA last_rss="" sample hwm pid start now wall
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  case "${diag}" in
    stats) cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1 HZ11_DUMP_CURRENT_SPAN_POOL=1) ;;
    source) cmd+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CENTRAL_CLASSES=1 HZ11_DUMP_CURRENT_SPAN_POOL=1 HZ11_DUMP_SPAN_SOURCE=1) ;;
  esac
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
  parse_classes "${allocator}" "${run}" "${err}" "${out}"
}

for run in $(seq 1 "${RUNS}"); do
  [[ -n "${tcmalloc_lib}" ]] && run_sampled tcmalloc "${run}" "${tcmalloc_lib}" none
  run_sampled hz11-span-transfer "${run}" "${ROOT}/libhz11_span_transfer.so" stats
  run_sampled hz11-thread-exit-cap "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap.so" stats
  run_sampled hz11-thread-exit-cap-source-diag "${run}" "${ROOT}/libhz11_span_transfer_thread_exit_cap_source_diag.so" source
  run_sampled hz11-span-return-source-diag "${run}" "${ROOT}/libhz11_span_return_source_diag.so" source
done

python3 - "${samples}" "${classes}" "${summary}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
samples=list(csv.DictReader(open(sys.argv[1], newline="")))
classes=list(csv.DictReader(open(sys.argv[2], newline="")))
sg=defaultdict(list)
cg=defaultdict(list)
for r in samples: sg[r["allocator"]].append(r)
for r in classes: cg[(r["allocator"], r["class"])].append(r)
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
base_wall=med(sg.get("tcmalloc",[]),"wall_sec")
base_rss=med(sg.get("tcmalloc",[]),"max_rss_kb")
with open(sys.argv[3],"w") as f:
    f.write("# HZ11 Sh6bench Path Cost Attribution L1\n\n")
    f.write("## Samples\n\n")
    f.write("| Allocator | status | median wall sec | wall/tcmalloc | median max RSS KiB | max RSS/tcmalloc | xfer_hit | xfer_miss | xfer_insert | xfer_spill | central_hit | central_miss | central_insert | span_create |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for alloc in sorted(sg):
        items=sg[alloc]
        wall=med(items,"wall_sec"); rss=med(items,"max_rss_kb")
        f.write(f"| {alloc} | {status(items)} | {fmt(wall)} | {fmt(wall/base_wall if wall and base_wall else None)} | {fmt(rss)} | {fmt(rss/base_rss if rss and base_rss else None)} | {total(items,'xfer_hit')} | {total(items,'xfer_miss')} | {total(items,'xfer_insert')} | {total(items,'xfer_spill')} | {total(items,'central_hit')} | {total(items,'central_miss')} | {total(items,'central_insert')} | {total(items,'span_create')} |\n")
    f.write("\n## Top Class Source Counters\n\n")
    f.write("| Allocator | class | span_create | span_reuse | arena_carve | live_current_span | transfer_refill_hit | transfer_refill_miss | central_refill_hit | central_refill_miss | sweep_scanned | meta_lock |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for (alloc, cls), items in sorted(cg.items(), key=lambda kv: (kv[0][0], -total(kv[1],"span_create"))):
        if total(items, "span_create") == 0 and total(items, "meta_lock") == 0:
            continue
        f.write(f"| {alloc} | {cls} | {total(items,'span_create')} | {total(items,'span_reuse')} | {total(items,'arena_carve')} | {total(items,'live_current_span')} | {total(items,'transfer_refill_hit')} | {total(items,'transfer_refill_miss')} | {total(items,'central_refill_hit')} | {total(items,'central_refill_miss')} | {total(items,'sweep_scanned')} | {total(items,'meta_lock')} |\n")
    f.write("\nRaw samples: `samples.csv`; raw class counters: `class_source.csv`\n")
PY

echo "[DONE] HZ11 sh6bench path-cost logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
