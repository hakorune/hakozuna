#!/usr/bin/env python3
"""Summarize HZ9 ProductEntry gate logs.

The script accepts one or more bench result directories.  It understands the
focused gate convention:

  <row>_base.log / <row>_cand.log

and standalone pressure logs such as:

  medium_local0_t16.log

It emits a compact Markdown summary with throughput, RSS, and HZ9 segment
counters.  Bench result directories are intentionally ignored by git, so this
script is the reproducible bridge from raw local logs to docs/ledger text.
"""

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path


KEY_VALUE_RE = re.compile(r"([A-Za-z0-9_]+)=([^ \n]+)")


def parse_key_values(line: str) -> dict[str, str]:
    return {key: value.rstrip(",") for key, value in KEY_VALUE_RE.findall(line)}


def to_float(value: str | None) -> float:
    if value is None or value == "":
        return math.nan
    try:
        return float(value)
    except ValueError:
        return math.nan


def to_int(value: str | None) -> int:
    if value is None or value == "":
        return 0
    try:
        return int(float(value))
    except ValueError:
        return 0


def mib(value: float) -> float:
    if math.isnan(value):
        return math.nan
    return value / (1024.0 * 1024.0)


def fmt_float(value: float, digits: int = 3) -> str:
    if math.isnan(value):
        return "n/a"
    return f"{value:.{digits}f}"


def fmt_mib(value: float) -> str:
    if math.isnan(value):
        return "n/a"
    return f"{mib(value):.2f}"


def parse_log(path: Path) -> dict[str, object]:
    data: dict[str, object] = {"path": path, "segments": {}}
    for line in path.read_text(errors="replace").splitlines():
        if line.startswith("summary "):
            data["summary"] = parse_key_values(line)
        elif line.startswith("throughput "):
            kv = parse_key_values(line)
            data["ops"] = to_float(kv.get("median"))
            data["ops_p25"] = to_float(kv.get("p25"))
            data["ops_min"] = to_float(kv.get("min"))
        elif line.startswith("post_rss "):
            kv = parse_key_values(line)
            data["post_rss"] = to_float(kv.get("median"))
        elif line.startswith("peak_rss "):
            kv = parse_key_values(line)
            data["peak_rss"] = to_float(kv.get("median"))
        elif line.startswith("page_faults "):
            kv = parse_key_values(line)
            data["minor_faults"] = to_float(kv.get("minor_median"))
        elif line.startswith("h9_lsp_segments "):
            data["segments"] = parse_key_values(line)
        elif line.startswith("hz9_lsp "):
            data["segments"] = parse_key_values(line)
    return data


def row_name_from_file(path: Path, suffix: str) -> str:
    name = path.name
    if name.endswith(suffix):
        return name[: -len(suffix)]
    return path.stem


def collect_pairs(directory: Path) -> list[tuple[str, dict[str, object], dict[str, object]]]:
    pairs = []
    for base_path in sorted(directory.glob("*_base.log")):
        row = row_name_from_file(base_path, "_base.log")
        cand_path = directory / f"{row}_cand.log"
        if cand_path.exists():
            pairs.append((row, parse_log(base_path), parse_log(cand_path)))
    return pairs


def collect_standalone(directory: Path) -> list[tuple[str, dict[str, object]]]:
    paired = {p.name for p in directory.glob("*_base.log")}
    paired.update(p.name for p in directory.glob("*_cand.log"))
    rows = []
    for path in sorted(directory.glob("*.log")):
        if path.name in paired:
            continue
        rows.append((path.stem, parse_log(path)))
    return rows


def segment_int(data: dict[str, object], key: str) -> int:
    segments = data.get("segments", {})
    if not isinstance(segments, dict):
        return 0
    value = segments.get(key)
    if value is None:
        value = segments.get(f"segment_{key}")
    return to_int(value)


def segment_triplet(data: dict[str, object]) -> str:
    return (
        f"{segment_int(data, 'create')}/"
        f"{segment_int(data, 'release')}/"
        f"{segment_int(data, 'live')}"
    )


def remote_triplet(data: dict[str, object]) -> str:
    return (
        f"{segment_int(data, 'remote_claim')}/"
        f"{segment_int(data, 'drain')}/"
        f"{segment_int(data, 'drain_invalid')}"
    )


