#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-focused}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_midpage_rss_audit_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILD=0
PRELOAD_SO_OVERRIDE="${PRELOAD_SO:-}"
LABEL="${LABEL:-selected_diag}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh [options]

Options:
  --arch ARCH     target arch (default: x86_64)
  --iters N       iterations per row (default: 200000)
  --ws N          working set (default: 4096)
  --rows CSV      row groups: focused,fixed_mid,large_span (default: focused)
  --outdir DIR    output directory
  --preload-so SO use an existing preload DSO instead of the selected diag DSO
  --label LABEL   label stored in README.log (default: selected_diag)
  --skip-build    reuse existing diagnostic preload and benchmark
  --help          show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --ws)
      WS="$2"
      shift 2
      ;;
    --rows)
      ROWS_CSV="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --preload-so)
      PRELOAD_SO_OVERRIDE="$2"
      shift 2
      ;;
    --label)
      LABEL="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
PRELOAD_SO="${PRELOAD_SO_OVERRIDE:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_diag/libhakozuna_hz6_preload.so}"

if [[ "$SKIP_BUILD" -ne 1 && -z "$PRELOAD_SO_OVERRIDE" ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_diag.sh"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
elif [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }
[[ -f "$PRELOAD_SO" ]] || { echo "missing preload DSO: $PRELOAD_SO" >&2; exit 2; }

mkdir -p "$OUTDIR"
{
  echo "bench=${BENCH}"
  echo "preload=${PRELOAD_SO}"
  echo "label=${LABEL}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "rows=${ROWS_CSV}"
} > "${OUTDIR}/README.log"

run_row() {
  local row="$1"
  local min_size="$2"
  local max_size="$3"
  env HZ6_PRELOAD_STATS=per_allocator LD_PRELOAD="$PRELOAD_SO" \
    "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
    > "${OUTDIR}/${row}.log" 2>&1
}

rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    focused)
      rows+=(
        "16_4096 16 4096"
        "1024_4096 1024 4096"
        "4096_16384 4096 16384"
      )
      ;;
    fixed_mid)
      rows+=(
        "fixed_4k 4096 4096"
        "fixed_8k 8192 8192"
        "fixed_16k 16384 16384"
      )
      ;;
    large_span)
      rows+=(
        "fixed_32k 32768 32768"
        "fixed_64k 65536 65536"
        "fixed_128k 131072 131072"
        "fixed_256k 262144 262144"
      )
      ;;
    *)
      echo "unknown row group: ${row_group}" >&2
      exit 2
      ;;
  esac
done

for row_spec in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_spec"
  run_row "$row" "$min_size" "$max_size"
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]
kv_re = re.compile(r"([A-Za-z0-9_/]+)=([0-9]+(?:\\.[0-9]+)?)")

def parse_line(text, tag):
    for line in text.splitlines():
        if line.startswith(tag):
            return {k: float(v) for k, v in kv_re.findall(line)}
    return {}

print("# HZ6 MidPage RSS Audit\n")
print(f"root: `{root}`\n")
print("| row | ops/s | peak MiB | attributed MiB | static MiB | descriptor MiB | route MiB | source-table MiB | frontcache MiB | payload MiB | mid8 payload MiB | mid32 payload MiB | mid8 blocks | mid32 blocks | mid8 active | mid32 active | mid8 local-free | mid32 local-free | mid8 all-local-free MiB | mid32 all-local-free MiB | mid32 retire MiB | mid32 retire desc | mid32 retire fc entries | mid32 low-active blocks | mid32 low-active MiB | ref mismatch | retain MiB | toy map MiB | midpage map MiB | active source blocks | frontcache total |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    text = (root / f"{row}.log").read_text(errors="replace")
    bench = parse_line(text, "threads=")
    mem = parse_line(text, "[HZ6_PRELOAD_MEMORY_ATTR]")
    ops = bench.get("ops/s", 0.0)
    peak_mib = bench.get("peak_kb", 0.0) / 1024.0
    def mib(name):
        return mem.get(name, 0.0) / (1024.0 * 1024.0)
    print(
        f"| `{row}` | {ops:.3f} | {peak_mib:.2f} | "
        f"{mib('preload_attributed_bytes'):.2f} | "
        f"{mib('static_table_bytes'):.2f} | "
        f"{mib('descriptor_table_bytes'):.2f} | "
        f"{mib('route_table_bytes'):.2f} | "
        f"{mib('source_block_table_bytes'):.2f} | "
        f"{mib('frontcache_table_bytes'):.2f} | "
        f"{mib('source_block_payload_bytes'):.2f} | "
        f"{mib('midpage_8k_payload_bytes'):.2f} | "
        f"{mib('midpage_32k_payload_bytes'):.2f} | "
        f"{int(mem.get('midpage_8k_source_blocks', 0))} | "
        f"{int(mem.get('midpage_32k_source_blocks', 0))} | "
        f"{int(mem.get('midpage_8k_active', 0))} | "
        f"{int(mem.get('midpage_32k_active', 0))} | "
        f"{int(mem.get('midpage_8k_local_free', 0))} | "
        f"{int(mem.get('midpage_32k_local_free', 0))} | "
        f"{mib('midpage_8k_all_local_free_payload_bytes'):.2f} | "
        f"{mib('midpage_32k_all_local_free_payload_bytes'):.2f} | "
        f"{mib('midpage_32k_retire_candidate_payload_bytes'):.2f} | "
        f"{int(mem.get('midpage_32k_retire_candidate_descriptors', 0))} | "
        f"{int(mem.get('midpage_32k_retire_candidate_frontcache_entries', 0))} | "
        f"{int(mem.get('midpage_32k_low_active_1_4_blocks', 0))} | "
        f"{mib('midpage_32k_low_active_1_4_payload_bytes'):.2f} | "
        f"{int(mem.get('midpage_ref_mismatch_blocks', 0))} | "
        f"{mib('retain_retained_bytes'):.2f} | "
        f"{mib('toy_active_map_table_bytes'):.2f} | "
        f"{mib('midpage_active_map_table_bytes'):.2f} | "
        f"{int(mem.get('active_source_blocks', 0))} | "
        f"{int(mem.get('frontcache_total', 0))} |"
    )

print("\nNote: attributed bytes are diagnostic attribution, not exact RSS.")
print("Source block payload bytes are logical backing capacity and can exceed resident pages.")
PY
