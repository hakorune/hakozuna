#include <stdint.h>
#include <stdio.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_shadow.h"

int main(void) {
  H12InboxDeferred deferred;
  H12AdoptionShadow before_adopt;
  H12AdoptionShadow after_adopt;
  const uint32_t object_count = 8u;
  uint32_t i;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_inbox_deferred_init(&deferred);
  for (i = 0u; i < object_count; ++i) {
    void* ptr = hz12_malloc(64u);
    if (!ptr) return 2;
    h12_shadow_on_alloc(ptr, 0u);
    h12_inbox_defer_free(&deferred, ptr);
  }
  h12_inbox_flush(&deferred);
  if (h12_inbox_adopt_retired_owner(0u) != 0u) return 3;
  h12_inbox_mark_owner_retired(0u);
  before_adopt = h12_inbox_adoption_shadow_scan();
  if (before_adopt.pending_objects != object_count) return 4;
  if (h12_inbox_adopt_retired_owner(0u) != object_count) return 5;
  after_adopt = h12_inbox_adoption_shadow_scan();
  if (after_adopt.pending_owners != 0u || after_adopt.pending_objects != 0u) {
    return 6;
  }
  if (h12_inbox_adopt_retired_owner(0u) != 0u) return 7;
  printf("[HZ12_RETIRED_ADOPTION_SMOKE] pending_before=%llu adopted=%u "
         "pending_after=%llu\n",
         (unsigned long long)before_adopt.pending_objects, object_count,
         (unsigned long long)after_adopt.pending_objects);
  h12_inbox_dump(stdout);
  return 0;
}
