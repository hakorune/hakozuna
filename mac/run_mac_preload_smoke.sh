#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_preload_smoke.sh <hz3|hz4|/abs/path/to/lib.dylib> [command ...]

Examples:
  ./mac/run_mac_preload_smoke.sh hz3 /usr/bin/env true
  ./mac/run_mac_preload_smoke.sh hz4 /usr/bin/env bash -lc 'echo smoke'
EOF
}

[[ $# -ge 1 ]] || {
  usage >&2
  exit 1
}

case "$1" in
  --help|-h)
    usage
    exit 0
    ;;
esac

ALLOCATOR="$1"
shift

if [[ $# -eq 0 ]]; then
  set -- /usr/bin/env true
fi

case "$ALLOCATOR" in
  hz3)
    LIB_PATH="$ROOT_DIR/libhakozuna_hz3_scale.so"
    ;;
  hz4)
    LIB_PATH="$ROOT_DIR/hakozuna-mt/libhakozuna_hz4.so"
    ;;
  /*)
    LIB_PATH="$ALLOCATOR"
    ;;
  *)
    echo "unknown allocator selector: $ALLOCATOR" >&2
    usage >&2
    exit 1
    ;;
esac

[[ -f "$LIB_PATH" ]] || {
  echo "allocator library not found: $LIB_PATH" >&2
  echo "build first with ./mac/build_mac_release_lane.sh" >&2
  exit 1
}

echo "[mac] DYLD_INSERT_LIBRARIES=$LIB_PATH"
exec DYLD_INSERT_LIBRARIES="$LIB_PATH" "$@"
