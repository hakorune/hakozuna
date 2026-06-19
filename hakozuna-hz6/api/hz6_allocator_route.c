#include "hz6_allocator.h"
#include "hz6_allocator_route_domain.h"
#include "hz6_allocator_route_last_hit.h"
#include "hz6_allocator_route_owner_locality.h"
#include "hz6_allocator_route_shared_directory.h"

#include <stdatomic.h>
#include <stdint.h>

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_note_route_probe_hist(size_t* hist, size_t probes);
#endif

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


#if HZ6_DIAGNOSTIC_PROBES
static size_t hz6_allocator_route_probe_bucket(size_t probes) {
  if (probes <= 1) {
    return 0;
  }
  if (probes <= 4) {
    return 1;
  }
  if (probes <= 8) {
    return 2;
  }
  if (probes <= 16) {
    return 3;
  }
  if (probes <= 64) {
    return 4;
  }
  return 5;
}

static void hz6_allocator_note_route_probe_hist(size_t* hist,
                                                size_t probes) {
  if (!hist) {
    return;
  }
  ++hist[hz6_allocator_route_probe_bucket(probes)];
}

static void hz6_allocator_note_route_tombstones(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  allocator->stats.route_tombstone_current = table->tombstone_count;
  if (table->tombstone_count > allocator->stats.route_tombstone_max) {
    allocator->stats.route_tombstone_max = table->tombstone_count;
  }
  allocator->stats.route_register_used_tombstone =
      table->register_used_tombstone;
  allocator->stats.route_register_full_probe_with_tombstone =
      table->register_full_probe_with_tombstone;
}
#endif

#if HZ6_ROUTE_TOMBSTONE_COMPACT_L1 && \
    !HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
static size_t hz6_allocator_route_tombstone_compact_threshold(
    const Hz6RouteTable* table) {
  if (!table || table->capacity == 0) {
    return (size_t)-1;
  }
  size_t threshold = HZ6_ROUTE_TOMBSTONE_COMPACT_MIN;
#if !HZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1
  size_t half = table->capacity / 2;
  if (threshold < half) {
    threshold = half;
  }
#endif
  if (threshold >= table->capacity) {
    threshold = table->capacity - 1;
  }
  return threshold == 0 ? 1 : threshold;
}
#endif

#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1 || \
    HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
static int hz6_allocator_route_conditional_tombstone_ready(
    Hz6Allocator* allocator,
    int note_stats) {
  if (!allocator) {
    return 0;
  }
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  if (!table || table->capacity == 0) {
    return 0;
  }
  size_t tombstones = table->tombstone_count;
  size_t abs_min = HZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN;
  if (abs_min == 0) {
    abs_min = 1;
  }
  if (abs_min >= table->capacity) {
    abs_min = table->capacity - 1;
  }
  if (tombstones < abs_min) {
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    ++allocator->stats.route_tombstone_cond_probe;
  }
#else
  (void)note_stats;
#endif
  size_t active = table->active_count;
  int ratio25 = active != 0 && tombstones * 4u >= active;
  int occupancy75 =
      (active + tombstones) * 4u >= table->capacity * 3u;
#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    if (ratio25) {
      ++allocator->stats.route_tombstone_cond_ratio25;
    }
    if (occupancy75) {
      ++allocator->stats.route_tombstone_cond_occupancy75;
    }
  }
#endif
  if (!ratio25 && !occupancy75) {
    return 0;
  }

  size_t cooldown = HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN;
  size_t highwater = allocator->stats.route_tombstone_cond_highwater;
  if (cooldown != 0 && highwater != 0 &&
      tombstones < highwater + cooldown) {
#if HZ6_DIAGNOSTIC_PROBES
    if (note_stats) {
      ++allocator->stats.route_tombstone_cond_cooldown_blocked;
    }
#endif
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    ++allocator->stats.route_tombstone_cond_would_compact;
  }
#endif
  allocator->stats.route_tombstone_cond_highwater = tombstones;
  return 1;
}
#endif

static void hz6_allocator_route_note_conditional_tombstone_dryrun(
    Hz6Allocator* allocator) {
#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES
  (void)hz6_allocator_route_conditional_tombstone_ready(allocator, 1);
#else
  (void)allocator;
#endif
}

