#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"

ts="$(date +%Y%m%d_%H%M%S)"
git_rev="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"
out_base="/tmp/hz3_chatgpt_pro_bundle_${git_rev}_${ts}"
mkdir -p "$out_base"

copy() {
  local p="$1"
  mkdir -p "${out_base}/$(dirname "$p")"
  cp -a "$p" "${out_base}/$p"
}

# Prompt + key docs
copy "hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S18_POST_S17_NEXT_1PCT.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md"
copy "hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md"
copy "hakozuna/hz3/docs/NO_GO_SUMMARY.md"

# Core code involved in S17
copy "hakozuna/hz3/include/hz3_arena.h"
copy "hakozuna/hz3/src/hz3_arena.c"
copy "hakozuna/hz3/src/hz3_hot.c"
copy "hakozuna/hz3/include/hz3_types.h"
copy "hakozuna/hz3/include/hz3_tcache.h"
copy "hakozuna/hz3/src/hz3_tcache.c"
copy "hakozuna/hz3/src/hz3_epoch.c"
copy "hakozuna/hz3/src/hz3_small.c"
copy "hakozuna/hz3/src/hz3_small_v2.c"
copy "hakozuna/hz3/include/hz3_config.h"
copy "hakozuna/hz3/Makefile"

# Archive reference for S17 TLS NO-GO
if [[ -f "hakozuna/hz3/archive/research/s17_ptag_dstbin_tls/README.md" ]]; then
  copy "hakozuna/hz3/archive/research/s17_ptag_dstbin_tls/README.md"
fi

cat > "${out_base}/BUNDLE_README.txt" <<EOF
hz3 ChatGPT Pro bundle (post-S17)

- git: ${git_rev}
- created: ${ts}

Open:
- hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S18_POST_S17_NEXT_1PCT.md
EOF

zip_path="/tmp/hz3_chatgpt_pro_bundle_${git_rev}_${ts}.zip"
(
  cd "$out_base"
  # Store paths relative to bundle root
  zip -qr "$zip_path" .
)

echo "$zip_path"

