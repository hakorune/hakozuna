#if defined(HZ_BENCH_USE_HZ6)
        hz6_stats.route_valid += args[i].hz6_stats_after.route_valid;
        hz6_stats.route_invalid += args[i].hz6_stats_after.route_invalid;
        hz6_stats.route_miss += args[i].hz6_stats_after.route_miss;
        hz6_stats.transfer_push += args[i].hz6_stats_after.transfer_push;
        hz6_stats.transfer_pop += args[i].hz6_stats_after.transfer_pop;
        hz6_stats.source_alloc += args[i].hz6_stats_after.source_alloc;
        hz6_stats.source_refill_starvation +=
            args[i].hz6_stats_after.source_refill_starvation;
        hz6_stats.source_refill_saturation +=
            args[i].hz6_stats_after.source_refill_saturation;
        hz6_stats.source_refill_boost +=
            args[i].hz6_stats_after.source_refill_boost;
        hz6_stats.source_refill_clamp +=
            args[i].hz6_stats_after.source_refill_clamp;
        hz6_stats.source_admission_open +=
            args[i].hz6_stats_after.source_admission_open;
        hz6_stats.source_admission_boosted +=
            args[i].hz6_stats_after.source_admission_boosted;
        hz6_stats.source_admission_clamped +=
            args[i].hz6_stats_after.source_admission_clamped;
        hz6_stats.alloc_fail += args[i].hz6_stats_after.alloc_fail;
        hz6_stats.descriptor_exhausted +=
            args[i].hz6_stats_after.descriptor_exhausted;
        hz6_stats.route_register_fail +=
            args[i].hz6_stats_after.route_register_fail;
        hz6_stats.source_block_exhausted +=
            args[i].hz6_stats_after.source_block_exhausted;
        hz6_stats.descriptor_probe_total +=
            args[i].hz6_stats_after.descriptor_probe_total;
        if (args[i].hz6_stats_after.descriptor_probe_max >
            hz6_stats.descriptor_probe_max) {
            hz6_stats.descriptor_probe_max =
                args[i].hz6_stats_after.descriptor_probe_max;
        }
        hz6_stats.frontcache_reuse_hit +=
            args[i].hz6_stats_after.frontcache_reuse_hit;
        hz6_stats.frontcache_reuse_invalid +=
            args[i].hz6_stats_after.frontcache_reuse_invalid;
        hz6_stats.transfer_reuse_hit +=
            args[i].hz6_stats_after.transfer_reuse_hit;
        hz6_stats.transfer_reuse_invalid +=
            args[i].hz6_stats_after.transfer_reuse_invalid;
        hz6_stats.route_visibility_lookup +=
            args[i].hz6_stats_after.route_visibility_lookup;
        hz6_stats.route_visibility_hit +=
            args[i].hz6_stats_after.route_visibility_hit;
        hz6_stats.route_visibility_hit_local_owner +=
            args[i].hz6_stats_after.route_visibility_hit_local_owner;
        hz6_stats.route_visibility_hit_foreign_owner +=
            args[i].hz6_stats_after.route_visibility_hit_foreign_owner;
        hz6_stats.route_visibility_miss +=
            args[i].hz6_stats_after.route_visibility_miss;
        hz6_stats.route_visibility_probe_total +=
            args[i].hz6_stats_after.route_visibility_probe_total;
        if (args[i].hz6_stats_after.route_visibility_probe_max >
            hz6_stats.route_visibility_probe_max) {
            hz6_stats.route_visibility_probe_max =
                args[i].hz6_stats_after.route_visibility_probe_max;
        }
        hz6_stats.lifecycle_owner_mismatch +=
            args[i].hz6_stats_after.lifecycle_owner_mismatch;
        hz6_stats.lifecycle_foreign_free_attempt +=
            args[i].hz6_stats_after.lifecycle_foreign_free_attempt;
        hz6_stats.lifecycle_foreign_free_handled +=
            args[i].hz6_stats_after.lifecycle_foreign_free_handled;
        hz6_stats.lifecycle_foreign_free_invalid +=
            args[i].hz6_stats_after.lifecycle_foreign_free_invalid;
        hz6_stats.free_invalid_route_kind_invalid +=
            args[i].hz6_stats_after.free_invalid_route_kind_invalid;
        hz6_stats.free_invalid_no_front +=
            args[i].hz6_stats_after.free_invalid_no_front;
        hz6_stats.free_invalid_local_owner +=
            args[i].hz6_stats_after.free_invalid_local_owner;
        hz6_stats.free_invalid_remote_owner +=
            args[i].hz6_stats_after.free_invalid_remote_owner;
        hz6_stats.free_invalid_same_owner_fast +=
            args[i].hz6_stats_after.free_invalid_same_owner_fast;
        hz6_stats.free_invalid_local_cache_direct +=
            args[i].hz6_stats_after.free_invalid_local_cache_direct;
        hz6_stats.free_invalid_front_tagged +=
            args[i].hz6_stats_after.free_invalid_front_tagged;
        hz6_stats.free_invalid_remote_tagged +=
            args[i].hz6_stats_after.free_invalid_remote_tagged;
        hz6_stats.activation_route_repair_attempt +=
            args[i].hz6_stats_after.activation_route_repair_attempt;
        hz6_stats.activation_route_repair_success +=
            args[i].hz6_stats_after.activation_route_repair_success;
        hz6_stats.activation_route_repair_fail +=
            args[i].hz6_stats_after.activation_route_repair_fail;
        hz6_stats.activation_route_repair_conflict +=
            args[i].hz6_stats_after.activation_route_repair_conflict;
        hz6_stats.source_block_release_live_guard +=
            args[i].hz6_stats_after.source_block_release_live_guard;
        if (args[i].hz6_stats_after.source_block_release_live_descriptors_max >
            hz6_stats.source_block_release_live_descriptors_max) {
            hz6_stats.source_block_release_live_descriptors_max =
                args[i].hz6_stats_after
                    .source_block_release_live_descriptors_max;
        }
        hz6_stats.shared_dir_lookup +=
            args[i].hz6_stats_after.shared_dir_lookup;
        hz6_stats.shared_dir_hit +=
            args[i].hz6_stats_after.shared_dir_hit;
        hz6_stats.shared_dir_miss +=
            args[i].hz6_stats_after.shared_dir_miss;
        hz6_stats.shared_dir_stale +=
            args[i].hz6_stats_after.shared_dir_stale;
        hz6_stats.shared_dir_hit_local_allocator +=
            args[i].hz6_stats_after.shared_dir_hit_local_allocator;
        hz6_stats.shared_dir_hit_foreign_allocator +=
            args[i].hz6_stats_after.shared_dir_hit_foreign_allocator;
        hz6_stats.shared_dir_would_skip_local +=
            args[i].hz6_stats_after.shared_dir_would_skip_local;
        hz6_stats.shared_dir_register +=
            args[i].hz6_stats_after.shared_dir_register;
        hz6_stats.shared_dir_unregister +=
            args[i].hz6_stats_after.shared_dir_unregister;
        hz6_stats.shared_dir_probe_total +=
            args[i].hz6_stats_after.shared_dir_probe_total;
        if (args[i].hz6_stats_after.shared_dir_probe_max >
            hz6_stats.shared_dir_probe_max) {
            hz6_stats.shared_dir_probe_max =
                args[i].hz6_stats_after.shared_dir_probe_max;
        }
        hz6_stats.shared_dir_first_attempt +=
            args[i].hz6_stats_after.shared_dir_first_attempt;
        hz6_stats.shared_dir_first_hit +=
            args[i].hz6_stats_after.shared_dir_first_hit;
        hz6_stats.shared_dir_first_fallback +=
            args[i].hz6_stats_after.shared_dir_first_fallback;
        hz6_stats.shared_dir_first_invalid +=
            args[i].hz6_stats_after.shared_dir_first_invalid;
        hz6_stats.elastic_route_overflow_register +=
            args[i].hz6_stats_after.elastic_route_overflow_register;
        hz6_stats.elastic_route_overflow_register_fail +=
            args[i].hz6_stats_after.elastic_route_overflow_register_fail;
        hz6_stats.elastic_route_overflow_lookup +=
            args[i].hz6_stats_after.elastic_route_overflow_lookup;
        hz6_stats.elastic_route_overflow_hit +=
            args[i].hz6_stats_after.elastic_route_overflow_hit;
        hz6_stats.elastic_descriptor_overflow_alloc +=
            args[i].hz6_stats_after.elastic_descriptor_overflow_alloc;
        hz6_stats.elastic_descriptor_overflow_reset +=
            args[i].hz6_stats_after.elastic_descriptor_overflow_reset;
        hz6_stats.elastic_descriptor_overflow_exhausted +=
            args[i].hz6_stats_after.elastic_descriptor_overflow_exhausted;
        hz6_stats.elastic_source_block_overflow_alloc +=
            args[i].hz6_stats_after.elastic_source_block_overflow_alloc;
        hz6_stats.elastic_source_block_overflow_release +=
            args[i].hz6_stats_after.elastic_source_block_overflow_release;
        hz6_stats.elastic_source_block_overflow_exhausted +=
            args[i].hz6_stats_after.elastic_source_block_overflow_exhausted;
        hz6_stats.source_owned_prepare +=
            args[i].hz6_stats_after.source_owned_prepare;
        hz6_stats.source_owned_route_hit_local_owner +=
            args[i].hz6_stats_after.source_owned_route_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_local_owner +=
            args[i].hz6_stats_after.source_owned_visibility_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_foreign_owner +=
            args[i].hz6_stats_after.source_owned_visibility_hit_foreign_owner;
        hz6_stats.source_owned_remote_free_attempt +=
            args[i].hz6_stats_after.source_owned_remote_free_attempt;
        hz6_stats.source_owned_release +=
            args[i].hz6_stats_after.source_owned_release;
        hz6_stats.route_register_reason_unknown +=
            args[i].hz6_stats_after.route_register_reason_unknown;
        hz6_stats.route_register_reason_source_run_slot +=
            args[i].hz6_stats_after.route_register_reason_source_run_slot;
        hz6_stats.route_register_reason_direct_source +=
            args[i].hz6_stats_after.route_register_reason_direct_source;
        hz6_stats.route_register_reason_materialize +=
            args[i].hz6_stats_after.route_register_reason_materialize;
        hz6_stats.route_register_reason_rehome +=
            args[i].hz6_stats_after.route_register_reason_rehome;
        hz6_stats.route_unregister_reason_unknown +=
            args[i].hz6_stats_after.route_unregister_reason_unknown;
        hz6_stats.route_unregister_reason_frontcache_overflow +=
            args[i].hz6_stats_after.route_unregister_reason_frontcache_overflow;
        hz6_stats.route_unregister_reason_cap_release +=
            args[i].hz6_stats_after.route_unregister_reason_cap_release;
        hz6_stats.route_unregister_reason_descriptorless_detach +=
            args[i].hz6_stats_after.route_unregister_reason_descriptorless_detach;
        hz6_stats.route_unregister_reason_source_slot_release +=
            args[i].hz6_stats_after.route_unregister_reason_source_slot_release;
        hz6_stats.route_unregister_reason_rehome +=
            args[i].hz6_stats_after.route_unregister_reason_rehome;
