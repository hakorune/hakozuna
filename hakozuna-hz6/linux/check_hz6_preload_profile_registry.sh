#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 - <<'PY' "$ROOT_DIR"
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
registry_path = root / "hakozuna-hz6/docs/HZ6_UBUNTU_PROFILE_REGISTRY.md"
frontier_path = root / "hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh"
broad_guard_path = root / "hakozuna-hz6/linux/run_hz6_broad_guard.sh"
aliases_path = root / "hakozuna-hz6/linux/hz6_preload_aliases.sh"
bench_common_path = root / "bench/lib/bench_common.sh"

registry = registry_path.read_text()
frontier = frontier_path.read_text()
broad_guard = broad_guard_path.read_text()
aliases = aliases_path.read_text()
bench_common = bench_common_path.read_text()

section_re = re.compile(r"^## (?P<name>.+)$", re.MULTILINE)
sections = {}
matches = list(section_re.finditer(registry))
for index, match in enumerate(matches):
    start = match.end()
    end = matches[index + 1].start() if index + 1 < len(matches) else len(registry)
    sections[match.group("name").strip()] = registry[start:end]

alias_re = re.compile(r"^\| `(?P<alias>hz6[^`]+)` \| `(?P<builder>[^`]+)` \|", re.MULTILINE)

standard = {
    match.group("alias"): match.group("builder")
    for match in alias_re.finditer(sections.get("Standard Profile Frontier", ""))
}
explicit = {
    match.group("alias"): match.group("builder")
    for match in alias_re.finditer(sections.get("Explicit Controls", ""))
}

frontier_match = re.search(r'ALLOCATORS="\$\{ALLOCATORS:-(?P<allocators>[^"}]+)\}"', frontier)
if not frontier_match:
    raise SystemExit("could not find default ALLOCATORS in run_hz6_preload_profile_frontier.sh")

frontier_aliases = frontier_match.group("allocators").split(",")
frontier_standard = [alias for alias in frontier_aliases if alias != "hz6"]

errors = []
if frontier_standard != list(standard):
    errors.append(
        "standard registry aliases do not match profile frontier default order:\n"
        f"  registry={list(standard)}\n"
        f"  frontier={frontier_standard}"
    )

broad_profile_match = re.search(
    r'PROFILE_ALLOCATORS="\$\{PROFILE_ALLOCATORS:-(?P<allocators>[^"}]+)\}"',
    broad_guard,
)
if not broad_profile_match:
    errors.append("could not find PROFILE_ALLOCATORS in run_hz6_broad_guard.sh")
else:
    broad_profile_aliases = broad_profile_match.group("allocators").split(",")
    broad_profile_standard = [alias for alias in broad_profile_aliases if alias != "hz6"]
    if broad_profile_standard != list(standard):
        errors.append(
            "standard registry aliases do not match broad guard profile order:\n"
            f"  registry={list(standard)}\n"
            f"  broad_guard={broad_profile_standard}"
        )

for alias in explicit:
    if alias in frontier_aliases:
        errors.append(f"explicit control alias appears in default frontier: {alias}")

all_aliases = {}
all_aliases.update(standard)
all_aliases.update(explicit)

external_or_legacy_aliases = {"system", "hz3", "hz4", "hz5", "hz6", "mimalloc", "tcmalloc"}
for variable in ("FIXED_ALLOCATORS", "WORKLOAD_ALLOCATORS"):
    match = re.search(
        rf'{variable}="\$\{{{variable}:-(?P<allocators>[^"}}]+)\}}"',
        broad_guard,
    )
    if not match:
        errors.append(f"could not find {variable} in run_hz6_broad_guard.sh")
        continue
    for alias in match.group("allocators").split(","):
        if alias in external_or_legacy_aliases:
            continue
        if alias not in all_aliases:
            errors.append(f"{variable} uses unregistered HZ6 alias: {alias}")

for alias, builder in all_aliases.items():
    builder_path = root / "hakozuna-hz6/linux" / builder
    if not builder_path.is_file():
        errors.append(f"missing builder for {alias}: {builder_path}")
    underscore_alias = alias.replace("-", "_")
    for spelling in (alias, underscore_alias):
        if spelling not in aliases:
            errors.append(f"missing alias-builder wiring for {spelling}")
        if spelling not in bench_common:
            errors.append(f"missing bench resolver wiring for {spelling}")

if errors:
    for error in errors:
        print(f"[profile-registry-check] {error}", file=sys.stderr)
    raise SystemExit(1)

print("[profile-registry-check] ok")
print(f"[profile-registry-check] standard={len(standard)} explicit={len(explicit)}")
PY
