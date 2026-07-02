#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

exec "${ROOT}/scripts/run_hz9_largedirect_probe_gate_impl.sh" "$@"
