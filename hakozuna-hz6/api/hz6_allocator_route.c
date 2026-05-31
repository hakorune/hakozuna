#include "hz6_allocator.h"

#include <stdatomic.h>

static _Atomic(Hz6Allocator*) g_hz6_visible_allocators
    [HZ6_ALLOCATOR_VISIBILITY_CAPACITY];

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
    (void)atomic_compare_exchange_strong_explicit(
        &g_hz6_visible_allocators[i],
        &expected,
        NULL,
        memory_order_release,
        memory_order_relaxed);
  }
}

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  Hz6RouteResult route =
      hz6_route_backend_lookup_probe(&allocator->route_backend,
                                     ptr,
                                     &lookup_probes);
  ((Hz6Allocator*)allocator)->stats.route_lookup_probe_total += lookup_probes;
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max = lookup_probes;
  }
#else
  Hz6RouteResult route = hz6_route_backend_lookup(&allocator->route_backend, ptr);
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_DIAGNOSTIC_PROBES
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (descriptor->source_release) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  }
  return route;
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
    route = hz6_route_backend_lookup(&visible->route_backend, ptr);
    if (route.kind != HZ6_ROUTE_MISS) {
      route.route_allocator = visible;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.route_visibility_hit;
      if (route.descriptor) {
        const Hz6ObjectDescriptor* descriptor =
            (const Hz6ObjectDescriptor*)route.descriptor;
        if (hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
          ++allocator->stats.route_visibility_hit_local_owner;
          if (descriptor->source_release) {
            ++allocator->stats.source_owned_visibility_hit_local_owner;
          }
        } else {
          ++allocator->stats.route_visibility_hit_foreign_owner;
          if (descriptor->source_release) {
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
  hz6_allocator_route_unregister_exact(origin, ptr);
  if (!hz6_allocator_route_register_exact(allocator,
                                          ptr,
                                          bytes,
                                          route->front_id,
                                          route->class_id,
                                          route->generation,
                                          (void*)descriptor)) {
    hz6_allocator_route_register_exact(origin,
                                      ptr,
                                      bytes,
                                      route->front_id,
                                      route->class_id,
                                      route->generation,
                                      (void*)descriptor);
    return 0;
  }

  return 1;
}

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr) {
  if (!allocator || !ptr) {
    return;
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, &probes);
  allocator->stats.route_unregister_probe_total += probes;
  if (probes > allocator->stats.route_unregister_probe_max) {
    allocator->stats.route_unregister_probe_max = probes;
  }
  allocator->stats.route_active_current =
      allocator->route_backend.exact_table.active_count;
  if (allocator->stats.route_active_current >
      allocator->stats.route_active_max) {
    allocator->stats.route_active_max =
        allocator->stats.route_active_current;
  }
#else
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, NULL);
#endif
}

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  if (!allocator || !base || bytes == 0) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  int ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                            base,
                                            bytes,
                                            front_id,
                                            class_id,
                                            generation,
                                            descriptor,
                                            &probes);
  allocator->stats.route_register_probe_total += probes;
  if (probes > allocator->stats.route_register_probe_max) {
    allocator->stats.route_register_probe_max = probes;
  }
  allocator->stats.route_active_current =
      allocator->route_backend.exact_table.active_count;
  if (allocator->stats.route_active_current >
      allocator->stats.route_active_max) {
    allocator->stats.route_active_max =
        allocator->stats.route_active_current;
  }
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
  return ok;
#else
  return hz6_route_backend_register_exact(&allocator->route_backend,
                                          base,
                                          bytes,
                                          front_id,
                                          class_id,
                                          generation,
                                          descriptor,
                                          NULL);
#endif
}

Hz6RouteBackendKind hz6_allocator_route_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_ROUTE_BACKEND_EXACT_TABLE;
  }
  return allocator->route_backend.kind;
}

size_t hz6_allocator_route_page_granularity(const Hz6Allocator* allocator) {
  return allocator ? allocator->route_backend.page_granularity : 0;
}
