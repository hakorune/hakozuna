#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_small_transition_inventory_redis_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
DURATION="${DURATION:-5}"
REDIS_SERVER="${REDIS_SERVER:-$(command -v redis-server)}"
REDIS_CLI="${REDIS_CLI:-$(command -v redis-cli)}"
MEMTIER="${MEMTIER:-/mnt/workdisk/public_share/hakmem/hakozuna/bench/linux/memtier_benchmark/memtier_benchmark}"
MEMTIER_LIB="${MEMTIER_LIB:-/mnt/workdisk/public_share/hakmem/hakozuna/bench/linux/_local/libevent/lib}"
BASELINE_SO="${ROOT}/libhakozuna_hz8_preload.so"
CANDIDATE_SO="${ROOT}/libhakozuna_hz8_preload_small_transition_inventory.so"
ACTIVE_SERVER_PID=""
ACTIVE_MONITOR_PID=""

cleanup() {
  if [[ -n "${ACTIVE_SERVER_PID}" ]]; then
    kill "${ACTIVE_SERVER_PID}" >/dev/null 2>&1 || true
    wait "${ACTIVE_SERVER_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${ACTIVE_MONITOR_PID}" ]]; then
    wait "${ACTIVE_MONITOR_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

mkdir -p "${OUTDIR}"
make -C "${ROOT}" preload preload-small-transition-inventory >/dev/null
[[ -x "${REDIS_SERVER}" && -x "${REDIS_CLI}" && -x "${MEMTIER}" ]]

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
duration_seconds=${DURATION}
redis_server=${REDIS_SERVER}
memtier=${MEMTIER}
memtier_threads=4
memtier_clients=50
memtier_pipeline=8
memtier_ratio=1:10
memtier_data_size=128
memtier_key_max=1000000
baseline_flags=HZ8_DEFAULT_CFLAGS
candidate_flags=HZ8_DEFAULT_CFLAGS,H8_SMALL_TRANSITION_INVENTORY_L1
environment=$(uname -a)
EOF

csv="${OUTDIR}/samples.csv"
printf 'run,order,allocator,ops_sec,post_rss_kib,peak_rss_kib,log\n' >"${csv}"

monitor_rss() {
  local pid="$1" output="$2" peak=0 rss
  while [[ -r "/proc/${pid}/status" ]]; do
    rss="$(awk '$1 == "VmRSS:" { print $2 }' "/proc/${pid}/status" 2>/dev/null || true)"
    if [[ -n "${rss}" ]] && ((rss > peak)); then
      peak="${rss}"
    fi
    sleep 0.01
  done
  printf '%s\n' "${peak}" >"${output}"
}

run_one() {
  local run="$1" order="$2" allocator="$3" preload="$4" port="$5"
  local dir="${OUTDIR}/${allocator}_r${run}" server_log bench_log peak_file pid monitor_pid post ops
  dir="${OUTDIR}/${allocator}_r${run}"
  server_log="${dir}/redis.log"
  bench_log="${dir}/memtier.log"
  peak_file="${dir}/peak_rss_kib.txt"
  mkdir -p "${dir}"

  env LD_PRELOAD="${preload}" "${REDIS_SERVER}" \
    --save "" --appendonly no --bind 127.0.0.1 --port "${port}" \
    --dir "${dir}" --dbfilename dump.rdb --protected-mode no --daemonize no \
    >"${server_log}" 2>&1 &
  pid=$!
  ACTIVE_SERVER_PID="${pid}"
  monitor_rss "${pid}" "${peak_file}" &
  monitor_pid=$!
  ACTIVE_MONITOR_PID="${monitor_pid}"

  for _ in $(seq 1 100); do
    if "${REDIS_CLI}" -h 127.0.0.1 -p "${port}" ping >/dev/null 2>&1; then
      break
    fi
    sleep 0.05
  done
  "${REDIS_CLI}" -h 127.0.0.1 -p "${port}" ping >/dev/null

  LD_LIBRARY_PATH="${MEMTIER_LIB}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    "${MEMTIER}" --protocol=redis --server=127.0.0.1 --port="${port}" \
    --threads=4 --clients=50 --test-time="${DURATION}" --pipeline=8 \
    --ratio=1:10 --data-size=128 --key-maximum=1000000 --hide-histogram \
    >"${bench_log}" 2>&1

  ops="$(awk '$1 == "Totals" { print $2 }' "${bench_log}")"
  post="$(awk '$1 == "VmRSS:" { print $2 }' "/proc/${pid}/status")"
  [[ -n "${ops}" && -n "${post}" ]]
  kill "${pid}" >/dev/null 2>&1 || true
  wait "${pid}" >/dev/null 2>&1 || true
  wait "${monitor_pid}"
  ACTIVE_SERVER_PID=""
  ACTIVE_MONITOR_PID=""
  printf '%s,%s,%s,%s,%s,%s,%s\n' \
    "${run}" "${order}" "${allocator}" "${ops}" "${post}" "$(<"${peak_file}")" "${bench_log}" >>"${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  port=$((23000 + run))
  if ((run % 2 == 1)); then
    run_one "${run}" AB baseline "${BASELINE_SO}" "${port}"
    run_one "${run}" AB candidate "${CANDIDATE_SO}" "${port}"
  else
    run_one "${run}" BA candidate "${CANDIDATE_SO}" "${port}"
    run_one "${run}" BA baseline "${BASELINE_SO}" "${port}"
  fi
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1:]
groups = defaultdict(list)
for item in csv.DictReader(open(src, newline="")):
    groups[item["allocator"]].append(item)

def median(name, field, cast=float):
    return statistics.median(cast(x[field]) for x in groups[name])

bops = median("baseline", "ops_sec")
cops = median("candidate", "ops_sec")
bpost = median("baseline", "post_rss_kib", int)
cpost = median("candidate", "post_rss_kib", int)
bpeak = median("baseline", "peak_rss_kib", int)
cpeak = median("candidate", "peak_rss_kib", int)

with open(dst, "w") as out:
    out.write("# HZ8 Small Transition Inventory Redis Gate\n\n")
    out.write("Fresh-process AB/BA Redis + memtier SET:GET 1:10.\n\n")
    out.write("| allocator | median ops/s | post RSS | peak RSS |\n")
    out.write("| --- | ---: | ---: | ---: |\n")
    out.write(f"| default | {bops/1e6:.3f}M | {bpost/1024:.2f} MiB | {bpeak/1024:.2f} MiB |\n")
    out.write(f"| inventory | {cops/1e6:.3f}M | {cpost/1024:.2f} MiB | {cpeak/1024:.2f} MiB |\n")
    out.write(f"\n- throughput delta: `{(cops/bops-1)*100:+.2f}%`\n")
    out.write(f"- post RSS delta: `{(cpost/bpost-1)*100:+.2f}%`\n")
    out.write(f"- peak RSS delta: `{(cpeak/bpeak-1)*100:+.2f}%`\n")

print(open(dst).read(), end="")
PY

echo "results=${OUTDIR}"
