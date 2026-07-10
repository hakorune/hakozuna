#include "hz12_snapshot_recycle.h"

#include "hz12_current_span_install.h"
#include "hz12_flush_owner_route.h"
#include "hz12_span.h"
#include "hz12_span_depot_core.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_shadow.h"
#include "hz12_thread_cache.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int h12_snapshot_recycle_take(uint8_t class_id,
                              H12SnapshotRecycleResult* out) {
  H12SpanDepotCoreEntry entry;
  H12ThreadCache* cache;
  H12OwnerToken owner = {0u, 0u};
  if (!out || class_id >= HZ12_CLASS_COUNT) return 0;
  memset(out, 0, sizeof(*out));
  if (!h12_span_depot_core_take(class_id, &entry)) return 0;
  out->span_base = entry.span_base;
  out->class_id = entry.class_id;
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION before;
    MEMORY_BASIC_INFORMATION after;
    if (VirtualQuery(entry.span_base, &before, sizeof(before)) !=
            sizeof(before) ||
        before.State != MEM_RESERVE) {
      goto rollback;
    }
    out->before_state = before.State;
    if (VirtualAlloc(entry.span_base, HZ12_SPAN_BYTES, MEM_COMMIT,
                     PAGE_READWRITE) != entry.span_base) {
      goto rollback;
    }
    out->recommitted = 1u;
    if (VirtualQuery(entry.span_base, &after, sizeof(after)) != sizeof(after) ||
        after.State != MEM_COMMIT) {
      goto rollback;
    }
    out->after_state = after.State;
  }
#else
  goto rollback;
#endif
  out->route_attached = (uint8_t)hz12_span_route_attach(entry.span_base,
                                                        entry.class_id);
  if (!out->route_attached) goto rollback;
  cache = hz12_thread_cache_get();
#if HZ12_FLUSH_OWNER_ROUTE
  hz12_flush_owner_route_attach(cache);
  if (cache && cache->flush_owner_valid) {
    owner.slot = cache->flush_owner_id;
    owner.generation = cache->flush_owner_generation;
  }
  out->owner_assigned = (uint8_t)(owner.generation != 0u &&
      h12_shadow_rehome_token(entry.span_base, owner.slot,
                              owner.generation) &&
      h12_span_owner_shadow_assign(entry.span_base, owner));
  if (!out->owner_assigned) goto rollback;
#else
  goto rollback;
#endif
  out->current_installed = (uint8_t)h12_current_span_install(
      cache, entry.class_id, entry.span_base);
  if (!out->current_installed) goto rollback;
  return 1;

rollback:
  out->rollback = 1u;
  if (out->owner_assigned) {
    (void)h12_shadow_clear_token_if(entry.span_base, owner.slot,
                                    owner.generation);
    (void)h12_span_owner_shadow_clear_if(entry.span_base, owner);
    out->owner_assigned = 0u;
  }
  if (out->route_attached) {
    (void)hz12_span_route_detach(entry.span_base, entry.class_id);
    out->route_attached = 0u;
  }
#if defined(_WIN32)
  if (out->recommitted) {
    (void)VirtualFree(entry.span_base, HZ12_SPAN_BYTES, MEM_DECOMMIT);
    out->recommitted = 0u;
  }
#endif
  (void)h12_span_depot_core_put_decommitted(entry.span_base, entry.class_id);
  return 0;
}
