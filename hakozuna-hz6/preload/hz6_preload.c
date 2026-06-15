#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_toy_small_diag.h"
#include "hz6_midpage_front.h"
#include "linux_source_mmap.h"
#include "hz6_preload_real.h"
#include "hz6_preload_stats.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>

#define HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY 256u

static pthread_mutex_t g_hz6_preload_allocator_registry_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static Hz6Allocator*
    g_hz6_preload_allocator_registry[HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY];
static size_t g_hz6_preload_allocator_registry_count;

Hz6PreloadPhaseStats g_hz6_preload_phase_stats;
static int g_hz6_preload_phase_stats_enabled;

void hz6_preload_phase_count(_Atomic(size_t)* counter) {
  if (g_hz6_preload_phase_stats_enabled) {
    (void)atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
  }
}

void hz6_preload_phase_add(_Atomic(size_t)* counter, size_t value) {
  if (g_hz6_preload_phase_stats_enabled && value != 0) {
    (void)atomic_fetch_add_explicit(counter, value, memory_order_relaxed);
  }
}

static size_t hz6_preload_phase_load(const _Atomic(size_t)* counter) {
  return atomic_load_explicit(counter, memory_order_relaxed);
}

#define HZ6_PRELOAD_PHASE_LOAD_FIELD(name) hz6_preload_phase_load(&g_hz6_preload_phase_stats.name)

void hz6_preload_phase_count_size_bucket(
    size_t size,
    _Atomic(size_t)* zero,
    _Atomic(size_t)* le1024,
    _Atomic(size_t)* range1025_4096,
    _Atomic(size_t)* range4097_16384,
    _Atomic(size_t)* gt16384) {
  if (size == 0) {
    hz6_preload_phase_count(zero);
  } else if (size <= 1024u) {
    hz6_preload_phase_count(le1024);
  } else if (size <= 4096u) {
    hz6_preload_phase_count(range1025_4096);
  } else if (size <= 16384u) {
    hz6_preload_phase_count(range4097_16384);
  } else {
    hz6_preload_phase_count(gt16384);
  }
}

__attribute__((constructor)) static void hz6_preload_on_load(void) {
  const char* value = getenv("HZ6_PRELOAD_STATS");
  g_hz6_preload_phase_stats_enabled =
      value && value[0] != '\0' && strcmp(value, "0") != 0;
}

void hz6_preload_register_allocator(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
  pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
  for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
    if (g_hz6_preload_allocator_registry[i] == allocator) {
      pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
      return;
    }
  }
  if (g_hz6_preload_allocator_registry_count <
      HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY) {
    g_hz6_preload_allocator_registry[g_hz6_preload_allocator_registry_count++] =
        allocator;
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
}

