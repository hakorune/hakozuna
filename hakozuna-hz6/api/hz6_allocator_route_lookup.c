#include "hz6_allocator.h"
#include "hz6_allocator_route_domain.h"
#include "hz6_allocator_route_last_hit.h"
#include "hz6_allocator_route_owner_locality.h"
#include "hz6_allocator_route_shared_directory.h"

#include <stdatomic.h>
#include <stdint.h>

static _Atomic(Hz6Allocator*) g_hz6_visible_allocators
    [HZ6_ALLOCATOR_VISIBILITY_CAPACITY];
static _Atomic(size_t) g_hz6_visible_allocator_count;

void hz6_allocator_route_visibility_register(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (visible == allocator) {
      return;
    }
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(
            &g_hz6_visible_allocators[i],
            &expected,
            allocator,
            memory_order_release,
            memory_order_relaxed)) {
      atomic_fetch_add_explicit(&g_hz6_visible_allocator_count, 1u,
                                memory_order_acq_rel);
      return;
    }
  }
}

void hz6_allocator_route_visibility_unregister(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (visible != allocator) {
      continue;
    }
    Hz6Allocator* expected = allocator;
    if (atomic_compare_exchange_strong_explicit(&g_hz6_visible_allocators[i],
                                                &expected,
                                                NULL,
                                                memory_order_release,
                                                memory_order_relaxed)) {
      atomic_fetch_sub_explicit(&g_hz6_visible_allocator_count, 1u,
                                memory_order_acq_rel);
      return;
    }
  }
}

Hz6RouteResult hz6_allocator_route_lookup_visible(Hz6Allocator* allocator,
                                                  const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
  if (route.kind != HZ6_ROUTE_MISS) {
    return route;
  }

  return hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
}

Hz6RouteResult hz6_allocator_route_lookup_visible_only(Hz6Allocator* allocator,
                                                       const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }

  if (atomic_load_explicit(&g_hz6_visible_allocator_count,
                           memory_order_acquire) <= 1u) {
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_route_miss();
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ++allocator->stats.route_visibility_lookup;
#endif
  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (!visible || visible == allocator) {
      continue;
    }
    if (!hz6_owner_is_alive(&visible->owner, visible->owner.token)) {
      continue;
    }

#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    hz6_allocator_route_domain_read_lock(visible);
#if HZ6_ROUTE_VISIBLE_EXACT_ONLY_L1
    route = hz6_route_backend_lookup_exact(&visible->route_backend, ptr);
#else
    route = hz6_route_backend_lookup(&visible->route_backend, ptr);
#endif
    hz6_allocator_route_domain_read_unlock(visible);
    if (route.kind != HZ6_ROUTE_MISS) {
      route.route_allocator = visible;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.route_visibility_hit;
      if (route.descriptor) {
        const Hz6ObjectDescriptor* descriptor =
            (const Hz6ObjectDescriptor*)route.descriptor;
        if (hz6_allocator_descriptor_owner_equal_at(
                visible, descriptor, allocator->owner.token,
                HZ6_OWNER_EQUAL_SITE_VISIBLE_LOOKUP)) {
          ++allocator->stats.route_visibility_hit_local_owner;
          if (hz6_allocator_descriptor_has_source_release(visible,
                                                          descriptor)) {
            ++allocator->stats.source_owned_visibility_hit_local_owner;
          }
        } else {
          ++allocator->stats.route_visibility_hit_foreign_owner;
          if (hz6_allocator_descriptor_has_source_release(visible,
                                                          descriptor)) {
            ++allocator->stats.source_owned_visibility_hit_foreign_owner;
          }
        }
      }
      allocator->stats.route_visibility_probe_total += probes;
      if (probes > allocator->stats.route_visibility_probe_max) {
        allocator->stats.route_visibility_probe_max = probes;
      }
#endif
      return route;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_visibility_miss;
  allocator->stats.route_visibility_probe_total += probes;
  if (probes > allocator->stats.route_visibility_probe_max) {
    allocator->stats.route_visibility_probe_max = probes;
  }
#endif
  return hz6_route_miss();
}

Hz6RouteResult hz6_allocator_route_lookup_visible_after_local_miss(
    Hz6Allocator* allocator,
    const void* ptr) {
#if HZ6_ROUTE_VISIBLE_AFTER_LOCAL_MISS_L1
  return hz6_allocator_route_lookup_visible_only(allocator, ptr);
#else
  (void)allocator;
  (void)ptr;
  return hz6_route_miss();
#endif
}

#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
Hz6Allocator* hz6_allocator_descriptor_storage_owner(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count) {
  if (probe_count) {
    *probe_count = 0;
  }
  if (!observer || !descriptor) {
    return NULL;
  }

#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  if (hz6_allocator_descriptor_is_depot(descriptor)) {
    return hz6_allocator_owner_source_side_meta_storage(observer, descriptor);
  }
#endif

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (!visible) {
      continue;
    }
    if (probe_count) {
      ++*probe_count;
    }
    if (hz6_allocator_descriptor_belongs_to(visible, descriptor)) {
      return visible;
    }
  }
  return NULL;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES
