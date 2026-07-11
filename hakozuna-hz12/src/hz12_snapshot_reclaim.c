#include "hz12_snapshot_reclaim.h"

#include "hz12_flush_owner_route.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_reclaim_entry.h"
#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_span_backing.h"
#include "hz12_span_depot_core.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

typedef struct H12SnapshotCandidate {
  void* span_base;
  uint8_t class_id;
  uint8_t detached;
} H12SnapshotCandidate;

static int h12_snapshot_select_batch(uint8_t class_id,
                                     const void* const* spans,
                                     uint32_t count,
                                     H12SnapshotCandidate* selected,
                                     uint32_t* selected_count,
                                     uint32_t limit) {
  H12ReturnedSpanSnapshot snapshots[HZ12_RETURNED_BATCH_MAX];
  uint32_t i;
  if (!hz12_returned_snapshot_batch(class_id, spans, count, snapshots)) {
    return 0;
  }
  for (i = 0u; i < count && *selected_count < limit; ++i) {
    if (snapshots[i].complete) {
      selected[*selected_count].span_base = (void*)spans[i];
      selected[*selected_count].class_id = class_id;
      selected[*selected_count].detached = 0u;
      *selected_count += 1u;
    }
  }
  return 1;
}

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

static int h12_snapshot_rollback_decommitted(void* span_base,
                                             uint8_t class_id,
                                             H12OwnerToken owner) {
  H12ReturnedSpanSnapshot snapshot;
  if (!h12_span_backing_recommit(span_base, NULL, NULL) ||
      !hz12_span_route_attach(span_base, class_id) ||
      !h12_shadow_rehome_token(span_base, owner.slot, owner.generation) ||
      !h12_span_owner_shadow_assign(span_base, owner)) {
    return 0;
  }
  h12_snapshot_restore_span(class_id, span_base);
  return hz12_returned_snapshot_span(class_id, span_base, &snapshot) &&
         snapshot.complete;
}

