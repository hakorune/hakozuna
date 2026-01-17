#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"

ts="$(date +%Y%m%d_%H%M%S)"
git_rev="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"

# User request: put the zip at repo root (easy to find/share).
zip_path="${ROOT}/hz3_chatgpt_pro_bundle_s130_memory_${git_rev}_${ts}.zip"

tmp_dir="/tmp/hz3_chatgpt_pro_bundle_s130_memory_${git_rev}_${ts}"
rm -rf "$tmp_dir"
mkdir -p "$tmp_dir"

copy() {
  local p="$1"
  mkdir -p "${tmp_dir}/$(dirname "$p")"
  cp -a "$p" "${tmp_dir}/$p"
}

copy_if_exists() {
  local src="$1"
  local dst_rel="$2"
  if [[ -f "$src" ]]; then
    mkdir -p "${tmp_dir}/$(dirname "$dst_rel")"
    cp -a "$src" "${tmp_dir}/$dst_rel"
  fi
}

# High-level status / results
copy "AGENTS.md"
copy "CLAUDE.md"
copy "README.md"
copy "LICENSE"
copy "NOTICE"
copy "THIRD_PARTY_NOTICES.md"
copy "CURRENT_TASK.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S130_S132_RSS_TIMESERIES_3WAY_RESULTS.md"
copy "hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S130_MEMORY.md"
copy "hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md"
copy "hakozuna/hz3/README.md"

# Lane defaults / build context
copy "hakozuna/hz3/Makefile"
copy "hakozuna/hz3/include/hz3_config.h"
copy "hakozuna/hz3/include/hz3_config_core.h"
copy "hakozuna/hz3/include/hz3_config_small_v2.h"
copy "hakozuna/hz3/include/hz3_config_scale.h"
copy "hakozuna/hz3/include/hz3_config_rss_memory.h"
copy "hakozuna/hz3/include/hz3_config_tcache_soa.h"
copy "hakozuna/hz3/include/hz3_config_sub4k.h"
copy "hakozuna/hz3/include/hz3_config_archive.h"

# Implementation (memory-related)
copy "hakozuna/hz3/src/hz3_arena.c"
copy "hakozuna/hz3/include/hz3_arena.h"
copy "hakozuna/hz3/src/hz3_mem_budget.c"
copy "hakozuna/hz3/include/hz3_mem_budget.h"
copy "hakozuna/hz3/src/hz3_release_boundary.c"
copy "hakozuna/hz3/src/hz3_release_ledger.c"
copy "hakozuna/hz3/include/hz3_release_boundary.h"
copy "hakozuna/hz3/include/hz3_release_ledger.h"
copy "hakozuna/hz3/src/hz3_large.c"
copy "hakozuna/hz3/include/hz3_large.h"
copy "hakozuna/hz3/src/hz3_os_purge.c"
copy "hakozuna/hz3/include/hz3_os_purge.h"
copy "hakozuna/hz3/src/hz3_epoch.c"
copy "hakozuna/hz3/src/hz3_segment.c"
copy "hakozuna/hz3/include/hz3_segment.h"
copy "hakozuna/hz3/src/hz3_tcache.c"
copy "hakozuna/hz3/include/hz3_tcache.h"
copy "hakozuna/hz3/include/hz3_tcache_internal.h"
copy "hakozuna/hz3/src/hz3_tcache_decay.c"
copy "hakozuna/hz3/src/hz3_tcache_pressure.c"
copy "hakozuna/hz3/include/hz3_types.h"
copy "hakozuna/hz3/include/hz3_dtor_stats.h"

# Measurement harness
copy "hakozuna/hz3/scripts/measure_mem_timeseries.sh"
copy "hakozuna/out/bench_random_mixed_malloc_dist"
copy "hakozuna/bench/bench_random_mixed_malloc_dist.c"

# CSV data (from /tmp) — copied into bundle for offline analysis.
copy_if_exists "/tmp/s130_mem_hz3.csv" "data/s130_mem_hz3.csv"
copy_if_exists "/tmp/s130_mem_tcmalloc.csv" "data/s130_mem_tcmalloc.csv"
copy_if_exists "/tmp/s130_mem_mimalloc.csv" "data/s130_mem_mimalloc.csv"
copy_if_exists "/tmp/s131_mem_hz3_lcb.csv" "data/s131_mem_hz3_lcb.csv"

cat > "${tmp_dir}/POLICY.md" <<'EOF'
# Policy (for this bundle)

This repository follows "Box Theory" (箱理論):

- Design and changes are split into small "boxes" with clear boundaries.
- Transformations happen at one boundary function (avoid spreading logic).
- Everything must be reversible (feature flags / lane split).
- Prefer SSOT one-shot stats over always-on logging.
- Fail fast on invariant violations; do not hide errors with silent fallbacks.

For this bundle, the goal is memory efficiency (RSS reduction) without changing allocator hot paths.
Prefer event-only policies (epoch/retire/release boundaries), and propose A/B-friendly changes.
EOF

cat > "${tmp_dir}/BUNDLE_README.txt" <<EOF
hz3 ChatGPT Pro bundle: S130 memory efficiency (RSS gap vs tcmalloc/mimalloc)

- git: ${git_rev}
- created: ${ts}
- zip: ${zip_path}

Entry docs:
- hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S130_MEMORY.md
- hakozuna/hz3/docs/PHASE_HZ3_S130_S132_RSS_TIMESERIES_3WAY_RESULTS.md
- POLICY.md (bundle-level rules/constraints)

CSV (copied from /tmp at bundle creation time):
- data/s130_mem_hz3.csv
- data/s130_mem_tcmalloc.csv
- data/s130_mem_mimalloc.csv
- data/s131_mem_hz3_lcb.csv
EOF

(
  cd "$tmp_dir"
  zip -qr "$zip_path" .
)

echo "$zip_path"
