#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_layout_audit}"
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE}"

mkdir -p "${OUTDIR}"
probe="${OUTDIR}/layout_probe.c"
cat >"${probe}" <<'C'
#include "h8_internal.h"
#include <stdio.h>

#define SIZE(type) printf("sizeof." #type "=%zu\n", sizeof(type))
#define ALIGN(type) printf("alignof." #type "=%zu\n", _Alignof(type))
#define OFF(type, field) printf("offsetof." #type "." #field "=%zu\n", offsetof(type, field))

int main(void) {
  SIZE(H8OwnerRecord);
  ALIGN(H8OwnerRecord);
  OFF(H8OwnerRecord, control);
  OFF(H8OwnerRecord, medium_publish_ctl);
  OFF(H8OwnerRecord, owned_by_class);
  OFF(H8OwnerRecord, medium_by_class);
  OFF(H8OwnerRecord, pending_head);
  OFF(H8OwnerRecord, pending_span_count);
  OFF(H8OwnerRecord, medium_pending_head);
  OFF(H8OwnerRecord, medium_pending_count);
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
  SIZE(H9SlabOwnerState);
  OFF(H9SlabOwnerState, pending_head);
  OFF(H9SlabOwnerState, pending_count);
#if !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
#if defined(H9_SLAB_OWNER_SIDECAR_L1)
  OFF(H8OwnerRecord, h9_slab_owner_state);
#else
  OFF(H8OwnerRecord, h9_slab_state);
#endif
#endif
#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
  OFF(H9SlabOwnerState, remote_hot_mask);
  OFF(H9SlabOwnerState, remote_hot);
#endif
#endif
  OFF(H8OwnerRecord, owned_lock);
  OFF(H8OwnerRecord, pending_lock);
  OFF(H8OwnerRecord, free_next);

  SIZE(H8ThreadCtx);
  ALIGN(H8ThreadCtx);
  OFF(H8ThreadCtx, owner);
  OFF(H8ThreadCtx, active_spans);
  OFF(H8ThreadCtx, active_medium_runs);
  OFF(H8ThreadCtx, medium_collect_credit);
#if defined(H9_MEDIUM_LOCAL_SLAB_SHADOW)
  OFF(H8ThreadCtx, h9_slab_shadow_free_count);
  OFF(H8ThreadCtx, h9_slab_shadow_cached_bytes);
#endif
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
  SIZE(H9SlabThreadState);
#if !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
#if defined(H9_SLAB_THREAD_SIDECAR_L1)
  OFF(H8ThreadCtx, h9_slab_state);
#else
  OFF(H8ThreadCtx, h9_slab_active);
  OFF(H8ThreadCtx, h9_slab_pages);
#endif
#endif
#endif
  return 0;
}
C