static void hz6_allocator_route_maybe_compact_tombstones(
    Hz6Allocator* allocator) {
#if HZ6_ROUTE_TOMBSTONE_COMPACT_L1
  if (!allocator) {
    return;
  }
#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
  if (!hz6_allocator_route_conditional_tombstone_ready(allocator, 1)) {
    return;
  }
#else
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  size_t threshold =
      hz6_allocator_route_tombstone_compact_threshold(table);
  if (table->tombstone_count < threshold) {
    return;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_tombstone_compact_attempt;
#endif
  size_t moved = 0;
  if (hz6_route_backend_compact_tombstones(&allocator->route_backend,
                                           &moved)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.route_tombstone_compact_success;
    allocator->stats.route_tombstone_compact_moved += moved;
#endif
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.route_tombstone_compact_fail_alloc;
#endif
  }
#else
  (void)allocator;
#endif
}

static void hz6_allocator_note_route_register_reason(
    Hz6Allocator* allocator,
    Hz6RouteRegisterReason reason) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  switch (reason) {
    case HZ6_ROUTE_REGISTER_REASON_SOURCE_RUN_SLOT:
      ++allocator->stats.route_register_reason_source_run_slot;
      break;
    case HZ6_ROUTE_REGISTER_REASON_DIRECT_SOURCE:
      ++allocator->stats.route_register_reason_direct_source;
      break;
    case HZ6_ROUTE_REGISTER_REASON_MATERIALIZE:
      ++allocator->stats.route_register_reason_materialize;
      break;
    case HZ6_ROUTE_REGISTER_REASON_REHOME:
      ++allocator->stats.route_register_reason_rehome;
      break;
    case HZ6_ROUTE_REGISTER_REASON_UNKNOWN:
    default:
      ++allocator->stats.route_register_reason_unknown;
      break;
  }
#else
  (void)allocator;
  (void)reason;
#endif
}

static void hz6_allocator_note_route_unregister_reason(
    Hz6Allocator* allocator,
    Hz6RouteUnregisterReason reason) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  switch (reason) {
    case HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW:
      ++allocator->stats.route_unregister_reason_frontcache_overflow;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_CAP_RELEASE:
      ++allocator->stats.route_unregister_reason_cap_release;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_DESCRIPTORLESS_DETACH:
      ++allocator->stats.route_unregister_reason_descriptorless_detach;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE:
      ++allocator->stats.route_unregister_reason_source_slot_release;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_REHOME:
      ++allocator->stats.route_unregister_reason_rehome;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_UNKNOWN:
    default:
      ++allocator->stats.route_unregister_reason_unknown;
      break;
  }
#else
  (void)allocator;
  (void)reason;
#endif
}

void hz6_allocator_route_unregister_exact_reason(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteUnregisterReason reason) {
  if (!allocator || !ptr) {
    return;
  }
  hz6_allocator_note_route_unregister_reason(allocator, reason);
  hz6_allocator_route_domain_lock(allocator);
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
  hz6_allocator_route_last_hit_clear(allocator);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  hz6_shared_route_directory_unregister(allocator, ptr);
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  hz6_owner_locality_index_unregister(allocator, ptr);
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, &probes);
  hz6_allocator_route_note_conditional_tombstone_dryrun(allocator);
  if (HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1 &&
      reason == HZ6_ROUTE_UNREGISTER_REASON_REHOME) {
    hz6_allocator_route_domain_note_compact_debt(allocator);
  } else {
    hz6_allocator_route_maybe_compact_tombstones(allocator);
  }
  allocator->stats.route_unregister_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_unregister_probe_hist, probes);
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
  hz6_allocator_note_route_tombstones(allocator);
#else
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, NULL);
  hz6_allocator_route_note_conditional_tombstone_dryrun(allocator);
  if (HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1 &&
      reason == HZ6_ROUTE_UNREGISTER_REASON_REHOME) {
    hz6_allocator_route_domain_note_compact_debt(allocator);
  } else {
    hz6_allocator_route_maybe_compact_tombstones(allocator);
  }
#endif
  hz6_allocator_route_domain_unlock(allocator);
}

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr) {
  hz6_allocator_route_unregister_exact_reason(
      allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_UNKNOWN);
}

int hz6_allocator_route_register_exact_reason(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t generation,
    void* descriptor,
    Hz6RouteRegisterReason reason) {
  if (!allocator || !base || bytes == 0) {
    return 0;
  }
  int ok = 0;
  hz6_allocator_note_route_register_reason(allocator, reason);
  hz6_allocator_route_domain_lock(allocator);
  int compact_debt =
      hz6_allocator_route_domain_consume_compact_debt(allocator);
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                        base,
                                        bytes,
                                        front_id,
                                        class_id,
                                        generation,
                                        descriptor,
                                        &probes);
  allocator->stats.route_register_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_register_probe_hist, probes);
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
  hz6_allocator_note_route_tombstones(allocator);
