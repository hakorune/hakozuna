#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

HZ6_WORKLOAD_CAPACITY_LEVEL=mid \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_target.sh"
