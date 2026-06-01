#ifndef HZ6_H
#define HZ6_H

#include "hz6_contract.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

typedef struct Hz6StatsSnapshot {
  size_t route_valid;
  size_t route_invalid;
  size_t route_miss;
  size_t route_visibility_lookup;
  size_t route_visibility_hit;
  size_t route_visibility_hit_local_owner;
  size_t route_visibility_hit_foreign_owner;
  size_t route_visibility_miss;
  size_t route_visibility_probe_total;
  size_t route_visibility_probe_max;
  size_t transfer_push;
  size_t transfer_pop;
  size_t transfer_current;
  size_t transfer_current_max;
  size_t remote_free_attempt;
  size_t remote_free_strict_owner_block;
  size_t remote_free_transfer_fail;
  size_t route_rehome_attempt;
  size_t route_rehome_success;
  size_t route_rehome_fail;
  size_t source_owned_prepare;
  size_t source_owned_route_hit_local_owner;
  size_t source_owned_visibility_hit_local_owner;
  size_t source_owned_visibility_hit_foreign_owner;
  size_t source_owned_remote_free_attempt;
  size_t source_owned_release;
  size_t source_alloc;
  size_t frontcache_reuse_hit;
  size_t frontcache_reuse_invalid;
  size_t transfer_reuse_hit;
  size_t transfer_reuse_invalid;
  size_t source_refill_starvation;
  size_t source_refill_saturation;
  size_t source_refill_boost;
  size_t source_refill_clamp;
  size_t source_admission_open;
  size_t source_admission_boosted;
  size_t source_admission_clamped;
  size_t source_prefill_attempt;
  size_t source_prefill_filled;
  size_t source_prefill_fallback;
  size_t front_source_prefill_attempt[HZ6_FRONT_ATTR_COUNT];
  size_t front_source_prefill_filled[HZ6_FRONT_ATTR_COUNT];
  size_t front_source_prefill_fallback[HZ6_FRONT_ATTR_COUNT];
  size_t front_source_ops_alloc;
  size_t front_source_slot_alloc;
  size_t front_source_prefill_alloc;
  size_t toy_source_prefill_call;
  size_t local2p_source_alloc;
  size_t midpage_source_alloc;
  size_t large_source_alloc;
  size_t toy_source_alloc;
  size_t front_alloc_path[HZ6_FRONT_ATTR_COUNT][HZ6_ALLOC_PATH_COUNT];
  size_t alloc_fail;
  size_t descriptor_exhausted;
  size_t route_register_fail;
  size_t source_block_exhausted;
  size_t route_active_current;
  size_t route_active_max;
  size_t descriptor_probe_total;
  size_t descriptor_probe_max;
  size_t descriptor_fail_active_max;
  size_t descriptor_fail_local_free_max;
  size_t descriptor_fail_transfer_free_max;
  size_t descriptor_fail_remote_pending_max;
  size_t descriptor_fail_central_free_max;
  size_t descriptor_fail_released_max;
  size_t descriptor_fail_orphan_max;
  size_t descriptor_fail_dead_with_ptr_max;
  size_t descriptor_fail_frontcache_total_max;
  size_t descriptor_fail_frontcache_largest_bin_max;
  size_t descriptor_fail_frontcache_nonempty_bins_max;
  size_t frontcache_spill_dryrun_calls;
  size_t frontcache_spill_dryrun_requested_empty;
  size_t frontcache_spill_dryrun_candidate_calls;
  size_t frontcache_spill_dryrun_reclaimable_total;
  size_t frontcache_spill_dryrun_largest_donor_max;
  size_t frontcache_spill_dryrun_donor_bins_max;
  size_t frontcache_spill_attempt;
  size_t frontcache_spill_success;
  size_t frontcache_spill_no_candidate;
  size_t frontcache_spill_invalid;
  size_t frontcache_spill_retry_success;
  size_t route_lookup_probe_total;
  size_t route_lookup_probe_max;
  size_t route_register_probe_total;
  size_t route_register_probe_max;
  size_t route_unregister_probe_total;
  size_t route_unregister_probe_max;
  size_t source_block_probe_total;
  size_t source_block_probe_max;
  size_t source_block_fail_active_max;
  size_t source_block_fail_registered_max;
  size_t source_block_fail_ref_nonzero_max;
  size_t source_block_fail_ref_zero_max;
  size_t large_span_central_push;
  size_t large_span_central_pop;
  size_t large_span_source_alloc;
} Hz6StatsSnapshot;

void hz6_allocator_init(Hz6Allocator* allocator);

void hz6_allocator_destroy(Hz6Allocator* allocator);

void* hz6_malloc(Hz6Allocator* allocator, size_t size);

void hz6_free(Hz6Allocator* allocator, void* ptr);

int hz6_free_remote(Hz6Allocator* allocator, void* ptr);

int hz6_owns(Hz6Allocator* allocator, const void* ptr);

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
