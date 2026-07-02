#include "h8_internal.h"

#include <string.h>

void h8_debug_stats_snapshot(H8DebugStats* out) {
#if !defined(H8_ENABLE_DEBUG_STATS)
  memset(out, 0, sizeof(*out));
  return;
#else
#define H8_DEBUG_LOAD(field) \
  out->field = atomic_load_explicit(&h8g.field, memory_order_acquire)
#include "h8_stats_impl_part1_body.inc"
#include "h8_stats_impl_part2_body.inc"
#undef H8_DEBUG_LOAD
#endif
}

#include "h8_stats_query.inc"
