#include "hz12_reclaim_policy_shadow.h"

#include "hz12_flush_owner_route.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_span.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

int h12_reclaim_policy_shadow_query(H12OwnerToken owner,
                                    H12ReclaimPolicyShadow* out) {
  uint32_t span_id;
  if (!out || owner.generation == 0u) return 0;
  memset(out, 0, sizeof(*out));
  out->owner_gate_open = (uint8_t)h12_owner_retire_gate_ready(owner);
  out->flush_owner_found = (uint8_t)hz12_flush_owner_route_pending(
      owner.slot, owner.generation, &out->flush_owner_pending);
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT; ++span_id) {
    H12OwnerToken observed;
    H12ReturnedSpanSnapshot snapshot;
    void* span_base;
    uint8_t class_plus_one;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != owner.slot ||
        observed.generation != owner.generation) {
      continue;
    }
    out->owner_spans += 1u;
    class_plus_one = hz12_span_class[span_id];
    if (class_plus_one == 0u) {
      out->incomplete_spans += 1u;
      continue;
    }
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (hz12_returned_snapshot_span((uint8_t)(class_plus_one - 1u),
                                    span_base, &snapshot) &&
        snapshot.complete) {
      out->complete_spans += 1u;
    } else {
      out->incomplete_spans += 1u;
    }
  }
  out->reclaimable_bytes = (uint64_t)out->complete_spans * HZ12_SPAN_BYTES;
  out->planned_spans = out->complete_spans < HZ12_RECLAIM_POLICY_MAX_SPANS
      ? out->complete_spans : HZ12_RECLAIM_POLICY_MAX_SPANS;
  out->planned_bytes = (uint64_t)out->planned_spans * HZ12_SPAN_BYTES;
  out->threshold_met = (uint8_t)(
      out->reclaimable_bytes >= HZ12_RECLAIM_POLICY_MIN_BYTES);
  out->would_reclaim = (uint8_t)(out->owner_gate_open &&
      out->flush_owner_found && out->flush_owner_pending == 0u &&
      out->threshold_met && out->planned_spans != 0u);
  return 1;
}
