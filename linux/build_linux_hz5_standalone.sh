#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
OUT_DIR=""

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_hz5_standalone.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --out-dir DIR      output directory (default: hakozuna-hz5/out/linux/<arch>)
  --help             show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --out-dir" >&2; exit 1; }
      OUT_DIR="$2"
      shift 2
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

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

HZ5_DIR="${ROOT_DIR}/hakozuna-hz5"
OUT_DIR="${OUT_DIR:-${HZ5_DIR}/out/linux/${ARCH}}"
LIB="${OUT_DIR}/libhakozuna_hz5_standalone.so"
BENCH="${OUT_DIR}/bench_hz5_standalone_aligned64k"
GENERIC_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"

mkdir -p "$OUT_DIR"

COMMON_FLAGS=(
  -O3
  -fPIC
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-unused-variable
  -std=c11
  -D_GNU_SOURCE
  -DNDEBUG
  -DHZ5_DIAGNOSTIC_STATS=0
  -DBENCHLAB_HZ5_SPEED_LANE=1
  -DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1
  -DBENCHLAB_HZ5_STANDALONE_EXACT_ONLY=1
  -DBENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192=1
  -DBENCHLAB_HZ5_P25_STATS=0
  -I"${HZ5_DIR}/include"
  -I"${HZ5_DIR}/contract"
  -I"${HZ5_DIR}/policy"
  -I"${HZ5_DIR}/route"
  -I"${HZ5_DIR}/wrapper"
  -I"${HZ5_DIR}/lowpage"
  -I"${HZ5_DIR}/fallback"
)

HZ5_SRCS=(
  "${HZ5_DIR}/api/hz5_api.c"
  "${HZ5_DIR}/contract/hz5_contract.c"
  "${HZ5_DIR}/core/hz5_segment.c"
  "${HZ5_DIR}/core/hz5_run.c"
  "${HZ5_DIR}/core/hz5_owner.c"
  "${HZ5_DIR}/core/hz5_remote.c"
  "${HZ5_DIR}/core/hz5_tcache.c"
  "${HZ5_DIR}/core/hz5_stats.c"
  "${HZ5_DIR}/policy/hz5_policy.c"
  "${HZ5_DIR}/route/hz5_route.c"
  "${HZ5_DIR}/wrapper/hz5_wrapper.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64_p43_segment.c"
  "${HZ5_DIR}/linux/hz5_linux_compat.c"
)

echo "[linux][hz5] arch: ${ARCH}"
echo "[linux][hz5] building library: ${LIB}"
gcc "${COMMON_FLAGS[@]}" -shared "${HZ5_SRCS[@]}" -pthread -ldl -o "$LIB"

echo "[linux][hz5] building benchmark: ${BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_aligned64k.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$BENCH"

mkdir -p "$(dirname "$GENERIC_BENCH")"
echo "[linux][hz5] building generic aligned benchmark: ${GENERIC_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_aligned64k.c" \
  -pthread -o "$GENERIC_BENCH"

echo "[linux][hz5] done"