def emit_pair_table(lines: list[str], pairs: list[tuple[str, dict[str, object], dict[str, object]]]) -> None:
    if not pairs:
        return
    lines.extend(
        [
            "### Same-Run Pairs",
            "",
            "| row | base Mops/s | cand Mops/s | ratio | cand post MiB | cand peak MiB | create/release/live | cap_reject | remote/drain/invalid |",
            "|---|---:|---:|---:|---:|---:|---:|---:|---:|",
        ]
    )
    for row, base, cand in pairs:
        base_ops = float(base.get("ops", math.nan))
        cand_ops = float(cand.get("ops", math.nan))
        ratio = cand_ops / base_ops if base_ops and not math.isnan(base_ops) else math.nan
        lines.append(
            f"| {row} | {fmt_float(base_ops / 1e6)} | {fmt_float(cand_ops / 1e6)} | "
            f"{fmt_float(ratio)} | {fmt_mib(float(cand.get('post_rss', math.nan)))} | "
            f"{fmt_mib(float(cand.get('peak_rss', math.nan)))} | {segment_triplet(cand)} | "
            f"{segment_int(cand, 'cap_reject')} | {remote_triplet(cand)} |"
        )
    lines.append("")


def emit_standalone_table(lines: list[str], rows: list[tuple[str, dict[str, object]]]) -> None:
    if not rows:
        return
    lines.extend(
        [
            "### Standalone / Pressure Logs",
            "",
            "| log | Mops/s | post MiB | peak MiB | create/release/live | cache drop | cap_reject | remote/drain/slots/invalid |",
            "|---|---:|---:|---:|---:|---:|---:|---:|",
        ]
    )
    for row, data in rows:
        ops = float(data.get("ops", math.nan))
        remote = (
            f"{segment_int(data, 'remote_claim')}/"
            f"{segment_int(data, 'drain')}/"
            f"{segment_int(data, 'drain_slots')}/"
            f"{segment_int(data, 'drain_invalid')}"
        )
        lines.append(
            f"| {row} | {fmt_float(ops / 1e6)} | "
            f"{fmt_mib(float(data.get('post_rss', math.nan)))} | "
            f"{fmt_mib(float(data.get('peak_rss', math.nan)))} | "
            f"{segment_triplet(data)} | {segment_int(data, 'cache_drop')} | "
            f"{segment_int(data, 'cap_reject')} | {remote} |"
        )
    lines.append("")


def emit_gates(lines: list[str], datasets: list[dict[str, object]]) -> None:
    if not datasets:
        return
    cap_reject = sum(segment_int(data, "cap_reject") for data in datasets)
    drain_invalid = sum(segment_int(data, "drain_invalid") for data in datasets)
    remote_dup = sum(segment_int(data, "remote_dup") for data in datasets)
    remote_invalid = sum(segment_int(data, "remote_invalid") for data in datasets)
    releases = sum(segment_int(data, "release") for data in datasets)
    lines.extend(
        [
            "### Gate Read",
            "",
            f"- `cap_reject` total: `{cap_reject}`",
            f"- `drain_invalid` total: `{drain_invalid}`",
            f"- `remote_dup` total: `{remote_dup}`",
            f"- `remote_invalid` total: `{remote_invalid}`",
            f"- `segment_release` total: `{releases}`",
            "",
        ]
    )


def summarize_directory(directory: Path) -> str:
    pairs = collect_pairs(directory)
    standalone = collect_standalone(directory)
    datasets = [data for _, data in standalone]
    datasets.extend(data for _, _, data in pairs)
    lines = [f"## {directory}", ""]
    emit_pair_table(lines, pairs)
    emit_standalone_table(lines, standalone)
    emit_gates(lines, datasets)
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directories", nargs="+", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    sections = ["# HZ9 ProductEntry Gate Summary", ""]
    for directory in args.directories:
        sections.append(summarize_directory(directory))
    output = "\n".join(section.rstrip() for section in sections).rstrip() + "\n"
    if args.output:
        args.output.write_text(output)
    else:
        print(output, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
