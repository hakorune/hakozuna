#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/hz10_bench_common.sh"
HZ8_ROOT="${HZ8_ROOT:-${HZ10_EXT_ROOT}/hakozuna-hz8}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/$(date -u +%Y%m%dT%H%M%SZ)_hz10_hz8_public_purge_matrix_l0}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
ROWS="${ROWS:-small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,main_local0,medium_local0}"
HZ10_LIB="${HZ10_LIB:-${ROOT}/libhz10.so}"
HZ8_MATRIX_SRC="${HZ8_MATRIX_SRC:-${HZ8_ROOT}/bench/bench_matrix_malloc.c}"
BASE_BIN="${OUTDIR}/bench_matrix_malloc_base"
PURGE_SRC="${OUTDIR}/bench_matrix_malloc_hz10_purge.c"
PURGE_BIN="${OUTDIR}/bench_matrix_malloc_hz10_purge"

mkdir -p "${OUTDIR}"
if [[ ! -f "${HZ8_MATRIX_SRC}" ]]; then
  echo "[ERR] missing HZ8 bench_matrix source: ${HZ8_MATRIX_SRC}" >&2
  echo "[ERR] set HZ10_EXT_ROOT to the parent directory containing the HZ8 checkout, or set HZ8_ROOT/HZ8_MATRIX_SRC explicitly" >&2
  exit 2
fi
make -C "${ROOT}" preload >/dev/null

"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE -pthread \
  -o "${BASE_BIN}" "${HZ8_MATRIX_SRC}"

python3 - "${HZ8_MATRIX_SRC}" "${PURGE_SRC}" <<'PY'
import pathlib
import sys

src_path = pathlib.Path(sys.argv[1])
dst_path = pathlib.Path(sys.argv[2])
src = src_path.read_text()
src = src.replace(
    "#include <unistd.h>\n",
    "#include <unistd.h>\n#if defined(HZ10_MATRIX_CALL_ORPHAN_PURGE)\n#include <dlfcn.h>\n#endif\n",
)
needle = "    post_rss[run] = (double)rss_bytes();\n"
insert = """#if defined(HZ10_MATRIX_CALL_ORPHAN_PURGE)
    {
      typedef void (*hz10_purge_fn)(void*);
      hz10_purge_fn purge = (hz10_purge_fn)dlsym(
          RTLD_DEFAULT, \"hz10_public_entry_purge_orphan_registry_quiescent\");
      if (purge) {
        purge(NULL);
      }
    }
#endif
"""
if needle not in src:
    raise SystemExit("bench_matrix post_rss sample point not found")
dst_path.write_text(src.replace(needle, insert + needle))
PY

"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -DHZ10_MATRIX_CALL_ORPHAN_PURGE -pthread -o "${PURGE_BIN}" "${PURGE_SRC}" -ldl

cat >"${OUTDIR}/README.log" <<EOF
box=HZ10ExplicitQuiescentOrphanPurge-L0 formal matrix lane
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
hz10_sha=$(git -C "${ROOT}" rev-parse HEAD)
hz8_sha=$(git -C "${HZ8_ROOT}" rev-parse HEAD 2>/dev/null || true)
runs=${RUNS}
threads=${THREADS}
iters=${ITERS}
rows=${ROWS}
hz10_lib=${HZ10_LIB}
base_bin=${BASE_BIN}
purge_bin=${PURGE_BIN}
note=purge lane calls hz10_public_entry_purge_orphan_registry_quiescent(NULL) immediately before post_rss sampling via dlsym(RTLD_DEFAULT).
EOF

row_args() {
  local row="$1"
  case "${row}" in
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    main_interleaved_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    medium_interleaved_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    *)
      echo "[ERR] unknown row ${row}" >&2
      exit 3
      ;;
  esac
}

csv="${OUTDIR}/samples.csv"
printf 'lane,row,run,throughput,post_rss,peak_rss,minor_faults,work_ms,tail_ms,log\n' >"${csv}"
IFS=',' read -r -a row_list <<<"${ROWS}"

run_one() {
  local lane="$1" row="$2" run="$3" bin args log
  bin="${BASE_BIN}"
  [[ "${lane}" == "purge" ]] && bin="${PURGE_BIN}"
  args="$(row_args "${row}")"
  log="${OUTDIR}/${lane}_${row}_${run}.log"
  # shellcheck disable=SC2086
  timeout "${RUN_TIMEOUT}" env LD_PRELOAD="${HZ10_LIB}" "${bin}" \
    --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} >"${log}" 2>&1
  awk -v lane="${lane}" -v row="${row}" -v run="${run}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); throughput = a[2] }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); post = a[2] }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] }
    }
    /^phase_ms / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^work_median=/) { split($i, a, "="); work = a[2] }
        if ($i ~ /^tail_median=/) { split($i, a, "="); tail = a[2] }
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        lane, row, run, throughput, post, peak, faults, work, tail, logfile
    }
  ' "${log}" >>"${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    run_one baseline "${row}" "${run}"
    run_one purge "${row}" "${run}"
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" "${OUTDIR}/notes.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

samples, summary_path, notes_path = sys.argv[1], sys.argv[2], sys.argv[3]
rows = list(csv.DictReader(open(samples, newline="")))
groups = defaultdict(list)
order = []
for row in rows:
    key = (row["row"], row["lane"])
    groups[key].append(row)
    if row["row"] not in order:
        order.append(row["row"])

def med(items, key):
    return statistics.median(float(x[key]) for x in items if x.get(key))

lines = [
    "# HZ10 HZ8 Public Purge Matrix L0",
    "",
    "Median results. `purge` calls `hz10_public_entry_purge_orphan_registry_quiescent(NULL)` before post_rss sampling.",
    "",
    "| row | baseline post RSS | purge post RSS | RSS delta | baseline ops/s | purge ops/s | ops delta |",
    "|---|---:|---:|---:|---:|---:|---:|",
]
for row in order:
    b = groups[(row, "baseline")]
    p = groups[(row, "purge")]
    bp, pp = med(b, "post_rss"), med(p, "post_rss")
    bt, pt = med(b, "throughput"), med(p, "throughput")
    ops_delta = ((pt / bt) - 1.0) * 100.0 if bt else 0.0
    lines.append(
        f"| {row} | {bp:.0f} | {pp:.0f} | {pp - bp:.0f} | "
        f"{bt:.3f} | {pt:.3f} | {ops_delta:.1f}% |"
    )
lines.append("")
lines.append("Verdict: formal lane for the explicit quiescent purge RSS gate.")
open(summary_path, "w").write("\n".join(lines) + "\n")
notes = lines + [
    "",
    "## Method",
    "",
    "The baseline and purge binaries are both built from the HZ8 public `bench_matrix_malloc.c` source.",
    "The purge binary is generated under OUTDIR with one extra dlsym call at the post_rss sample point.",
    "HZ8 source files are not modified by this lane.",
]
open(notes_path, "w").write("\n".join(notes) + "\n")
print("\n".join(lines))
PY

echo "[hz10-hz8-public-purge-matrix] ok outdir=${OUTDIR}"
