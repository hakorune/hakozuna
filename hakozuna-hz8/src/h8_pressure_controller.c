#include "h8_internal.h"

static size_t h8_pressure_collect_budget(H8OwnerRecord* owner) {
  size_t pending =
      atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
  if (pending == 0) {
    return 0;
  }
  if (pending <= 2) {
    return 1;
  }
  if (pending <= 8) {
    return 2;
  }
  if (pending <= 32) {
    return 4;
  }
  return 8;
}

void h8_pressure_owner_collect(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  size_t budget = h8_pressure_collect_budget(owner);
  if (budget == 0) {
    return;
  }
  h8_collect_owner_pending_budget(owner, budget);
}

H8Span* h8_pressure_refill(H8OwnerRecord* owner, uint32_t class_id) {
  return h8_span_commit_for_class(owner ? owner : h8_owner_current(), class_id);
}
