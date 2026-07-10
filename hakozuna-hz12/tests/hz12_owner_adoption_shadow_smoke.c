#include <stdint.h>
#include <stdio.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_shadow.h"

int main(void) {
  H12InboxDeferred deferred;
  H12AdoptionShadow before_drain;
  H12AdoptionShadow after_drain;
  uint32_t i;
  const uint32_t object_count = 8u;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_inbox_deferred_init(&deferred);
  for (i = 0u; i < object_count; ++i) {
    void* ptr = hz12_malloc(64u);
    if (!ptr) return 2;
    h12_shadow_on_alloc(ptr, 0u);
    h12_inbox_defer_free(&deferred, ptr);
  }
  h12_inbox_flush(&deferred);
  h12_inbox_mark_owner_retired(0u);
  before_drain = h12_inbox_adoption_shadow_scan();
  if (before_drain.retired_owners != 1u ||
      before_drain.pending_owners != 1u ||
      before_drain.pending_objects != object_count) {
    return 3;
  }
  if (h12_inbox_drain_owner(0u) != object_count) return 4;
  after_drain = h12_inbox_adoption_shadow_scan();
  if (after_drain.retired_owners != 1u ||
      after_drain.pending_owners != 0u ||
      after_drain.pending_objects != 0u) {
    return 5;
  }
  printf("[HZ12_ADOPTION_SHADOW_SMOKE] pending_before=%llu pending_after=%llu\n",
         (unsigned long long)before_drain.pending_objects,
         (unsigned long long)after_drain.pending_objects);
  return 0;
}
