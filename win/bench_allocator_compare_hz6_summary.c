#include "bench_allocator_compare_hz6_summary.h"

#include <stdio.h>

#if defined(HZ_BENCH_USE_HZ6)
void bench_print_hz6_summary(const Hz6StatsSnapshot* hz6_stats,
                             size_t hz6_pre_free_owns_false,
                             size_t hz6_duplicate_alloc_ptr,
                             size_t peak_kb) {
    printf(" hz6_route_valid=%zu hz6_route_invalid=%zu hz6_route_miss=%zu "
           "hz6_transfer_push=%zu hz6_transfer_pop=%zu "
           "hz6_source_alloc=%zu hz6_alloc_fail=%zu "
           "hz6_descriptor_exhausted=%zu hz6_route_register_fail=%zu "
           "hz6_source_block_exhausted=%zu hz6_descriptor_probe_total=%zu "
           "hz6_descriptor_probe_max=%zu "
           "hz6_frontcache_reuse_hit=%zu "
           "hz6_frontcache_reuse_invalid=%zu "
           "hz6_transfer_reuse_hit=%zu "
           "hz6_transfer_reuse_invalid=%zu "
           "hz6_route_visibility_lookup=%zu "
           "hz6_route_visibility_hit=%zu "
           "hz6_route_visibility_hit_local_owner=%zu "
           "hz6_route_visibility_hit_foreign_owner=%zu "
           "hz6_route_visibility_miss=%zu "
           "hz6_route_visibility_probe_total=%zu "
           "hz6_route_visibility_probe_max=%zu "
           "hz6_lifecycle_owner_mismatch=%zu "
           "hz6_lifecycle_foreign_free_attempt=%zu "
           "hz6_lifecycle_foreign_free_handled=%zu "
           "hz6_lifecycle_foreign_free_invalid=%zu "
           "hz6_free_invalid_route_kind_invalid=%zu "
           "hz6_free_invalid_no_front=%zu "
           "hz6_free_invalid_local_owner=%zu "
           "hz6_free_invalid_remote_owner=%zu "
           "hz6_free_invalid_same_owner_fast=%zu "
           "hz6_free_invalid_local_cache_direct=%zu "
           "hz6_free_invalid_front_tagged=%zu "
           "hz6_free_invalid_remote_tagged=%zu "
           "hz6_activation_route_repair_attempt=%zu "
           "hz6_activation_route_repair_success=%zu "
           "hz6_activation_route_repair_fail=%zu "
           "hz6_activation_route_repair_conflict=%zu "
           "hz6_source_block_release_live_guard=%zu "
           "hz6_source_block_release_live_descriptors_max=%zu "
           "hz6_shared_dir_lookup=%zu "
           "hz6_shared_dir_hit=%zu "
           "hz6_shared_dir_miss=%zu "
           "hz6_shared_dir_stale=%zu "
           "hz6_shared_dir_hit_local_allocator=%zu "
           "hz6_shared_dir_hit_foreign_allocator=%zu "
           "hz6_shared_dir_would_skip_local=%zu "
           "hz6_shared_dir_register=%zu "
           "hz6_shared_dir_unregister=%zu "
           "hz6_shared_dir_probe_total=%zu "
           "hz6_shared_dir_probe_max=%zu "
           "hz6_shared_dir_first_attempt=%zu "
           "hz6_shared_dir_first_hit=%zu "
           "hz6_shared_dir_first_fallback=%zu "
           "hz6_shared_dir_first_invalid=%zu "
           "hz6_elastic_route_overflow_register=%zu "
           "hz6_elastic_route_overflow_register_fail=%zu "
           "hz6_elastic_route_overflow_lookup=%zu "
           "hz6_elastic_route_overflow_hit=%zu "
           "hz6_source_owned_prepare=%zu "
           "hz6_source_owned_route_hit_local_owner=%zu "
           "hz6_source_owned_visibility_hit_local_owner=%zu "
           "hz6_source_owned_visibility_hit_foreign_owner=%zu "
           "hz6_source_owned_remote_free_attempt=%zu "
           "hz6_source_owned_release=%zu "
           "hz6_route_register_reason_unknown=%zu "
           "hz6_route_register_reason_source_run_slot=%zu "
           "hz6_route_register_reason_direct_source=%zu "
           "hz6_route_register_reason_materialize=%zu "
           "hz6_route_register_reason_rehome=%zu "
           "hz6_route_unregister_reason_unknown=%zu "
           "hz6_route_unregister_reason_frontcache_overflow=%zu "
           "hz6_route_unregister_reason_cap_release=%zu "
           "hz6_route_unregister_reason_descriptorless_detach=%zu "
           "hz6_route_unregister_reason_source_slot_release=%zu "
           "hz6_route_unregister_reason_rehome=%zu "
           "hz6_descriptor_fail_active_max=%zu "
           "hz6_descriptor_fail_local_free_max=%zu "
           "hz6_descriptor_fail_transfer_free_max=%zu "
           "hz6_descriptor_fail_remote_pending_max=%zu "
           "hz6_descriptor_fail_central_free_max=%zu "
           "hz6_descriptor_fail_released_max=%zu "
           "hz6_descriptor_fail_orphan_max=%zu "
           "hz6_descriptor_fail_dead_with_ptr_max=%zu "
           "hz6_descriptor_fail_frontcache_total_max=%zu "
           "hz6_descriptor_fail_frontcache_largest_bin_max=%zu "
           "hz6_descriptor_fail_frontcache_nonempty_bins_max=%zu "
           "hz6_descriptor_frontcache_reuse_dryrun_calls=%zu "
           "hz6_descriptor_frontcache_reuse_requested_nonempty=%zu "
           "hz6_descriptor_frontcache_reuse_requested_total=%zu "
           "hz6_descriptor_frontcache_reuse_donor_total=%zu "
           "hz6_descriptor_frontcache_reuse_largest_donor_max=%zu "
           "hz6_descriptor_frontcache_reuse_donor_bins_max=%zu "
           "hz6_descriptorless_frontcache_push=%zu "
           "hz6_descriptorless_frontcache_pop=%zu "
           "hz6_descriptorless_frontcache_descriptor_fail=%zu "
           "hz6_descriptorless_frontcache_route_fail=%zu "
           "hz6_descriptorless_frontcache_invalid=%zu "
           "hz6_descriptorreserve_frontcache_push=%zu "
           "hz6_descriptorreserve_frontcache_pop=%zu "
           "hz6_descriptorreserve_frontcache_missing=%zu "
           "hz6_descriptorreserve_frontcache_invalid=%zu "
           "hz6_descgov_trigger_descriptor_fail=%zu "
           "hz6_descgov_detach_attempt=%zu "
           "hz6_descgov_detach_success=%zu "
           "hz6_descgov_detach_budget_denied=%zu "
           "hz6_descgov_detach_class_denied=%zu "
           "hz6_descgov_materialize_admit=%zu "
           "hz6_descgov_materialize_block_no_descriptor=%zu "
           "hz6_descgov_materialize_fail=%zu "
           "hz6_descgov_detached_current=%zu "
           "hz6_descgov_detached_max=%zu "
           "hz6_frontcache_spill_dryrun_calls=%zu "
           "hz6_frontcache_spill_dryrun_requested_empty=%zu "
           "hz6_frontcache_spill_dryrun_candidate_calls=%zu "
           "hz6_frontcache_spill_dryrun_reclaimable_total=%zu "
           "hz6_frontcache_spill_dryrun_largest_donor_max=%zu "
           "hz6_frontcache_spill_dryrun_donor_bins_max=%zu "
           "hz6_frontcache_spill_attempt=%zu "
           "hz6_frontcache_spill_success=%zu "
           "hz6_frontcache_spill_no_candidate=%zu "
           "hz6_frontcache_spill_invalid=%zu "
           "hz6_frontcache_spill_retry_success=%zu "
           "hz6_frontcache_borrow_dryrun_calls=%zu "
           "hz6_frontcache_borrow_dryrun_candidate_calls=%zu "
           "hz6_frontcache_borrow_dryrun_candidate_total=%zu "
           "hz6_frontcache_borrow_dryrun_largest_candidate_max=%zu "
           "hz6_frontcache_borrow_attempt=%zu "
           "hz6_frontcache_borrow_success=%zu "
           "hz6_frontcache_borrow_no_candidate=%zu "
           "hz6_frontcache_borrow_invalid=%zu "
           "hz6_frontcache_cap_dryrun_push=%zu "
           "hz6_frontcache_cap_dryrun_over_cap=%zu "
           "hz6_frontcache_cap_dryrun_would_release=%zu "
           "hz6_frontcache_cap_dryrun_soft_cap_max=%zu "
           "hz6_frontcache_cap_dryrun_bin_count_max=%zu "
           "hz6_frontcache_cap_release=%zu "
           "hz6_source_run_reuse_dryrun_calls=%zu "
           "hz6_source_run_reuse_dryrun_candidate_calls=%zu "
           "hz6_source_run_reuse_dryrun_candidate_blocks_total=%zu "
           "hz6_source_run_reuse_dryrun_free_slots_total=%zu "
           "hz6_source_run_reuse_dryrun_largest_free_slots_max=%zu "
           "hz6_source_run_reuse_attempt=%zu "
           "hz6_source_run_reuse_candidate=%zu "
           "hz6_source_run_reuse_hit=%zu "
           "hz6_source_run_reuse_miss_no_block=%zu "
           "hz6_source_run_reuse_miss_no_slot=%zu "
           "hz6_source_run_reuse_reserved=%zu "
           "hz6_source_run_reuse_slot_fail=%zu "
           "hz6_source_run_reuse_descriptor_fail=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_attempt=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_success=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_no_candidate=%zu "
           "hz6_source_run_reuse_same_class_reclaim_attempt=%zu "
           "hz6_source_run_reuse_same_class_reclaim_success=%zu "
           "hz6_source_run_reuse_same_class_reclaim_no_candidate=%zu "
           "hz6_source_run_reuse_route_fail=%zu "
           "hz6_source_run_reuse_prepare_fail=%zu "
           "hz6_source_run_reuse_rollback=%zu "
           "hz6_source_run_reuse_used_count_mismatch=%zu "
           "hz6_source_block_route_dryrun_attempt=%zu "
           "hz6_source_block_route_block_hit=%zu "
           "hz6_source_block_route_slot_hit=%zu "
           "hz6_source_block_route_descriptor_hit=%zu "
           "hz6_source_block_route_miss_no_block=%zu "
           "hz6_source_block_route_invalid_alignment=%zu "
           "hz6_source_block_route_invalid_unused=%zu "
           "hz6_source_block_route_descriptor_miss=%zu "
           "hz6_source_block_route_class_mismatch=%zu "
           "hz6_source_block_route_probe_total=%zu "
           "hz6_source_block_route_probe_max=%zu "
           "hz6_source_block_route_descriptor_map_hit=%zu "
           "hz6_source_block_route_descriptor_map_miss=%zu "
           "hz6_source_block_route_descriptor_map_stale=%zu "
           "hz6_source_block_route_descriptor_map_set=%zu "
           "hz6_source_block_route_descriptor_map_clear=%zu "
           "hz6_source_block_route_range_index_register=%zu "
           "hz6_source_block_route_range_index_unregister=%zu "
           "hz6_source_block_route_range_index_register_fail=%zu "
           "hz6_source_block_route_range_index_lookup=%zu "
           "hz6_source_block_route_range_index_hit=%zu "
           "hz6_source_block_route_range_index_miss=%zu "
           "hz6_source_block_route_range_index_stale=%zu "
           "hz6_source_block_route_range_index_probe_total=%zu "
           "hz6_source_block_route_range_index_probe_max=%zu",
           hz6_stats->route_valid, hz6_stats->route_invalid,
           hz6_stats->route_miss, hz6_stats->transfer_push,
           hz6_stats->transfer_pop, hz6_stats->source_alloc,
           hz6_stats->alloc_fail, hz6_stats->descriptor_exhausted,
           hz6_stats->route_register_fail, hz6_stats->source_block_exhausted,
           hz6_stats->descriptor_probe_total,
           hz6_stats->descriptor_probe_max, hz6_stats->frontcache_reuse_hit,
           hz6_stats->frontcache_reuse_invalid,
           hz6_stats->transfer_reuse_hit, hz6_stats->transfer_reuse_invalid,
           hz6_stats->route_visibility_lookup, hz6_stats->route_visibility_hit,
           hz6_stats->route_visibility_hit_local_owner,
           hz6_stats->route_visibility_hit_foreign_owner,
           hz6_stats->route_visibility_miss,
           hz6_stats->route_visibility_probe_total,
           hz6_stats->route_visibility_probe_max,
           hz6_stats->lifecycle_owner_mismatch,
           hz6_stats->lifecycle_foreign_free_attempt,
           hz6_stats->lifecycle_foreign_free_handled,
           hz6_stats->lifecycle_foreign_free_invalid,
           hz6_stats->free_invalid_route_kind_invalid,
           hz6_stats->free_invalid_no_front, hz6_stats->free_invalid_local_owner,
           hz6_stats->free_invalid_remote_owner,
           hz6_stats->free_invalid_same_owner_fast,
           hz6_stats->free_invalid_local_cache_direct,
           hz6_stats->free_invalid_front_tagged,
           hz6_stats->free_invalid_remote_tagged,
           hz6_stats->activation_route_repair_attempt,
           hz6_stats->activation_route_repair_success,
           hz6_stats->activation_route_repair_fail,
           hz6_stats->activation_route_repair_conflict,
           hz6_stats->source_block_release_live_guard,
           hz6_stats->source_block_release_live_descriptors_max,
           hz6_stats->shared_dir_lookup, hz6_stats->shared_dir_hit,
           hz6_stats->shared_dir_miss, hz6_stats->shared_dir_stale,
           hz6_stats->shared_dir_hit_local_allocator,
           hz6_stats->shared_dir_hit_foreign_allocator,
           hz6_stats->shared_dir_would_skip_local,
           hz6_stats->shared_dir_register, hz6_stats->shared_dir_unregister,
           hz6_stats->shared_dir_probe_total, hz6_stats->shared_dir_probe_max,
           hz6_stats->shared_dir_first_attempt, hz6_stats->shared_dir_first_hit,
           hz6_stats->shared_dir_first_fallback,
           hz6_stats->shared_dir_first_invalid,
           hz6_stats->elastic_route_overflow_register,
           hz6_stats->elastic_route_overflow_register_fail,
           hz6_stats->elastic_route_overflow_lookup,
           hz6_stats->elastic_route_overflow_hit,
           hz6_stats->source_owned_prepare,
           hz6_stats->source_owned_route_hit_local_owner,
           hz6_stats->source_owned_visibility_hit_local_owner,
           hz6_stats->source_owned_visibility_hit_foreign_owner,
           hz6_stats->source_owned_remote_free_attempt,
           hz6_stats->source_owned_release,
           hz6_stats->route_register_reason_unknown,
           hz6_stats->route_register_reason_source_run_slot,
           hz6_stats->route_register_reason_direct_source,
           hz6_stats->route_register_reason_materialize,
           hz6_stats->route_register_reason_rehome,
           hz6_stats->route_unregister_reason_unknown,
           hz6_stats->route_unregister_reason_frontcache_overflow,
           hz6_stats->route_unregister_reason_cap_release,
           hz6_stats->route_unregister_reason_descriptorless_detach,
           hz6_stats->route_unregister_reason_source_slot_release,
           hz6_stats->route_unregister_reason_rehome,
           hz6_stats->descriptor_fail_active_max,
           hz6_stats->descriptor_fail_local_free_max,
           hz6_stats->descriptor_fail_transfer_free_max,
           hz6_stats->descriptor_fail_remote_pending_max,
           hz6_stats->descriptor_fail_central_free_max,
           hz6_stats->descriptor_fail_released_max,
           hz6_stats->descriptor_fail_orphan_max,
           hz6_stats->descriptor_fail_dead_with_ptr_max,
           hz6_stats->descriptor_fail_frontcache_total_max,
           hz6_stats->descriptor_fail_frontcache_largest_bin_max,
           hz6_stats->descriptor_fail_frontcache_nonempty_bins_max,
           hz6_stats->descriptor_frontcache_reuse_dryrun_calls,
           hz6_stats->descriptor_frontcache_reuse_requested_nonempty,
           hz6_stats->descriptor_frontcache_reuse_requested_total,
           hz6_stats->descriptor_frontcache_reuse_donor_total,
           hz6_stats->descriptor_frontcache_reuse_largest_donor_max,
           hz6_stats->descriptor_frontcache_reuse_donor_bins_max,
           hz6_stats->descriptorless_frontcache_push,
           hz6_stats->descriptorless_frontcache_pop,
           hz6_stats->descriptorless_frontcache_descriptor_fail,
           hz6_stats->descriptorless_frontcache_route_fail,
           hz6_stats->descriptorless_frontcache_invalid,
           hz6_stats->descriptorreserve_frontcache_push,
           hz6_stats->descriptorreserve_frontcache_pop,
           hz6_stats->descriptorreserve_frontcache_missing,
           hz6_stats->descriptorreserve_frontcache_invalid,
           hz6_stats->descgov_trigger_descriptor_fail,
           hz6_stats->descgov_detach_attempt,
           hz6_stats->descgov_detach_success,
           hz6_stats->descgov_detach_budget_denied,
           hz6_stats->descgov_detach_class_denied,
           hz6_stats->descgov_materialize_admit,
           hz6_stats->descgov_materialize_block_no_descriptor,
           hz6_stats->descgov_materialize_fail,
           hz6_stats->descgov_detached_current,
           hz6_stats->descgov_detached_max,
           hz6_stats->frontcache_spill_dryrun_calls,
           hz6_stats->frontcache_spill_dryrun_requested_empty,
           hz6_stats->frontcache_spill_dryrun_candidate_calls,
           hz6_stats->frontcache_spill_dryrun_reclaimable_total,
           hz6_stats->frontcache_spill_dryrun_largest_donor_max,
           hz6_stats->frontcache_spill_dryrun_donor_bins_max,
           hz6_stats->frontcache_spill_attempt,
           hz6_stats->frontcache_spill_success,
           hz6_stats->frontcache_spill_no_candidate,
           hz6_stats->frontcache_spill_invalid,
           hz6_stats->frontcache_spill_retry_success,
           hz6_stats->frontcache_borrow_dryrun_calls,
           hz6_stats->frontcache_borrow_dryrun_candidate_calls,
           hz6_stats->frontcache_borrow_dryrun_candidate_total,
           hz6_stats->frontcache_borrow_dryrun_largest_candidate_max,
           hz6_stats->frontcache_borrow_attempt,
           hz6_stats->frontcache_borrow_success,
           hz6_stats->frontcache_borrow_no_candidate,
           hz6_stats->frontcache_borrow_invalid,
           hz6_stats->frontcache_cap_dryrun_push,
           hz6_stats->frontcache_cap_dryrun_over_cap,
           hz6_stats->frontcache_cap_dryrun_would_release,
           hz6_stats->frontcache_cap_dryrun_soft_cap_max,
           hz6_stats->frontcache_cap_dryrun_bin_count_max,
           hz6_stats->frontcache_cap_release,
           hz6_stats->source_run_reuse_dryrun_calls,
           hz6_stats->source_run_reuse_dryrun_candidate_calls,
           hz6_stats->source_run_reuse_dryrun_candidate_blocks_total,
           hz6_stats->source_run_reuse_dryrun_free_slots_total,
           hz6_stats->source_run_reuse_dryrun_largest_free_slots_max,
           hz6_stats->source_run_reuse_attempt,
           hz6_stats->source_run_reuse_candidate,
           hz6_stats->source_run_reuse_hit,
           hz6_stats->source_run_reuse_miss_no_block,
           hz6_stats->source_run_reuse_miss_no_slot,
           hz6_stats->source_run_reuse_reserved,
           hz6_stats->source_run_reuse_slot_fail,
           hz6_stats->source_run_reuse_descriptor_fail,
           hz6_stats->source_run_reuse_descriptor_reclaim_attempt,
           hz6_stats->source_run_reuse_descriptor_reclaim_success,
           hz6_stats->source_run_reuse_descriptor_reclaim_no_candidate,
           hz6_stats->source_run_reuse_same_class_reclaim_attempt,
           hz6_stats->source_run_reuse_same_class_reclaim_success,
           hz6_stats->source_run_reuse_same_class_reclaim_no_candidate,
           hz6_stats->source_run_reuse_route_fail,
           hz6_stats->source_run_reuse_prepare_fail,
           hz6_stats->source_run_reuse_rollback,
           hz6_stats->source_run_reuse_used_count_mismatch,
           hz6_stats->source_block_route_dryrun_attempt,
           hz6_stats->source_block_route_block_hit,
           hz6_stats->source_block_route_slot_hit,
           hz6_stats->source_block_route_descriptor_hit,
           hz6_stats->source_block_route_miss_no_block,
           hz6_stats->source_block_route_invalid_alignment,
           hz6_stats->source_block_route_invalid_unused,
           hz6_stats->source_block_route_descriptor_miss,
           hz6_stats->source_block_route_class_mismatch,
           hz6_stats->source_block_route_probe_total,
           hz6_stats->source_block_route_probe_max,
           hz6_stats->source_block_route_descriptor_map_hit,
           hz6_stats->source_block_route_descriptor_map_miss,
           hz6_stats->source_block_route_descriptor_map_stale,
           hz6_stats->source_block_route_descriptor_map_set,
           hz6_stats->source_block_route_descriptor_map_clear,
           hz6_stats->source_block_route_range_index_register,
           hz6_stats->source_block_route_range_index_unregister,
           hz6_stats->source_block_route_range_index_register_fail,
           hz6_stats->source_block_route_range_index_lookup,
           hz6_stats->source_block_route_range_index_hit,
           hz6_stats->source_block_route_range_index_miss,
           hz6_stats->source_block_route_range_index_stale,
           hz6_stats->source_block_route_range_index_probe_total,
           hz6_stats->source_block_route_range_index_probe_max);
#if HZ6_DIAGNOSTIC_PROBES
    printf(" hz6_route_lookup_probe_total=%zu "
           "hz6_route_lookup_probe_max=%zu "
           "hz6_route_register_probe_total=%zu "
           "hz6_route_register_probe_max=%zu "
           "hz6_route_unregister_probe_total=%zu "
           "hz6_route_unregister_probe_max=%zu "
           "hz6_source_block_probe_total=%zu "
           "hz6_source_block_probe_max=%zu "
           "hz6_source_block_fail_active_max=%zu "
           "hz6_source_block_fail_registered_max=%zu "
           "hz6_source_block_fail_ref_nonzero_max=%zu "
           "hz6_source_block_fail_ref_zero_max=%zu "
           "hz6_toy_small_malloc_fast_attempt=%zu "
           "hz6_toy_small_malloc_fast_hit=%zu "
           "hz6_toy_small_malloc_front_dispatch=%zu "
           "hz6_toy_small_free_route_lookup=%zu "
           "hz6_toy_small_free_owner_equal=%zu "
           "hz6_toy_small_free_fast_hit=%zu "
           "hz6_toy_small_free_cache_push=%zu "
           "hz6_toy_small_activate_descriptor=%zu "
           "hz6_toy_small_active_map_register=%zu "
           "hz6_toy_small_active_map_register_collision=%zu "
           "hz6_toy_small_active_map_free_attempt=%zu "
           "hz6_toy_small_active_map_free_hit=%zu "
           "hz6_toy_small_active_map_free_miss=%zu "
           "hz6_toy_small_active_map_free_stale=%zu "
           "hz6_toy_small_active_map_free_cache_fail=%zu "
           "hz6_toy_small_active_map_route_bypass=%zu "
           "hz6_large_central_push=%zu hz6_large_central_pop=%zu "
           "hz6_large_source_alloc=%zu",
           hz6_stats->route_lookup_probe_total,
           hz6_stats->route_lookup_probe_max,
           hz6_stats->route_register_probe_total,
           hz6_stats->route_register_probe_max,
           hz6_stats->route_unregister_probe_total,
           hz6_stats->route_unregister_probe_max,
           hz6_stats->source_block_probe_total,
           hz6_stats->source_block_probe_max,
           hz6_stats->source_block_fail_active_max,
           hz6_stats->source_block_fail_registered_max,
           hz6_stats->source_block_fail_ref_nonzero_max,
           hz6_stats->source_block_fail_ref_zero_max,
           hz6_stats->toy_small_malloc_fast_attempt,
           hz6_stats->toy_small_malloc_fast_hit,
           hz6_stats->toy_small_malloc_front_dispatch,
           hz6_stats->toy_small_free_route_lookup,
           hz6_stats->toy_small_free_owner_equal,
           hz6_stats->toy_small_free_fast_hit,
           hz6_stats->toy_small_free_cache_push,
           hz6_stats->toy_small_activate_descriptor,
           hz6_stats->toy_small_active_map_register,
           hz6_stats->toy_small_active_map_register_collision,
           hz6_stats->toy_small_active_map_free_attempt,
           hz6_stats->toy_small_active_map_free_hit,
           hz6_stats->toy_small_active_map_free_miss,
           hz6_stats->toy_small_active_map_free_stale,
           hz6_stats->toy_small_active_map_free_cache_fail,
           hz6_stats->toy_small_active_map_route_bypass,
           hz6_stats->large_span_central_push,
           hz6_stats->large_span_central_pop,
           hz6_stats->large_span_source_alloc);
#if defined(HZ_BENCH_TRACE_LAST_OP)
    printf(" hz6_pre_free_owns_false=%zu hz6_duplicate_alloc_ptr=%zu",
           hz6_pre_free_owns_false, hz6_duplicate_alloc_ptr);
#endif
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
        size_t push = hz6_stats->frontcache_push_by_class[class_id];
        size_t pop_empty = hz6_stats->frontcache_pop_empty_by_class[class_id];
        if (push == 0 && pop_empty == 0) {
            continue;
        }
        printf("\n[HZ6_FRONTCACHE_CLASS] class=%zu push=%zu pop_empty=%zu",
               class_id, push, pop_empty);
    }
    printf("\n[HZ6_ROUTE_PROBE_SHAPE] kind=lookup b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu\n",
           hz6_stats->route_lookup_probe_hist[0],
           hz6_stats->route_lookup_probe_hist[1],
           hz6_stats->route_lookup_probe_hist[2],
           hz6_stats->route_lookup_probe_hist[3],
           hz6_stats->route_lookup_probe_hist[4],
           hz6_stats->route_lookup_probe_hist[5]);
    printf("[HZ6_ROUTE_PROBE_SHAPE] kind=register b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu\n",
           hz6_stats->route_register_probe_hist[0],
           hz6_stats->route_register_probe_hist[1],
           hz6_stats->route_register_probe_hist[2],
           hz6_stats->route_register_probe_hist[3],
           hz6_stats->route_register_probe_hist[4],
           hz6_stats->route_register_probe_hist[5]);
    printf("[HZ6_ROUTE_PROBE_SHAPE] kind=unregister b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu",
           hz6_stats->route_unregister_probe_hist[0],
           hz6_stats->route_unregister_probe_hist[1],
           hz6_stats->route_unregister_probe_hist[2],
           hz6_stats->route_unregister_probe_hist[3],
           hz6_stats->route_unregister_probe_hist[4],
           hz6_stats->route_unregister_probe_hist[5]);
#endif
    printf(" peak_kb=%zu\n", peak_kb);
}
#endif