int h12_snapshot_reclaim_retired_bounded(H12OwnerToken owner,
                                         uint32_t budget,
                                         H12SnapshotReclaimResult* out) {
  H12SnapshotCandidate candidates[HZ12_RETURNED_BATCH_MAX];
  const void* pending_spans[HZ12_CLASS_COUNT][HZ12_RETURNED_BATCH_MAX];
  uint32_t pending_counts[HZ12_CLASS_COUNT];
  const void* class_spans[HZ12_RETURNED_BATCH_MAX];
  uint32_t class_indices[HZ12_RETURNED_BATCH_MAX];
  H12ReturnedSpanSnapshot snapshots[HZ12_RETURNED_BATCH_MAX];
  uint32_t candidate_count = 0u;
  uint32_t class_id;
  uint32_t pending = 0u;
  uint32_t span_id;
  int entered = 0;
  if (!out || budget == 0u || budget > 64u) return 0;
  memset(out, 0, sizeof(*out));
  memset(pending_counts, 0, sizeof(pending_counts));
  out->budget = budget;
  if (!h12_owner_retire_gate_ready(owner) ||
      !hz12_flush_owner_route_pending(owner.slot, owner.generation, &pending) ||
      pending != 0u) {
    return 0;
  }
  if (!h12_reclaim_entry_begin(owner)) return 0;
  entered = 1;
  out->reserved = h12_span_depot_core_reserve(budget);
  if (out->reserved == 0u) goto done;
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT &&
       candidate_count < out->reserved; ++span_id) {
    H12OwnerToken observed;
    uint8_t class_plus_one;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != owner.slot ||
        observed.generation != owner.generation) {
      continue;
    }
    class_plus_one = hz12_span_class[span_id];
    if (class_plus_one == 0u) continue;
    class_id = (uint32_t)(class_plus_one - 1u);
    pending_spans[class_id][pending_counts[class_id]++] =
        hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (pending_counts[class_id] == HZ12_RETURNED_BATCH_MAX) {
      if (!h12_snapshot_select_batch((uint8_t)class_id,
              pending_spans[class_id], pending_counts[class_id], candidates,
              &candidate_count, out->reserved)) {
        out->failed += 1u;
        break;
      }
      pending_counts[class_id] = 0u;
    }
  }
  if (out->failed != 0u) goto restore_detached;
  for (class_id = 0u; class_id < HZ12_CLASS_COUNT &&
       candidate_count < out->reserved; ++class_id) {
    if (pending_counts[class_id] != 0u &&
        !h12_snapshot_select_batch((uint8_t)class_id,
            pending_spans[class_id], pending_counts[class_id], candidates,
            &candidate_count, out->reserved)) {
      out->failed += 1u;
      goto restore_detached;
    }
  }
  for (class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    uint32_t count = 0u;
    uint32_t i;
    for (i = 0u; i < candidate_count; ++i) {
      if (candidates[i].class_id == class_id) {
        class_spans[count] = candidates[i].span_base;
        class_indices[count++] = i;
      }
    }
    if (count == 0u) continue;
    if (!hz12_returned_detach_complete_batch((uint8_t)class_id,
                                              class_spans, count,
                                              snapshots)) {
      out->failed += 1u;
      break;
    }
    for (i = 0u; i < count; ++i) {
      if (snapshots[i].complete) {
        candidates[class_indices[i]].detached = 1u;
        out->candidates += 1u;
      }
    }
  }
  if (out->failed != 0u) goto restore_detached;
  for (span_id = 0u; span_id < candidate_count; ++span_id) {
    void* span_base = candidates[span_id].span_base;
    class_id = candidates[span_id].class_id;
    if (!candidates[span_id].detached) continue;
    if (!hz12_span_route_detach(span_base, (uint8_t)class_id)) {
      h12_snapshot_restore_span((uint8_t)class_id, span_base);
      candidates[span_id].detached = 0u;
      out->failed += 1u;
      break;
    }
    out->detached += 1u;
    if (!h12_span_backing_discard(span_base, NULL, NULL)) {
      (void)hz12_span_route_attach(span_base,
                                  (uint8_t)class_id);
      h12_snapshot_restore_span((uint8_t)class_id, span_base);
      candidates[span_id].detached = 0u;
      out->failed += 1u;
      break;
    }
    out->decommitted += 1u;
    out->decommitted_bytes += HZ12_SPAN_BYTES;
    if (!h12_shadow_clear_token_if(span_base, owner.slot, owner.generation) ||
        !h12_span_owner_shadow_clear_if(span_base, owner)) {
      if (!h12_snapshot_rollback_decommitted(
              span_base, (uint8_t)class_id, owner)) {
        out->limbo_spans += 1u;
      }
      candidates[span_id].detached = 0u;
      out->failed += 1u;
      break;
    }
    out->owner_cleared += 1u;
    if (!h12_span_depot_core_put_reserved(
            span_base, (uint8_t)class_id)) {
      if (!h12_snapshot_rollback_decommitted(
              span_base, (uint8_t)class_id, owner)) {
        out->limbo_spans += 1u;
      }
      candidates[span_id].detached = 0u;
      out->failed += 1u;
      break;
    }
    out->depot_inserted += 1u;
    candidates[span_id].detached = 0u;
  }
restore_detached:
  if (out->failed != 0u) {
    for (span_id = 0u; span_id < candidate_count; ++span_id) {
      if (candidates[span_id].detached) {
        h12_snapshot_restore_span(candidates[span_id].class_id,
                                  candidates[span_id].span_base);
        candidates[span_id].detached = 0u;
      }
    }
  }
done:
  if (out->reserved > out->depot_inserted) {
    h12_span_depot_core_release_reservation(
        out->reserved - out->depot_inserted);
  }
  if (entered && !h12_reclaim_entry_end(owner)) out->failed += 1u;
  return out->failed == 0u && out->limbo_spans == 0u &&
         out->depot_inserted != 0u;
}
