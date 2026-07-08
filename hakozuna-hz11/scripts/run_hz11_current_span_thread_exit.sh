#!/usr/bin/env bash
# HZ11CurrentSpanPoolThreadExit-L1: larson thread-churn RSS gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_current_span_thread_exit_${STAMP}}"
RUNS="${RUNS:-5}"
BUILD="${BUILD:-1}"
LARSON_SECONDS="${LARSON_SECONDS:-2}"
LARSON_THREADS="${LARSON_THREADS:-4}"

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

if [[ -z "${larson_bin}" ]]; then
  echo "larson binary missing; set LARSON_BIN" >&2
  exit 1
fi

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-transfer preload-span-transfer-thread-exit >/dev/null
fi

samples="${OUTDIR}/samples.csv"
summary="${OUTDIR}/summary.md"
printf 'allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,log,stderr_log,stdout_log\n' >"${samples}"

csv_escape() {
  local s="${1//$'\n'/ }"; s="${s//\"/\"\"}"; printf '"%s"' "${s}"
}

read_kb() {
  awk -v key="$2:" '$1 == key { print $2; exit }' "/proc/$1/status" 2>/dev/null || true
}

write_sample() {
  {
    csv_escape "$1"; printf ',%s,' "$2"; csv_escape "$3"; printf ','
    printf '%s,%s,%s,%s,' "$4" "$5" "$6" "$7"
    csv_escape "$8"; printf ','; csv_escape "$9"; printf ','; csv_escape "${10}"; printf '\n'
  } >>"${samples}"
}

run_sampled() {
  local allocator="$1" run="$2" lib="$3"
  local log="${OUTDIR}/larson_${allocator}_${run}.log"
  local err="${OUTDIR}/larson_${allocator}_${run}.err"
  local out="${OUTDIR}/larson_${allocator}_${run}.out"
  local status=OK rc=0 max_rss=NA cur_rss=NA sample hwm pid start now wall
  local -a cmd=(env)
  [[ -n "${lib}" ]] && cmd+=(LD_PRELOAD="${lib}")
  [[ "${allocator}" == hz11-thread-exit ]] && cmd+=(HZ11_DUMP_CURRENT_SPAN_POOL=1)
  cmd+=("${larson_bin}" "${LARSON_SECONDS}" 8 128 128 1 12345 "${LARSON_THREADS}")

  : >"${log}"; : >"${err}"; : >"${out}"
  printf '[cmd]' >"${log}"; printf ' %q' "${cmd[@]}" >>"${log}"; printf '\n' >>"${log}"
  start="$(date +%s%N)"
  "${cmd[@]}" >"${out}" 2>"${err}" &
  pid=$!
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_kb "${pid}" VmRSS || true)"
    hwm="$(read_kb "${pid}" VmHWM || true)"
    [[ -n "${sample}" ]] && cur_rss="${sample}"
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
  [[ "${max_rss}" == NA && "${cur_rss}" != NA ]] && max_rss="${cur_rss}"
  cat "${out}" "${err}" >>"${log}" || true
  write_sample "${allocator}" "${run}" "${status}" "${rc}" "${wall}" \
    "${max_rss}" "${cur_rss}" "${log}" "${err}" "${out}"
}

for run in $(seq 1 "${RUNS}"); do
  if [[ -n "${tcmalloc_lib}" ]]; then
    run_sampled tcmalloc "${run}" "${tcmalloc_lib}"
  fi
  run_sampled hz11-span-transfer "${run}" "${ROOT}/libhz11_span_transfer.so"
  run_sampled hz11-thread-exit "${run}" "${ROOT}/libhz11_span_transfer_thread_exit.so"
done

python3 - "${samples}" "${summary}" <<'PY'
import csv, statistics, sys
from collections import defaultdict
rows=list(csv.DictReader(open(sys.argv[1], newline="")))
groups=defaultdict(list)
for r in rows:
    groups[r["allocator"]].append(r)
def num(v):
    return None if v in ("","NA","-") else float(v)
def med(items,k):
    vals=[num(r[k]) for r in items if r["status"]=="OK" and num(r[k]) is not None]
    return statistics.median(vals) if vals else None
def status(items):
    out=defaultdict(int)
    for r in items:
        out[r["status"]]+=1
    return " ".join(f"{k}:{out[k]}" for k in sorted(out))
def fmt(v):
    return "NA" if v is None else f"{v:.3f}"
base=med(groups.get("hz11-span-transfer",[]), "max_rss_kb")
with open(sys.argv[2],"w") as f:
    f.write("# HZ11 Current Span Pool Thread Exit L1\n\n")
    f.write("## Larson Samples\n\n")
    f.write("| Allocator | status | median wall sec | median max RSS KiB | RSS vs transfer |\n")
    f.write("|---|---:|---:|---:|---:|\n")
    for alloc in sorted(groups):
        rss=med(groups[alloc], "max_rss_kb")
        ratio=(rss/base) if rss is not None and base else None
        f.write(f"| {alloc} | {status(groups[alloc])} | {fmt(med(groups[alloc], 'wall_sec'))} | {fmt(rss)} | {fmt(ratio)} |\n")
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] HZ11 current-span thread-exit logs saved to ${OUTDIR}"
echo "[DONE] Summary: ${summary}"
