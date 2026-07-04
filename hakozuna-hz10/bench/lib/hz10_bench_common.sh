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

# Locates a real tcmalloc shared object to LD_PRELOAD over
# hz10_public_entry_bench's mech=system_malloc path (plain libc
# malloc/free calls -- LD_PRELOAD swaps them for tcmalloc's with zero new
# code, the same ad hoc technique used to first measure this in
# current_task.md). Opt-in like the HZ9 comparison above: an explicit
# TCMALLOC_LIB wins, otherwise a handful of common distro install paths
# are probed; returns empty (not an error) if none exist, so callers can
# skip the comparison gracefully rather than fail the whole run.
hz10_bench_find_tcmalloc_lib() {
  local candidate="${TCMALLOC_LIB:-}"
  if [[ -n "${candidate}" && -f "${candidate}" ]]; then
    printf '%s\n' "${candidate}"
    return 0
  fi
  local path
  for path in \
    /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/local/lib/libtcmalloc_minimal.so.4 \
    /usr/lib/libtcmalloc_minimal.so.4; do
    if [[ -f "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

# Locates HZ8's own default preload shared object (libhakozuna_hz8_preload.so,
# built by HZ8's own `make preload` -- HZ8-v2/KeepRefill baked in as the
# current default, see HZ8's own current_task.md), to LD_PRELOAD over
# hz10_public_entry_bench's mech=system_malloc path -- same technique as
# hz10_bench_find_tcmalloc_lib() above, needed because current_task.md's
# "first GO" bar is stated relative to HZ8, not tcmalloc. Opt-in like the
# HZ9 comparison above: an explicit HZ8_PRELOAD_LIB wins, otherwise looks
# under HZ10_EXT_ROOT (a sibling checkout the caller must point at
# explicitly -- HZ10 must not implicitly depend on sibling source trees).
# Returns empty (not an error) if unavailable, so callers can skip this
# comparison gracefully.
hz10_bench_find_hz8_preload_lib() {
  local candidate="${HZ8_PRELOAD_LIB:-}"
  if [[ -n "${candidate}" && -f "${candidate}" ]]; then
    printf '%s\n' "${candidate}"
    return 0
  fi
  candidate="${HZ10_EXT_ROOT}/hakozuna-hz8/libhakozuna_hz8_preload.so"
  if [[ -f "${candidate}" ]]; then
    printf '%s\n' "${candidate}"
    return 0
  fi
  return 1
}