#if HZ6_DIAGNOSTIC_PROBES
#define HZ6_MAX_STAT(field) \
        do { \
            if (args[i].hz6_stats_after.field > hz6_stats.field) { \
                hz6_stats.field = args[i].hz6_stats_after.field; \
            } \
        } while (0)
        HZ6_MAX_STAT(descriptor_fail_active_max);
        HZ6_MAX_STAT(descriptor_fail_local_free_max);
        HZ6_MAX_STAT(descriptor_fail_transfer_free_max);
        HZ6_MAX_STAT(descriptor_fail_remote_pending_max);
        HZ6_MAX_STAT(descriptor_fail_central_free_max);
        HZ6_MAX_STAT(descriptor_fail_released_max);
        HZ6_MAX_STAT(descriptor_fail_orphan_max);
        HZ6_MAX_STAT(descriptor_fail_dead_with_ptr_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_total_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_largest_bin_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_nonempty_bins_max);
        hz6_stats.descriptor_frontcache_reuse_dryrun_calls +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_dryrun_calls;
        hz6_stats.descriptor_frontcache_reuse_requested_nonempty +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_requested_nonempty;
        hz6_stats.descriptor_frontcache_reuse_requested_total +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_requested_total;
        hz6_stats.descriptor_frontcache_reuse_donor_total +=
            args[i].hz6_stats_after.descriptor_frontcache_reuse_donor_total;
        HZ6_MAX_STAT(descriptor_frontcache_reuse_largest_donor_max);
        HZ6_MAX_STAT(descriptor_frontcache_reuse_donor_bins_max);
        hz6_stats.descriptorless_frontcache_push +=
            args[i].hz6_stats_after.descriptorless_frontcache_push;
        hz6_stats.descriptorless_frontcache_pop +=
            args[i].hz6_stats_after.descriptorless_frontcache_pop;
        hz6_stats.descriptorless_frontcache_descriptor_fail +=
            args[i].hz6_stats_after
                .descriptorless_frontcache_descriptor_fail;
        hz6_stats.descriptorless_frontcache_route_fail +=
            args[i].hz6_stats_after.descriptorless_frontcache_route_fail;
        hz6_stats.descriptorless_frontcache_invalid +=
            args[i].hz6_stats_after.descriptorless_frontcache_invalid;
        hz6_stats.descriptorreserve_frontcache_push +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_push;
        hz6_stats.descriptorreserve_frontcache_pop +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_pop;
        hz6_stats.descriptorreserve_frontcache_missing +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_missing;
        hz6_stats.descriptorreserve_frontcache_invalid +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_invalid;
        hz6_stats.descgov_trigger_descriptor_fail +=
            args[i].hz6_stats_after.descgov_trigger_descriptor_fail;
        hz6_stats.descgov_detach_attempt +=
            args[i].hz6_stats_after.descgov_detach_attempt;
        hz6_stats.descgov_detach_success +=
            args[i].hz6_stats_after.descgov_detach_success;
        hz6_stats.descgov_detach_budget_denied +=
            args[i].hz6_stats_after.descgov_detach_budget_denied;
        hz6_stats.descgov_detach_class_denied +=
            args[i].hz6_stats_after.descgov_detach_class_denied;
        hz6_stats.descgov_materialize_admit +=
            args[i].hz6_stats_after.descgov_materialize_admit;
        hz6_stats.descgov_materialize_block_no_descriptor +=
            args[i].hz6_stats_after.descgov_materialize_block_no_descriptor;
        hz6_stats.descgov_materialize_fail +=
            args[i].hz6_stats_after.descgov_materialize_fail;
        hz6_stats.descgov_detached_current +=
            args[i].hz6_stats_after.descgov_detached_current;
        HZ6_MAX_STAT(descgov_detached_max);
        hz6_stats.frontcache_spill_dryrun_calls +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_calls;
        hz6_stats.frontcache_spill_dryrun_requested_empty +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_requested_empty;
        hz6_stats.frontcache_spill_dryrun_candidate_calls +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_candidate_calls;
        hz6_stats.frontcache_spill_dryrun_reclaimable_total +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_reclaimable_total;
        HZ6_MAX_STAT(frontcache_spill_dryrun_largest_donor_max);
        HZ6_MAX_STAT(frontcache_spill_dryrun_donor_bins_max);
        hz6_stats.frontcache_spill_attempt +=
            args[i].hz6_stats_after.frontcache_spill_attempt;
        hz6_stats.frontcache_spill_success +=
            args[i].hz6_stats_after.frontcache_spill_success;
        hz6_stats.frontcache_spill_no_candidate +=
            args[i].hz6_stats_after.frontcache_spill_no_candidate;
        hz6_stats.frontcache_spill_invalid +=
            args[i].hz6_stats_after.frontcache_spill_invalid;
        hz6_stats.frontcache_spill_retry_success +=
            args[i].hz6_stats_after.frontcache_spill_retry_success;
        hz6_stats.frontcache_borrow_dryrun_calls +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_calls;
        hz6_stats.frontcache_borrow_dryrun_candidate_calls +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_candidate_calls;
        hz6_stats.frontcache_borrow_dryrun_candidate_total +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_candidate_total;
        HZ6_MAX_STAT(frontcache_borrow_dryrun_largest_candidate_max);
        hz6_stats.frontcache_borrow_attempt +=
            args[i].hz6_stats_after.frontcache_borrow_attempt;
        hz6_stats.frontcache_borrow_success +=
            args[i].hz6_stats_after.frontcache_borrow_success;
        hz6_stats.frontcache_borrow_no_candidate +=
            args[i].hz6_stats_after.frontcache_borrow_no_candidate;
        hz6_stats.frontcache_borrow_invalid +=
            args[i].hz6_stats_after.frontcache_borrow_invalid;
        hz6_stats.frontcache_cap_dryrun_push +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_push;
        hz6_stats.frontcache_cap_dryrun_over_cap +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_over_cap;
        hz6_stats.frontcache_cap_dryrun_would_release +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_would_release;
        HZ6_MAX_STAT(frontcache_cap_dryrun_soft_cap_max);
        HZ6_MAX_STAT(frontcache_cap_dryrun_bin_count_max);
        hz6_stats.frontcache_cap_release +=
            args[i].hz6_stats_after.frontcache_cap_release;
        hz6_stats.source_run_reuse_dryrun_calls +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_calls;
        hz6_stats.source_run_reuse_dryrun_candidate_calls +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_candidate_calls;
        hz6_stats.source_run_reuse_dryrun_candidate_blocks_total +=
            args[i].hz6_stats_after
                .source_run_reuse_dryrun_candidate_blocks_total;
        hz6_stats.source_run_reuse_dryrun_free_slots_total +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_free_slots_total;
        HZ6_MAX_STAT(source_run_reuse_dryrun_largest_free_slots_max);
        hz6_stats.source_run_reuse_attempt +=
            args[i].hz6_stats_after.source_run_reuse_attempt;
        hz6_stats.source_run_reuse_candidate +=
            args[i].hz6_stats_after.source_run_reuse_candidate;
        hz6_stats.source_run_reuse_hit +=
            args[i].hz6_stats_after.source_run_reuse_hit;
        hz6_stats.source_run_reuse_miss_no_block +=
            args[i].hz6_stats_after.source_run_reuse_miss_no_block;
        hz6_stats.source_run_reuse_miss_no_slot +=
            args[i].hz6_stats_after.source_run_reuse_miss_no_slot;
        hz6_stats.source_run_reuse_reserved +=
            args[i].hz6_stats_after.source_run_reuse_reserved;
        hz6_stats.source_run_reuse_slot_fail +=
            args[i].hz6_stats_after.source_run_reuse_slot_fail;
        hz6_stats.source_run_reuse_descriptor_fail +=
            args[i].hz6_stats_after.source_run_reuse_descriptor_fail;
        hz6_stats.source_run_reuse_descriptor_reclaim_attempt +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_attempt;
        hz6_stats.source_run_reuse_descriptor_reclaim_success +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_success;
        hz6_stats.source_run_reuse_descriptor_reclaim_no_candidate +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_no_candidate;
        hz6_stats.source_run_reuse_same_class_reclaim_attempt +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_attempt;
        hz6_stats.source_run_reuse_same_class_reclaim_success +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_success;
        hz6_stats.source_run_reuse_same_class_reclaim_no_candidate +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_no_candidate;
        hz6_stats.source_run_reuse_route_fail +=
            args[i].hz6_stats_after.source_run_reuse_route_fail;
        hz6_stats.source_run_reuse_prepare_fail +=
            args[i].hz6_stats_after.source_run_reuse_prepare_fail;
        hz6_stats.source_run_reuse_rollback +=
            args[i].hz6_stats_after.source_run_reuse_rollback;
        hz6_stats.source_run_reuse_used_count_mismatch +=
            args[i].hz6_stats_after.source_run_reuse_used_count_mismatch;
        hz6_stats.source_block_route_dryrun_attempt +=
            args[i].hz6_stats_after.source_block_route_dryrun_attempt;
        hz6_stats.source_block_route_block_hit +=
            args[i].hz6_stats_after.source_block_route_block_hit;
        hz6_stats.source_block_route_slot_hit +=
            args[i].hz6_stats_after.source_block_route_slot_hit;
        hz6_stats.source_block_route_descriptor_hit +=
            args[i].hz6_stats_after.source_block_route_descriptor_hit;
        hz6_stats.source_block_route_miss_no_block +=
            args[i].hz6_stats_after.source_block_route_miss_no_block;
        hz6_stats.source_block_route_invalid_alignment +=
            args[i].hz6_stats_after.source_block_route_invalid_alignment;
        hz6_stats.source_block_route_invalid_unused +=
            args[i].hz6_stats_after.source_block_route_invalid_unused;
        hz6_stats.source_block_route_descriptor_miss +=
            args[i].hz6_stats_after.source_block_route_descriptor_miss;
        hz6_stats.source_block_route_class_mismatch +=
            args[i].hz6_stats_after.source_block_route_class_mismatch;
        hz6_stats.source_block_route_probe_total +=
            args[i].hz6_stats_after.source_block_route_probe_total;
        HZ6_MAX_STAT(source_block_route_probe_max);
        hz6_stats.source_block_route_descriptor_map_hit +=
            args[i].hz6_stats_after.source_block_route_descriptor_map_hit;
        hz6_stats.source_block_route_descriptor_map_miss +=
            args[i].hz6_stats_after.source_block_route_descriptor_map_miss;
        hz6_stats.source_block_route_descriptor_map_stale +=
            args[i].hz6_stats_after.source_block_route_descriptor_map_stale;
        hz6_stats.source_block_route_descriptor_map_set +=
            args[i].hz6_stats_after.source_block_route_descriptor_map_set;
        hz6_stats.source_block_route_descriptor_map_clear +=
            args[i].hz6_stats_after.source_block_route_descriptor_map_clear;
        hz6_stats.source_block_route_range_index_register +=
            args[i].hz6_stats_after.source_block_route_range_index_register;
        hz6_stats.source_block_route_range_index_unregister +=
            args[i].hz6_stats_after.source_block_route_range_index_unregister;
        hz6_stats.source_block_route_range_index_register_fail +=
            args[i].hz6_stats_after.source_block_route_range_index_register_fail;
        hz6_stats.source_block_route_range_index_lookup +=
            args[i].hz6_stats_after.source_block_route_range_index_lookup;
        hz6_stats.source_block_route_range_index_hit +=
            args[i].hz6_stats_after.source_block_route_range_index_hit;
        hz6_stats.source_block_route_range_index_miss +=
            args[i].hz6_stats_after.source_block_route_range_index_miss;
        hz6_stats.source_block_route_range_index_stale +=
            args[i].hz6_stats_after.source_block_route_range_index_stale;
        hz6_stats.source_block_route_range_index_probe_total +=
            args[i].hz6_stats_after.source_block_route_range_index_probe_total;
        HZ6_MAX_STAT(source_block_route_range_index_probe_max);
        hz6_stats.source_block_route_behavior_attempt +=
            args[i].hz6_stats_after.source_block_route_behavior_attempt;
        hz6_stats.source_block_route_behavior_valid +=
            args[i].hz6_stats_after.source_block_route_behavior_valid;
        hz6_stats.source_block_route_behavior_fallback +=
            args[i].hz6_stats_after.source_block_route_behavior_fallback;
        hz6_stats.source_block_route_behavior_invalid_front +=
            args[i].hz6_stats_after.source_block_route_behavior_invalid_front;
        hz6_stats.source_block_route_behavior_invalid_descriptor +=
            args[i].hz6_stats_after.source_block_route_behavior_invalid_descriptor;
        hz6_stats.smallrun_route_attempt +=
            args[i].hz6_stats_after.smallrun_route_attempt;
        hz6_stats.smallrun_range_hit +=
            args[i].hz6_stats_after.smallrun_range_hit;
        hz6_stats.smallrun_active_slot_hit +=
            args[i].hz6_stats_after.smallrun_active_slot_hit;
        hz6_stats.smallrun_descriptor_match +=
            args[i].hz6_stats_after.smallrun_descriptor_match;
        hz6_stats.smallrun_generation_match +=
            args[i].hz6_stats_after.smallrun_generation_match;
        hz6_stats.smallrun_would_valid +=
            args[i].hz6_stats_after.smallrun_would_valid;
        hz6_stats.smallrun_would_invalid +=
            args[i].hz6_stats_after.smallrun_would_invalid;
        hz6_stats.smallrun_exact_fallback_needed +=
            args[i].hz6_stats_after.smallrun_exact_fallback_needed;
        hz6_stats.smallrun_false_positive +=
            args[i].hz6_stats_after.smallrun_false_positive;
        hz6_stats.smallrun_behavior_attempt +=
            args[i].hz6_stats_after.smallrun_behavior_attempt;
        hz6_stats.smallrun_behavior_valid +=
            args[i].hz6_stats_after.smallrun_behavior_valid;
        hz6_stats.smallrun_behavior_fallback +=
            args[i].hz6_stats_after.smallrun_behavior_fallback;
        hz6_stats.smallrun_behavior_invalid_slot +=
            args[i].hz6_stats_after.smallrun_behavior_invalid_slot;
        hz6_stats.smallrun_behavior_invalid_descriptor +=
            args[i].hz6_stats_after.smallrun_behavior_invalid_descriptor;
        hz6_stats.toy_small_malloc_fast_attempt +=
            args[i].hz6_stats_after.toy_small_malloc_fast_attempt;
        hz6_stats.toy_small_malloc_fast_hit +=
            args[i].hz6_stats_after.toy_small_malloc_fast_hit;
        hz6_stats.toy_small_malloc_front_dispatch +=
            args[i].hz6_stats_after.toy_small_malloc_front_dispatch;
        hz6_stats.toy_small_free_route_lookup +=
            args[i].hz6_stats_after.toy_small_free_route_lookup;
        hz6_stats.toy_small_free_owner_equal +=
            args[i].hz6_stats_after.toy_small_free_owner_equal;
        hz6_stats.toy_small_free_fast_hit +=
            args[i].hz6_stats_after.toy_small_free_fast_hit;
        hz6_stats.toy_small_free_cache_push +=
            args[i].hz6_stats_after.toy_small_free_cache_push;
        hz6_stats.toy_small_activate_descriptor +=
            args[i].hz6_stats_after.toy_small_activate_descriptor;
        hz6_stats.toy_small_active_map_register +=
            args[i].hz6_stats_after.toy_small_active_map_register;
        hz6_stats.toy_small_active_map_register_collision +=
            args[i].hz6_stats_after.toy_small_active_map_register_collision;
        hz6_stats.toy_small_active_map_free_attempt +=
            args[i].hz6_stats_after.toy_small_active_map_free_attempt;
        hz6_stats.toy_small_active_map_free_hit +=
            args[i].hz6_stats_after.toy_small_active_map_free_hit;
        hz6_stats.toy_small_active_map_free_miss +=
            args[i].hz6_stats_after.toy_small_active_map_free_miss;
        hz6_stats.toy_small_active_map_free_stale +=
            args[i].hz6_stats_after.toy_small_active_map_free_stale;
        hz6_stats.toy_small_active_map_free_cache_fail +=
            args[i].hz6_stats_after.toy_small_active_map_free_cache_fail;
        hz6_stats.toy_small_active_map_route_bypass +=
            args[i].hz6_stats_after.toy_small_active_map_route_bypass;
        for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT;
             ++class_id) {
            hz6_stats.frontcache_push_by_class[class_id] +=
                args[i].hz6_stats_after.frontcache_push_by_class[class_id];
            hz6_stats.frontcache_pop_empty_by_class[class_id] +=
                args[i].hz6_stats_after
                    .frontcache_pop_empty_by_class[class_id];
        }
