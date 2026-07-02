#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_code_shape_audit}"
MODE="${MODE:-localmag}"

DEFAULT_BIN="${ROOT}/h8_bench_release"
HZ9_BIN="${ROOT}/h8_bench_release_hz9mediumlocalmag"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" bench-release bench-release-hz9mediumlocalmag >/dev/null

local_head() {
  if [[ -e "${ROOT}/.git" ]]; then
    git -C "${ROOT}" rev-parse --short HEAD 2>/dev/null || printf 'unknown'
  else
    printf 'standalone-copy-no-local-git'
  fi
}

dump_bin() {
  local name="$1"
  local bin="$2"
  nm -an "${bin}" >"${OUTDIR}/${name}.nm.txt"
  nm -S --size-sort "${bin}" >"${OUTDIR}/${name}.nm_size.txt"
  objdump -d -Mintel "${bin}" >"${OUTDIR}/${name}.objdump.txt"
}

dump_function() {
  local name="$1"
  local symbol="$2"
  awk -v sym="<${symbol}>:" '
    $0 ~ sym {printing=1}
    printing {print}
    printing && /^$/ {printing=0}
  ' "${OUTDIR}/${name}.objdump.txt" >"${OUTDIR}/${name}.${symbol}.asm.txt"
}

if [[ "${MODE}" == "slab" ]]; then
  make -C "${ROOT}" \
    bench-release \
    bench-release-hz9mediumslabpage-adaptive \
    bench-release-hz9mediumslabpage-classes \
    bench-release-hz9mediumslabpage-classes-min0 \
    bench-release-hz9mediumslabpage-classes-min0-midcap \
    bench-release-hz9mediumslabpage-classes-min0-layout32 \
    bench-release-hz9mediumslabpage-classes-min0-entry \
    bench-release-hz9mediumslabpage-classes-min0-sidecar \
    bench-release-hz9mediumslabpage-classes-min0-sidecar2 \
    bench-release-hz9mediumslabpage-classes-min0-localfast-adaptive-poll32 \
    bench-release-hz9mediumslabpage-classes-min4-localfast-adaptive-poll32 \
    bench-release-hz9mediumslabpage-classes-min4-localfast-adaptive-poll32-bypass \
    bench-release-hz9mediumslabpage-classes-min4-integrated-localfast-adaptive \
    bench-release-hz9mediumslabpage-classes-min4-integrated-no-free-route-proof \
    bench-release-hz9mediumslabpage-classes-min4-integrated-no-route-proof \
    bench-release-hz9mediumslabpage-classes-min4-integrated-layout-neutral-proof \
    bench-release-hz9mediumslabpage-adaptive-min5 \
    bench-release-hz9mediumslabpage-adaptive-layout32 \
    bench-release-hz9mediumslabpage-adaptive-entry \
    bench-release-hz9mediumslabpage-adaptive-entry-hotmask >/dev/null

  declare -A bins=(
    [baseline]="${ROOT}/h8_bench_release"
    [slabclasses]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes"
    [slabclasses_min0]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0"
    [slabclasses_min0_midcap]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_midcap"
    [slabclasses_min0_layout32]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_layout32"
    [slabclasses_min0_entry]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_entry"
    [slabclasses_min0_sidecar]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_sidecar"
    [slabclasses_min0_sidecar2]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_sidecar2"
    [slablocalfast_poll32]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_localfast_adaptive_poll32"
    [slablocalfast_poll32_min4]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_localfast_adaptive_poll32"
    [slablocalfast_poll32_min4_bypass]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_localfast_adaptive_poll32_bypass"
    [slabintegrated_min4]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_localfast_adaptive"
    [slabintegrated_min4_nofreeroute]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_no_free_route_proof"
    [slabintegrated_min4_noroute]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_no_route_proof"
    [slabintegrated_min4_layoutneutral]="${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_layout_neutral_proof"
    [slabadaptive]="${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive"
    [slabadaptive_min5]="${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_min5"
    [slabadaptive_layout32]="${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_layout32"
    [slabadaptive_entry]="${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_entry"
    [slabadaptive_entry_hotmask]="${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_entry_hotmask"
  )
  variants=(baseline slabclasses slabclasses_min0 slabclasses_min0_midcap
            slabclasses_min0_layout32 slabclasses_min0_entry
            slabclasses_min0_sidecar slabclasses_min0_sidecar2
            slablocalfast_poll32 slablocalfast_poll32_min4
            slablocalfast_poll32_min4_bypass slabintegrated_min4
            slabintegrated_min4_nofreeroute slabintegrated_min4_noroute
            slabintegrated_min4_layoutneutral slabadaptive slabadaptive_min5
            slabadaptive_layout32 slabadaptive_entry slabadaptive_entry_hotmask)
  symbols=(h8_malloc h8_free h8_public_free_dispatch h8_malloc_inner
           h8_malloc_non_small_inner h8_free_inner h8_free_non_arena_inner
           h9_slab_alloc h9_slab_free_inner)

  for variant in "${variants[@]}"; do
    dump_bin "${variant}" "${bins[$variant]}"
    for symbol in "${symbols[@]}"; do
      dump_function "${variant}" "${symbol}" || true
    done
    size "${bins[$variant]}" >"${OUTDIR}/${variant}.size.txt" || true
  done

  python3 - "${OUTDIR}" "${variants[@]}" <<'PY'
