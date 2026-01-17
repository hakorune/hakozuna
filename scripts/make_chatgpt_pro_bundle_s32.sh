#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"

ts="$(date +%Y%m%d_%H%M%S)"
git_rev="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"

# User request: put the zip under the hakozuna folder (easy to find/share).
zip_path="${ROOT}/hakozuna/hz3_chatgpt_pro_bundle_s32_${git_rev}_${ts}.zip"

tmp_dir="/tmp/hz3_chatgpt_pro_bundle_s32_${git_rev}_${ts}"
rm -rf "$tmp_dir"
mkdir -p "$tmp_dir"

copy() {
  local p="$1"
  mkdir -p "${tmp_dir}/$(dirname "$p")"
  cp -a "$p" "${tmp_dir}/$p"
}

# Prompt + key docs (S29/S30/S31)
copy "hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S32_TLS_INIT_CHECK_AND_DST_COMPARE.md"
copy "hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md"
copy "hakozuna/hz3/docs/NO_GO_SUMMARY.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S29_DISTAPP_GAP_REFRESH_PERF_WORK_ORDER.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S30_RESULTS.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md"

# Current lane defaults
copy "hakozuna/hz3/Makefile"
copy "hakozuna/hz3/include/hz3_config.h"

# Core code (TLS init check + dst compare + PTAG32 lookup)
copy "hakozuna/hz3/include/hz3_arena.h"
copy "hakozuna/hz3/src/hz3_arena.c"
copy "hakozuna/hz3/include/hz3_tcache.h"
copy "hakozuna/hz3/src/hz3_tcache.c"
copy "hakozuna/hz3/include/hz3_types.h"
copy "hakozuna/hz3/src/hz3_hot.c"
copy "hakozuna/hz3/src/hz3_small.c"
copy "hakozuna/hz3/src/hz3_small_v2.c"
copy "hakozuna/hz3/src/hz3_epoch.c"

# Bench distribution definition (app/trimix)
copy "hakozuna/bench/bench_random_mixed_malloc_dist.c"

cat > "${tmp_dir}/BUNDLE_README.txt" <<EOF
hz3 ChatGPT Pro bundle: S32 (TLS init check + dst compare)

- git: ${git_rev}
- created: ${ts}
- zip: ${zip_path}

Open:
- hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S32_TLS_INIT_CHECK_AND_DST_COMPARE.md
EOF

(
  cd "$tmp_dir"
  zip -qr "$zip_path" .
)

echo "$zip_path"

