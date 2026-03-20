#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
CACHE_DIR="${CACHE_DIR:-${ROOT_DIR}/private/bench-assets/linux/allocators}"

usage() {
  cat <<'EOF'
Usage:
  ./linux/prepare_linux_bench_allocators.sh [options]

Options:
  --arch <arch>   override detected arch (default: auto)
  --cache-dir DIR local package cache root (default: private/bench-assets/linux/allocators)
  --help          show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --cache-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --cache-dir" >&2; exit 1; }
      CACHE_DIR="$2"
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

command -v apt >/dev/null 2>&1 || {
  echo "apt not found in PATH" >&2
  exit 1
}
command -v dpkg-deb >/dev/null 2>&1 || {
  echo "dpkg-deb not found in PATH" >&2
  exit 1
}
command -v find >/dev/null 2>&1 || {
  echo "find not found in PATH" >&2
  exit 1
}

mkdir -p "${CACHE_DIR}/${ARCH}"

download_and_extract() {
  local pkg="$1"
  local target_dir="${CACHE_DIR}/${ARCH}/${pkg}"
  local archive_dir="${CACHE_DIR}/${ARCH}/archives"

  mkdir -p "${archive_dir}" "${target_dir}"
  if ! find "${target_dir}" -type f | grep -q .; then
    echo "[linux] downloading ${pkg}"
    apt download "${pkg}" >/dev/null
    local deb
    deb="$(ls -1 "${pkg}"_*.deb | tail -n 1)"
    echo "[linux] extracting ${deb} -> ${target_dir}"
    dpkg-deb -x "${deb}" "${target_dir}"
    rm -f "${deb}"
  fi
}

download_and_extract libmimalloc2.0
download_and_extract libgoogle-perftools4t64
download_and_extract libtcmalloc-minimal4t64

MIMALLOC_SO="$(find "${CACHE_DIR}/${ARCH}/libmimalloc2.0" -type f \( -name 'libmimalloc.so*' -o -name 'libmimalloc_minimal.so*' \) | sort | head -n 1)"
TCMALLOC_SO="$(find "${CACHE_DIR}/${ARCH}/libtcmalloc-minimal4t64" -type f \( -name 'libtcmalloc_minimal.so*' -o -name 'libtcmalloc.so*' \) | sort | head -n 1)"

if [[ -z "${MIMALLOC_SO}" ]]; then
  echo "failed to locate mimalloc shared library under ${CACHE_DIR}/${ARCH}/libmimalloc2.0" >&2
  exit 1
fi

if [[ -z "${TCMALLOC_SO}" ]]; then
  TCMALLOC_SO="$(find "${CACHE_DIR}/${ARCH}/libgoogle-perftools4t64" -type f \( -name 'libtcmalloc_minimal.so*' -o -name 'libtcmalloc.so*' \) | sort | head -n 1)"
fi

if [[ -z "${TCMALLOC_SO}" ]]; then
  echo "failed to locate tcmalloc shared library under ${CACHE_DIR}/${ARCH}" >&2
  exit 1
fi

echo "[linux] arch: ${ARCH}" >&2
echo "[linux] mimalloc: ${MIMALLOC_SO}" >&2
echo "[linux] tcmalloc: ${TCMALLOC_SO}" >&2

printf 'export MIMALLOC_SO=%q\n' "${MIMALLOC_SO}"
printf 'export TCMALLOC_SO=%q\n' "${TCMALLOC_SO}"