static void hz6_preload_print_stats(void) {
  const char* value = getenv("HZ6_PRELOAD_STATS");
  if (!value || value[0] == '\0' || strcmp(value, "0") == 0) {
    return;
  }
  int print_per_allocator = strcmp(value, "2") == 0 ||
                            strcmp(value, "per_allocator") == 0 ||
                            strcmp(value, "per-allocator") == 0;

  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;

  size_t allocator_count = 0;
  size_t route_valid = 0;
  size_t route_invalid = 0;
  size_t route_miss = 0;
  size_t transfer_push = 0;
  size_t transfer_pop = 0;
  size_t source_alloc = 0;
  size_t toy_source_alloc = 0;
  size_t midpage_source_alloc = 0;
  size_t large_source_alloc = 0;
  size_t local2p_source_alloc = 0;
  size_t alloc_fail = 0;
  size_t descriptor_exhausted = 0;
  size_t route_register_fail = 0;
  size_t source_block_exhausted = 0;
  size_t source_prefill_attempt = 0;
  size_t source_prefill_filled = 0;
  size_t source_prefill_fallback = 0;
  size_t front_source_prefill_alloc = 0;
  size_t toy_source_prefill_call = 0;
  size_t route_lookup_probe_total = 0;
  size_t route_lookup_probe_max = 0;
  size_t route_register_probe_total = 0;
  size_t route_register_probe_max = 0;
  size_t route_unregister_probe_total = 0;
  size_t route_unregister_probe_max = 0;
  size_t route_register_reason_unknown = 0;
  size_t route_register_reason_source_run_slot = 0;
  size_t route_register_reason_direct_source = 0;
  size_t route_register_reason_materialize = 0;
  size_t route_register_reason_rehome = 0;
  size_t route_unregister_reason_unknown = 0;
  size_t route_unregister_reason_frontcache_overflow = 0;
  size_t route_unregister_reason_cap_release = 0;
  size_t route_unregister_reason_descriptorless_detach = 0;
  size_t route_unregister_reason_source_slot_release = 0;
  size_t route_unregister_reason_rehome = 0;
  size_t route_active_current = 0;
  size_t route_active_max = 0;
  size_t route_tombstone_current = 0;
  size_t route_tombstone_max = 0;
  size_t route_register_used_tombstone = 0;
  size_t route_register_full_probe_with_tombstone = 0;
  size_t route_tombstone_compact_attempt = 0;
  size_t route_tombstone_compact_success = 0;
  size_t route_tombstone_compact_fail_alloc = 0;
  size_t route_tombstone_compact_moved = 0;
  size_t route_tombstone_cond_probe = 0;
  size_t route_tombstone_cond_would_compact = 0;
  size_t route_tombstone_cond_ratio25 = 0;
  size_t route_tombstone_cond_occupancy75 = 0;
  size_t route_tombstone_cond_cooldown_blocked = 0;
  size_t route_tombstone_cond_highwater = 0;
  size_t descriptor_live_max = 0;
  size_t source_block_active_max = 0;
  size_t frontcache_total_max = 0;
  size_t frontcache_reuse_hit = 0;
  size_t frontcache_reuse_invalid = 0;
  size_t frontcache_push_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_pop_empty_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_bin_max_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_spill_success = 0;
  size_t frontcache_spill_retry_success = 0;
  size_t frontcache_borrow_success = 0;
  size_t toy_small_malloc_frontcache_pop = 0;
  size_t toy_small_malloc_activate_success = 0;
  size_t toy_small_free_cache_attempt = 0;
  size_t toy_small_free_cache_success = 0;
  size_t toy_small_active_map_register = 0;
  size_t toy_small_active_map_register_collision = 0;
  size_t toy_small_active_map_free_attempt = 0;
  size_t toy_small_active_map_free_hit = 0;
  size_t toy_small_active_map_free_miss = 0;
  size_t toy_small_active_map_free_stale = 0;
  size_t toy_small_active_map_free_cache_fail = 0;
  size_t toy_small_active_map_route_bypass = 0;
  size_t midpage_active_map_register = 0;
  size_t midpage_active_map_register_direct = 0;
  size_t midpage_active_map_register_front_alloc = 0;
  size_t midpage_active_map_register_route_fallback = 0;
  size_t midpage_active_map_register_collision = 0;
  size_t midpage_active_map_register_empty_slot = 0;
  size_t midpage_active_map_register_same_ptr = 0;
  size_t midpage_active_map_register_base_slot = 0;
  size_t midpage_active_map_register_probe_total = 0;
  size_t midpage_active_map_register_probe_max = 0;
  size_t midpage_active_map_register_overwrite = 0;
  size_t midpage_8k_active_map_register_overwrite = 0;
  size_t midpage_32k_active_map_register_overwrite = 0;
  size_t midpage_active_map_register_overwrite_same_class_alt = 0;
  size_t midpage_active_map_register_overwrite_stale_alt = 0;
  size_t midpage_active_map_free_attempt = 0;
  size_t midpage_active_map_free_hit = 0;
  size_t midpage_active_map_free_hit_base_slot = 0;
  size_t midpage_active_map_free_hit_probe_total = 0;
  size_t midpage_active_map_free_hit_probe_max = 0;
  size_t midpage_active_map_free_miss = 0;
  size_t midpage_active_map_free_miss_probe_empty = 0;
  size_t midpage_active_map_free_miss_probe_occupied = 0;
  size_t midpage_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_8k_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_32k_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_active_map_free_stale = 0;
  size_t midpage_active_map_free_cache_attempt = 0;
  size_t midpage_active_map_free_cache_success = 0;
  size_t midpage_active_map_free_cache_fail = 0;
  size_t midpage_active_map_alignment_skip = 0;
  size_t midpage_active_map_addr_envelope_skip = 0;
  size_t midpage_active_map_route_bypass = 0;
  size_t midpage_8k_alloc_call = 0;
  size_t midpage_32k_alloc_call = 0;
  size_t midpage_8k_prefill_run_call = 0;
  size_t midpage_32k_prefill_run_call = 0;
  size_t midpage_8k_prefill_run_filled = 0;
  size_t midpage_32k_prefill_run_filled = 0;
  size_t midpage_8k_active_map_register = 0;
  size_t midpage_32k_active_map_register = 0;
  size_t midpage_8k_active_map_free_hit = 0;
  size_t midpage_32k_active_map_free_hit = 0;
  size_t midpage_8k_frontcache_push = 0;
  size_t midpage_32k_frontcache_push = 0;
  size_t midpage_8k_frontcache_pop_empty = 0;
  size_t midpage_32k_frontcache_pop_empty = 0;
  size_t midpage_direct_transfer_probe_attempt = 0;
  size_t midpage_direct_transfer_probe_hit = 0;
  size_t midpage_direct_transfer_probe_empty = 0;
  size_t midpage_8k_direct_transfer_probe_attempt = 0;
  size_t midpage_32k_direct_transfer_probe_attempt = 0;
  size_t midpage_8k_preload_local_route_valid = 0;
  size_t midpage_32k_preload_local_route_valid = 0;
  size_t midpage_8k_source_run_slot_route_register = 0;
  size_t midpage_32k_source_run_slot_route_register = 0;
  size_t smallrun_route_attempt = 0;
  size_t smallrun_range_hit = 0;
  size_t smallrun_active_slot_hit = 0;
  size_t smallrun_descriptor_match = 0;
  size_t smallrun_generation_match = 0;
  size_t smallrun_would_valid = 0;
  size_t smallrun_would_invalid = 0;
  size_t smallrun_exact_fallback_needed = 0;
  size_t smallrun_false_positive = 0;
  size_t source_block_route_range_index_lookup = 0;
  size_t source_block_route_range_index_hit = 0;
  size_t source_block_route_range_index_miss = 0;
  size_t source_block_route_range_index_stale = 0;
  size_t source_block_route_range_index_probe_total = 0;
  size_t source_block_route_range_index_probe_max = 0;
#if HZ6_DIAGNOSTIC_PROBES
  size_t memory_descriptor_table_bytes = 0;
  size_t memory_route_table_bytes = 0;
  size_t memory_source_block_table_bytes = 0;
  size_t memory_frontcache_table_bytes = 0;
  size_t memory_transfer_table_bytes = 0;
  size_t memory_ownerlocality_index_bytes = 0;
  size_t memory_static_table_bytes = 0;
  size_t memory_static_plus_payload_bytes = 0;
  size_t memory_source_block_payload_bytes = 0;
  size_t memory_frontcache_total = 0;
  size_t memory_frontcache_largest_bin = 0;
  size_t memory_active_source_blocks = 0;
  size_t memory_registered_source_blocks = 0;
  size_t memory_ref_nonzero_source_blocks = 0;
  size_t memory_ref_zero_source_blocks = 0;
  size_t memory_midpage_8k_source_blocks = 0;
  size_t memory_midpage_32k_source_blocks = 0;
  size_t memory_midpage_8k_payload_bytes = 0;
  size_t memory_midpage_32k_payload_bytes = 0;
  size_t memory_midpage_8k_active_descriptors = 0;
  size_t memory_midpage_32k_active_descriptors = 0;
  size_t memory_midpage_8k_local_free_descriptors = 0;
  size_t memory_midpage_32k_local_free_descriptors = 0;
  size_t memory_midpage_8k_transfer_free_descriptors = 0;
  size_t memory_midpage_32k_transfer_free_descriptors = 0;
  size_t memory_midpage_8k_remote_pending_descriptors = 0;
  size_t memory_midpage_32k_remote_pending_descriptors = 0;
  size_t memory_midpage_8k_all_local_free_blocks = 0;
  size_t memory_midpage_32k_all_local_free_blocks = 0;
  size_t memory_midpage_8k_all_local_free_payload_bytes = 0;
  size_t memory_midpage_32k_all_local_free_payload_bytes = 0;
  size_t memory_midpage_8k_active_zero_local_free_nonzero_blocks = 0;
  size_t memory_midpage_32k_active_zero_local_free_nonzero_blocks = 0;
  size_t memory_midpage_8k_low_active_1_4_blocks = 0;
  size_t memory_midpage_32k_low_active_1_4_blocks = 0;
  size_t memory_midpage_8k_low_active_1_4_payload_bytes = 0;
  size_t memory_midpage_32k_low_active_1_4_payload_bytes = 0;
  size_t memory_midpage_ref_mismatch_blocks = 0;
#endif

  pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
  allocator_count = g_hz6_preload_allocator_registry_count;
  for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
    Hz6Allocator* allocator = g_hz6_preload_allocator_registry[i];
    if (!allocator) {
      continue;
    }
    Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
    route_valid += stats.route_valid;
    route_invalid += stats.route_invalid;
    route_miss += stats.route_miss;
    transfer_push += stats.transfer_push;
    transfer_pop += stats.transfer_pop;
    source_alloc += stats.source_alloc;
    toy_source_alloc += stats.toy_source_alloc;
    midpage_source_alloc += stats.midpage_source_alloc;
    large_source_alloc += stats.large_source_alloc;
    local2p_source_alloc += stats.local2p_source_alloc;
    alloc_fail += stats.alloc_fail;
    descriptor_exhausted += stats.descriptor_exhausted;
    route_register_fail += stats.route_register_fail;
    source_block_exhausted += stats.source_block_exhausted;
    source_prefill_attempt += stats.source_prefill_attempt;
    source_prefill_filled += stats.source_prefill_filled;
    source_prefill_fallback += stats.source_prefill_fallback;
    front_source_prefill_alloc += stats.front_source_prefill_alloc;
    toy_source_prefill_call += stats.toy_source_prefill_call;
    route_lookup_probe_total += stats.route_lookup_probe_total;
    if (stats.route_lookup_probe_max > route_lookup_probe_max) {
      route_lookup_probe_max = stats.route_lookup_probe_max;
    }
    route_register_probe_total += stats.route_register_probe_total;
    if (stats.route_register_probe_max > route_register_probe_max) {
      route_register_probe_max = stats.route_register_probe_max;
    }
    route_unregister_probe_total += stats.route_unregister_probe_total;
    if (stats.route_unregister_probe_max > route_unregister_probe_max) {
      route_unregister_probe_max = stats.route_unregister_probe_max;
    }
    route_register_reason_unknown += stats.route_register_reason_unknown;
    route_register_reason_source_run_slot +=
        stats.route_register_reason_source_run_slot;
    route_register_reason_direct_source +=
        stats.route_register_reason_direct_source;
    route_register_reason_materialize +=
        stats.route_register_reason_materialize;
    route_register_reason_rehome += stats.route_register_reason_rehome;
    route_unregister_reason_unknown += stats.route_unregister_reason_unknown;
    route_unregister_reason_frontcache_overflow +=
        stats.route_unregister_reason_frontcache_overflow;
    route_unregister_reason_cap_release +=
        stats.route_unregister_reason_cap_release;
    route_unregister_reason_descriptorless_detach +=
        stats.route_unregister_reason_descriptorless_detach;
    route_unregister_reason_source_slot_release +=
        stats.route_unregister_reason_source_slot_release;
    route_unregister_reason_rehome += stats.route_unregister_reason_rehome;
    route_active_current += stats.route_active_current;
    if (stats.route_active_max > route_active_max) {
      route_active_max = stats.route_active_max;
    }
    route_tombstone_current += stats.route_tombstone_current;
    if (stats.route_tombstone_max > route_tombstone_max) {
      route_tombstone_max = stats.route_tombstone_max;
    }
    route_register_used_tombstone += stats.route_register_used_tombstone;
    route_register_full_probe_with_tombstone +=
        stats.route_register_full_probe_with_tombstone;
    route_tombstone_compact_attempt += stats.route_tombstone_compact_attempt;
    route_tombstone_compact_success += stats.route_tombstone_compact_success;
    route_tombstone_compact_fail_alloc +=
        stats.route_tombstone_compact_fail_alloc;
    route_tombstone_compact_moved += stats.route_tombstone_compact_moved;
    route_tombstone_cond_probe += stats.route_tombstone_cond_probe;
    route_tombstone_cond_would_compact +=
        stats.route_tombstone_cond_would_compact;
    route_tombstone_cond_ratio25 += stats.route_tombstone_cond_ratio25;
    route_tombstone_cond_occupancy75 +=
        stats.route_tombstone_cond_occupancy75;
    route_tombstone_cond_cooldown_blocked +=
        stats.route_tombstone_cond_cooldown_blocked;
    if (stats.route_tombstone_cond_highwater >
        route_tombstone_cond_highwater) {
      route_tombstone_cond_highwater =
          stats.route_tombstone_cond_highwater;
    }
    if (stats.descriptor_live_max > descriptor_live_max) {
      descriptor_live_max = stats.descriptor_live_max;
    }
    if (stats.source_block_active_max > source_block_active_max) {
      source_block_active_max = stats.source_block_active_max;
    }
    if (stats.frontcache_total_max > frontcache_total_max) {
      frontcache_total_max = stats.frontcache_total_max;
    }
    frontcache_reuse_hit += stats.frontcache_reuse_hit;
    frontcache_reuse_invalid += stats.frontcache_reuse_invalid;
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
      frontcache_push_by_class[class_id] +=
          stats.frontcache_push_by_class[class_id];
      frontcache_pop_empty_by_class[class_id] +=
          stats.frontcache_pop_empty_by_class[class_id];
      if (stats.frontcache_bin_max_by_class[class_id] >
          frontcache_bin_max_by_class[class_id]) {
        frontcache_bin_max_by_class[class_id] =
            stats.frontcache_bin_max_by_class[class_id];
      }
    }
    frontcache_spill_success += stats.frontcache_spill_success;
    frontcache_spill_retry_success += stats.frontcache_spill_retry_success;
    frontcache_borrow_success += stats.frontcache_borrow_success;
    toy_small_malloc_frontcache_pop += stats.toy_small_malloc_frontcache_pop;
    toy_small_malloc_activate_success +=
        stats.toy_small_malloc_activate_success;
    toy_small_free_cache_attempt += stats.toy_small_free_cache_attempt;
    toy_small_free_cache_success += stats.toy_small_free_cache_success;
    toy_small_active_map_register += stats.toy_small_active_map_register;
    toy_small_active_map_register_collision +=
        stats.toy_small_active_map_register_collision;
    toy_small_active_map_free_attempt +=
        stats.toy_small_active_map_free_attempt;
    toy_small_active_map_free_hit += stats.toy_small_active_map_free_hit;
    toy_small_active_map_free_miss += stats.toy_small_active_map_free_miss;
    toy_small_active_map_free_stale += stats.toy_small_active_map_free_stale;
    toy_small_active_map_free_cache_fail +=
        stats.toy_small_active_map_free_cache_fail;
    toy_small_active_map_route_bypass +=
        stats.toy_small_active_map_route_bypass;
    midpage_active_map_register += stats.midpage_active_map_register;
    midpage_active_map_register_direct +=
        stats.midpage_active_map_register_direct;
    midpage_active_map_register_front_alloc +=
        stats.midpage_active_map_register_front_alloc;
    midpage_active_map_register_route_fallback +=
        stats.midpage_active_map_register_route_fallback;
    midpage_active_map_register_collision +=
        stats.midpage_active_map_register_collision;
    midpage_active_map_register_empty_slot +=
        stats.midpage_active_map_register_empty_slot;
    midpage_active_map_register_same_ptr +=
        stats.midpage_active_map_register_same_ptr;
    midpage_active_map_register_base_slot +=
        stats.midpage_active_map_register_base_slot;
    midpage_active_map_register_probe_total +=
        stats.midpage_active_map_register_probe_total;
    if (stats.midpage_active_map_register_probe_max >
        midpage_active_map_register_probe_max) {
      midpage_active_map_register_probe_max =
          stats.midpage_active_map_register_probe_max;
    }
    midpage_active_map_register_overwrite +=
        stats.midpage_active_map_register_overwrite;
    midpage_8k_active_map_register_overwrite +=
        stats.midpage_8k_active_map_register_overwrite;
    midpage_32k_active_map_register_overwrite +=
        stats.midpage_32k_active_map_register_overwrite;
    midpage_active_map_register_overwrite_same_class_alt +=
        stats.midpage_active_map_register_overwrite_same_class_alt;
    midpage_active_map_register_overwrite_stale_alt +=
        stats.midpage_active_map_register_overwrite_stale_alt;
    midpage_active_map_free_attempt +=
        stats.midpage_active_map_free_attempt;
    midpage_active_map_free_hit += stats.midpage_active_map_free_hit;
    midpage_active_map_free_hit_base_slot +=
        stats.midpage_active_map_free_hit_base_slot;
    midpage_active_map_free_hit_probe_total +=
        stats.midpage_active_map_free_hit_probe_total;
    if (stats.midpage_active_map_free_hit_probe_max >
        midpage_active_map_free_hit_probe_max) {
      midpage_active_map_free_hit_probe_max =
          stats.midpage_active_map_free_hit_probe_max;
    }
    midpage_active_map_free_miss += stats.midpage_active_map_free_miss;
    midpage_active_map_free_miss_probe_empty +=
        stats.midpage_active_map_free_miss_probe_empty;
    midpage_active_map_free_miss_probe_occupied +=
        stats.midpage_active_map_free_miss_probe_occupied;
    midpage_active_map_free_miss_found_elsewhere +=
        stats.midpage_active_map_free_miss_found_elsewhere;
    midpage_8k_active_map_free_miss_found_elsewhere +=
        stats.midpage_8k_active_map_free_miss_found_elsewhere;
    midpage_32k_active_map_free_miss_found_elsewhere +=
        stats.midpage_32k_active_map_free_miss_found_elsewhere;
    midpage_active_map_free_stale += stats.midpage_active_map_free_stale;
    midpage_active_map_free_cache_attempt +=
        stats.midpage_active_map_free_cache_attempt;
    midpage_active_map_free_cache_success +=
        stats.midpage_active_map_free_cache_success;
    midpage_active_map_free_cache_fail +=
        stats.midpage_active_map_free_cache_fail;
    midpage_active_map_alignment_skip +=
        stats.midpage_active_map_alignment_skip;
    midpage_active_map_addr_envelope_skip +=
        stats.midpage_active_map_addr_envelope_skip;
    midpage_active_map_route_bypass +=
        stats.midpage_active_map_route_bypass;
    midpage_8k_alloc_call += stats.midpage_8k_alloc_call;
    midpage_32k_alloc_call += stats.midpage_32k_alloc_call;
    midpage_8k_prefill_run_call += stats.midpage_8k_prefill_run_call;
    midpage_32k_prefill_run_call += stats.midpage_32k_prefill_run_call;
    midpage_8k_prefill_run_filled += stats.midpage_8k_prefill_run_filled;
    midpage_32k_prefill_run_filled += stats.midpage_32k_prefill_run_filled;
    midpage_8k_active_map_register += stats.midpage_8k_active_map_register;
    midpage_32k_active_map_register += stats.midpage_32k_active_map_register;
    midpage_8k_active_map_free_hit += stats.midpage_8k_active_map_free_hit;
    midpage_32k_active_map_free_hit += stats.midpage_32k_active_map_free_hit;
    midpage_8k_frontcache_push +=
        stats.frontcache_push_by_class[HZ6_MIDPAGE_8K_CLASS_ID];
    midpage_32k_frontcache_push +=
        stats.frontcache_push_by_class[HZ6_MIDPAGE_32K_CLASS_ID];
    midpage_8k_frontcache_pop_empty +=
        stats.frontcache_pop_empty_by_class[HZ6_MIDPAGE_8K_CLASS_ID];
    midpage_32k_frontcache_pop_empty +=
        stats.frontcache_pop_empty_by_class[HZ6_MIDPAGE_32K_CLASS_ID];
    midpage_direct_transfer_probe_attempt +=
        stats.midpage_direct_transfer_probe_attempt;
    midpage_direct_transfer_probe_hit +=
        stats.midpage_direct_transfer_probe_hit;
    midpage_direct_transfer_probe_empty +=
        stats.midpage_direct_transfer_probe_empty;
    midpage_8k_direct_transfer_probe_attempt +=
        stats.midpage_8k_direct_transfer_probe_attempt;
    midpage_32k_direct_transfer_probe_attempt +=
        stats.midpage_32k_direct_transfer_probe_attempt;
    midpage_8k_preload_local_route_valid +=
        stats.midpage_8k_preload_local_route_valid;
    midpage_32k_preload_local_route_valid +=
        stats.midpage_32k_preload_local_route_valid;
    midpage_8k_source_run_slot_route_register +=
        stats.midpage_8k_source_run_slot_route_register;
    midpage_32k_source_run_slot_route_register +=
        stats.midpage_32k_source_run_slot_route_register;
    smallrun_route_attempt += stats.smallrun_route_attempt;
    smallrun_range_hit += stats.smallrun_range_hit;
    smallrun_active_slot_hit += stats.smallrun_active_slot_hit;
    smallrun_descriptor_match += stats.smallrun_descriptor_match;
    smallrun_generation_match += stats.smallrun_generation_match;
    smallrun_would_valid += stats.smallrun_would_valid;
    smallrun_would_invalid += stats.smallrun_would_invalid;
    smallrun_exact_fallback_needed += stats.smallrun_exact_fallback_needed;
    smallrun_false_positive += stats.smallrun_false_positive;
    source_block_route_range_index_lookup +=
        stats.source_block_route_range_index_lookup;
    source_block_route_range_index_hit +=
        stats.source_block_route_range_index_hit;
    source_block_route_range_index_miss +=
        stats.source_block_route_range_index_miss;
    source_block_route_range_index_stale +=
        stats.source_block_route_range_index_stale;
    source_block_route_range_index_probe_total +=
        stats.source_block_route_range_index_probe_total;
    if (stats.source_block_route_range_index_probe_max >
        source_block_route_range_index_probe_max) {
      source_block_route_range_index_probe_max =
          stats.source_block_route_range_index_probe_max;
    }