import pathlib
import re
import sys

out = pathlib.Path(sys.argv[1])
variants = sys.argv[2:]
symbols = ["h8_malloc", "h8_free", "h8_public_free_dispatch",
           "h8_malloc_inner", "h8_malloc_non_small_inner",
           "h8_free_inner", "h8_free_non_arena_inner",
           "h9_slab_alloc", "h9_slab_free_inner"]

def nm_sizes(variant):
    sizes = {}
    for line in (out / f"{variant}.nm_size.txt").read_text().splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[-1] in symbols:
            sizes[parts[-1]] = int(parts[1], 16)
    return sizes

def insn_count(variant, symbol):
    path = out / f"{variant}.{symbol}.asm.txt"
    if not path.exists():
        return 0
    return sum(1 for line in path.read_text().splitlines()
               if re.match(r"^\s*[0-9a-f]+:", line))

def has_call(variant, symbol, callee):
    path = out / f"{variant}.{symbol}.asm.txt"
    return path.exists() and callee in path.read_text()

lines = [
    "# HZ9 Slab Code Shape Audit",
    "",
    "```text",
    f"outdir: {out}",
    "mode: slab",
    "```",
    "",
    "| variant | text bytes | non-small bytes | non-small insn | non-arena-free bytes | non-arena-free insn | h9_slab_alloc bytes | non-small calls slab | non-arena-free calls slab |",
    "|---|---:|---:|---:|---:|---:|---:|---:|---:|",
]
for variant in variants:
    rows = (out / f"{variant}.size.txt").read_text().splitlines()
    text = rows[1].split()[0] if len(rows) >= 2 else "0"
    sizes = nm_sizes(variant)
    lines.append(
        f"| {variant} | {text} | "
        f"{sizes.get('h8_malloc_non_small_inner', 0)} | "
        f"{insn_count(variant, 'h8_malloc_non_small_inner')} | "
        f"{sizes.get('h8_free_non_arena_inner', 0)} | "
        f"{insn_count(variant, 'h8_free_non_arena_inner')} | "
        f"{sizes.get('h9_slab_alloc', 0)} | "
        f"{int(has_call(variant, 'h8_malloc_non_small_inner', '<h9_slab_alloc>'))} | "
        f"{int(has_call(variant, 'h8_free_non_arena_inner', '<h9_slab_free_inner>'))} |"
    )
lines += ["", "Interpretation:", "", "```text",
          "baseline should have zero h9_slab_alloc bytes and no slab calls",
          "slabadaptive/min5/layout32 are evidence targets, not promotion proof",
          "```"]
lines += [
    "",
    "## Public Entry Shape",
    "",
    "| variant | h8_malloc bytes | h8_malloc insn | h8_free bytes | h8_free insn | public_free_dispatch bytes | public_free_dispatch insn |",
    "|---|---:|---:|---:|---:|---:|---:|",
]
for variant in variants:
    sizes = nm_sizes(variant)
    lines.append(
        f"| {variant} | "
        f"{sizes.get('h8_malloc', 0)} | "
        f"{insn_count(variant, 'h8_malloc')} | "
        f"{sizes.get('h8_free', 0)} | "
        f"{insn_count(variant, 'h8_free')} | "
        f"{sizes.get('h8_public_free_dispatch', 0)} | "
        f"{insn_count(variant, 'h8_public_free_dispatch')} |"
    )
