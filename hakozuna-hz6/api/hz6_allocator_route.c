#include "hz6_allocator.h"
#include "hz6_allocator_route_domain.h"
#include "hz6_allocator_route_last_hit.h"
#include "hz6_allocator_route_probe_hist.h"
#include "hz6_allocator_route_shared_directory.h"

#include <stdint.h>

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
  hz6_allocator_route_domain_read_lock(allocator);
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
  Hz6RouteResult cached_route =
      hz6_allocator_route_last_hit_lookup(allocator, ptr);
  if (cached_route.kind == HZ6_ROUTE_VALID) {
    hz6_allocator_route_domain_read_unlock(allocator);
    return cached_route;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  size_t page_exact_hash_probes = 0;
  size_t page_exact_range_probes = 0;
  size_t page_exact_page_seed_probes = 0;
  size_t page_invalid_probes = 0;
  if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    ++((Hz6Allocator*)allocator)->stats.route_lookup_page_backend;
  } else {
    ++((Hz6Allocator*)allocator)->stats.route_lookup_exact_backend;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup_probe_ex(&allocator->route_backend,
                                        ptr,
                                        &lookup_probes,
                                        &page_exact_hash_probes,
                                        &page_exact_range_probes,
                                        &page_exact_page_seed_probes,
                                        &page_invalid_probes);
  ((Hz6Allocator*)allocator)->stats.route_lookup_probe_total += lookup_probes;
  hz6_allocator_note_route_probe_hist(
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_hist,
      lookup_probes);
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max = lookup_probes;
  }
  if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_total +=
        lookup_probes;
    if (lookup_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_max =
          lookup_probes;
    }
    size_t page_exact_probes =
        page_exact_hash_probes + page_exact_range_probes;
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_total +=
        page_exact_probes;
    if (page_exact_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_max =
          page_exact_probes;
    }
    ((Hz6Allocator*)allocator)->stats
        .route_lookup_page_exact_hash_probe_total += page_exact_hash_probes;
    if (page_exact_hash_probes >
        ((Hz6Allocator*)allocator)
            ->stats.route_lookup_page_exact_hash_probe_max) {
      ((Hz6Allocator*)allocator)
          ->stats.route_lookup_page_exact_hash_probe_max =
          page_exact_hash_probes;
    }
    ((Hz6Allocator*)allocator)->stats
        .route_lookup_page_exact_range_probe_total += page_exact_range_probes;
    if (page_exact_range_probes >
        ((Hz6Allocator*)allocator)
            ->stats.route_lookup_page_exact_range_probe_max) {
      ((Hz6Allocator*)allocator)
          ->stats.route_lookup_page_exact_range_probe_max =
          page_exact_range_probes;
    }
    ((Hz6Allocator*)allocator)->stats
        .route_lookup_page_exact_page_seed_probe_total +=
        page_exact_page_seed_probes;
    if (page_exact_page_seed_probes >
        ((Hz6Allocator*)allocator)
            ->stats.route_lookup_page_exact_page_seed_probe_max) {
      ((Hz6Allocator*)allocator)
          ->stats.route_lookup_page_exact_page_seed_probe_max =
          page_exact_page_seed_probes;
    }
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_total +=
        page_invalid_probes;
    if (page_invalid_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_max =
          page_invalid_probes;
    }
  }
#else
  Hz6RouteResult route = hz6_route_backend_lookup(&allocator->route_backend, ptr);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (route.kind != HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_lookup;
#endif
    Hz6RouteResult overflow_route =
        hz6_shared_route_directory_lookup_raw(ptr);
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_lookup;
#endif
    if (overflow_route.kind == HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_hit;
      ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_hit;
#endif
      hz6_allocator_route_domain_read_unlock(allocator);
      return overflow_route;
    }
    if (route.kind == HZ6_ROUTE_MISS) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_range_lookup;
#endif
      Hz6RouteResult overflow_range =
          hz6_shared_route_range_lookup_raw(ptr);
      if (overflow_range.kind == HZ6_ROUTE_INVALID) {
#if HZ6_DIAGNOSTIC_PROBES
        ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_range_hit;
#endif
        hz6_allocator_route_domain_read_unlock(allocator);
        return overflow_range;
      }
    }
  }
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_ROUTE_LAST_HIT_CACHE_L1 && !HZ6_ROUTE_DOMAIN_RWLOCK_L1
    hz6_allocator_route_last_hit_fill_route((Hz6Allocator*)allocator, ptr,
                                            route);
#endif
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
      if (route.kind == HZ6_ROUTE_VALID) {
        ++((Hz6Allocator*)allocator)->stats.route_lookup_page_valid;
      } else if (route.kind == HZ6_ROUTE_INVALID) {
        ++((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid;
      }
    }
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
      ++((Hz6Allocator*)allocator)->stats.route_lookup_page_miss;
    }
#endif
  }
  hz6_allocator_route_domain_read_unlock(allocator);
  return route;
}

Hz6RouteResult hz6_allocator_route_lookup_exact(const Hz6Allocator* allocator,
                                                const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
  hz6_allocator_route_domain_read_lock(allocator);
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
  Hz6RouteResult cached_route =
      hz6_allocator_route_last_hit_lookup(allocator, ptr);
  if (cached_route.kind == HZ6_ROUTE_VALID) {
    hz6_allocator_route_domain_read_unlock(allocator);
    return cached_route;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  Hz6RouteResult route = hz6_route_backend_lookup_exact_probe(
      &allocator->route_backend, ptr, &lookup_probes);
  ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_total +=
      lookup_probes;
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_max =
        lookup_probes;
  }
#else
  Hz6RouteResult route =
      hz6_route_backend_lookup_exact(&allocator->route_backend, ptr);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (route.kind == HZ6_ROUTE_MISS) {
    Hz6RouteResult overflow_route =
        hz6_shared_route_directory_lookup_raw(ptr);
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_lookup;
#endif
    if (overflow_route.kind == HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_hit;
#endif
      hz6_allocator_route_domain_read_unlock(allocator);
      return overflow_route;
    }
  }
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_ROUTE_LAST_HIT_CACHE_L1 && !HZ6_ROUTE_DOMAIN_RWLOCK_L1
    hz6_allocator_route_last_hit_fill_route((Hz6Allocator*)allocator, ptr,
                                            route);
#endif
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE &&
        route.kind == HZ6_ROUTE_MISS) {
      ++((Hz6Allocator*)allocator)->stats.route_lookup_page_miss;
    }
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  }
  hz6_allocator_route_domain_read_unlock(allocator);
  return route;
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
