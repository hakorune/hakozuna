#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${ROOT_DIR}/linux/build_linux_arm64_order_gate_release_lane.sh"
exec "${ROOT_DIR}/linux/run_linux_arm64_bench_compare.sh" "$@"
