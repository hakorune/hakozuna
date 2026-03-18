#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HZ3_TARGET="all_ldpreload_scale"
HZ4_TARGET="all_perf_lib"
BUILD_HZ3=1
BUILD_HZ4=1

usage() {
  cat <<'EOF'
Usage:
  ./mac/build_mac_release_lane.sh [options]

Options:
  --hz3-target <target>  hz3 make target (default: all_ldpreload_scale)
  --hz4-target <target>  hz4 make target (default: all_perf_lib)
  --skip-hz3             skip hz3 build
  --skip-hz4             skip hz4 build
  --help                 show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --hz3-target)
      [[ $# -ge 2 ]] || { echo "missing value for --hz3-target" >&2; exit 1; }
      HZ3_TARGET="$2"
      shift 2
      ;;
    --hz4-target)
      [[ $# -ge 2 ]] || { echo "missing value for --hz4-target" >&2; exit 1; }
      HZ4_TARGET="$2"
      shift 2
      ;;
    --skip-hz3)
      BUILD_HZ3=0
      shift
      ;;
    --skip-hz4)
      BUILD_HZ4=0
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

"$ROOT_DIR/mac/check_mac_env.sh"

if [[ "$BUILD_HZ3" -eq 1 ]]; then
  echo "[mac] building hz3 target: $HZ3_TARGET"
  gmake -C "$ROOT_DIR/hakozuna" clean "$HZ3_TARGET"
  echo "[mac] hz3 build lane complete"
fi

if [[ "$BUILD_HZ4" -eq 1 ]]; then
  echo "[mac] building hz4 target: $HZ4_TARGET"
  gmake -C "$ROOT_DIR/hakozuna-mt" clean "$HZ4_TARGET"
  echo "[mac] hz4 build lane complete"
fi

echo "[mac] done"
