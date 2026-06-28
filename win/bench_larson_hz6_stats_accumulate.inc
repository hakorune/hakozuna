        hz6_stats.route_valid += tds[t].hz6_stats_after.route_valid;
        hz6_stats.route_invalid += tds[t].hz6_stats_after.route_invalid;
        hz6_stats.route_miss += tds[t].hz6_stats_after.route_miss;
        hz6_stats.route_visibility_lookup +=
            tds[t].hz6_stats_after.route_visibility_lookup;
        hz6_stats.route_visibility_hit +=
            tds[t].hz6_stats_after.route_visibility_hit;
        hz6_stats.route_visibility_hit_local_owner +=
            tds[t].hz6_stats_after.route_visibility_hit_local_owner;
        hz6_stats.route_visibility_hit_foreign_owner +=
            tds[t].hz6_stats_after.route_visibility_hit_foreign_owner;
        hz6_stats.route_visibility_miss +=
            tds[t].hz6_stats_after.route_visibility_miss;
        hz6_stats.route_visibility_probe_total +=
            tds[t].hz6_stats_after.route_visibility_probe_total;
        if (tds[t].hz6_stats_after.route_visibility_probe_max >
            hz6_stats.route_visibility_probe_max) {
            hz6_stats.route_visibility_probe_max =
                tds[t].hz6_stats_after.route_visibility_probe_max;
        }
        hz6_stats.transfer_push += tds[t].hz6_stats_after.transfer_push;
        hz6_stats.transfer_pop += tds[t].hz6_stats_after.transfer_pop;
        hz6_stats.transfer_current +=
            tds[t].hz6_stats_after.transfer_current;
        if (tds[t].hz6_stats_after.transfer_current_max >
            hz6_stats.transfer_current_max) {
            hz6_stats.transfer_current_max =
                tds[t].hz6_stats_after.transfer_current_max;
        }
        hz6_stats.remote_free_attempt +=
            tds[t].hz6_stats_after.remote_free_attempt;
        hz6_stats.remote_free_strict_owner_block +=
            tds[t].hz6_stats_after.remote_free_strict_owner_block;
        hz6_stats.remote_free_transfer_fail +=
            tds[t].hz6_stats_after.remote_free_transfer_fail;
        hz6_stats.route_rehome_attempt +=
            tds[t].hz6_stats_after.route_rehome_attempt;
        hz6_stats.route_rehome_success +=
            tds[t].hz6_stats_after.route_rehome_success;
        hz6_stats.route_rehome_fail +=
            tds[t].hz6_stats_after.route_rehome_fail;
        hz6_stats.descriptor_source_route_allocator_match +=
            tds[t].hz6_stats_after.descriptor_source_route_allocator_match;
        hz6_stats.descriptor_source_route_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_source_route_allocator_mismatch;
        hz6_stats.descriptor_source_current_allocator_match +=
            tds[t].hz6_stats_after.descriptor_source_current_allocator_match;
        hz6_stats.descriptor_source_current_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_source_current_allocator_mismatch;
        hz6_stats.descriptor_storage_lookup +=
            tds[t].hz6_stats_after.descriptor_storage_lookup;
        hz6_stats.descriptor_storage_hit +=
            tds[t].hz6_stats_after.descriptor_storage_hit;
        hz6_stats.descriptor_storage_miss +=
            tds[t].hz6_stats_after.descriptor_storage_miss;
        hz6_stats.descriptor_storage_probe_total +=
            tds[t].hz6_stats_after.descriptor_storage_probe_total;
        if (tds[t].hz6_stats_after.descriptor_storage_probe_max >
            hz6_stats.descriptor_storage_probe_max) {
            hz6_stats.descriptor_storage_probe_max =
                tds[t].hz6_stats_after.descriptor_storage_probe_max;
        }
        hz6_stats.descriptor_storage_route_allocator_match +=
            tds[t].hz6_stats_after.descriptor_storage_route_allocator_match;
        hz6_stats.descriptor_storage_route_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_storage_route_allocator_mismatch;
        hz6_stats.descriptor_storage_current_allocator_match +=
            tds[t].hz6_stats_after.descriptor_storage_current_allocator_match;
        hz6_stats.descriptor_storage_current_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_storage_current_allocator_mismatch;
        hz6_stats.owner_source_side_meta_lookup +=
            tds[t].hz6_stats_after.owner_source_side_meta_lookup;
        hz6_stats.owner_source_side_meta_local +=
            tds[t].hz6_stats_after.owner_source_side_meta_local;
        hz6_stats.owner_source_side_meta_foreign +=
            tds[t].hz6_stats_after.owner_source_side_meta_foreign;
        hz6_stats.owner_source_side_meta_miss +=
            tds[t].hz6_stats_after.owner_source_side_meta_miss;
        hz6_stats.owner_source_side_meta_probe_total +=
            tds[t].hz6_stats_after.owner_source_side_meta_probe_total;
        if (tds[t].hz6_stats_after.owner_source_side_meta_probe_max >
            hz6_stats.owner_source_side_meta_probe_max) {
            hz6_stats.owner_source_side_meta_probe_max =
                tds[t].hz6_stats_after.owner_source_side_meta_probe_max;
        }
        hz6_stats.owner_source_side_meta_l2_lookup +=
            tds[t].hz6_stats_after.owner_source_side_meta_l2_lookup;
        hz6_stats.owner_source_side_meta_l2_hit +=
            tds[t].hz6_stats_after.owner_source_side_meta_l2_hit;
        hz6_stats.owner_source_side_meta_l2_miss_no_block +=
            tds[t].hz6_stats_after.owner_source_side_meta_l2_miss_no_block;
        hz6_stats.owner_source_side_meta_l2_miss_inactive +=
            tds[t].hz6_stats_after.owner_source_side_meta_l2_miss_inactive;
        hz6_stats.owner_source_side_meta_l2_storage_mismatch +=
            tds[t].hz6_stats_after.owner_source_side_meta_l2_storage_mismatch;
        hz6_stats.lifecycle_owner_mismatch +=
            tds[t].hz6_stats_after.lifecycle_owner_mismatch;
        hz6_stats.lifecycle_foreign_free_attempt +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_attempt;
        hz6_stats.lifecycle_foreign_free_handled +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_handled;
        hz6_stats.lifecycle_foreign_free_invalid +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_invalid;
        hz6_stats.visible_first_attempt +=
            tds[t].hz6_stats_after.visible_first_attempt;
        hz6_stats.visible_first_hit += tds[t].hz6_stats_after.visible_first_hit;
        hz6_stats.visible_first_miss +=
            tds[t].hz6_stats_after.visible_first_miss;
        hz6_stats.visible_first_visible_invalid +=
            tds[t].hz6_stats_after.visible_first_visible_invalid;
        hz6_stats.visible_first_local_fallback +=
            tds[t].hz6_stats_after.visible_first_local_fallback;
        hz6_stats.visible_first_local_fallback_invalid +=
            tds[t].hz6_stats_after.visible_first_local_fallback_invalid;
        hz6_stats.visible_first_local_lookup_skipped +=
            tds[t].hz6_stats_after.visible_first_local_lookup_skipped;
        hz6_stats.negative_filter_attempt +=
            tds[t].hz6_stats_after.negative_filter_attempt;
        hz6_stats.negative_filter_not_armed +=
            tds[t].hz6_stats_after.negative_filter_not_armed;
        hz6_stats.negative_filter_rehome_blocked +=
            tds[t].hz6_stats_after.negative_filter_rehome_blocked;
        hz6_stats.negative_filter_skip_local +=
            tds[t].hz6_stats_after.negative_filter_skip_local;
        hz6_stats.negative_filter_maybe_local +=
            tds[t].hz6_stats_after.negative_filter_maybe_local;
        hz6_stats.negative_filter_shadow_false_skip +=
            tds[t].hz6_stats_after.negative_filter_shadow_false_skip;
        hz6_stats.negative_filter_shadow_local_valid +=
            tds[t].hz6_stats_after.negative_filter_shadow_local_valid;
        hz6_stats.negative_filter_shadow_local_invalid +=
            tds[t].hz6_stats_after.negative_filter_shadow_local_invalid;
        hz6_stats.negative_filter_range_probe_total +=
            tds[t].hz6_stats_after.negative_filter_range_probe_total;
        if (tds[t].hz6_stats_after.negative_filter_range_probe_max >
            hz6_stats.negative_filter_range_probe_max) {
            hz6_stats.negative_filter_range_probe_max =
                tds[t].hz6_stats_after.negative_filter_range_probe_max;
        }
        hz6_stats.shared_dir_lookup +=
            tds[t].hz6_stats_after.shared_dir_lookup;
        hz6_stats.shared_dir_hit +=
            tds[t].hz6_stats_after.shared_dir_hit;
        hz6_stats.shared_dir_miss +=
            tds[t].hz6_stats_after.shared_dir_miss;
        hz6_stats.shared_dir_stale +=
            tds[t].hz6_stats_after.shared_dir_stale;
        hz6_stats.shared_dir_hit_local_allocator +=
            tds[t].hz6_stats_after.shared_dir_hit_local_allocator;
        hz6_stats.shared_dir_hit_foreign_allocator +=
            tds[t].hz6_stats_after.shared_dir_hit_foreign_allocator;
        hz6_stats.shared_dir_would_skip_local +=
            tds[t].hz6_stats_after.shared_dir_would_skip_local;
        hz6_stats.shared_dir_register +=
            tds[t].hz6_stats_after.shared_dir_register;
        hz6_stats.shared_dir_unregister +=
            tds[t].hz6_stats_after.shared_dir_unregister;
        hz6_stats.shared_dir_probe_total +=
            tds[t].hz6_stats_after.shared_dir_probe_total;
        if (tds[t].hz6_stats_after.shared_dir_probe_max >
            hz6_stats.shared_dir_probe_max) {
            hz6_stats.shared_dir_probe_max =
                tds[t].hz6_stats_after.shared_dir_probe_max;
        }
        hz6_stats.shared_dir_first_attempt +=
            tds[t].hz6_stats_after.shared_dir_first_attempt;
        hz6_stats.shared_dir_first_hit +=
            tds[t].hz6_stats_after.shared_dir_first_hit;
        hz6_stats.shared_dir_first_fallback +=
            tds[t].hz6_stats_after.shared_dir_first_fallback;
        hz6_stats.shared_dir_first_invalid +=
            tds[t].hz6_stats_after.shared_dir_first_invalid;
        hz6_stats.elastic_route_overflow_register +=
            tds[t].hz6_stats_after.elastic_route_overflow_register;
        hz6_stats.elastic_route_overflow_register_fail +=
            tds[t].hz6_stats_after.elastic_route_overflow_register_fail;
        hz6_stats.elastic_route_overflow_lookup +=
            tds[t].hz6_stats_after.elastic_route_overflow_lookup;
        hz6_stats.elastic_route_overflow_hit +=
            tds[t].hz6_stats_after.elastic_route_overflow_hit;
        hz6_stats.elastic_descriptor_overflow_alloc +=
            tds[t].hz6_stats_after.elastic_descriptor_overflow_alloc;
        hz6_stats.elastic_descriptor_overflow_reset +=
            tds[t].hz6_stats_after.elastic_descriptor_overflow_reset;
        hz6_stats.elastic_descriptor_overflow_exhausted +=
            tds[t].hz6_stats_after.elastic_descriptor_overflow_exhausted;
        hz6_stats.elastic_source_block_overflow_alloc +=
            tds[t].hz6_stats_after.elastic_source_block_overflow_alloc;
        hz6_stats.elastic_source_block_overflow_release +=
            tds[t].hz6_stats_after.elastic_source_block_overflow_release;
        hz6_stats.elastic_source_block_overflow_exhausted +=
            tds[t].hz6_stats_after.elastic_source_block_overflow_exhausted;
        hz6_stats.elastic_source_block_localize_probe +=
            tds[t].hz6_stats_after.elastic_source_block_localize_probe;
        hz6_stats.elastic_source_block_localize_storage_match +=
            tds[t].hz6_stats_after
                .elastic_source_block_localize_storage_match;
        hz6_stats.elastic_source_block_localize_storage_mismatch +=
            tds[t].hz6_stats_after
                .elastic_source_block_localize_storage_mismatch;
        hz6_stats.elastic_source_block_localize_would_move +=
            tds[t].hz6_stats_after.elastic_source_block_localize_would_move;
        hz6_stats.elastic_source_block_localize_block_shared +=
            tds[t].hz6_stats_after.elastic_source_block_localize_block_shared;
        hz6_stats.elastic_source_block_localize_no_local_slot +=
            tds[t].hz6_stats_after
                .elastic_source_block_localize_no_local_slot;
        hz6_stats.elastic_source_run_locality_probe +=
            tds[t].hz6_stats_after.elastic_source_run_locality_probe;
        hz6_stats.elastic_source_run_locality_storage_mismatch +=
            tds[t].hz6_stats_after
                .elastic_source_run_locality_storage_mismatch;
        hz6_stats.elastic_source_run_locality_run_miss +=
            tds[t].hz6_stats_after.elastic_source_run_locality_run_miss;
        hz6_stats.elastic_source_run_locality_class_mismatch +=
            tds[t].hz6_stats_after
                .elastic_source_run_locality_class_mismatch;
        hz6_stats.elastic_source_run_locality_slot_match +=
            tds[t].hz6_stats_after.elastic_source_run_locality_slot_match;
        hz6_stats.elastic_source_run_locality_would_rehome_slot +=
            tds[t].hz6_stats_after
                .elastic_source_run_locality_would_rehome_slot;
        hz6_stats.elastic_depot_run_meta_init +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_init;
        hz6_stats.elastic_depot_run_meta_mark +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_mark;
        hz6_stats.elastic_depot_run_meta_clear +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_clear;
        hz6_stats.elastic_depot_run_meta_class_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_class_mismatch;
        hz6_stats.elastic_depot_run_meta_slot_misaligned +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_slot_misaligned;
        hz6_stats.elastic_depot_run_meta_too_many_slots +=
            tds[t].hz6_stats_after.elastic_depot_run_meta_too_many_slots;
        hz6_stats.elastic_depot_run_meta_used_count_mismatch +=
            tds[t].hz6_stats_after
                .elastic_depot_run_meta_used_count_mismatch;
        hz6_stats.elastic_slot_owner_locality_probe +=
            tds[t].hz6_stats_after.elastic_slot_owner_locality_probe;
        hz6_stats.elastic_slot_owner_locality_storage_match +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_storage_match;
        hz6_stats.elastic_slot_owner_locality_storage_mismatch +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_storage_mismatch;
        hz6_stats.elastic_slot_owner_locality_run_miss +=
            tds[t].hz6_stats_after.elastic_slot_owner_locality_run_miss;
        hz6_stats.elastic_slot_owner_locality_class_mismatch +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_class_mismatch;
        hz6_stats.elastic_slot_owner_locality_slot_match +=
            tds[t].hz6_stats_after.elastic_slot_owner_locality_slot_match;
        hz6_stats.elastic_slot_owner_locality_owner_match +=
            tds[t].hz6_stats_after.elastic_slot_owner_locality_owner_match;
        hz6_stats.elastic_slot_owner_locality_owner_mismatch +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_owner_mismatch;
        hz6_stats.elastic_slot_owner_locality_would_set_owner +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_would_set_owner;
        hz6_stats.elastic_slot_owner_locality_would_hit_owner +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_locality_would_hit_owner;
        hz6_stats.elastic_depot_drain_probe +=
            tds[t].hz6_stats_after.elastic_depot_drain_probe;
        hz6_stats.elastic_depot_drain_storage_match +=
            tds[t].hz6_stats_after.elastic_depot_drain_storage_match;
        hz6_stats.elastic_depot_drain_storage_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_drain_storage_mismatch;
        hz6_stats.elastic_depot_drain_run_match +=
            tds[t].hz6_stats_after.elastic_depot_drain_run_match;
        hz6_stats.elastic_depot_drain_run_miss +=
            tds[t].hz6_stats_after.elastic_depot_drain_run_miss;
        hz6_stats.elastic_depot_drain_class_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_drain_class_mismatch;
        hz6_stats.elastic_depot_drain_slot_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_drain_slot_mismatch;
        hz6_stats.elastic_depot_drain_ref_exclusive +=
            tds[t].hz6_stats_after.elastic_depot_drain_ref_exclusive;
        hz6_stats.elastic_depot_drain_ref_shared +=
            tds[t].hz6_stats_after.elastic_depot_drain_ref_shared;
        hz6_stats.elastic_depot_drain_owner_match +=
            tds[t].hz6_stats_after.elastic_depot_drain_owner_match;
        hz6_stats.elastic_depot_drain_owner_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_drain_owner_mismatch;
        hz6_stats.elastic_depot_drain_would_slot_localize +=
            tds[t].hz6_stats_after.elastic_depot_drain_would_slot_localize;
        hz6_stats.elastic_depot_drain_would_keep_shared +=
            tds[t].hz6_stats_after.elastic_depot_drain_would_keep_shared;
        hz6_stats.elastic_depot_drain_would_block_whole_localize +=
            tds[t].hz6_stats_after
                .elastic_depot_drain_would_block_whole_localize;
        hz6_stats.elastic_depot_slot_localize_attempt +=
            tds[t].hz6_stats_after.elastic_depot_slot_localize_attempt;
        hz6_stats.elastic_depot_slot_localize_success +=
            tds[t].hz6_stats_after.elastic_depot_slot_localize_success;
        hz6_stats.elastic_depot_slot_localize_ineligible +=
            tds[t].hz6_stats_after.elastic_depot_slot_localize_ineligible;
        hz6_stats.elastic_depot_slot_localize_storage_hit +=
            tds[t].hz6_stats_after
                .elastic_depot_slot_localize_storage_hit;
        hz6_stats.elastic_depot_slot_localize_storage_miss +=
            tds[t].hz6_stats_after
                .elastic_depot_slot_localize_storage_miss;
        hz6_stats.elastic_depot_slot_localize_storage_stale +=
            tds[t].hz6_stats_after
                .elastic_depot_slot_localize_storage_stale;
        hz6_stats.elastic_depot_descriptor_rehome_probe +=
            tds[t].hz6_stats_after.elastic_depot_descriptor_rehome_probe;
        hz6_stats.elastic_depot_descriptor_rehome_depot_descriptor +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_depot_descriptor;
        hz6_stats.elastic_depot_descriptor_rehome_already_local +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_already_local;
        hz6_stats.elastic_depot_descriptor_rehome_run_match +=
            tds[t].hz6_stats_after.elastic_depot_descriptor_rehome_run_match;
        hz6_stats.elastic_depot_descriptor_rehome_run_mismatch +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_run_mismatch;
        hz6_stats
            .elastic_depot_descriptor_rehome_local_descriptor_available +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_local_descriptor_available;
        hz6_stats.elastic_depot_descriptor_rehome_no_local_descriptor +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_no_local_descriptor;
        hz6_stats.elastic_depot_descriptor_rehome_would_rehome +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_would_rehome;
        hz6_stats.elastic_depot_route_replace_probe +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_probe;
        hz6_stats.elastic_depot_route_replace_depot_descriptor +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_depot_descriptor;
        hz6_stats.elastic_depot_route_replace_run_match +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_run_match;
        hz6_stats.elastic_depot_route_replace_run_mismatch +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_run_mismatch;
        hz6_stats.elastic_depot_route_replace_origin_missing +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_origin_missing;
        hz6_stats.elastic_depot_route_replace_old_route_found +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_old_route_found;
        hz6_stats.elastic_depot_route_replace_old_route_missing +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_old_route_missing;
        hz6_stats.elastic_depot_route_replace_old_route_invalid +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_old_route_invalid;
        hz6_stats.elastic_depot_route_replace_descriptor_match +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_descriptor_match;
        hz6_stats.elastic_depot_route_replace_descriptor_mismatch +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_descriptor_mismatch;
        hz6_stats.elastic_depot_route_replace_generation_match +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_generation_match;
        hz6_stats.elastic_depot_route_replace_generation_mismatch +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_generation_mismatch;
        hz6_stats.elastic_depot_route_replace_front_class_match +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_front_class_match;
        hz6_stats.elastic_depot_route_replace_front_class_mismatch +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_front_class_mismatch;
        hz6_stats.elastic_depot_route_replace_current_route_empty +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_current_route_empty;
        hz6_stats.elastic_depot_route_replace_current_route_same +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_current_route_same;
        hz6_stats.elastic_depot_route_replace_current_route_conflict +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_current_route_conflict;
        hz6_stats.elastic_depot_route_replace_would_commit +=
            tds[t].hz6_stats_after.elastic_depot_route_replace_would_commit;
        hz6_stats.elastic_depot_route_replace_would_rollback +=
            tds[t].hz6_stats_after
                .elastic_depot_route_replace_would_rollback;
        hz6_stats.elastic_depot_descriptor_rehome_l1_attempt +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_attempt;
        hz6_stats.elastic_depot_descriptor_rehome_l1_success +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_success;
        hz6_stats.elastic_depot_descriptor_rehome_l1_ineligible +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_ineligible;
        hz6_stats.elastic_depot_descriptor_rehome_l1_no_local_descriptor +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_no_local_descriptor;
        hz6_stats.elastic_depot_descriptor_rehome_l1_prepare_fail +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_prepare_fail;
        hz6_stats.elastic_depot_descriptor_rehome_l1_route_replace_fail +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_route_replace_fail;
        hz6_stats.elastic_depot_descriptor_rehome_l1_detach_fail +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_detach_fail;
        hz6_stats.elastic_depot_descriptor_rehome_l1_rollback +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_rollback;
        hz6_stats.elastic_depot_descriptor_rehome_l1_budget_denied +=
            tds[t].hz6_stats_after
                .elastic_depot_descriptor_rehome_l1_budget_denied;
        hz6_stats.elastic_dftlc_rehome_intersection_directfree_hit +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_directfree_hit;
        hz6_stats.elastic_dftlc_rehome_intersection_directfree_fail +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_directfree_fail;
        hz6_stats.elastic_dftlc_rehome_intersection_transfer_probe +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_transfer_probe;
        hz6_stats.elastic_dftlc_rehome_intersection_transfer_depot +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_transfer_depot;
        hz6_stats.elastic_dftlc_rehome_intersection_transfer_already_local +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_transfer_already_local;
        hz6_stats.elastic_dftlc_rehome_intersection_transfer_would_rehome +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_transfer_would_rehome;
        hz6_stats.elastic_dftlc_rehome_intersection_rehome_success +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_rehome_success;
        hz6_stats.elastic_dftlc_rehome_intersection_rehome_ineligible +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_rehome_ineligible;
        hz6_stats.elastic_dftlc_rehome_intersection_rehome_budget_denied +=
            tds[t]
                .hz6_stats_after
                .elastic_dftlc_rehome_intersection_rehome_budget_denied;
        hz6_stats.elastic_slot_owner_sparse_lookup +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_lookup;
        hz6_stats.elastic_slot_owner_sparse_hit +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_hit;
        hz6_stats.elastic_slot_owner_sparse_miss +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_miss;
        hz6_stats.elastic_slot_owner_sparse_insert +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_insert;
        hz6_stats.elastic_slot_owner_sparse_update +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_update;
        hz6_stats.elastic_slot_owner_sparse_owner_match +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_owner_match;
        hz6_stats.elastic_slot_owner_sparse_owner_mismatch +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_owner_mismatch;
        hz6_stats.elastic_slot_owner_sparse_collision +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_collision;
        hz6_stats.elastic_slot_owner_sparse_full +=
            tds[t].hz6_stats_after.elastic_slot_owner_sparse_full;
        hz6_stats.elastic_slot_owner_consumer_probe +=
            tds[t].hz6_stats_after.elastic_slot_owner_consumer_probe;
        hz6_stats.elastic_slot_owner_consumer_hit +=
            tds[t].hz6_stats_after.elastic_slot_owner_consumer_hit;
        hz6_stats.elastic_slot_owner_consumer_miss +=
            tds[t].hz6_stats_after.elastic_slot_owner_consumer_miss;
        hz6_stats.elastic_slot_owner_consumer_owner_match +=
            tds[t].hz6_stats_after.elastic_slot_owner_consumer_owner_match;
        hz6_stats.elastic_slot_owner_consumer_owner_mismatch +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_consumer_owner_mismatch;
        hz6_stats.elastic_slot_owner_consumer_stale_generation +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_consumer_stale_generation;
        hz6_stats.elastic_slot_owner_consumer_false_positive +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_consumer_false_positive;
        hz6_stats.elastic_slot_owner_consumer_would_skip_l2 +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_consumer_would_skip_l2;
        hz6_stats.elastic_slot_owner_consumer_fallback +=
            tds[t].hz6_stats_after.elastic_slot_owner_consumer_fallback;
        hz6_stats.elastic_slot_owner_logical_fastpath_probe +=
            tds[t].hz6_stats_after.elastic_slot_owner_logical_fastpath_probe;
        hz6_stats.elastic_slot_owner_logical_fastpath_hit +=
            tds[t].hz6_stats_after.elastic_slot_owner_logical_fastpath_hit;
        hz6_stats.elastic_slot_owner_logical_fastpath_miss +=
            tds[t].hz6_stats_after.elastic_slot_owner_logical_fastpath_miss;
        hz6_stats.elastic_slot_owner_logical_fastpath_stale_generation +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_logical_fastpath_stale_generation;
        hz6_stats.elastic_slot_owner_logical_fastpath_owner_mismatch +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_logical_fastpath_owner_mismatch;
        hz6_stats.elastic_slot_owner_logical_fastpath_fallback +=
            tds[t].hz6_stats_after
                .elastic_slot_owner_logical_fastpath_fallback;
        hz6_stats.owner_equal_site_free +=
            tds[t].hz6_stats_after.owner_equal_site_free;
        hz6_stats.owner_equal_site_remote_free +=
            tds[t].hz6_stats_after.owner_equal_site_remote_free;
        hz6_stats.owner_equal_site_local_cache +=
            tds[t].hz6_stats_after.owner_equal_site_local_cache;
        hz6_stats.owner_equal_site_visible_lookup +=
            tds[t].hz6_stats_after.owner_equal_site_visible_lookup;
        hz6_stats.owner_equal_site_transfer_locality +=
            tds[t].hz6_stats_after.owner_equal_site_transfer_locality;
        hz6_stats.owner_equal_site_large_central +=
            tds[t].hz6_stats_after.owner_equal_site_large_central;
        hz6_stats.owner_equal_site_remote_pending +=
            tds[t].hz6_stats_after.owner_equal_site_remote_pending;
        hz6_stats.owner_equal_site_owner_dead +=
            tds[t].hz6_stats_after.owner_equal_site_owner_dead;
        hz6_stats.owner_equal_site_same_owner_fast +=
            tds[t].hz6_stats_after.owner_equal_site_same_owner_fast;
        hz6_stats.owner_equal_site_unknown +=
            tds[t].hz6_stats_after.owner_equal_site_unknown;
        hz6_stats.flc_owner_predicate_probe +=
            tds[t].hz6_stats_after.flc_owner_predicate_probe;
        hz6_stats.flc_owner_predicate_site_free +=
            tds[t].hz6_stats_after.flc_owner_predicate_site_free;
        hz6_stats.flc_owner_predicate_site_local_cache +=
            tds[t].hz6_stats_after.flc_owner_predicate_site_local_cache;
        hz6_stats.flc_owner_predicate_depot_descriptor +=
            tds[t].hz6_stats_after.flc_owner_predicate_depot_descriptor;
        hz6_stats.flc_owner_predicate_local_descriptor +=
            tds[t].hz6_stats_after.flc_owner_predicate_local_descriptor;
        hz6_stats.flc_owner_predicate_foreign_descriptor +=
            tds[t].hz6_stats_after.flc_owner_predicate_foreign_descriptor;
        hz6_stats.flc_owner_predicate_no_source_block +=
            tds[t].hz6_stats_after.flc_owner_predicate_no_source_block;
        hz6_stats.flc_owner_predicate_source_block +=
            tds[t].hz6_stats_after.flc_owner_predicate_source_block;
        hz6_stats.flc_owner_predicate_source_block_active +=
            tds[t].hz6_stats_after.flc_owner_predicate_source_block_active;
        hz6_stats.flc_owner_predicate_source_block_shared +=
            tds[t].hz6_stats_after.flc_owner_predicate_source_block_shared;
        hz6_stats.flc_owner_predicate_source_run_active +=
            tds[t].hz6_stats_after.flc_owner_predicate_source_run_active;
        hz6_stats.flc_owner_predicate_source_release +=
            tds[t].hz6_stats_after.flc_owner_predicate_source_release;
        hz6_stats.depot_owner_equal_fastpath_probe +=
            tds[t].hz6_stats_after.depot_owner_equal_fastpath_probe;
        hz6_stats.depot_owner_equal_fastpath_hit +=
            tds[t].hz6_stats_after.depot_owner_equal_fastpath_hit;
        hz6_stats.depot_owner_equal_fastpath_miss +=
            tds[t].hz6_stats_after.depot_owner_equal_fastpath_miss;
        hz6_stats.depot_owner_equal_fastpath_fallback +=
            tds[t].hz6_stats_after.depot_owner_equal_fastpath_fallback;
        hz6_stats.depot_owner_equal_fastpath_other_site +=
            tds[t].hz6_stats_after.depot_owner_equal_fastpath_other_site;
        hz6_stats.source_owned_prepare +=
            tds[t].hz6_stats_after.source_owned_prepare;
        hz6_stats.source_owned_route_hit_local_owner +=
            tds[t].hz6_stats_after.source_owned_route_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_local_owner +=
            tds[t].hz6_stats_after.source_owned_visibility_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_foreign_owner +=
            tds[t].hz6_stats_after.source_owned_visibility_hit_foreign_owner;
        hz6_stats.source_owned_remote_free_attempt +=
            tds[t].hz6_stats_after.source_owned_remote_free_attempt;
        hz6_stats.source_owned_release +=
            tds[t].hz6_stats_after.source_owned_release;
        hz6_stats.source_alloc += tds[t].hz6_stats_after.source_alloc;
        hz6_stats.local2p_source_alloc +=
            tds[t].hz6_stats_after.local2p_source_alloc;
        hz6_stats.midpage_source_alloc +=
            tds[t].hz6_stats_after.midpage_source_alloc;
        hz6_stats.large_source_alloc +=
            tds[t].hz6_stats_after.large_source_alloc;
        hz6_stats.toy_source_alloc += tds[t].hz6_stats_after.toy_source_alloc;
        for (size_t front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
            for (size_t path = 0; path < HZ6_ALLOC_PATH_COUNT; ++path) {
                hz6_stats.front_alloc_path[front][path] +=
                    tds[t].hz6_stats_after.front_alloc_path[front][path];
            }
        }
        hz6_stats.frontcache_reuse_hit +=
            tds[t].hz6_stats_after.frontcache_reuse_hit;
        hz6_stats.frontcache_reuse_invalid +=
            tds[t].hz6_stats_after.frontcache_reuse_invalid;
        for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT;
             ++class_id) {
            hz6_stats.frontcache_push_by_class[class_id] +=
                tds[t].hz6_stats_after.frontcache_push_by_class[class_id];
            hz6_stats.frontcache_pop_empty_by_class[class_id] +=
                tds[t].hz6_stats_after.frontcache_pop_empty_by_class[class_id];
        }
        hz6_stats.transfer_reuse_hit +=
            tds[t].hz6_stats_after.transfer_reuse_hit;
        hz6_stats.transfer_reuse_invalid +=
            tds[t].hz6_stats_after.transfer_reuse_invalid;
        hz6_stats.source_refill_starvation +=
            tds[t].hz6_stats_after.source_refill_starvation;
        hz6_stats.source_refill_saturation +=
            tds[t].hz6_stats_after.source_refill_saturation;
        hz6_stats.source_refill_boost +=
            tds[t].hz6_stats_after.source_refill_boost;
        hz6_stats.source_refill_clamp +=
            tds[t].hz6_stats_after.source_refill_clamp;
        hz6_stats.source_admission_open +=
            tds[t].hz6_stats_after.source_admission_open;
        hz6_stats.source_admission_boosted +=
            tds[t].hz6_stats_after.source_admission_boosted;
        hz6_stats.source_admission_clamped +=
            tds[t].hz6_stats_after.source_admission_clamped;
        hz6_stats.control_plane_normal +=
            tds[t].hz6_stats_after.control_plane_normal;
        hz6_stats.control_plane_burst_supply_would_open +=
            tds[t].hz6_stats_after.control_plane_burst_supply_would_open;
        hz6_stats.control_plane_close_would_start +=
            tds[t].hz6_stats_after.control_plane_close_would_start;
        hz6_stats.source_prefill_attempt +=
            tds[t].hz6_stats_after.source_prefill_attempt;
        hz6_stats.source_prefill_filled +=
            tds[t].hz6_stats_after.source_prefill_filled;
        hz6_stats.source_prefill_fallback +=
            tds[t].hz6_stats_after.source_prefill_fallback;
        for (size_t front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
            hz6_stats.front_source_prefill_attempt[front] +=
                tds[t].hz6_stats_after.front_source_prefill_attempt[front];
            hz6_stats.front_source_prefill_filled[front] +=
                tds[t].hz6_stats_after.front_source_prefill_filled[front];
            hz6_stats.front_source_prefill_fallback[front] +=
                tds[t].hz6_stats_after.front_source_prefill_fallback[front];
        }
        hz6_stats.front_source_ops_alloc +=
            tds[t].hz6_stats_after.front_source_ops_alloc;
        hz6_stats.front_source_slot_alloc +=
            tds[t].hz6_stats_after.front_source_slot_alloc;
        hz6_stats.front_source_prefill_alloc +=
            tds[t].hz6_stats_after.front_source_prefill_alloc;
        hz6_stats.alloc_fail += tds[t].hz6_stats_after.alloc_fail;
        hz6_stats.descriptor_exhausted +=
            tds[t].hz6_stats_after.descriptor_exhausted;
        hz6_stats.route_register_fail +=
            tds[t].hz6_stats_after.route_register_fail;
        hz6_stats.source_block_exhausted +=
            tds[t].hz6_stats_after.source_block_exhausted;
        hz6_stats.descriptor_probe_total +=
            tds[t].hz6_stats_after.descriptor_probe_total;
        if (tds[t].hz6_stats_after.descriptor_probe_max >
            hz6_stats.descriptor_probe_max) {
            hz6_stats.descriptor_probe_max =
                tds[t].hz6_stats_after.descriptor_probe_max;
        }
        hz6_stats.route_exact_lookup_probe_total +=
            tds[t].hz6_stats_after.route_exact_lookup_probe_total;
        if (tds[t].hz6_stats_after.route_exact_lookup_probe_max >
            hz6_stats.route_exact_lookup_probe_max) {
            hz6_stats.route_exact_lookup_probe_max =
                tds[t].hz6_stats_after.route_exact_lookup_probe_max;
        }
        hz6_stats.owner_locality_lookup +=
            tds[t].hz6_stats_after.owner_locality_lookup;
        hz6_stats.owner_locality_hit_local_allocator +=
            tds[t].hz6_stats_after.owner_locality_hit_local_allocator;
        hz6_stats.owner_locality_hit_foreign_allocator +=
            tds[t].hz6_stats_after.owner_locality_hit_foreign_allocator;
        hz6_stats.owner_locality_miss +=
            tds[t].hz6_stats_after.owner_locality_miss;
        hz6_stats.owner_locality_register +=
            tds[t].hz6_stats_after.owner_locality_register;
        hz6_stats.owner_locality_unregister +=
            tds[t].hz6_stats_after.owner_locality_unregister;
        hz6_stats.owner_locality_probe_total +=
            tds[t].hz6_stats_after.owner_locality_probe_total;
        if (tds[t].hz6_stats_after.owner_locality_probe_max >
            hz6_stats.owner_locality_probe_max) {
            hz6_stats.owner_locality_probe_max =
                tds[t].hz6_stats_after.owner_locality_probe_max;
        }
        hz6_stats.route_lookup_probe_total +=
            tds[t].hz6_stats_after.route_lookup_probe_total;
        if (tds[t].hz6_stats_after.route_lookup_probe_max >
            hz6_stats.route_lookup_probe_max) {
            hz6_stats.route_lookup_probe_max =
                tds[t].hz6_stats_after.route_lookup_probe_max;
        }
        hz6_stats.route_register_probe_total +=
            tds[t].hz6_stats_after.route_register_probe_total;
        if (tds[t].hz6_stats_after.route_register_probe_max >
            hz6_stats.route_register_probe_max) {
            hz6_stats.route_register_probe_max =
                tds[t].hz6_stats_after.route_register_probe_max;
        }
        hz6_stats.route_unregister_probe_total +=
            tds[t].hz6_stats_after.route_unregister_probe_total;
        if (tds[t].hz6_stats_after.route_unregister_probe_max >
            hz6_stats.route_unregister_probe_max) {
            hz6_stats.route_unregister_probe_max =
                tds[t].hz6_stats_after.route_unregister_probe_max;
        }
        if (tds[t].hz6_stats_after.descriptor_live_max >
            hz6_stats.descriptor_live_max) {
            hz6_stats.descriptor_live_max =
                tds[t].hz6_stats_after.descriptor_live_max;
        }
        if (tds[t].hz6_stats_after.source_block_active_max >
            hz6_stats.source_block_active_max) {
            hz6_stats.source_block_active_max =
                tds[t].hz6_stats_after.source_block_active_max;
        }
        if (tds[t].hz6_stats_after.frontcache_total_max >
            hz6_stats.frontcache_total_max) {
            hz6_stats.frontcache_total_max =
                tds[t].hz6_stats_after.frontcache_total_max;
        }
        hz6_stats.source_block_probe_total +=
            tds[t].hz6_stats_after.source_block_probe_total;
        if (tds[t].hz6_stats_after.source_block_probe_max >
            hz6_stats.source_block_probe_max) {
            hz6_stats.source_block_probe_max =
                tds[t].hz6_stats_after.source_block_probe_max;
        }
        hz6_stats.memory_descriptor_table_bytes +=
            tds[t].hz6_stats_after.memory_descriptor_table_bytes;
        hz6_stats.memory_route_table_bytes +=
            tds[t].hz6_stats_after.memory_route_table_bytes;
        hz6_stats.memory_source_block_table_bytes +=
            tds[t].hz6_stats_after.memory_source_block_table_bytes;
        if (tds[t].hz6_stats_after.memory_source_block_depot_bytes >
            hz6_stats.memory_source_block_depot_bytes) {
            hz6_stats.memory_source_block_depot_bytes =
                tds[t].hz6_stats_after.memory_source_block_depot_bytes;
        }
        hz6_stats.memory_frontcache_table_bytes +=
            tds[t].hz6_stats_after.memory_frontcache_table_bytes;
        hz6_stats.memory_transfer_table_bytes +=
            tds[t].hz6_stats_after.memory_transfer_table_bytes;
        if (tds[t].hz6_stats_after.memory_ownerlocality_index_bytes >
            hz6_stats.memory_ownerlocality_index_bytes) {
            hz6_stats.memory_ownerlocality_index_bytes =
                tds[t].hz6_stats_after.memory_ownerlocality_index_bytes;
        }
        if (tds[t].hz6_stats_after.memory_shared_route_directory_bytes >
            hz6_stats.memory_shared_route_directory_bytes) {
            hz6_stats.memory_shared_route_directory_bytes =
                tds[t].hz6_stats_after.memory_shared_route_directory_bytes;
        }
        if (tds[t].hz6_stats_after.memory_owner_locality_index_bytes >
            hz6_stats.memory_owner_locality_index_bytes) {
            hz6_stats.memory_owner_locality_index_bytes =
                tds[t].hz6_stats_after.memory_owner_locality_index_bytes;
        }
        hz6_stats.memory_active_descriptors +=
            tds[t].hz6_stats_after.memory_active_descriptors;
        hz6_stats.memory_local_free_descriptors +=
            tds[t].hz6_stats_after.memory_local_free_descriptors;
        hz6_stats.memory_transfer_free_descriptors +=
            tds[t].hz6_stats_after.memory_transfer_free_descriptors;
        hz6_stats.memory_remote_pending_descriptors +=
            tds[t].hz6_stats_after.memory_remote_pending_descriptors;
        hz6_stats.memory_dead_with_ptr_descriptors +=
            tds[t].hz6_stats_after.memory_dead_with_ptr_descriptors;
        hz6_stats.memory_active_source_blocks +=
            tds[t].hz6_stats_after.memory_active_source_blocks;
        hz6_stats.memory_registered_source_blocks +=
            tds[t].hz6_stats_after.memory_registered_source_blocks;
        hz6_stats.memory_ref_nonzero_source_blocks +=
            tds[t].hz6_stats_after.memory_ref_nonzero_source_blocks;
        hz6_stats.memory_ref_zero_source_blocks +=
            tds[t].hz6_stats_after.memory_ref_zero_source_blocks;
        hz6_stats.memory_source_block_payload_bytes +=
            tds[t].hz6_stats_after.memory_source_block_payload_bytes;
        hz6_stats.memory_source_block_committed_estimate +=
            tds[t].hz6_stats_after.memory_source_block_committed_estimate;
        hz6_stats.route_active_current +=
            tds[t].hz6_stats_after.route_active_current;
        hz6_stats.route_active_max +=
            tds[t].hz6_stats_after.route_active_max;
        hz6_stats.route_tombstone_current +=
            tds[t].hz6_stats_after.route_tombstone_current;
        hz6_stats.memory_frontcache_total +=
            tds[t].hz6_stats_after.memory_frontcache_total;
        if (tds[t].hz6_stats_after.memory_frontcache_largest_bin >
            hz6_stats.memory_frontcache_largest_bin) {
            hz6_stats.memory_frontcache_largest_bin =
                tds[t].hz6_stats_after.memory_frontcache_largest_bin;
        }

#include "bench_larson_hz6_stats_accumulate_tail.inc"