#if HZ6_DIAGNOSTIC_PROBES
    memory_descriptor_table_bytes += stats.memory_descriptor_table_bytes;
    memory_route_table_bytes += stats.memory_route_table_bytes;
    memory_source_block_table_bytes += stats.memory_source_block_table_bytes;
    memory_frontcache_table_bytes += stats.memory_frontcache_table_bytes;
    memory_transfer_table_bytes += stats.memory_transfer_table_bytes;
    memory_ownerlocality_index_bytes += stats.memory_ownerlocality_index_bytes;
    memory_static_table_bytes += stats.memory_static_table_bytes;
    memory_static_plus_payload_bytes += stats.memory_static_plus_payload_bytes;
    memory_source_block_payload_bytes += stats.memory_source_block_payload_bytes;
    memory_frontcache_total += stats.memory_frontcache_total;
    if (stats.memory_frontcache_largest_bin > memory_frontcache_largest_bin) {
      memory_frontcache_largest_bin = stats.memory_frontcache_largest_bin;
    }
    memory_active_source_blocks += stats.memory_active_source_blocks;
    memory_registered_source_blocks += stats.memory_registered_source_blocks;
    memory_ref_nonzero_source_blocks += stats.memory_ref_nonzero_source_blocks;
    memory_ref_zero_source_blocks += stats.memory_ref_zero_source_blocks;
    memory_midpage_8k_source_blocks +=
        stats.memory_midpage_8k_source_blocks;
    memory_midpage_32k_source_blocks +=
        stats.memory_midpage_32k_source_blocks;
    memory_midpage_8k_payload_bytes +=
        stats.memory_midpage_8k_payload_bytes;
    memory_midpage_32k_payload_bytes +=
        stats.memory_midpage_32k_payload_bytes;
    memory_midpage_8k_active_descriptors +=
        stats.memory_midpage_8k_active_descriptors;
    memory_midpage_32k_active_descriptors +=
        stats.memory_midpage_32k_active_descriptors;
    memory_midpage_8k_local_free_descriptors +=
        stats.memory_midpage_8k_local_free_descriptors;
    memory_midpage_32k_local_free_descriptors +=
        stats.memory_midpage_32k_local_free_descriptors;
    memory_midpage_8k_transfer_free_descriptors +=
        stats.memory_midpage_8k_transfer_free_descriptors;
    memory_midpage_32k_transfer_free_descriptors +=
        stats.memory_midpage_32k_transfer_free_descriptors;
    memory_midpage_8k_remote_pending_descriptors +=
        stats.memory_midpage_8k_remote_pending_descriptors;
    memory_midpage_32k_remote_pending_descriptors +=
        stats.memory_midpage_32k_remote_pending_descriptors;
    memory_midpage_8k_all_local_free_blocks +=
        stats.memory_midpage_8k_all_local_free_blocks;
    memory_midpage_32k_all_local_free_blocks +=
        stats.memory_midpage_32k_all_local_free_blocks;
    memory_midpage_8k_all_local_free_payload_bytes +=
        stats.memory_midpage_8k_all_local_free_payload_bytes;
    memory_midpage_32k_all_local_free_payload_bytes +=
        stats.memory_midpage_32k_all_local_free_payload_bytes;
    memory_midpage_8k_active_zero_local_free_nonzero_blocks +=
        stats.memory_midpage_8k_active_zero_local_free_nonzero_blocks;
    memory_midpage_32k_active_zero_local_free_nonzero_blocks +=
        stats.memory_midpage_32k_active_zero_local_free_nonzero_blocks;
    memory_midpage_8k_low_active_1_4_blocks +=
        stats.memory_midpage_8k_low_active_1_4_blocks;
    memory_midpage_32k_low_active_1_4_blocks +=
        stats.memory_midpage_32k_low_active_1_4_blocks;
    memory_midpage_8k_low_active_1_4_payload_bytes +=
        stats.memory_midpage_8k_low_active_1_4_payload_bytes;
    memory_midpage_32k_low_active_1_4_payload_bytes +=
        stats.memory_midpage_32k_low_active_1_4_payload_bytes;
    memory_midpage_ref_mismatch_blocks +=
        stats.memory_midpage_ref_mismatch_blocks;
