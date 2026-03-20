#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HZ4_LIB="${HZ4_LIB:-${ROOT_DIR}/mac/out/observe/libhakozuna_hz4_macos_foreign_safe.so}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/build_mac_foreign_safe_lane.sh

Build the Mac foreign-pointer safety lane:
  - hz3 stays skipped
  - hz4 is rebuilt with HZ4_MACOS_FOREIGN_SAFE=1
  - the helper boundary lives in hakozuna-mt/src/hz4_macos_foreign.[ch]
  - this is a compatibility alias; the default Mac lanes already include the helper
EOF
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  usage
  exit 0
fi

if [[ $# -gt 0 ]]; then
  echo "unknown option: $1" >&2
  usage >&2
  exit 1
fi

"$ROOT_DIR/mac/check_mac_env.sh"

echo "[mac] building foreign-pointer safety lane"
echo "[mac] hz4 safety lib: ${HZ4_LIB}"
"$ROOT_DIR/mac/build_mac_observe_lane.sh" \
  --skip-hz3 \
  --hz4-lib "$HZ4_LIB" \
  --hz4-extra "-DHZ4_MACOS_FOREIGN_SAFE=1"

echo "[mac] safety lane ready"
echo "export HZ4_SO=${HZ4_LIB}"
echo "[mac] example: pair with an existing HZ3_SO from the release or observe lane"
printf '  HZ3_SO=%q HZ4_SO=%q ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4\n' \
  "${ROOT_DIR}/libhakozuna_hz3_scale.so" \
  "${HZ4_LIB}"