variant_flags() {
  case "$1" in
    baseline)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1'
      ;;
    slab4)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1'
      ;;
    slabclasses_min0)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=0u'
      ;;
    slabclasses_min0_sidecar)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=0u -DH9_SLAB_THREAD_SIDECAR_L1'
      ;;
    slabclasses_min0_sidecar2)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=0u -DH9_SLAB_THREAD_SIDECAR_L1 -DH9_SLAB_OWNER_SIDECAR_L1'
      ;;
    slabadaptive)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_REMOTE_ADAPTIVE_L1'
      ;;
    slabintegrated_min4)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=4u -DH9_SLAB_THREAD_SIDECAR_L1 -DH9_SLAB_OWNER_SIDECAR_L1 -DH9_SLAB_LOCAL_FREE_BITS_L1 -DH9_SLAB_LOCAL_STORE_MUTATION_L1 -DH9_SLAB_ALLOC_FREE_FIRST_L1 -DH9_SLAB_NO_PAGE_FAST_REJECT_L1 -DH9_SLAB_REMOTE_ADAPTIVE_L1 -DH9_SLAB_REMOTE_HOT_MASK_L1'
      ;;
    slabintegrated_min4_nofreeroute)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=4u -DH9_SLAB_THREAD_SIDECAR_L1 -DH9_SLAB_OWNER_SIDECAR_L1 -DH9_SLAB_LOCAL_FREE_BITS_L1 -DH9_SLAB_LOCAL_STORE_MUTATION_L1 -DH9_SLAB_ALLOC_FREE_FIRST_L1 -DH9_SLAB_NO_PAGE_FAST_REJECT_L1 -DH9_SLAB_REMOTE_ADAPTIVE_L1 -DH9_SLAB_REMOTE_HOT_MASK_L1 -DH9_SLAB_FREE_ROUTE_OFF_PROOF_L1'
      ;;
    slabintegrated_min4_noroute)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=4u -DH9_SLAB_THREAD_SIDECAR_L1 -DH9_SLAB_OWNER_SIDECAR_L1 -DH9_SLAB_LOCAL_FREE_BITS_L1 -DH9_SLAB_LOCAL_STORE_MUTATION_L1 -DH9_SLAB_ALLOC_FREE_FIRST_L1 -DH9_SLAB_NO_PAGE_FAST_REJECT_L1 -DH9_SLAB_REMOTE_ADAPTIVE_L1 -DH9_SLAB_REMOTE_HOT_MASK_L1 -DH9_SLAB_FREE_ROUTE_OFF_PROOF_L1 -DH9_SLAB_ALLOC_ROUTE_OFF_PROOF_L1'
      ;;
    slabintegrated_min4_layoutneutral)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY -DH9_MEDIUM_LOCAL_SLAB_PAGE_L1 -DH9_SLAB_CLASS_COVERAGE_L1 -DH9_SLAB_CLASS_MIN_ID=4u -DH9_SLAB_THREAD_SIDECAR_L1 -DH9_SLAB_OWNER_SIDECAR_L1 -DH9_SLAB_LOCAL_FREE_BITS_L1 -DH9_SLAB_LOCAL_STORE_MUTATION_L1 -DH9_SLAB_ALLOC_FREE_FIRST_L1 -DH9_SLAB_NO_PAGE_FAST_REJECT_L1 -DH9_SLAB_REMOTE_ADAPTIVE_L1 -DH9_SLAB_REMOTE_HOT_MASK_L1 -DH9_SLAB_FREE_ROUTE_OFF_PROOF_L1 -DH9_SLAB_ALLOC_ROUTE_OFF_PROOF_L1 -DH9_SLAB_LAYOUT_NEUTRAL_PROOF_L1'
      ;;
    ownerpagepool_scaffold)
      printf '%s\n' '-DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_OWNER_LOCAL_PAGE_POOL_L0'
      ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
  esac
}

VARIANTS="${VARIANTS:-baseline,slab4,slabclasses_min0,slabclasses_min0_sidecar,slabclasses_min0_sidecar2,slabadaptive}"
IFS=',' read -r -a variants <<< "${VARIANTS}"

for variant in "${variants[@]}"; do
  bin="${OUTDIR}/${variant}_layout_probe"
  flags="$(variant_flags "${variant}")"
  # shellcheck disable=SC2086
  "${CC}" ${CFLAGS} ${flags} -I"${ROOT}/include" -I"${ROOT}/src" \
    -o "${bin}" "${probe}"
  "${bin}" >"${OUTDIR}/${variant}.txt"
done

python3 - "${OUTDIR}" "${variants[@]}" <<'PY'
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
variants = sys.argv[2:]

def read_values(variant):
    values = {}
    for line in (out / f"{variant}.txt").read_text().splitlines():
        key, value = line.split("=", 1)
        values[key] = int(value)
    return values

all_values = {variant: read_values(variant) for variant in variants}
keys = []
for values in all_values.values():
    for key in values:
        if key not in keys:
            keys.append(key)

lines = [
    "# HZ9 Layout Audit",
    "",
    "Size/offset evidence for HZ9 state contamination.",
    "",
    "| item | " + " | ".join(variants) + " |",
    "|---|" + "|".join(["---:"] * len(variants)) + "|",
]
for key in keys:
    row = [str(all_values[v].get(key, "")) for v in variants]
    lines.append("| " + key + " | " + " | ".join(row) + " |")

(out / "summary.md").write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-layout-audit] logs=${OUTDIR}"
