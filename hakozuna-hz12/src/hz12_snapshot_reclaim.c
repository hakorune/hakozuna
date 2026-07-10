#include "hz12_snapshot_reclaim.h"

#include "hz12_flush_owner_route.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_span.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static void h12_snapshot_restore_span(uint8_t class_id, void* span_base) {
  void* batch[256];
  const size_t slot_bytes = hz12_class_slot_size(class_id);
  const uint32_t slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  uint32_t slot = 0u;
  while (slot < slot_count) {
    uint32_t count = 0u;
    while (count < 256u && slot < slot_count) {
      batch[count++] = (char*)span_base + (size_t)slot++ * slot_bytes;
    }
    hz12_returned_push_range(class_id, batch, count);
  }
}

int h12_snapshot_reclaim_retired_bounded(H12OwnerToken owner,
                                         uint32_t budget,
                                         H12SnapshotReclaimResult* out) {
  uint32_t pending = 0u;
  uint32_t span_id;
  if (!out || budget == 0u || budget > 64u) return 0;
  memset(out, 0, sizeof(*out));
  out->budget = budget;
  if (!h12_owner_retire_gate_ready(owner) ||
      !hz12_flush_owner_route_pending(owner.slot, owner.generation, &pending) ||
      pending != 0u) {
    return 0;
  }
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT && out->detached < budget;
       ++span_id) {
    H12OwnerToken observed;
    H12ReturnedSpanSnapshot snapshot;
    void* span_base;
    uint8_t class_plus_one;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != owner.slot ||
        observed.generation != owner.generation) {
      continue;
    }
    class_plus_one = hz12_span_class[span_id];
    if (class_plus_one == 0u) continue;
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!hz12_returned_snapshot_span(
            (uint8_t)(class_plus_one - 1u), span_base, &snapshot) ||
        !snapshot.complete) {
      continue;
    }
    out->candidates += 1u;
    if (!hz12_returned_detach_complete_span(
            (uint8_t)(class_plus_one - 1u), span_base, &snapshot)) {
      out->failed += 1u;
      break;
    }
    if (!hz12_span_route_detach(span_base,
                               (uint8_t)(class_plus_one - 1u))) {
      h12_snapshot_restore_span((uint8_t)(class_plus_one - 1u), span_base);
      out->failed += 1u;
      break;
    }
    out->detached += 1u;
#if defined(_WIN32)
    if (!VirtualFree(span_base, HZ12_SPAN_BYTES, MEM_DECOMMIT)) {
      (void)hz12_span_route_attach(span_base,
                                  (uint8_t)(class_plus_one - 1u));
      h12_snapshot_restore_span((uint8_t)(class_plus_one - 1u), span_base);
      out->failed += 1u;
      break;
    }
    out->decommitted += 1u;
    out->decommitted_bytes += HZ12_SPAN_BYTES;
#endif
  }
  return out->failed == 0u && out->decommitted != 0u;
}
