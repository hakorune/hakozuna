#!/usr/bin/env bash
set -euo pipefail

# Minimal HZ10 counterpart of HZ9's own bench/lib/bench_common.sh, scoped
# to exactly what Box 1 needs: an opt-in pointer to a sibling checkout for
# the "compare same-run HZ8/HZ9/HZ10 where possible" bench/README.md rule.
# HZ10 development must not implicitly depend on sibling source trees next
# to hakozuna-hz10/.

HZ10_BENCH_COMMON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HZ10_ROOT="$(cd "${HZ10_BENCH_COMMON_DIR}/../.." && pwd)"
HZ10_EXT_ROOT="${HZ10_EXT_ROOT:-${HZ10_ROOT}/.external-disabled}"

# Locates a prebuilt HZ9 route-boundary bench binary under HZ10_EXT_ROOT, if
# the caller has pointed HZ10_EXT_ROOT at a real sibling checkout and built
# it there. Returns empty (not an error) when unavailable, so callers can
# treat the external comparison as purely opt-in.
hz10_bench_find_hz9_route_boundary_bench() {
  local candidate="${HZ9_ROUTE_BOUNDARY_BENCH:-}"
  if [[ -n "${candidate}" && -x "${candidate}" ]]; then
    printf '%s\n' "${candidate}"
    return 0
  fi
  candidate="${HZ10_EXT_ROOT}/hakozuna-hz9/h8_bench_hz9localslabrouteboundary"
  if [[ -x "${candidate}" ]]; then
    printf '%s\n' "${candidate}"
    return 0
  fi
  return 1
}