#else
  ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                        base,
                                        bytes,
                                        front_id,
                                        class_id,
                                        generation,
                                        descriptor,
                                        NULL);
#endif
  if (compact_debt) {
    hz6_allocator_route_maybe_compact_tombstones(allocator);
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  int mandatory_publish_failed = 0;
  if (ok) {
    int shared_ok = hz6_shared_route_directory_register(allocator,
                                                        base,
                                                        front_id,
                                                        class_id,
                                                        generation,
                                                        descriptor);
    if (HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1 && !shared_ok) {
      hz6_route_backend_unregister_exact(&allocator->route_backend, base,
                                         NULL);
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
      hz6_allocator_route_last_hit_clear(allocator);
#endif
      ok = 0;
      mandatory_publish_failed = 1;
    }
  }
#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (!ok && !mandatory_publish_failed) {
    ok = hz6_shared_route_directory_register(allocator,
                                             base,
                                             front_id,
                                             class_id,
                                             generation,
                                             descriptor);
#if HZ6_DIAGNOSTIC_PROBES
    if (ok) {
      ++allocator->stats.elastic_route_overflow_register;
    } else {
      ++allocator->stats.elastic_route_overflow_register_fail;
    }
#endif
  }
#else
  (void)mandatory_publish_failed;
#endif
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (ok) {
    hz6_owner_locality_index_register(allocator, base, generation);
  }
#endif
  if (ok) {
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
    hz6_allocator_route_last_hit_fill(allocator, base, front_id, class_id,
                                      generation, descriptor);
#endif
  }
  hz6_allocator_route_domain_unlock(allocator);
#if HZ6_DIAGNOSTIC_PROBES
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
#endif
  return ok;
}

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  return hz6_allocator_route_register_exact_reason(
      allocator, base, bytes, front_id, class_id, generation, descriptor,
      HZ6_ROUTE_REGISTER_REASON_UNKNOWN);
}

int hz6_allocator_route_replace_exact_descriptor(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t old_generation,
    void* old_descriptor,
    uint32_t new_generation,
    void* new_descriptor) {
  if (!allocator || !base || bytes == 0 || !old_descriptor ||
      !new_descriptor) {
    return 0;
  }
  int ok = 0;
  hz6_allocator_route_domain_lock(allocator);
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ok = hz6_route_backend_replace_exact_descriptor(
      &allocator->route_backend, base, bytes, front_id, class_id,
      old_generation, old_descriptor, new_generation, new_descriptor,
      &probes);
  allocator->stats.route_register_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_register_probe_hist, probes);
  if (probes > allocator->stats.route_register_probe_max) {
    allocator->stats.route_register_probe_max = probes;
  }
#else
  ok = hz6_route_backend_replace_exact_descriptor(
      &allocator->route_backend, base, bytes, front_id, class_id,
      old_generation, old_descriptor, new_generation, new_descriptor, NULL);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (!ok) {
    Hz6RouteResult shared_route = hz6_shared_route_directory_lookup_raw(base);
    if (shared_route.kind == HZ6_ROUTE_VALID &&
        shared_route.descriptor == old_descriptor &&
        shared_route.generation == old_generation &&
        shared_route.front_id == front_id &&
        shared_route.class_id == class_id) {
      ok = hz6_shared_route_directory_register(allocator,
                                               base,
                                               front_id,
                                               class_id,
                                               new_generation,
                                               new_descriptor);
    }
  } else {
    int shared_ok = hz6_shared_route_directory_register(allocator,
                                                        base,
                                                        front_id,
                                                        class_id,
                                                        new_generation,
                                                        new_descriptor);
    if (HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1 && !shared_ok) {
      (void)hz6_route_backend_replace_exact_descriptor(
          &allocator->route_backend, base, bytes, front_id, class_id,
          new_generation, new_descriptor, old_generation, old_descriptor,
          NULL);
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
      hz6_allocator_route_last_hit_clear(allocator);
#endif
      ok = 0;
    }
  }
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (ok) {
    hz6_owner_locality_index_register(allocator, base, new_generation);
  }
#endif
  if (ok) {
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
    hz6_allocator_route_last_hit_fill(allocator, base, front_id, class_id,
                                      new_generation, new_descriptor);
#endif
  }
  hz6_allocator_route_domain_unlock(allocator);
#if HZ6_DIAGNOSTIC_PROBES
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
#endif
  return ok;
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