#endif
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);

  Hz6LinuxMmapRetainStats retain_stats =
      hz6_linux_mmap_retain_stats_snapshot();
#if HZ6_DIAGNOSTIC_PROBES
  size_t toy_active_map_table_bytes = 0;
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  toy_active_map_table_bytes =
      allocator_count * sizeof(Hz6ToySmallActiveMapEntry) *
      HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY;
#endif
  size_t midpage_active_map_table_bytes = 0;
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  midpage_active_map_table_bytes =
      allocator_count * sizeof(Hz6MidPageActiveMapEntry) *
      HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY;
#endif
  size_t preload_attributed_bytes =
      memory_static_plus_payload_bytes + toy_active_map_table_bytes +
      midpage_active_map_table_bytes + retain_stats.retained_bytes;
#endif

  fprintf(stderr,
          "[HZ6_PRELOAD_STATS] allocators=%zu route_valid=%zu "
          "route_invalid=%zu route_miss=%zu transfer_push=%zu "
          "transfer_pop=%zu source_alloc=%zu toy_source_alloc=%zu "
          "midpage_source_alloc=%zu large_source_alloc=%zu "
          "local2p_source_alloc=%zu alloc_fail=%zu "
          "descriptor_exhausted=%zu route_register_fail=%zu "
          "source_block_exhausted=%zu source_prefill_attempt=%zu "
          "source_prefill_filled=%zu source_prefill_fallback=%zu "
          "front_source_prefill_alloc=%zu toy_source_prefill_call=%zu "
          "route_lookup_probe_total=%zu "
          "route_lookup_probe_max=%zu route_register_probe_total=%zu "
          "route_register_probe_max=%zu route_unregister_probe_total=%zu "
          "route_unregister_probe_max=%zu "
          "retain_reserve_calls=%zu retain_reserve_64k_calls=%zu "
          "retain_64k_take_hit=%zu retain_64k_take_miss=%zu "
          "retain_generic_take_hit=%zu retain_generic_take_miss=%zu "
          "retain_mmap_fallback=%zu retain_release_calls=%zu "
          "retain_release_64k_calls=%zu retain_64k_put_hit=%zu "
          "retain_64k_put_full=%zu retain_generic_put_hit=%zu "
          "retain_generic_put_full=%zu retain_munmap_fallback=%zu "
          "retain_retained_bytes=%zu retain_retained_64k_count=%zu\n",
          allocator_count, route_valid, route_invalid, route_miss,
          transfer_push, transfer_pop, source_alloc, toy_source_alloc,
          midpage_source_alloc, large_source_alloc, local2p_source_alloc,
          alloc_fail, descriptor_exhausted, route_register_fail,
          source_block_exhausted, source_prefill_attempt,
          source_prefill_filled, source_prefill_fallback,
          front_source_prefill_alloc, toy_source_prefill_call,
          route_lookup_probe_total,
          route_lookup_probe_max, route_register_probe_total,
          route_register_probe_max, route_unregister_probe_total,
          route_unregister_probe_max, retain_stats.reserve_calls,
          retain_stats.reserve_64k_calls, retain_stats.retain_64k_take_hit,
          retain_stats.retain_64k_take_miss,
          retain_stats.retain_generic_take_hit,
          retain_stats.retain_generic_take_miss, retain_stats.mmap_fallback,
          retain_stats.release_calls, retain_stats.release_64k_calls,
          retain_stats.retain_64k_put_hit, retain_stats.retain_64k_put_full,
          retain_stats.retain_generic_put_hit,
          retain_stats.retain_generic_put_full, retain_stats.munmap_fallback,
          retain_stats.retained_bytes, retain_stats.retained_64k_count);

  fprintf(stderr,
          "[HZ6_PRELOAD_ROUTE_DETAIL] "
          "route_register_reason_unknown=%zu "
          "route_register_reason_source_run_slot=%zu "
          "route_register_reason_direct_source=%zu "
          "route_register_reason_materialize=%zu "
          "route_register_reason_rehome=%zu "
          "route_unregister_reason_unknown=%zu "
          "route_unregister_reason_frontcache_overflow=%zu "
          "route_unregister_reason_cap_release=%zu "
          "route_unregister_reason_descriptorless_detach=%zu "
          "route_unregister_reason_source_slot_release=%zu "
          "route_unregister_reason_rehome=%zu "
          "route_active_current=%zu route_active_max=%zu "
          "route_tombstone_current=%zu route_tombstone_max=%zu "
          "route_register_used_tombstone=%zu "
          "route_register_full_probe_with_tombstone=%zu "
          "route_tombstone_compact_attempt=%zu "
          "route_tombstone_compact_success=%zu "
          "route_tombstone_compact_fail_alloc=%zu "
          "route_tombstone_compact_moved=%zu "
          "route_tombstone_cond_probe=%zu "
          "route_tombstone_cond_would_compact=%zu "
          "route_tombstone_cond_ratio25=%zu "
          "route_tombstone_cond_occupancy75=%zu "
          "route_tombstone_cond_cooldown_blocked=%zu "
          "route_tombstone_cond_highwater=%zu\n",
          route_register_reason_unknown,
          route_register_reason_source_run_slot,
          route_register_reason_direct_source,
          route_register_reason_materialize, route_register_reason_rehome,
          route_unregister_reason_unknown,
          route_unregister_reason_frontcache_overflow,
          route_unregister_reason_cap_release,
          route_unregister_reason_descriptorless_detach,
          route_unregister_reason_source_slot_release,
          route_unregister_reason_rehome, route_active_current,
          route_active_max, route_tombstone_current, route_tombstone_max,
          route_register_used_tombstone,
          route_register_full_probe_with_tombstone,
          route_tombstone_compact_attempt, route_tombstone_compact_success,
          route_tombstone_compact_fail_alloc,
          route_tombstone_compact_moved, route_tombstone_cond_probe,
          route_tombstone_cond_would_compact,
          route_tombstone_cond_ratio25, route_tombstone_cond_occupancy75,
          route_tombstone_cond_cooldown_blocked,
          route_tombstone_cond_highwater);

  fprintf(stderr,
          "[HZ6_PRELOAD_FRONT_DETAIL] descriptor_live_max=%zu "
          "source_block_active_max=%zu frontcache_total_max=%zu "
          "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
          "frontcache_spill_success=%zu "
          "frontcache_spill_retry_success=%zu "
          "frontcache_borrow_success=%zu "
          "toy_small_malloc_frontcache_pop=%zu "
          "toy_small_malloc_activate_success=%zu "
          "toy_small_free_cache_attempt=%zu "
          "toy_small_free_cache_success=%zu "
          "toy_small_active_map_register=%zu "
          "toy_small_active_map_register_collision=%zu "
          "toy_small_active_map_free_attempt=%zu "
          "toy_small_active_map_free_hit=%zu "
          "toy_small_active_map_free_miss=%zu "
          "toy_small_active_map_free_stale=%zu "
          "toy_small_active_map_free_cache_fail=%zu "
          "toy_small_active_map_route_bypass=%zu "
          "midpage_active_map_register=%zu "
          "midpage_active_map_register_direct=%zu "
          "midpage_active_map_register_front_alloc=%zu "
          "midpage_active_map_register_route_fallback=%zu "
          "midpage_active_map_register_collision=%zu "
          "midpage_active_map_register_empty_slot=%zu "
          "midpage_active_map_register_same_ptr=%zu "
          "midpage_active_map_register_base_slot=%zu "
          "midpage_active_map_register_probe_total=%zu "
          "midpage_active_map_register_probe_max=%zu "
          "midpage_active_map_register_overwrite=%zu "
          "midpage_8k_active_map_register_overwrite=%zu "
          "midpage_32k_active_map_register_overwrite=%zu "
          "midpage_active_map_register_overwrite_same_class_alt=%zu "
          "midpage_active_map_register_overwrite_stale_alt=%zu "
          "midpage_active_map_free_attempt=%zu "
          "midpage_active_map_free_hit=%zu "
          "midpage_active_map_free_hit_base_slot=%zu "
          "midpage_active_map_free_hit_probe_total=%zu "
          "midpage_active_map_free_hit_probe_max=%zu "
          "midpage_active_map_free_miss=%zu "
          "midpage_active_map_free_miss_probe_empty=%zu "
          "midpage_active_map_free_miss_probe_occupied=%zu "
          "midpage_active_map_free_miss_found_elsewhere=%zu "
          "midpage_8k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_32k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_active_map_free_stale=%zu "
          "midpage_active_map_free_cache_attempt=%zu "
          "midpage_active_map_free_cache_success=%zu "
          "midpage_active_map_free_cache_fail=%zu "
          "midpage_active_map_alignment_skip=%zu "
          "midpage_active_map_addr_envelope_skip=%zu "
          "midpage_active_map_route_bypass=%zu\n",
          descriptor_live_max, source_block_active_max,
          frontcache_total_max, frontcache_reuse_hit,
          frontcache_reuse_invalid, frontcache_spill_success,
          frontcache_spill_retry_success, frontcache_borrow_success,
          toy_small_malloc_frontcache_pop,
          toy_small_malloc_activate_success,
          toy_small_free_cache_attempt, toy_small_free_cache_success,
          toy_small_active_map_register,
          toy_small_active_map_register_collision,
          toy_small_active_map_free_attempt,
          toy_small_active_map_free_hit, toy_small_active_map_free_miss,
          toy_small_active_map_free_stale,
          toy_small_active_map_free_cache_fail,
          toy_small_active_map_route_bypass,
          midpage_active_map_register,
          midpage_active_map_register_direct,
          midpage_active_map_register_front_alloc,
          midpage_active_map_register_route_fallback,
          midpage_active_map_register_collision,
          midpage_active_map_register_empty_slot,
          midpage_active_map_register_same_ptr,
          midpage_active_map_register_base_slot,
          midpage_active_map_register_probe_total,
          midpage_active_map_register_probe_max,
          midpage_active_map_register_overwrite,
          midpage_8k_active_map_register_overwrite,
          midpage_32k_active_map_register_overwrite,
          midpage_active_map_register_overwrite_same_class_alt,
          midpage_active_map_register_overwrite_stale_alt,
          midpage_active_map_free_attempt,
          midpage_active_map_free_hit,
          midpage_active_map_free_hit_base_slot,
          midpage_active_map_free_hit_probe_total,
          midpage_active_map_free_hit_probe_max,
          midpage_active_map_free_miss,
          midpage_active_map_free_miss_probe_empty,
          midpage_active_map_free_miss_probe_occupied,
          midpage_active_map_free_miss_found_elsewhere,
          midpage_8k_active_map_free_miss_found_elsewhere,
          midpage_32k_active_map_free_miss_found_elsewhere,
          midpage_active_map_free_stale,
          midpage_active_map_free_cache_attempt,
          midpage_active_map_free_cache_success,
          midpage_active_map_free_cache_fail,
          midpage_active_map_alignment_skip,
          midpage_active_map_addr_envelope_skip,
          midpage_active_map_route_bypass);

  fprintf(stderr, "[HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL]");
  for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
    fprintf(stderr,
            " c%zu_push=%zu c%zu_empty=%zu c%zu_max=%zu",
            class_id,
            frontcache_push_by_class[class_id],
            class_id,
            frontcache_pop_empty_by_class[class_id],
            class_id,
            frontcache_bin_max_by_class[class_id]);
  }
  fprintf(stderr, "\n");

  fprintf(stderr,
          "[HZ6_PRELOAD_MIDPAGE_CLASS_DETAIL] "
          "midpage_8k_alloc_call=%zu midpage_32k_alloc_call=%zu "
          "midpage_8k_prefill_run_call=%zu "
          "midpage_32k_prefill_run_call=%zu "
          "midpage_8k_prefill_run_filled=%zu "
          "midpage_32k_prefill_run_filled=%zu "
          "midpage_8k_active_map_register=%zu "
          "midpage_32k_active_map_register=%zu "
          "midpage_8k_active_map_free_hit=%zu "
          "midpage_32k_active_map_free_hit=%zu "
          "midpage_8k_frontcache_push=%zu "
          "midpage_32k_frontcache_push=%zu "
          "midpage_8k_frontcache_pop_empty=%zu "
          "midpage_32k_frontcache_pop_empty=%zu "
          "midpage_direct_transfer_probe_attempt=%zu "
          "midpage_direct_transfer_probe_hit=%zu "
          "midpage_direct_transfer_probe_empty=%zu "
          "midpage_8k_direct_transfer_probe_attempt=%zu "
          "midpage_32k_direct_transfer_probe_attempt=%zu "
          "midpage_8k_preload_local_route_valid=%zu "
          "midpage_32k_preload_local_route_valid=%zu "
          "midpage_8k_source_run_slot_route_register=%zu "
          "midpage_32k_source_run_slot_route_register=%zu\n",
          midpage_8k_alloc_call, midpage_32k_alloc_call,
          midpage_8k_prefill_run_call, midpage_32k_prefill_run_call,
          midpage_8k_prefill_run_filled, midpage_32k_prefill_run_filled,
          midpage_8k_active_map_register, midpage_32k_active_map_register,
          midpage_8k_active_map_free_hit, midpage_32k_active_map_free_hit,
          midpage_8k_frontcache_push, midpage_32k_frontcache_push,
          midpage_8k_frontcache_pop_empty, midpage_32k_frontcache_pop_empty,
          midpage_direct_transfer_probe_attempt,
          midpage_direct_transfer_probe_hit,
          midpage_direct_transfer_probe_empty,
          midpage_8k_direct_transfer_probe_attempt,
          midpage_32k_direct_transfer_probe_attempt,
          midpage_8k_preload_local_route_valid,
          midpage_32k_preload_local_route_valid,
          midpage_8k_source_run_slot_route_register,
          midpage_32k_source_run_slot_route_register);

  fprintf(stderr,
          "[HZ6_PRELOAD_RUNMETA_DETAIL] "
          "smallrun_route_attempt=%zu "
          "smallrun_range_hit=%zu "
          "smallrun_active_slot_hit=%zu "
          "smallrun_descriptor_match=%zu "
          "smallrun_generation_match=%zu "
          "smallrun_would_valid=%zu "
          "smallrun_would_invalid=%zu "
          "smallrun_exact_fallback_needed=%zu "
          "smallrun_false_positive=%zu "
          "source_block_route_range_index_lookup=%zu "
          "source_block_route_range_index_hit=%zu "
          "source_block_route_range_index_miss=%zu "
          "source_block_route_range_index_stale=%zu "
          "source_block_route_range_index_probe_total=%zu "
          "source_block_route_range_index_probe_max=%zu\n",
          smallrun_route_attempt, smallrun_range_hit,
          smallrun_active_slot_hit, smallrun_descriptor_match,
          smallrun_generation_match, smallrun_would_valid,
          smallrun_would_invalid, smallrun_exact_fallback_needed,
          smallrun_false_positive, source_block_route_range_index_lookup,
          source_block_route_range_index_hit,
          source_block_route_range_index_miss,
          source_block_route_range_index_stale,
          source_block_route_range_index_probe_total,
          source_block_route_range_index_probe_max);

