#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"

ts="$(date +%Y%m%d_%H%M%S)"
git_rev="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"

# User request: put the zip at repo root (easy to find/share).
zip_path="${ROOT}/hz3_chatgpt_pro_bundle_s97_${git_rev}_${ts}.zip"

tmp_dir="/tmp/hz3_chatgpt_pro_bundle_s97_${git_rev}_${ts}"
rm -rf "$tmp_dir"
mkdir -p "$tmp_dir"

copy() {
  local p="$1"
  mkdir -p "${tmp_dir}/$(dirname "$p")"
  cp -a "$p" "${tmp_dir}/$p"
}

# Key docs
copy "CURRENT_TASK.md"
copy "hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S97_1_REMOTE_STASH_BUCKET_BOX_WORK_ORDER.md"
copy "hakozuna/hz3/docs/PHASE_HZ3_S97_2_REMOTE_STASH_DIRECTMAP_BUCKET_BOX_WORK_ORDER.md"

# Lane defaults / build context
copy "hakozuna/hz3/Makefile"
copy "hakozuna/hz3/include/hz3_config.h"
copy "hakozuna/hz3/include/hz3_config_scale.h"
copy "hakozuna/hz3/include/hz3_config_tcache_soa.h"

# Core implementation (flush boundary + dispatch targets)
copy "hakozuna/hz3/src/hz3_tcache_remote.c"
copy "hakozuna/hz3/include/hz3_tcache_internal.h"
copy "hakozuna/hz3/include/hz3_tcache.h"
copy "hakozuna/hz3/src/hz3_tcache.c"
copy "hakozuna/hz3/include/hz3_types.h"
copy "hakozuna/hz3/include/hz3_owner_stash.h"
copy "hakozuna/hz3/src/hz3_owner_stash.c"
copy "hakozuna/hz3/src/hz3_small_v2.c"
copy "hakozuna/hz3/include/hz3_dtor_stats.h"

# Bench source (repro + variance discussion)
copy "hakozuna/bench/bench_random_mixed_mt_remote.c"
copy "hakozuna/include/hz_tcache.h"
copy "hakozuna/include/hz_stats.h"
copy "hakozuna/Makefile"

# Local A/B harness (paired-delta + thread sweep)
copy "hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh"

cat > "${tmp_dir}/BUNDLE_README.txt" <<EOF
hz3 ChatGPT Pro bundle: S97 remote stash bucketize (flush-side merge)

- git: ${git_rev}
- created: ${ts}
- zip: ${zip_path}

Open (docs):
- hakozuna/hz3/docs/PHASE_HZ3_S97_2_REMOTE_STASH_DIRECTMAP_BUCKET_BOX_WORK_ORDER.md
- hakozuna/hz3/src/hz3_tcache_remote.c (boundary: hz3_remote_stash_flush_budget_impl)

Suggested question (paste to ChatGPT Pro):

"We only change the flush-side boundary (hz3_remote_stash_flush_budget_impl) to merge repeated (dst,bin) within a flush window.
Push side is n=1 dominated. S97-1 (open addressing) causes branch-miss explosions on r90.
S97-2 (direct-map+stamp, probe-less) reduces branch-miss but increases instructions; effect seems thread-count sensitive and noisy.
Constraints: MAX_KEYS <= 256; avoid memset-clear per call (prefer epoch/stamp); keep hot path unchanged.
Please propose probe-less designs that reduce instruction count (touched list/bitset/key-sort/2-level tables), with pseudocode and expected tradeoffs."

Local A/B sweep (paired-delta):
- RUNS=20 PIN=1 THREADS_LIST='8 16 32' REMOTE_PCTS='50 90' ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
EOF

(
  cd "$tmp_dir"
  zip -qr "$zip_path" .
)

echo "$zip_path"

