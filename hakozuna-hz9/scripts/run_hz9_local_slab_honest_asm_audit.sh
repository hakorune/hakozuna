#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_local_slab_honest_asm_audit}"
BIN="${ROOT}/h8_bench_hz9localslabrouteboundary"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" bench-hz9localslabrouteboundary >/dev/null

objdump -d --no-show-raw-insn "${BIN}" >"${OUTDIR}/route_boundary.objdump.txt"
nm -an "${BIN}" >"${OUTDIR}/route_boundary.nm.txt"

python3 - "${OUTDIR}" <<'PY'
import pathlib
import re
import sys

out = pathlib.Path(sys.argv[1])
objdump = (out / "route_boundary.objdump.txt").read_text()

symbols = [
    ("integrated_worker", "h9_lsp_run_integrated_worker"),
    ("fastleaf_worker", "h9_lsp_run_fastleaf_worker"),
    ("routeleaf_segment", "h9_lsp_debug_routeleaf_bench"),
    ("routeleaf_compact", "h9_lsp_debug_routeleaf_compact_bench"),
    ("routeleaf_trim", "h9_lsp_debug_routeleaf_trim_bench"),
    ("routeleaf_tight", "h9_lsp_debug_routeleaf_tight_bench"),
    ("public_malloc", "h9_lsp_debug_public_malloc"),
    ("public_free", "h9_lsp_debug_public_free"),
    ("public_nosync_malloc", "h9_lsp_debug_public_nosync_malloc"),
    ("public_nosync_free", "h9_lsp_debug_public_nosync_free"),
]

def extract(symbol):
    pat = re.compile(rf"^[0-9a-f]+ <{re.escape(symbol)}>:\n", re.M)
    m = pat.search(objdump)
    if not m:
        return ""
    start = m.start()
    n = re.search(r"^[0-9a-f]+ <[^>]+>:\n", objdump[m.end():], re.M)
    end = m.end() + n.start() if n else len(objdump)
    return objdump[start:end]

def count_insn(asm):
    return sum(1 for line in asm.splitlines() if re.match(r"\s*[0-9a-f]+:", line))

def has(pattern, asm):
    return re.search(pattern, asm) is not None

rows = []
for label, symbol in symbols:
    asm = extract(symbol)
    (out / f"{label}.{symbol}.asm.txt").write_text(asm)
    slot_select = has(r"\b(tzcnt|bsf)\b", asm)
    cold_call = "cold_free" in asm or ".cold" in asm
    route_call = "route_direct_owned" in asm or "h9_lsp_debug_route_direct_owned" in asm
    state_stores = sum(
        1 for line in asm.splitlines()
        if any(op in line for op in ("\tmov", "\tand", "\tor"))
        and ("(" in line or "%rsp" in line)
    )
    if label in ("public_free", "public_nosync_free"):
        verdict = "route-authority-candidate" if route_call or state_stores else "phantom-ceiling"
    else:
        verdict = "honest-candidate" if slot_select else "phantom-ceiling"
    if not asm:
        verdict = "missing"
    rows.append((label, symbol, count_insn(asm), slot_select, cold_call,
                 route_call, state_stores, verdict))

lines = [
    "# HZ9 Local Slab Honest ASM Audit",
    "",
    "```text",
    f"outdir: {out}",
    "binary: h8_bench_hz9localslabrouteboundary",
    "rule: no slot-select instruction means the alloc/free body was likely DCE'd",
    "```",
    "",
    "| label | symbol | insn | slot_select | cold_edge | route_call | state_store_hint | verdict |",
    "|---|---|---:|---:|---:|---:|---:|---|",
]
for row in rows:
    label, symbol, insn, slot_select, cold_call, route_call, stores, verdict = row
    lines.append(
        f"| {label} | `{symbol}` | {insn} | {int(slot_select)} | "
        f"{int(cold_call)} | {int(route_call)} | {stores} | {verdict} |"
    )

lines += [
    "",
    "Interpretation:",
    "",
    "```text",
    "honest-candidate:",
    "  slot selection remains visible in the hot symbol; benchmark numbers may be",
    "  used as gate evidence after safety counters are clean",
    "",
    "phantom-ceiling:",
    "  cyclic alloc/free state was probably optimized away; keep as code-shape",
    "  ceiling only, not as promotion/performance gate evidence",
    "```",
]
(out / "summary.md").write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-local-slab-honest-asm-audit] logs=${OUTDIR}"