#undef HZ6_MAX_STAT
#endif
        hz6_stats.route_lookup_probe_total +=
            args[i].hz6_stats_after.route_lookup_probe_total;
#if HZ6_DIAGNOSTIC_PROBES
        for (size_t bucket = 0; bucket < HZ6_ROUTE_PROBE_BUCKET_COUNT;
             ++bucket) {
            hz6_stats.route_lookup_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_lookup_probe_hist[bucket];
            hz6_stats.route_register_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_register_probe_hist[bucket];
            hz6_stats.route_unregister_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_unregister_probe_hist[bucket];
        }
#endif
        if (args[i].hz6_stats_after.route_lookup_probe_max >
            hz6_stats.route_lookup_probe_max) {
            hz6_stats.route_lookup_probe_max =
                args[i].hz6_stats_after.route_lookup_probe_max;
        }
        hz6_stats.route_register_probe_total +=
            args[i].hz6_stats_after.route_register_probe_total;
        if (args[i].hz6_stats_after.route_register_probe_max >
            hz6_stats.route_register_probe_max) {
            hz6_stats.route_register_probe_max =
                args[i].hz6_stats_after.route_register_probe_max;
        }
        hz6_stats.route_unregister_probe_total +=
            args[i].hz6_stats_after.route_unregister_probe_total;
        if (args[i].hz6_stats_after.route_unregister_probe_max >
            hz6_stats.route_unregister_probe_max) {
            hz6_stats.route_unregister_probe_max =
                args[i].hz6_stats_after.route_unregister_probe_max;
        }
        hz6_stats.source_block_probe_total +=
            args[i].hz6_stats_after.source_block_probe_total;
        if (args[i].hz6_stats_after.source_block_probe_max >
            hz6_stats.source_block_probe_max) {
            hz6_stats.source_block_probe_max =
                args[i].hz6_stats_after.source_block_probe_max;
        }
#if HZ6_DIAGNOSTIC_PROBES
#define HZ6_MAX_STAT(field) \
        do { \
            if (args[i].hz6_stats_after.field > hz6_stats.field) { \
                hz6_stats.field = args[i].hz6_stats_after.field; \
            } \
        } while (0)
        HZ6_MAX_STAT(source_block_fail_active_max);
        HZ6_MAX_STAT(source_block_fail_registered_max);
        HZ6_MAX_STAT(source_block_fail_ref_nonzero_max);
        HZ6_MAX_STAT(source_block_fail_ref_zero_max);
        HZ6_MAX_STAT(source_block_active_max);
        HZ6_MAX_STAT(frontcache_total_max);
#undef HZ6_MAX_STAT
#endif
        hz6_stats.large_span_central_push +=
            args[i].hz6_stats_after.large_span_central_push;
        hz6_stats.large_span_central_pop +=
            args[i].hz6_stats_after.large_span_central_pop;
        hz6_stats.large_span_source_alloc +=
            args[i].hz6_stats_after.large_span_source_alloc;
#endif
