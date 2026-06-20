#include "h8_internal.h"

void h8_pressure_owner_collect(H8OwnerRecord* owner) {
  h8_collect_owner_pending(owner);
}

H8Span* h8_pressure_refill(H8OwnerRecord* owner, uint32_t class_id) {
  return h8_span_commit_for_class(owner ? owner : h8_owner_current(), class_id);
}
