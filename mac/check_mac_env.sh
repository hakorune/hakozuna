#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail=0

check_cmd() {
  local cmd="$1"
  local hint="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo "[mac] found: $cmd -> $(command -v "$cmd")"
  else
    echo "[mac] missing: $cmd" >&2
    echo "[mac] hint: $hint" >&2
    fail=1
  fi
}

echo "[mac] repo root: $ROOT_DIR"
echo "[mac] uname: $(uname -s)"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "[mac] this check is intended for macOS" >&2
  fail=1
fi

check_cmd clang "Install Xcode Command Line Tools with: xcode-select --install"
check_cmd git "Install Git or Xcode Command Line Tools"
check_cmd brew "Install Homebrew from https://brew.sh/"
check_cmd gmake "Install GNU Make with: brew install gnu-make"

if xcode-select -p >/dev/null 2>&1; then
  echo "[mac] xcode-select path: $(xcode-select -p)"
else
  echo "[mac] missing: xcode-select developer tools" >&2
  echo "[mac] hint: xcode-select --install" >&2
  fail=1
fi

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "[mac] environment looks ready"
