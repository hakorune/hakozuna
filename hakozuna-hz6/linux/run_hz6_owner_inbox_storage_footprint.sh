#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_owner_inbox_storage_footprint_$(date +%Y%m%d_%H%M%S)}"
VARIANTS_CSV="${VARIANTS:-p0_off,p1_external}"
CC_BIN="${CC:-cc}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_owner_inbox_storage_footprint.sh [options]

Options:
  --variants CSV p0_off,p1_external
  --outdir DIR    output directory
  --help          show this message

This is a compile-time storage audit. It does not run allocator workloads.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --variants) VARIANTS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

mkdir -p "$OUTDIR/build"

include_flags=(
  "-I${HZ6_DIR}/include"
  "-I${HZ6_DIR}/api"
  "-I${HZ6_DIR}/route"
  "-I${HZ6_DIR}/frontcache"
  "-I${HZ6_DIR}/transfer"
  "-I${HZ6_DIR}/owner"
  "-I${HZ6_DIR}/source"
  "-I${HZ6_DIR}/fronts"
)

variants=()
IFS=',' read -r -a raw_variants <<< "$VARIANTS_CSV"
for variant in "${raw_variants[@]}"; do
  case "$variant" in
    p0_off|p1_external) variants+=("$variant") ;;
    "") ;;
    *) echo "unknown variant: $variant" >&2; exit 2 ;;
  esac
done

build_variant() {
  local variant="$1"
  local flags=()
  case "$variant" in
    p0_off) hz6_preload_effective_owner_inbox_off_cflags flags 1 ;;
    p1_external) hz6_preload_effective_owner_inbox_external_cflags flags 1 ;;
  esac
  "$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 \
    -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
    "${flags[@]}" \
    "${include_flags[@]}" \
    "${HZ6_DIR}/tests/hz6_owner_inbox_storage_footprint.c" \
    -o "${OUTDIR}/build/${variant}_footprint"
}

printf 'variant\tkey\tvalue\n' > "${OUTDIR}/footprint.tsv"
for variant in "${variants[@]}"; do
  build_variant "$variant"
  "${OUTDIR}/build/${variant}_footprint" > "${OUTDIR}/${variant}.log"
  awk -F= -v variant="$variant" \
    '{ printf "%s\t%s\t%s\n", variant, $1, $2 }' \
    "${OUTDIR}/${variant}.log" >> "${OUTDIR}/footprint.tsv"
done

python3 - "$OUTDIR/footprint.tsv" <<'PY' | tee "${OUTDIR}/summary.tsv"
import csv
import sys

rows = {}
with open(sys.argv[1], newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        rows.setdefault(row["variant"], {})[row["key"]] = int(row["value"])

keys = [
    "sizeof_Hz6Allocator",
    "owner_inbox_bytes",
    "owner_inbox_class_inbox_bytes",
    "owner_inbox_inline_slot_bytes",
    "owner_inbox_external_ticket_bytes",
    "owner_inbox_external_dup_index_bytes",
]
print("variant\t" + "\t".join(keys))
for variant in sorted(rows):
    print(variant + "\t" + "\t".join(str(rows[variant].get(key, 0)) for key in keys))
PY

echo "[DONE] ${OUTDIR}"
