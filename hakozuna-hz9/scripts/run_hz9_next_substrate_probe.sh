#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_next_substrate_probe}"
RUNS_SHADOW="${RUNS_SHADOW:-2}"
RUNS_ROUTE="${RUNS_ROUTE:-3}"
RUNS_MUTATION="${RUNS_MUTATION:-1}"
RUNS_READINESS="${RUNS_READINESS:-5}"
ROWS_SHADOW="${ROWS_SHADOW:-medium_local0,main_local0,medium_r50,main_r90}"
ROWS_ROUTE="${ROWS_ROUTE:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"
ROWS_MUTATION="${ROWS_MUTATION:-medium_local0,main_local0,medium_r50,main_r90}"
ROWS_READINESS="${ROWS_READINESS:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" hz9-standalone-check >/dev/null

OUT_DIR="${OUTDIR}/class_shadow" \
  RUNS="${RUNS_SHADOW}" \
  ROWS="${ROWS_SHADOW}" \
  "${ROOT}/scripts/run_hz9_slab_shadow_probe.sh" \
  >"${OUTDIR}/class_shadow.log"

OUTDIR="${OUTDIR}/entry_route" \
  RUNS="${RUNS_ROUTE}" \
  ROWS="${ROWS_ROUTE}" \
  VARIANTS=baseline,entry,hotmask \
  "${ROOT}/scripts/run_hz9_entry_route_probe.sh" "${STAMP}_entry_route" \
  >"${OUTDIR}/entry_route.log"

OUTDIR="${OUTDIR}/mutation" \
  RUNS="${RUNS_MUTATION}" \
  ROWS="${ROWS_MUTATION}" \
  VARIANTS=baseline,slabclasses_min0_sidecar2 \
  "${ROOT}/scripts/run_hz9_slabpage_rss_probe.sh" "${STAMP}_mutation" \
  >"${OUTDIR}/mutation.log"

OUTDIR="${OUTDIR}/readiness" \
  RUNS="${RUNS_READINESS}" \
  ROWS="${ROWS_READINESS}" \
  VARIANTS=baseline,slabadaptive_entry,slabadaptive_entry_hotmask \
  "${ROOT}/scripts/run_hz9_substrate_readiness.sh" "${STAMP}_readiness" \
  >"${OUTDIR}/readiness.log"

python3 - "${OUTDIR}/class_shadow" "${OUTDIR}/class_summary.md" <<'PY'
import pathlib
import re
import sys

src = pathlib.Path(sys.argv[1])
dst = pathlib.Path(sys.argv[2])
classes = ["8K", "16K", "24K", "32K", "48K", "64K"]
class_pattern = re.compile(
    r"h9_slab_shadow_class alloc=\[([^\]]+)\].*remote=\[([^\]]+)\]")
shadow_pattern = re.compile(r"^h9_slab_shadow (.*)$", re.MULTILINE)

rows = []
mechanism = []
for log in sorted(src.glob("*.log")):
    text = log.read_text(errors="replace")
    shadow = shadow_pattern.search(text)
    if shadow:
        values = {}
        for token in shadow.group(1).split():
            if "=" not in token:
                continue
            key, raw = token.split("=", 1)
            try:
                values[key] = float(raw)
            except ValueError:
                pass
        mechanism.append(
            (
                log.stem,
                values.get("hit_ratio", 0.0),
                int(values.get("page_switch", 0.0)),
                int(values.get("page_full", 0.0)),
                int(values.get("cap_fallback", 0.0)),
                int(values.get("retained_peak", 0.0)),
                int(values.get("remote_escape", 0.0)),
            )
        )
    match = class_pattern.search(text)
    if not match:
        continue
    alloc = [int(x) for x in match.group(1).split(",")]
    remote = [int(x) for x in match.group(2).split(",")]
    for cls, a, r in zip(classes, alloc, remote):
        local = max(a - r, 0)
        remote_ratio = (r / a) if a else 0.0
        rows.append((log.stem, cls, a, local, r, remote_ratio))

lines = [
    "| row | class | alloc | local-like | remote | remote ratio |",
    "|---|---:|---:|---:|---:|---:|",
]
for row, cls, alloc, local, remote, ratio in rows:
    lines.append(
        f"| {row} | {cls} | {alloc} | {local} | {remote} | {ratio:.3f} |"
    )

lines += [
    "",
    "## Shadow Mechanism",
    "",
    "| row | hit ratio | page switch | page full | cap fallback | retained peak | remote escape |",
    "|---|---:|---:|---:|---:|---:|---:|",
]
for row, hit, switch, full, cap, retained, remote in mechanism:
    lines.append(
        f"| {row} | {hit:.3f} | {switch} | {full} | {cap} | {retained} | {remote} |"
    )

lines += [
    "",
    "Read:",
    "",
    "```text",
    "A viable next substrate should cover local-like classes without adding",
    "blocked entry checks to rows that will fall back to HZ8 medium.",
    "High remote-ratio classes explain SlabPage wins; high local-like classes",
    "define the local no-regression requirement.",
    "cap_fallback and retained_peak bound the RSS side of the local page model.",
    "```",
]
dst.write_text("\n".join(lines) + "\n")
PY

{
  echo "# HZ9 Next Substrate Probe"
  echo
  echo '```text'
  echo "outdir: ${OUTDIR}"
  echo "runs_shadow: ${RUNS_SHADOW}"
  echo "runs_route: ${RUNS_ROUTE}"
  echo "runs_mutation: ${RUNS_MUTATION}"
  echo "runs_readiness: ${RUNS_READINESS}"
  echo "rows_shadow: ${ROWS_SHADOW}"
  echo "rows_route: ${ROWS_ROUTE}"
  echo "rows_mutation: ${ROWS_MUTATION}"
  echo "rows_readiness: ${ROWS_READINESS}"
  echo '```'
  echo
  echo "## Class Shadow"
  echo
  cat "${OUTDIR}/class_summary.md"
  echo
  echo "## Entry Route"
  echo
  sed -n '/| row | variant |/,$p' "${OUTDIR}/entry_route/README.md"
  echo
  echo "## Slab Mutation"
  echo
  cat "${OUTDIR}/mutation/summary.md"
  echo
  echo "## Readiness"
  echo
  sed -n '/## Candidate Gate/,$p' "${OUTDIR}/readiness/README.md"
  echo
  echo "## Decision Rule"
  echo
  echo '```text'
  echo "If hotmask is not cleaner than slabadaptive_entry, freeze SlabPage entry"
  echo "micro-tuning and design a new substrate that avoids local blocked checks."
  echo '```'
} >"${OUTDIR}/README.md"

cat "${OUTDIR}/README.md"
