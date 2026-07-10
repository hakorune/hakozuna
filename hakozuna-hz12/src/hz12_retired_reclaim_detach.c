#include "hz12_retired_reclaim_detach.h"

#include "hz12_owner_retire_gate.h"
#include "hz12_reclaim_gate.h"
#include "hz12_span.h"
#include "hz12_span_detach.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

int h12_retired_reclaim_detach_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDetachResult* out) {
  uint32_t span_id;
  if (!out || budget == 0u) return 0;
  memset(out, 0, sizeof(*out));
  out->budget = budget;
  if (!h12_owner_retire_gate_ready(owner)) return 0;
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT && out->attempts < budget;
       ++span_id) {
    H12OwnerToken observed;
    H12ReclaimGate gate;
    H12SpanDetachResult detach;
    void* span_base;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != owner.slot ||
        observed.generation != owner.generation) {
      continue;
    }
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_reclaim_gate_query(span_base, 1, &gate) ||
        !gate.accounting_candidate || !gate.accounting_clean ||
        !gate.thread_scope_complete || gate.current_span_refs != 0u ||
        gate.front_cache_objects != 0u || !gate.route_attached ||
        gate.returned_sink_objects != gate.slot_capacity) {
      continue;
    }
    out->candidates += 1u;
    out->attempts += 1u;
    if (h12_span_detach_quiescent(span_base, &detach) && detach.completed) {
      out->succeeded += 1u;
      out->detached_bytes += HZ12_SPAN_BYTES;
    } else {
      out->failed += 1u;
      break;
    }
  }
  return out->failed == 0u && out->succeeded != 0u;
}

void h12_retired_reclaim_detach_dump(
    FILE* out, const H12RetiredReclaimDetachResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_RETIRED_RECLAIM_DETACH] budget=%u candidates=%u "
          "attempts=%u succeeded=%u failed=%u detached_bytes=%llu\n",
          result->budget, result->candidates, result->attempts,
          result->succeeded, result->failed,
          (unsigned long long)result->detached_bytes);
}
