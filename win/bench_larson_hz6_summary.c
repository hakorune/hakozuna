#include "bench_larson_hz6_summary.h"

#include <stdio.h>

#if defined(HZ_BENCH_USE_HZ6)
void bench_print_larson_hz6_summary(const Hz6StatsSnapshot* hz6_stats) {
    printf("[HZ6_STATS] label=%s route_valid=%zu route_invalid=%zu route_miss=%zu "
           "route_visibility_lookup=%zu route_visibility_hit=%zu "
           "route_visibility_hit_local_owner=%zu "
           "route_visibility_hit_foreign_owner=%zu "
           "route_visibility_miss=%zu route_visibility_probe_total=%zu "
           "route_visibility_probe_max=%zu "
           "route_exact_lookup_probe_total=%zu "
           "route_exact_lookup_probe_max=%zu "
           "owner_locality_lookup=%zu "
           "owner_locality_hit_local_allocator=%zu "
           "owner_locality_hit_foreign_allocator=%zu "
           "owner_locality_miss=%zu "
           "owner_locality_register=%zu "
           "owner_locality_unregister=%zu "
           "owner_locality_probe_total=%zu "
           "owner_locality_probe_max=%zu "
           "transfer_push=%zu transfer_pop=%zu transfer_current=%zu "
           "transfer_current_max=%zu remote_free_attempt=%zu "
           "remote_free_strict_owner_block=%zu remote_free_transfer_fail=%zu "
           "route_rehome_attempt=%zu route_rehome_success=%zu "
           "route_rehome_fail=%zu "
           "descriptor_source_route_allocator_match=%zu "
           "descriptor_source_route_allocator_mismatch=%zu "
           "descriptor_source_current_allocator_match=%zu "
           "descriptor_source_current_allocator_mismatch=%zu "
           "descriptor_storage_lookup=%zu "
           "descriptor_storage_hit=%zu "
           "descriptor_storage_miss=%zu "
           "descriptor_storage_probe_total=%zu "
           "descriptor_storage_probe_max=%zu "
           "descriptor_storage_route_allocator_match=%zu "
           "descriptor_storage_route_allocator_mismatch=%zu "
           "descriptor_storage_current_allocator_match=%zu "
           "descriptor_storage_current_allocator_mismatch=%zu "
           "owner_source_side_meta_lookup=%zu "
           "owner_source_side_meta_local=%zu "
           "owner_source_side_meta_foreign=%zu "
           "owner_source_side_meta_miss=%zu "
           "owner_source_side_meta_probe_total=%zu "
           "owner_source_side_meta_probe_max=%zu "
           "owner_source_side_meta_l2_lookup=%zu "
           "owner_source_side_meta_l2_hit=%zu "
           "owner_source_side_meta_l2_miss_no_block=%zu "
           "owner_source_side_meta_l2_miss_inactive=%zu "
           "owner_source_side_meta_l2_storage_mismatch=%zu "
           "lifecycle_owner_mismatch=%zu "
           "lifecycle_foreign_free_attempt=%zu "
           "lifecycle_foreign_free_handled=%zu "
           "lifecycle_foreign_free_invalid=%zu "
           "visible_first_attempt=%zu visible_first_hit=%zu "
           "visible_first_miss=%zu visible_first_visible_invalid=%zu "
           "visible_first_local_fallback=%zu "
           "visible_first_local_fallback_invalid=%zu "
           "visible_first_local_lookup_skipped=%zu "
           "negative_filter_attempt=%zu negative_filter_not_armed=%zu "
           "negative_filter_rehome_blocked=%zu "
           "negative_filter_skip_local=%zu "
           "negative_filter_maybe_local=%zu "
           "negative_filter_shadow_false_skip=%zu "
           "negative_filter_shadow_local_valid=%zu "
           "negative_filter_shadow_local_invalid=%zu "
           "negative_filter_range_probe_total=%zu "
           "negative_filter_range_probe_max=%zu "
           "shared_dir_lookup=%zu shared_dir_hit=%zu shared_dir_miss=%zu "
           "shared_dir_stale=%zu shared_dir_hit_local_allocator=%zu "
           "shared_dir_hit_foreign_allocator=%zu "
           "shared_dir_would_skip_local=%zu shared_dir_register=%zu "
           "shared_dir_unregister=%zu shared_dir_probe_total=%zu "
           "shared_dir_probe_max=%zu "
           "shared_dir_first_attempt=%zu shared_dir_first_hit=%zu "
           "shared_dir_first_fallback=%zu shared_dir_first_invalid=%zu "
           "elastic_route_overflow_register=%zu "
           "elastic_route_overflow_register_fail=%zu "
           "elastic_route_overflow_lookup=%zu "
           "elastic_route_overflow_hit=%zu "
           "elastic_descriptor_overflow_alloc=%zu "
           "elastic_descriptor_overflow_reset=%zu "
           "elastic_descriptor_overflow_exhausted=%zu "
           "elastic_source_block_overflow_alloc=%zu "
           "elastic_source_block_overflow_release=%zu "
           "elastic_source_block_overflow_exhausted=%zu "
           "elastic_source_block_localize_probe=%zu "
           "elastic_source_block_localize_storage_match=%zu "
           "elastic_source_block_localize_storage_mismatch=%zu "
           "elastic_source_block_localize_would_move=%zu "
           "elastic_source_block_localize_block_shared=%zu "
           "elastic_source_block_localize_no_local_slot=%zu "
           "elastic_source_run_locality_probe=%zu "
           "elastic_source_run_locality_storage_mismatch=%zu "
           "elastic_source_run_locality_run_miss=%zu "
           "elastic_source_run_locality_class_mismatch=%zu "
           "elastic_source_run_locality_slot_match=%zu "
           "elastic_source_run_locality_would_rehome_slot=%zu "
           "elastic_depot_run_meta_init=%zu "
           "elastic_depot_run_meta_mark=%zu "
           "elastic_depot_run_meta_clear=%zu "
           "elastic_depot_run_meta_class_mismatch=%zu "
           "elastic_depot_run_meta_slot_misaligned=%zu "
           "elastic_depot_run_meta_too_many_slots=%zu "
           "elastic_depot_run_meta_used_count_mismatch=%zu "
           "elastic_slot_owner_locality_probe=%zu "
           "elastic_slot_owner_locality_storage_match=%zu "
           "elastic_slot_owner_locality_storage_mismatch=%zu "
           "elastic_slot_owner_locality_run_miss=%zu "
           "elastic_slot_owner_locality_class_mismatch=%zu "
           "elastic_slot_owner_locality_slot_match=%zu "
           "elastic_slot_owner_locality_owner_match=%zu "
           "elastic_slot_owner_locality_owner_mismatch=%zu "
           "elastic_slot_owner_locality_would_set_owner=%zu "
           "elastic_slot_owner_locality_would_hit_owner=%zu "
           "elastic_depot_drain_probe=%zu "
           "elastic_depot_drain_storage_match=%zu "
           "elastic_depot_drain_storage_mismatch=%zu "
           "elastic_depot_drain_run_match=%zu "
           "elastic_depot_drain_run_miss=%zu "
           "elastic_depot_drain_class_mismatch=%zu "
           "elastic_depot_drain_slot_mismatch=%zu "
           "elastic_depot_drain_ref_exclusive=%zu "
           "elastic_depot_drain_ref_shared=%zu "
           "elastic_depot_drain_owner_match=%zu "
           "elastic_depot_drain_owner_mismatch=%zu "
           "elastic_depot_drain_would_slot_localize=%zu "
           "elastic_depot_drain_would_keep_shared=%zu "
           "elastic_depot_drain_would_block_whole_localize=%zu "
           "elastic_depot_slot_localize_attempt=%zu "
           "elastic_depot_slot_localize_success=%zu "
           "elastic_depot_slot_localize_ineligible=%zu "
           "elastic_depot_slot_localize_storage_hit=%zu "
           "elastic_depot_slot_localize_storage_miss=%zu "
           "elastic_depot_slot_localize_storage_stale=%zu "
           "elastic_depot_descriptor_rehome_probe=%zu "
           "elastic_depot_descriptor_rehome_depot_descriptor=%zu "
           "elastic_depot_descriptor_rehome_already_local=%zu "
           "elastic_depot_descriptor_rehome_run_match=%zu "
           "elastic_depot_descriptor_rehome_run_mismatch=%zu "
           "elastic_depot_descriptor_rehome_local_descriptor_available=%zu "
           "elastic_depot_descriptor_rehome_no_local_descriptor=%zu "
           "elastic_depot_descriptor_rehome_would_rehome=%zu "
           "elastic_depot_route_replace_probe=%zu "
           "elastic_depot_route_replace_depot_descriptor=%zu "
           "elastic_depot_route_replace_run_match=%zu "
           "elastic_depot_route_replace_run_mismatch=%zu "
           "elastic_depot_route_replace_origin_missing=%zu "
           "elastic_depot_route_replace_old_route_found=%zu "
           "elastic_depot_route_replace_old_route_missing=%zu "
           "elastic_depot_route_replace_old_route_invalid=%zu "
           "elastic_depot_route_replace_descriptor_match=%zu "
           "elastic_depot_route_replace_descriptor_mismatch=%zu "
           "elastic_depot_route_replace_generation_match=%zu "
           "elastic_depot_route_replace_generation_mismatch=%zu "
           "elastic_depot_route_replace_front_class_match=%zu "
           "elastic_depot_route_replace_front_class_mismatch=%zu "
           "elastic_depot_route_replace_current_route_empty=%zu "
           "elastic_depot_route_replace_current_route_same=%zu "
           "elastic_depot_route_replace_current_route_conflict=%zu "
           "elastic_depot_route_replace_would_commit=%zu "
           "elastic_depot_route_replace_would_rollback=%zu "
           "elastic_depot_descriptor_rehome_l1_attempt=%zu "
           "elastic_depot_descriptor_rehome_l1_success=%zu "
           "elastic_depot_descriptor_rehome_l1_ineligible=%zu "
           "elastic_depot_descriptor_rehome_l1_no_local_descriptor=%zu "
           "elastic_depot_descriptor_rehome_l1_prepare_fail=%zu "
           "elastic_depot_descriptor_rehome_l1_route_replace_fail=%zu "
           "elastic_depot_descriptor_rehome_l1_detach_fail=%zu "
           "elastic_depot_descriptor_rehome_l1_rollback=%zu "
           "elastic_depot_descriptor_rehome_l1_budget_denied=%zu "
           "elastic_dftlc_rehome_intersection_directfree_hit=%zu "
           "elastic_dftlc_rehome_intersection_directfree_fail=%zu "
           "elastic_dftlc_rehome_intersection_transfer_probe=%zu "
           "elastic_dftlc_rehome_intersection_transfer_depot=%zu "
           "elastic_dftlc_rehome_intersection_transfer_already_local=%zu "
           "elastic_dftlc_rehome_intersection_transfer_would_rehome=%zu "
           "elastic_dftlc_rehome_intersection_rehome_success=%zu "
           "elastic_dftlc_rehome_intersection_rehome_ineligible=%zu "
           "elastic_dftlc_rehome_intersection_rehome_budget_denied=%zu "
           "elastic_slot_owner_sparse_lookup=%zu "
           "elastic_slot_owner_sparse_hit=%zu "
           "elastic_slot_owner_sparse_miss=%zu "
           "elastic_slot_owner_sparse_insert=%zu "
           "elastic_slot_owner_sparse_update=%zu "
           "elastic_slot_owner_sparse_owner_match=%zu "
           "elastic_slot_owner_sparse_owner_mismatch=%zu "
           "elastic_slot_owner_sparse_collision=%zu "
           "elastic_slot_owner_sparse_full=%zu "
           "elastic_slot_owner_consumer_probe=%zu "
           "elastic_slot_owner_consumer_hit=%zu "
           "elastic_slot_owner_consumer_miss=%zu "
           "elastic_slot_owner_consumer_owner_match=%zu "
           "elastic_slot_owner_consumer_owner_mismatch=%zu "
           "elastic_slot_owner_consumer_stale_generation=%zu "
           "elastic_slot_owner_consumer_false_positive=%zu "
           "elastic_slot_owner_consumer_would_skip_l2=%zu "
           "elastic_slot_owner_consumer_fallback=%zu "
           "elastic_slot_owner_logical_fastpath_probe=%zu "
           "elastic_slot_owner_logical_fastpath_hit=%zu "
           "elastic_slot_owner_logical_fastpath_miss=%zu "
           "elastic_slot_owner_logical_fastpath_stale_generation=%zu "
           "elastic_slot_owner_logical_fastpath_owner_mismatch=%zu "
           "elastic_slot_owner_logical_fastpath_fallback=%zu "
           "owner_equal_site_free=%zu "
           "owner_equal_site_remote_free=%zu "
           "owner_equal_site_local_cache=%zu "
           "owner_equal_site_visible_lookup=%zu "
           "owner_equal_site_transfer_locality=%zu "
           "owner_equal_site_large_central=%zu "
           "owner_equal_site_remote_pending=%zu "
           "owner_equal_site_owner_dead=%zu "
           "owner_equal_site_same_owner_fast=%zu "
           "owner_equal_site_unknown=%zu "
           "flc_owner_predicate_probe=%zu "
           "flc_owner_predicate_site_free=%zu "
           "flc_owner_predicate_site_local_cache=%zu "
           "flc_owner_predicate_depot_descriptor=%zu "
           "flc_owner_predicate_local_descriptor=%zu "
           "flc_owner_predicate_foreign_descriptor=%zu "
           "flc_owner_predicate_no_source_block=%zu "
           "flc_owner_predicate_source_block=%zu "
           "flc_owner_predicate_source_block_active=%zu "
           "flc_owner_predicate_source_block_shared=%zu "
           "flc_owner_predicate_source_run_active=%zu "
           "flc_owner_predicate_source_release=%zu "
           "depot_owner_equal_fastpath_probe=%zu "
           "depot_owner_equal_fastpath_hit=%zu "
           "depot_owner_equal_fastpath_miss=%zu "
           "depot_owner_equal_fastpath_fallback=%zu "
           "depot_owner_equal_fastpath_other_site=%zu "
           "source_owned_prepare=%zu "
           "source_owned_route_hit_local_owner=%zu "
           "source_owned_visibility_hit_local_owner=%zu "
           "source_owned_visibility_hit_foreign_owner=%zu "
           "source_owned_remote_free_attempt=%zu "
           "source_owned_release=%zu source_alloc=%zu "
           "local2p_source_alloc=%zu midpage_source_alloc=%zu "
           "large_source_alloc=%zu toy_source_alloc=%zu "
           "front_source_ops_alloc=%zu front_source_slot_alloc=%zu "
           "front_source_prefill_alloc=%zu toy_source_prefill_call=%zu "
           "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
           "transfer_reuse_hit=%zu transfer_reuse_invalid=%zu "
           "source_refill_starvation=%zu source_refill_saturation=%zu "
           "source_refill_boost=%zu source_refill_clamp=%zu "
           "source_admission_open=%zu source_admission_boosted=%zu "
           "source_admission_clamped=%zu "
           "control_plane_normal=%zu "
           "control_plane_burst_supply_would_open=%zu "
           "control_plane_close_would_start=%zu "
           "source_prefill_attempt=%zu source_prefill_filled=%zu "
           "source_prefill_fallback=%zu alloc_fail=%zu "
           "descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu "
           "descriptor_probe_total=%zu descriptor_probe_max=%zu "
           "route_lookup_probe_total=%zu route_lookup_probe_max=%zu "
           "route_register_probe_total=%zu route_register_probe_max=%zu "
           "route_unregister_probe_total=%zu route_unregister_probe_max=%zu "
           "descriptor_live_max=%zu source_block_active_max=%zu "
           "frontcache_total_max=%zu "
           "source_block_probe_total=%zu source_block_probe_max=%zu "
           "large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
           "larson_main_final",
           hz6_stats->route_valid,
           hz6_stats->route_invalid,
           hz6_stats->route_miss,
           hz6_stats->route_visibility_lookup,
           hz6_stats->route_visibility_hit,
           hz6_stats->route_visibility_hit_local_owner,
           hz6_stats->route_visibility_hit_foreign_owner,
           hz6_stats->route_visibility_miss,
           hz6_stats->route_visibility_probe_total,
           hz6_stats->route_visibility_probe_max,
           hz6_stats->route_exact_lookup_probe_total,
           hz6_stats->route_exact_lookup_probe_max,
           hz6_stats->owner_locality_lookup,
           hz6_stats->owner_locality_hit_local_allocator,
           hz6_stats->owner_locality_hit_foreign_allocator,
           hz6_stats->owner_locality_miss,
           hz6_stats->owner_locality_register,
           hz6_stats->owner_locality_unregister,
           hz6_stats->owner_locality_probe_total,
           hz6_stats->owner_locality_probe_max,
           hz6_stats->transfer_push,
           hz6_stats->transfer_pop,
           hz6_stats->transfer_current,
           hz6_stats->transfer_current_max,
           hz6_stats->remote_free_attempt,
           hz6_stats->remote_free_strict_owner_block,
           hz6_stats->remote_free_transfer_fail,
           hz6_stats->route_rehome_attempt,
           hz6_stats->route_rehome_success,
           hz6_stats->route_rehome_fail,
           hz6_stats->descriptor_source_route_allocator_match,
           hz6_stats->descriptor_source_route_allocator_mismatch,
           hz6_stats->descriptor_source_current_allocator_match,
           hz6_stats->descriptor_source_current_allocator_mismatch,
           hz6_stats->descriptor_storage_lookup,
           hz6_stats->descriptor_storage_hit,
           hz6_stats->descriptor_storage_miss,
           hz6_stats->descriptor_storage_probe_total,
           hz6_stats->descriptor_storage_probe_max,
           hz6_stats->descriptor_storage_route_allocator_match,
           hz6_stats->descriptor_storage_route_allocator_mismatch,
           hz6_stats->descriptor_storage_current_allocator_match,
           hz6_stats->descriptor_storage_current_allocator_mismatch,
           hz6_stats->owner_source_side_meta_lookup,
           hz6_stats->owner_source_side_meta_local,
           hz6_stats->owner_source_side_meta_foreign,
           hz6_stats->owner_source_side_meta_miss,
           hz6_stats->owner_source_side_meta_probe_total,
           hz6_stats->owner_source_side_meta_probe_max,
           hz6_stats->owner_source_side_meta_l2_lookup,
           hz6_stats->owner_source_side_meta_l2_hit,
           hz6_stats->owner_source_side_meta_l2_miss_no_block,
           hz6_stats->owner_source_side_meta_l2_miss_inactive,
           hz6_stats->owner_source_side_meta_l2_storage_mismatch,
           hz6_stats->lifecycle_owner_mismatch,
           hz6_stats->lifecycle_foreign_free_attempt,
           hz6_stats->lifecycle_foreign_free_handled,
           hz6_stats->lifecycle_foreign_free_invalid,
           hz6_stats->visible_first_attempt,
           hz6_stats->visible_first_hit,
           hz6_stats->visible_first_miss,
           hz6_stats->visible_first_visible_invalid,
           hz6_stats->visible_first_local_fallback,
           hz6_stats->visible_first_local_fallback_invalid,
           hz6_stats->visible_first_local_lookup_skipped,
           hz6_stats->negative_filter_attempt,
           hz6_stats->negative_filter_not_armed,
           hz6_stats->negative_filter_rehome_blocked,
           hz6_stats->negative_filter_skip_local,
           hz6_stats->negative_filter_maybe_local,
           hz6_stats->negative_filter_shadow_false_skip,
           hz6_stats->negative_filter_shadow_local_valid,
           hz6_stats->negative_filter_shadow_local_invalid,
           hz6_stats->negative_filter_range_probe_total,
           hz6_stats->negative_filter_range_probe_max,
           hz6_stats->shared_dir_lookup,
           hz6_stats->shared_dir_hit,
           hz6_stats->shared_dir_miss,
           hz6_stats->shared_dir_stale,
           hz6_stats->shared_dir_hit_local_allocator,
           hz6_stats->shared_dir_hit_foreign_allocator,
           hz6_stats->shared_dir_would_skip_local,
           hz6_stats->shared_dir_register,
           hz6_stats->shared_dir_unregister,
           hz6_stats->shared_dir_probe_total,
           hz6_stats->shared_dir_probe_max,
           hz6_stats->shared_dir_first_attempt,
           hz6_stats->shared_dir_first_hit,
           hz6_stats->shared_dir_first_fallback,
           hz6_stats->shared_dir_first_invalid,
           hz6_stats->elastic_route_overflow_register,
           hz6_stats->elastic_route_overflow_register_fail,
           hz6_stats->elastic_route_overflow_lookup,
           hz6_stats->elastic_route_overflow_hit,
           hz6_stats->elastic_descriptor_overflow_alloc,
           hz6_stats->elastic_descriptor_overflow_reset,
           hz6_stats->elastic_descriptor_overflow_exhausted,
           hz6_stats->elastic_source_block_overflow_alloc,
           hz6_stats->elastic_source_block_overflow_release,
           hz6_stats->elastic_source_block_overflow_exhausted,
           hz6_stats->elastic_source_block_localize_probe,
           hz6_stats->elastic_source_block_localize_storage_match,
           hz6_stats->elastic_source_block_localize_storage_mismatch,
           hz6_stats->elastic_source_block_localize_would_move,
           hz6_stats->elastic_source_block_localize_block_shared,
           hz6_stats->elastic_source_block_localize_no_local_slot,
           hz6_stats->elastic_source_run_locality_probe,
           hz6_stats->elastic_source_run_locality_storage_mismatch,
           hz6_stats->elastic_source_run_locality_run_miss,
           hz6_stats->elastic_source_run_locality_class_mismatch,
           hz6_stats->elastic_source_run_locality_slot_match,
           hz6_stats->elastic_source_run_locality_would_rehome_slot,
           hz6_stats->elastic_depot_run_meta_init,
           hz6_stats->elastic_depot_run_meta_mark,
           hz6_stats->elastic_depot_run_meta_clear,
           hz6_stats->elastic_depot_run_meta_class_mismatch,
           hz6_stats->elastic_depot_run_meta_slot_misaligned,
           hz6_stats->elastic_depot_run_meta_too_many_slots,
           hz6_stats->elastic_depot_run_meta_used_count_mismatch,
           hz6_stats->elastic_slot_owner_locality_probe,
           hz6_stats->elastic_slot_owner_locality_storage_match,
           hz6_stats->elastic_slot_owner_locality_storage_mismatch,
           hz6_stats->elastic_slot_owner_locality_run_miss,
           hz6_stats->elastic_slot_owner_locality_class_mismatch,
           hz6_stats->elastic_slot_owner_locality_slot_match,
           hz6_stats->elastic_slot_owner_locality_owner_match,
           hz6_stats->elastic_slot_owner_locality_owner_mismatch,
           hz6_stats->elastic_slot_owner_locality_would_set_owner,
           hz6_stats->elastic_slot_owner_locality_would_hit_owner,
           hz6_stats->elastic_depot_drain_probe,
           hz6_stats->elastic_depot_drain_storage_match,
           hz6_stats->elastic_depot_drain_storage_mismatch,
           hz6_stats->elastic_depot_drain_run_match,
           hz6_stats->elastic_depot_drain_run_miss,
           hz6_stats->elastic_depot_drain_class_mismatch,
           hz6_stats->elastic_depot_drain_slot_mismatch,
           hz6_stats->elastic_depot_drain_ref_exclusive,
           hz6_stats->elastic_depot_drain_ref_shared,
           hz6_stats->elastic_depot_drain_owner_match,
           hz6_stats->elastic_depot_drain_owner_mismatch,
           hz6_stats->elastic_depot_drain_would_slot_localize,
           hz6_stats->elastic_depot_drain_would_keep_shared,
           hz6_stats->elastic_depot_drain_would_block_whole_localize,
           hz6_stats->elastic_depot_slot_localize_attempt,
           hz6_stats->elastic_depot_slot_localize_success,
           hz6_stats->elastic_depot_slot_localize_ineligible,
           hz6_stats->elastic_depot_slot_localize_storage_hit,
           hz6_stats->elastic_depot_slot_localize_storage_miss,
           hz6_stats->elastic_depot_slot_localize_storage_stale,
           hz6_stats->elastic_depot_descriptor_rehome_probe,
           hz6_stats->elastic_depot_descriptor_rehome_depot_descriptor,
           hz6_stats->elastic_depot_descriptor_rehome_already_local,
           hz6_stats->elastic_depot_descriptor_rehome_run_match,
           hz6_stats->elastic_depot_descriptor_rehome_run_mismatch,
           hz6_stats->elastic_depot_descriptor_rehome_local_descriptor_available,
           hz6_stats->elastic_depot_descriptor_rehome_no_local_descriptor,
           hz6_stats->elastic_depot_descriptor_rehome_would_rehome,
           hz6_stats->elastic_depot_route_replace_probe,
           hz6_stats->elastic_depot_route_replace_depot_descriptor,
           hz6_stats->elastic_depot_route_replace_run_match,
           hz6_stats->elastic_depot_route_replace_run_mismatch,
           hz6_stats->elastic_depot_route_replace_origin_missing,
           hz6_stats->elastic_depot_route_replace_old_route_found,
           hz6_stats->elastic_depot_route_replace_old_route_missing,
           hz6_stats->elastic_depot_route_replace_old_route_invalid,
           hz6_stats->elastic_depot_route_replace_descriptor_match,
           hz6_stats->elastic_depot_route_replace_descriptor_mismatch,
           hz6_stats->elastic_depot_route_replace_generation_match,
           hz6_stats->elastic_depot_route_replace_generation_mismatch,
           hz6_stats->elastic_depot_route_replace_front_class_match,
           hz6_stats->elastic_depot_route_replace_front_class_mismatch,
           hz6_stats->elastic_depot_route_replace_current_route_empty,
           hz6_stats->elastic_depot_route_replace_current_route_same,
           hz6_stats->elastic_depot_route_replace_current_route_conflict,
           hz6_stats->elastic_depot_route_replace_would_commit,
           hz6_stats->elastic_depot_route_replace_would_rollback,
           hz6_stats->elastic_depot_descriptor_rehome_l1_attempt,
           hz6_stats->elastic_depot_descriptor_rehome_l1_success,
           hz6_stats->elastic_depot_descriptor_rehome_l1_ineligible,
           hz6_stats->elastic_depot_descriptor_rehome_l1_no_local_descriptor,
           hz6_stats->elastic_depot_descriptor_rehome_l1_prepare_fail,
           hz6_stats->elastic_depot_descriptor_rehome_l1_route_replace_fail,
           hz6_stats->elastic_depot_descriptor_rehome_l1_detach_fail,
           hz6_stats->elastic_depot_descriptor_rehome_l1_rollback,
           hz6_stats->elastic_depot_descriptor_rehome_l1_budget_denied,
           hz6_stats->elastic_dftlc_rehome_intersection_directfree_hit,
           hz6_stats->elastic_dftlc_rehome_intersection_directfree_fail,
           hz6_stats->elastic_dftlc_rehome_intersection_transfer_probe,
           hz6_stats->elastic_dftlc_rehome_intersection_transfer_depot,
           hz6_stats->elastic_dftlc_rehome_intersection_transfer_already_local,
           hz6_stats->elastic_dftlc_rehome_intersection_transfer_would_rehome,
           hz6_stats->elastic_dftlc_rehome_intersection_rehome_success,
           hz6_stats->elastic_dftlc_rehome_intersection_rehome_ineligible,
           hz6_stats->elastic_dftlc_rehome_intersection_rehome_budget_denied,
           hz6_stats->elastic_slot_owner_sparse_lookup,
           hz6_stats->elastic_slot_owner_sparse_hit,
           hz6_stats->elastic_slot_owner_sparse_miss,
           hz6_stats->elastic_slot_owner_sparse_insert,
           hz6_stats->elastic_slot_owner_sparse_update,
           hz6_stats->elastic_slot_owner_sparse_owner_match,
           hz6_stats->elastic_slot_owner_sparse_owner_mismatch,
           hz6_stats->elastic_slot_owner_sparse_collision,
           hz6_stats->elastic_slot_owner_sparse_full,
           hz6_stats->elastic_slot_owner_consumer_probe,
           hz6_stats->elastic_slot_owner_consumer_hit,
           hz6_stats->elastic_slot_owner_consumer_miss,
           hz6_stats->elastic_slot_owner_consumer_owner_match,
           hz6_stats->elastic_slot_owner_consumer_owner_mismatch,
           hz6_stats->elastic_slot_owner_consumer_stale_generation,
           hz6_stats->elastic_slot_owner_consumer_false_positive,
           hz6_stats->elastic_slot_owner_consumer_would_skip_l2,
           hz6_stats->elastic_slot_owner_consumer_fallback,
           hz6_stats->elastic_slot_owner_logical_fastpath_probe,
           hz6_stats->elastic_slot_owner_logical_fastpath_hit,
           hz6_stats->elastic_slot_owner_logical_fastpath_miss,
           hz6_stats->elastic_slot_owner_logical_fastpath_stale_generation,
           hz6_stats->elastic_slot_owner_logical_fastpath_owner_mismatch,
           hz6_stats->elastic_slot_owner_logical_fastpath_fallback,
           hz6_stats->owner_equal_site_free,
           hz6_stats->owner_equal_site_remote_free,
           hz6_stats->owner_equal_site_local_cache,
           hz6_stats->owner_equal_site_visible_lookup,
           hz6_stats->owner_equal_site_transfer_locality,
           hz6_stats->owner_equal_site_large_central,
           hz6_stats->owner_equal_site_remote_pending,
           hz6_stats->owner_equal_site_owner_dead,
           hz6_stats->owner_equal_site_same_owner_fast,
           hz6_stats->owner_equal_site_unknown,
           hz6_stats->flc_owner_predicate_probe,
           hz6_stats->flc_owner_predicate_site_free,
           hz6_stats->flc_owner_predicate_site_local_cache,
           hz6_stats->flc_owner_predicate_depot_descriptor,
           hz6_stats->flc_owner_predicate_local_descriptor,
           hz6_stats->flc_owner_predicate_foreign_descriptor,
           hz6_stats->flc_owner_predicate_no_source_block,
           hz6_stats->flc_owner_predicate_source_block,
           hz6_stats->flc_owner_predicate_source_block_active,
           hz6_stats->flc_owner_predicate_source_block_shared,
           hz6_stats->flc_owner_predicate_source_run_active,
           hz6_stats->flc_owner_predicate_source_release,
           hz6_stats->depot_owner_equal_fastpath_probe,
           hz6_stats->depot_owner_equal_fastpath_hit,
           hz6_stats->depot_owner_equal_fastpath_miss,
           hz6_stats->depot_owner_equal_fastpath_fallback,
           hz6_stats->depot_owner_equal_fastpath_other_site,
           hz6_stats->source_owned_prepare,
           hz6_stats->source_owned_route_hit_local_owner,
           hz6_stats->source_owned_visibility_hit_local_owner,
           hz6_stats->source_owned_visibility_hit_foreign_owner,
           hz6_stats->source_owned_remote_free_attempt,
           hz6_stats->source_owned_release,
           hz6_stats->source_alloc,
           hz6_stats->local2p_source_alloc,
           hz6_stats->midpage_source_alloc,
           hz6_stats->large_source_alloc,
           hz6_stats->toy_source_alloc,
           hz6_stats->front_source_ops_alloc,
           hz6_stats->front_source_slot_alloc,
           hz6_stats->front_source_prefill_alloc,
           hz6_stats->toy_source_prefill_call,
           hz6_stats->frontcache_reuse_hit,
           hz6_stats->frontcache_reuse_invalid,
           hz6_stats->transfer_reuse_hit,
           hz6_stats->transfer_reuse_invalid,
           hz6_stats->source_refill_starvation,
           hz6_stats->source_refill_saturation,
           hz6_stats->source_refill_boost,
           hz6_stats->source_refill_clamp,
           hz6_stats->source_admission_open,
           hz6_stats->source_admission_boosted,
           hz6_stats->source_admission_clamped,
           hz6_stats->control_plane_normal,
           hz6_stats->control_plane_burst_supply_would_open,
           hz6_stats->control_plane_close_would_start,
           hz6_stats->source_prefill_attempt,
           hz6_stats->source_prefill_filled,
           hz6_stats->source_prefill_fallback,
           hz6_stats->alloc_fail,
           hz6_stats->descriptor_exhausted,
           hz6_stats->route_register_fail,
           hz6_stats->source_block_exhausted,
           hz6_stats->descriptor_probe_total,
           hz6_stats->descriptor_probe_max,
           hz6_stats->route_lookup_probe_total,
           hz6_stats->route_lookup_probe_max,
           hz6_stats->route_register_probe_total,
           hz6_stats->route_register_probe_max,
           hz6_stats->route_unregister_probe_total,
           hz6_stats->route_unregister_probe_max,
           hz6_stats->descriptor_live_max,
           hz6_stats->source_block_active_max,
           hz6_stats->frontcache_total_max,
           hz6_stats->source_block_probe_total,
           hz6_stats->source_block_probe_max,
           hz6_stats->large_span_central_push,
           hz6_stats->large_span_central_pop,
           hz6_stats->large_span_source_alloc);
}
#endif