(out / "summary.md").write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY
  exit 0
fi

if [[ "${MODE}" == "ownerpool" ]]; then
  make -C "${ROOT}" \
    bench-release \
    bench-release-hz9ownerpagepool-scaffold \
    bench-release-hz9ownerpagepool-purelocal-api >/dev/null

  declare -A bins=(
    [baseline]="${ROOT}/h8_bench_release"
    [ownerpagepool_scaffold]="${ROOT}/h8_bench_release_hz9ownerpagepool_scaffold"
    [ownerpagepool_purelocal]="${ROOT}/h8_bench_release_hz9ownerpagepool_purelocal_api"
  )
  variants=(baseline ownerpagepool_scaffold ownerpagepool_purelocal)
  symbols=(h8_malloc_inner h8_malloc_non_small_inner h8_free_inner
           h8_free_non_arena_inner h9_owner_page_pool_scaffold_enabled
           h9_owner_page_pool_scaffold_layout_bytes h9_owner_page_try_alloc
           h9_owner_page_try_free)

  for variant in "${variants[@]}"; do
    dump_bin "${variant}" "${bins[$variant]}"
    for symbol in "${symbols[@]}"; do
      dump_function "${variant}" "${symbol}" || true
    done
    size "${bins[$variant]}" >"${OUTDIR}/${variant}.size.txt" || true
  done

  python3 - "${OUTDIR}" "${variants[@]}" <<'PY'
import pathlib
import re
import sys

out = pathlib.Path(sys.argv[1])
variants = sys.argv[2:]
symbols = ["h8_malloc_inner", "h8_malloc_non_small_inner", "h8_free_inner",
           "h8_free_non_arena_inner",
           "h9_owner_page_pool_scaffold_enabled",
           "h9_owner_page_pool_scaffold_layout_bytes"]

def nm_sizes(variant):
    sizes = {}
    for line in (out / f"{variant}.nm_size.txt").read_text().splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[-1] in symbols:
            sizes[parts[-1]] = int(parts[1], 16)
    return sizes

def insn_count(variant, symbol):
    path = out / f"{variant}.{symbol}.asm.txt"
    if not path.exists():
        return 0
    return sum(1 for line in path.read_text().splitlines()
               if re.match(r"^\s*[0-9a-f]+:", line))

lines = [
    "# HZ9 Owner Page Pool Code Shape Audit",
    "",
    "```text",
    f"outdir: {out}",
    "mode: ownerpool",
    "```",
    "",
    "| variant | text bytes | malloc-inner bytes | malloc-inner insn | "
    "non-small bytes | non-small insn | free-inner bytes | "
    "free-inner insn | non-arena-free bytes | non-arena-free insn | "
    "ownerpool scaffold bytes | try_alloc bytes | try_free bytes |",
    "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
]
for variant in variants:
    rows = (out / f"{variant}.size.txt").read_text().splitlines()
    text = rows[1].split()[0] if len(rows) >= 2 else "0"
    sizes = nm_sizes(variant)
    scaffold = sizes.get("h9_owner_page_pool_scaffold_enabled", 0)
    scaffold += sizes.get("h9_owner_page_pool_scaffold_layout_bytes", 0)
    lines.append(
        f"| {variant} | {text} | "
        f"{sizes.get('h8_malloc_inner', 0)} | "
        f"{insn_count(variant, 'h8_malloc_inner')} | "
        f"{sizes.get('h8_malloc_non_small_inner', 0)} | "
        f"{insn_count(variant, 'h8_malloc_non_small_inner')} | "
        f"{sizes.get('h8_free_inner', 0)} | "
        f"{insn_count(variant, 'h8_free_inner')} | "
        f"{sizes.get('h8_free_non_arena_inner', 0)} | "
        f"{insn_count(variant, 'h8_free_non_arena_inner')} | "
        f"{scaffold} | "
        f"{sizes.get('h9_owner_page_try_alloc', 0)} | "
        f"{sizes.get('h9_owner_page_try_free', 0)} |"
    )
(out / "summary.md").write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY
  exit 0
fi

dump_bin default "${DEFAULT_BIN}"
dump_bin hz9 "${HZ9_BIN}"

