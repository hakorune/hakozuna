#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
runs=${RUNS:-5}
binary=$($root/linux/build_hz12_retirement_turnover.sh)

for ((run = 1; run <= runs; ++run)); do
  printf '[HZ12_LINUX_RUN] run=%d/%d\n' "$run" "$runs"
  "$binary"
done