#if HZ6_DIAGNOSTIC_PROBES
  fprintf(stderr,
          "[HZ6_PRELOAD_MEMORY_ATTR] "
          "allocator_count=%zu "
          "descriptor_table_bytes=%zu "
          "route_table_bytes=%zu "
          "source_block_table_bytes=%zu "
          "frontcache_table_bytes=%zu "
          "transfer_table_bytes=%zu "
          "ownerlocality_index_bytes=%zu "
          "toy_active_map_table_bytes=%zu "
          "midpage_active_map_table_bytes=%zu "
          "static_table_bytes=%zu "
          "source_block_payload_bytes=%zu "
          "static_plus_payload_bytes=%zu "
          "retain_retained_bytes=%zu "
          "preload_attributed_bytes=%zu "
          "frontcache_total=%zu "
          "frontcache_largest_bin=%zu "
          "active_source_blocks=%zu "
          "registered_source_blocks=%zu "
          "ref_nonzero_source_blocks=%zu "
          "ref_zero_source_blocks=%zu "
          "midpage_8k_source_blocks=%zu "
          "midpage_32k_source_blocks=%zu "
          "midpage_8k_payload_bytes=%zu "
          "midpage_32k_payload_bytes=%zu "
          "midpage_8k_active=%zu "
          "midpage_32k_active=%zu "
          "midpage_8k_local_free=%zu "
          "midpage_32k_local_free=%zu "
          "midpage_8k_transfer_free=%zu "
          "midpage_32k_transfer_free=%zu "
          "midpage_8k_remote_pending=%zu "
          "midpage_32k_remote_pending=%zu "
          "midpage_8k_all_local_free_blocks=%zu "
          "midpage_32k_all_local_free_blocks=%zu "
          "midpage_8k_all_local_free_payload_bytes=%zu "
          "midpage_32k_all_local_free_payload_bytes=%zu "
          "midpage_8k_active_zero_local_free_nonzero_blocks=%zu "
          "midpage_32k_active_zero_local_free_nonzero_blocks=%zu "
          "midpage_8k_low_active_1_4_blocks=%zu "
          "midpage_32k_low_active_1_4_blocks=%zu "
          "midpage_8k_low_active_1_4_payload_bytes=%zu "
          "midpage_32k_low_active_1_4_payload_bytes=%zu "
          "midpage_ref_mismatch_blocks=%zu\n",
          allocator_count, memory_descriptor_table_bytes,
          memory_route_table_bytes, memory_source_block_table_bytes,
          memory_frontcache_table_bytes, memory_transfer_table_bytes,
          memory_ownerlocality_index_bytes, toy_active_map_table_bytes,
          midpage_active_map_table_bytes, memory_static_table_bytes,
          memory_source_block_payload_bytes,
          memory_static_plus_payload_bytes, retain_stats.retained_bytes,
          preload_attributed_bytes, memory_frontcache_total,
          memory_frontcache_largest_bin, memory_active_source_blocks,
          memory_registered_source_blocks, memory_ref_nonzero_source_blocks,
          memory_ref_zero_source_blocks, memory_midpage_8k_source_blocks,
          memory_midpage_32k_source_blocks, memory_midpage_8k_payload_bytes,
          memory_midpage_32k_payload_bytes,
          memory_midpage_8k_active_descriptors,
          memory_midpage_32k_active_descriptors,
          memory_midpage_8k_local_free_descriptors,
          memory_midpage_32k_local_free_descriptors,
          memory_midpage_8k_transfer_free_descriptors,
          memory_midpage_32k_transfer_free_descriptors,
          memory_midpage_8k_remote_pending_descriptors,
          memory_midpage_32k_remote_pending_descriptors,
          memory_midpage_8k_all_local_free_blocks,
          memory_midpage_32k_all_local_free_blocks,
          memory_midpage_8k_all_local_free_payload_bytes,
          memory_midpage_32k_all_local_free_payload_bytes,
          memory_midpage_8k_active_zero_local_free_nonzero_blocks,
          memory_midpage_32k_active_zero_local_free_nonzero_blocks,
          memory_midpage_8k_low_active_1_4_blocks,
          memory_midpage_32k_low_active_1_4_blocks,
          memory_midpage_8k_low_active_1_4_payload_bytes,
          memory_midpage_32k_low_active_1_4_payload_bytes,
          memory_midpage_ref_mismatch_blocks);