symbols=(
  h8_medium_malloc_class_inner
  h8_medium_free_inner
  h8_medium_run_free_local_known_slot
  h8_medium_run_free_local_scaffold
  h8_hz9_local_mag_try_alloc
  h8_hz9_local_mag_try_free_known
)

for symbol in "${symbols[@]}"; do
  dump_function default "${symbol}" || true
  dump_function hz9 "${symbol}" || true
done

default_hz9_symbols=0
if grep -E '[[:space:]][Tt][[:space:]]+h8_hz9_local_mag_(try|flush)' \
    "${OUTDIR}/default.nm.txt" >/dev/null; then
  default_hz9_symbols=1
fi

hz9_symbols=0
if grep -E '[[:space:]][Tt][[:space:]]+h8_hz9_local_mag_flush_(class|owner|run_locked)' \
    "${OUTDIR}/hz9.nm.txt" >/dev/null; then
  hz9_symbols=1
fi

hz9_try_symbols=0
if grep -E '[[:space:]][Tt][[:space:]]+h8_hz9_local_mag_try_(alloc|free_known)' \
    "${OUTDIR}/hz9.nm.txt" >/dev/null; then
  hz9_try_symbols=1
fi

default_try_calls=0
if grep -E 'call.*<h8_hz9_local_mag_try_(alloc|free_known)>' \
    "${OUTDIR}/default.objdump.txt" >/dev/null; then
  default_try_calls=1
fi

hz9_try_alloc_calls=0
if grep -E 'call.*<h8_hz9_local_mag_try_alloc>' \
    "${OUTDIR}/hz9.objdump.txt" >/dev/null; then
  hz9_try_alloc_calls=1
fi

hz9_try_free_calls=0
if grep -E 'call.*<h8_hz9_local_mag_try_free_known>' \
    "${OUTDIR}/hz9.objdump.txt" >/dev/null; then
  hz9_try_free_calls=1
fi

{
  echo "# HZ9 Code Shape Audit"
  echo
  echo '```text'
  echo "root: ${ROOT}"
  echo "head: $(local_head)"
  echo "outdir: ${OUTDIR}"
  echo "default_bin: ${DEFAULT_BIN}"
  echo "hz9_bin: ${HZ9_BIN}"
  echo '```'
  echo
  echo "## Checks"
  echo
  echo '```text'
  echo "default_hz9_behavior_symbols=${default_hz9_symbols}"
  echo "default_hz9_try_calls=${default_try_calls}"
  echo "hz9_flush_symbols=${hz9_symbols}"
  echo "hz9_try_symbols=${hz9_try_symbols}"
  echo "hz9_try_alloc_calls=${hz9_try_alloc_calls}"
  echo "hz9_try_free_calls=${hz9_try_free_calls}"
  echo '```'
  echo
  echo "## Medium Symbol Sizes"
  echo
  echo '```text'
  grep -E ' h8_medium_(malloc_class_inner|free_inner|run_free_local_known_slot|run_free_local_scaffold)$| h8_hz9_local_mag_(try_alloc|try_free_known)$' \
    "${OUTDIR}/default.nm_size.txt" || true
  echo "---"
  grep -E ' h8_medium_(malloc_class_inner|free_inner|run_free_local_known_slot|run_free_local_scaffold)$| h8_hz9_local_mag_(try_alloc|try_free_known)$' \
    "${OUTDIR}/hz9.nm_size.txt" || true
  echo '```'
  echo
  if [[ "${default_hz9_symbols}" -eq 0 && "${default_try_calls}" -eq 0 &&
        "${hz9_symbols}" -eq 1 ]]; then
    echo "verdict: PASS for default HZ8 symbol isolation."
  else
    echo "verdict: FAIL. Do not run performance gates."
  fi
  if [[ "${hz9_try_alloc_calls}" -eq 1 || "${hz9_try_free_calls}" -eq 1 ]]; then
    echo "inline_status: HOLD. HZ9 release still has out-of-line magazine calls."
  elif [[ "${hz9_try_symbols}" -ne 0 ]]; then
    echo "inline_status: HOLD. HZ9 release still emits out-of-line try symbols."
  else
    echo "inline_status: candidate. HZ9 magazine calls were inlined or optimized away."
  fi
} >"${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"

if [[ "${default_hz9_symbols}" -ne 0 || "${default_try_calls}" -ne 0 ||
      "${hz9_symbols}" -ne 1 ]]; then
  exit 1
fi
