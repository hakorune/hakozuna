#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
out=${HZ12_OUT_DIR:-"$root/out_linux"}
cc=${CC:-cc}
mkdir -p "$out"

sources=(
  "$root/bench/bench_hz12_retirement_turnover.c"
  "$root/src/hz12_current_span_install.c"
  "$root/src/hz12_flush_owner_route.c"
  "$root/src/hz12_live_footprint.c"
  "$root/src/hz12_owner_epoch.c"
  "$root/src/hz12_owner_registry.c"
  "$root/src/hz12_owner_retire_gate.c"
  "$root/src/hz12_public_entry.c"
  "$root/src/hz12_reclaim_entry.c"
  "$root/src/hz12_reclaim_policy_shadow.c"
  "$root/src/hz12_shadow.c"
  "$root/src/hz12_size_class.c"
  "$root/src/hz12_snapshot_reclaim.c"
  "$root/src/hz12_snapshot_recycle.c"
  "$root/src/hz12_span.c"
  "$root/src/hz12_span_backing.c"
  "$root/src/hz12_span_depot_core.c"
  "$root/src/hz12_span_owner_shadow.c"
  "$root/src/hz12_sys_alloc.c"
  "$root/src/hz12_thread_cache.c"
  "$root/src/hz12_thread_cache_diag.c"
  "$root/src/hz12_token_inbox.c"
)

"$cc" -std=c11 -O2 -DNDEBUG -Wall -Wextra -Werror -D_GNU_SOURCE \
  -DHZ12_CLASSIFY_SPAN=1 -DHZ12_CACHE_CAP=256 \
  -DHZ12_FLUSH_OWNER_ROUTE=1 -DHZ12_FLUSH_OWNER_COLD_SPAN=1 \
  -DHZ12_FLUSH_OWNER_INBOX_CAP=2048 \
  -I"$root/include" -I"$root/src" "${sources[@]}" \
  -pthread -ldl -o "$out/bench_hz12_retirement_turnover"

printf '%s\n' "$out/bench_hz12_retirement_turnover"
