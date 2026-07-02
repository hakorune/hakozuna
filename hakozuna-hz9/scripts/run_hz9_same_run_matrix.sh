#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export MATRIX_TITLE="${MATRIX_TITLE:-HZ9 Same-Run Allocator Matrix}"
exec "${ROOT}/scripts/run_hz9_same_run_matrix_impl.sh" "$@"