Hz6Allocator* hz6_allocator_descriptor_storage_owner_diagnostic(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count) {
  return hz6_allocator_descriptor_storage_owner(observer, descriptor,
                                                probe_count);
}
#endif

int hz6_allocator_route_negative_filter_skip_local(
    Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  uintptr_t probe = (uintptr_t)ptr;
#if HZ6_DIAGNOSTIC_PROBES
  size_t range_probes = 0;
#endif
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
#if HZ6_DIAGNOSTIC_PROBES
    ++range_probes;
#endif
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!hz6_source_block_active(block) || !block->ptr ||
        block->bytes == 0) {
      continue;
    }
    uintptr_t base = (uintptr_t)block->ptr;
    uintptr_t limit = base + (uintptr_t)block->bytes;
    if (probe >= base && probe < limit) {
#if HZ6_DIAGNOSTIC_PROBES
      allocator->stats.negative_filter_range_probe_total += range_probes;
      if (range_probes >
          allocator->stats.negative_filter_range_probe_max) {
        allocator->stats.negative_filter_range_probe_max = range_probes;
      }
#endif
      return 0;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.negative_filter_range_probe_total += range_probes;
  if (range_probes > allocator->stats.negative_filter_range_probe_max) {
    allocator->stats.negative_filter_range_probe_max = range_probes;
  }
#endif
  return 1;
}

int hz6_allocator_route_rehome_exact(Hz6Allocator* allocator,
                                     const Hz6RouteResult* route) {
  if (!allocator || !route || route->kind != HZ6_ROUTE_VALID ||
      !route->descriptor || !route->route_allocator ||
      route->route_allocator == allocator) {
    return 0;
  }

  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)route->descriptor;
  Hz6Allocator* origin = route->route_allocator;
  void* ptr = descriptor->ptr;
  size_t bytes = descriptor->bytes;
#if HZ6_ROUTE_REHOME_REGISTER_BEFORE_UNREGISTER_L1
  if (!hz6_allocator_route_register_exact_reason(
          allocator, ptr, bytes, route->front_id, route->class_id,
          route->generation, (void*)descriptor,
          HZ6_ROUTE_REGISTER_REASON_REHOME)) {
    return 0;
  }
  hz6_allocator_route_unregister_exact_reason(
      origin, ptr, HZ6_ROUTE_UNREGISTER_REASON_REHOME);
  return 1;
#else
  hz6_allocator_route_unregister_exact_reason(
      origin, ptr, HZ6_ROUTE_UNREGISTER_REASON_REHOME);
  if (!hz6_allocator_route_register_exact_reason(
          allocator, ptr, bytes, route->front_id, route->class_id,
          route->generation, (void*)descriptor,
          HZ6_ROUTE_REGISTER_REASON_REHOME)) {
    hz6_allocator_route_register_exact_reason(
        origin, ptr, bytes, route->front_id, route->class_id,
        route->generation, (void*)descriptor,
        HZ6_ROUTE_REGISTER_REASON_REHOME);
    return 0;
  }

  return 1;
#endif
}