#endif

  fprintf(stderr,
          "[HZ6_PRELOAD_PHASE_STATS] malloc_calls=%zu "
          "malloc_hz6_success=%zu malloc_real_fallback=%zu "
          "malloc_midpage_boundary_attempt=%zu "
          "malloc_midpage_boundary_hit=%zu "
          "malloc_midpage_boundary_fallback=%zu "
          "calloc_calls=%zu free_calls=%zu free_null=%zu "
          "free_reentry_real=%zu free_local_route_valid=%zu "
          "free_visible_route_hit=%zu free_toy_active_map_hit=%zu "
          "free_midpage_active_map_hit=%zu "
          "free_route_invalid=%zu free_route_miss_real=%zu "
          "free_prechecked_candidate=%zu "
          "realloc_calls=%zu realloc_owned=%zu realloc_in_place=%zu "
          "realloc_real_fallback=%zu realloc_copy_bytes=%zu "
          "malloc_usable_size_calls=%zu "
          "malloc_usable_size_owned=%zu "
          "malloc_usable_size_real_fallback=%zu\n",
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.malloc_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_hz6_success),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_real_fallback),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_attempt),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_fallback),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.calloc_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_null),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_reentry_real),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_local_route_valid),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_visible_route_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_toy_active_map_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_midpage_active_map_hit),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_route_invalid),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_route_miss_real),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_prechecked_candidate),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_owned),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_in_place),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_real_fallback),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_copy_bytes),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_owned),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_real_fallback));

  fprintf(stderr,
          "[HZ6_PRELOAD_HOOK_DETAIL] free_toy_active_map_attempt=%zu free_toy_active_map_hit=%zu "
          "free_toy_active_map_miss=%zu free_midpage_active_map_attempt=%zu "
          "free_midpage_active_map_hit=%zu free_midpage_active_map_miss=%zu "
          "free_route_lookup_after_maps=%zu free_route_valid_owned=%zu "
          "free_route_valid_foreign_visible=%zu free_route_invalid=%zu "
          "free_route_miss_real=%zu mh_probe=%zu mh_would=%zu "
          "mh_true=%zu mh_false=%zu mh_miss=%zu\n",
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_miss),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_miss),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_lookup_after_maps),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_valid_owned),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_valid_foreign_visible),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_invalid),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_miss_real), HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_probe),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_would_first), HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_true_midpage),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_false_positive), HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_missed_midpage));
  fprintf(stderr,
          "[HZ6_PRELOAD_SIZE_STATS] "
          "malloc_size_zero=%zu malloc_size_le1024=%zu "
          "malloc_size_1025_4096=%zu malloc_size_4097_16384=%zu "
          "malloc_size_gt16384=%zu "
          "realloc_request_zero=%zu realloc_request_le1024=%zu "
          "realloc_request_1025_4096=%zu "
          "realloc_request_4097_16384=%zu "
          "realloc_request_gt16384=%zu "
          "realloc_owned_old_zero=%zu "
          "realloc_owned_old_le1024=%zu "
          "realloc_owned_old_1025_4096=%zu "
          "realloc_owned_old_4097_16384=%zu "
          "realloc_owned_old_gt16384=%zu "
          "realloc_copy_calls=%zu\n",
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_copy_calls));

  if (print_per_allocator) {
    pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
    for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
      Hz6Allocator* allocator = g_hz6_preload_allocator_registry[i];
      if (!allocator) {
        continue;
      }
      Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
      fprintf(stderr,
              "[HZ6_PRELOAD_ALLOCATOR_STATS] index=%zu allocator=%p "
              "route_valid=%zu route_invalid=%zu route_miss=%zu "
              "transfer_push=%zu transfer_pop=%zu source_alloc=%zu "
              "toy_source_alloc=%zu midpage_source_alloc=%zu "
              "large_source_alloc=%zu local2p_source_alloc=%zu "
              "alloc_fail=%zu descriptor_exhausted=%zu "
              "route_register_fail=%zu source_block_exhausted=%zu\n",
              i, (void*)allocator, stats.route_valid, stats.route_invalid,
              stats.route_miss, stats.transfer_push, stats.transfer_pop,
              stats.source_alloc, stats.toy_source_alloc,
              stats.midpage_source_alloc, stats.large_source_alloc,
              stats.local2p_source_alloc, stats.alloc_fail,
              stats.descriptor_exhausted, stats.route_register_fail,
              stats.source_block_exhausted);
    }
    pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
  }

  g_hz6_preload_reentry = saved_reentry;
}

__attribute__((destructor)) static void hz6_preload_on_unload(void) {
  hz6_preload_print_stats();
}
