#include "hz12_retired_reclaim_shadow.h"

#include "hz12_owner_retire_gate.h"
#include "hz12_reclaim_gate.h"
#include "hz12_span.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

H12RetiredReclaimShadow h12_retired_reclaim_shadow_scan(
    H12OwnerToken owner, int thread_scope_complete) {
  H12RetiredReclaimShadow result;
  uint32_t span_id;
  memset(&result, 0, sizeof(result));
  result.thread_scope_complete = (uint8_t)(thread_scope_complete != 0);
  result.owner_gate_open =
      (uint8_t)h12_owner_retire_gate_ready(owner);
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT; ++span_id) {
    H12OwnerToken observed;
    H12ReclaimGate gate;
    void* span_base;
    int pre_detach_ready;
    if (!h12_span_owner_shadow_query(span_id, &observed)) continue;
    if (observed.slot != owner.slot) continue;
    if (observed.generation != owner.generation) {
      result.stale_generation_spans += 1u;
      continue;
    }
    result.owner_spans += 1u;
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_reclaim_gate_query(span_base, thread_scope_complete, &gate)) {
      result.blocked_accounting += 1u;
      continue;
    }
    if (gate.accounting_candidate) {
      result.accounting_candidates += 1u;
      result.accounting_candidate_bytes += HZ12_SPAN_BYTES;
    }
    if (!result.owner_gate_open) result.blocked_owner_gate += 1u;
    if (!gate.thread_scope_complete) {
      result.blocked_foreign_cache_scope += 1u;
    }
    if (!gate.accounting_candidate || !gate.accounting_clean) {
      result.blocked_accounting += 1u;
    }
    if (gate.current_span_refs != 0u) result.blocked_current += 1u;
    if (gate.front_cache_objects != 0u) result.blocked_front_cache += 1u;
    if (gate.returned_sink_objects != 0u) result.blocked_returned_sink += 1u;
    if (gate.route_attached) result.blocked_route += 1u;
    pre_detach_ready = result.owner_gate_open && gate.accounting_candidate &&
        gate.accounting_clean && gate.thread_scope_complete &&
        gate.current_span_refs == 0u && gate.front_cache_objects == 0u &&
        gate.returned_sink_objects == 0u && gate.route_attached;
    if (pre_detach_ready) {
      result.detach_ready_spans += 1u;
      result.detach_ready_bytes += HZ12_SPAN_BYTES;
    }
    if (result.owner_gate_open && gate.gate_open) {
      result.reclaimable_spans += 1u;
      result.reclaimable_bytes += HZ12_SPAN_BYTES;
    }
  }
  return result;
}

void h12_retired_reclaim_shadow_dump(FILE* out,
                                     const H12RetiredReclaimShadow* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_RETIRED_RECLAIM_SHADOW] owner_spans=%u stale=%u "
          "accounting_candidates=%u detach_ready=%u reclaimable=%u "
          "candidate_bytes=%llu detach_ready_bytes=%llu "
          "reclaimable_bytes=%llu owner_gate=%u "
          "thread_scope=%u blocked_owner=%u blocked_foreign_cache=%u "
          "blocked_accounting=%u blocked_current=%u blocked_front=%u "
          "blocked_returned=%u blocked_route=%u\n",
          result->owner_spans, result->stale_generation_spans,
          result->accounting_candidates, result->detach_ready_spans,
          result->reclaimable_spans,
          (unsigned long long)result->accounting_candidate_bytes,
          (unsigned long long)result->detach_ready_bytes,
          (unsigned long long)result->reclaimable_bytes,
          (unsigned)result->owner_gate_open,
          (unsigned)result->thread_scope_complete,
          result->blocked_owner_gate, result->blocked_foreign_cache_scope,
          result->blocked_accounting, result->blocked_current,
          result->blocked_front_cache, result->blocked_returned_sink,
          result->blocked_route);
}
