#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"

ts="$(date +%Y%m%d_%H%M%S)"
git_rev="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"
out_base="/tmp/hz3_chatgpt_pro_bundle_${git_rev}_${ts}_s100"
mkdir -p "$out_base"

copy() {
  local p="$1"
  mkdir -p "${out_base}/$(dirname "$p")"
  cp -a "$p" "${out_base}/$p"
}

# Prompt + key docs
copy "hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S100_FREE_LEAF_FASTMETA.md"
copy "hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md"
copy "hakozuna/hz3/docs/NO_GO_SUMMARY.md"
copy "CURRENT_TASK.md"

# Core code involved in hz3_free fast path
copy "hakozuna/hz3/src/hz3_hot.c"
copy "hakozuna/hz3/include/hz3_arena.h"
copy "hakozuna/hz3/include/hz3_tcache.h"
copy "hakozuna/hz3/include/hz3_types.h"
copy "hakozuna/hz3/src/hz3_small_v2.c"
copy "hakozuna/hz3/src/hz3_tcache_remote.c"
copy "hakozuna/hz3/include/hz3_config.h"
copy "hakozuna/hz3/include/hz3_config_scale.h"
copy "hakozuna/hz3/include/hz3_config_tcache_soa.h"
copy "hakozuna/hz3/Makefile"

# Reference: archived S98-1 micro-opt writeup (measurement stability lesson)
if [[ -f "hakozuna/hz3/archive/research/s98_1_push_remote_microopt/README.md" ]]; then
  copy "hakozuna/hz3/archive/research/s98_1_push_remote_microopt/README.md"
fi

cat > "${out_base}/BUNDLE_README.txt" <<EOF
hz3 ChatGPT Pro bundle (S100: hz3_free leaf/fastmeta)

- git: ${git_rev}
- created: ${ts}

Open:
- hakozuna/hz3/docs/CHATGPT_PRO_PROMPT_S100_FREE_LEAF_FASTMETA.md

Optional local command to attach annotate snippet:
- perf annotate -i /tmp/perf_r90_pf2_s88on.data --symbol hz3_free --stdio | head -n 120
EOF

zip_path="/tmp/hz3_chatgpt_pro_bundle_${git_rev}_${ts}_s100.zip"
(
  cd "$out_base"
  zip -qr "$zip_path" .
)

echo "$zip_path"
