#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_preload_smoke.sh <hz3|hz4|/abs/path/to/lib.so> [command ...]

Examples:
  ./linux/run_linux_preload_smoke.sh hz3 /bin/true
  ./linux/run_linux_preload_smoke.sh hz4 /usr/bin/env bash -lc 'echo smoke'
EOF
}

[[ $# -ge 1 ]] || {
  usage >&2
  exit 1
}

ALLOCATOR="$1"
shift

if [[ $# -eq 0 ]]; then
  set -- /bin/true
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
  echo "build first with ./linux/build_linux_release_lane.sh" >&2
  exit 1
}

echo "[linux] LD_PRELOAD=$LIB_PATH"
exec env LD_PRELOAD="$LIB_PATH" "$@"
