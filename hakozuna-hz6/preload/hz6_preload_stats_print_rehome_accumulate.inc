    route_rehome_attempt += stats.route_rehome_attempt;
    route_rehome_success += stats.route_rehome_success;
    route_rehome_fail += stats.route_rehome_fail;
    route_rehome_expected_owner_mismatch +=
        stats.route_rehome_expected_owner_mismatch;
    route_rehome_destination_route_fail +=
        stats.route_rehome_destination_route_fail;
    route_rehome_directory_transfer_fail +=
        stats.route_rehome_directory_transfer_fail;
    route_rehome_directory_transfer_probe_total +=
        stats.route_rehome_directory_transfer_probe_total;
    size_t dir_probe_max =
        stats.route_rehome_directory_transfer_probe_max;
    if (dir_probe_max > route_rehome_directory_transfer_probe_max) {
      route_rehome_directory_transfer_probe_max = dir_probe_max;
    }
    route_rehome_origin_lookup_probe_total +=
        stats.route_rehome_origin_lookup_probe_total;
    size_t origin_lookup_probe_max =
        stats.route_rehome_origin_lookup_probe_max;
    if (origin_lookup_probe_max > route_rehome_origin_lookup_probe_max) {
      route_rehome_origin_lookup_probe_max = origin_lookup_probe_max;
    }
    route_rehome_destination_register_probe_total +=
        stats.route_rehome_destination_register_probe_total;
    size_t destination_register_probe_max =
        stats.route_rehome_destination_register_probe_max;
    if (destination_register_probe_max >
        route_rehome_destination_register_probe_max) {
      route_rehome_destination_register_probe_max =
          destination_register_probe_max;
    }
    route_rehome_origin_unregister_probe_total +=
        stats.route_rehome_origin_unregister_probe_total;
    size_t origin_unregister_probe_max =
        stats.route_rehome_origin_unregister_probe_max;
    if (origin_unregister_probe_max >
        route_rehome_origin_unregister_probe_max) {
      route_rehome_origin_unregister_probe_max =
          origin_unregister_probe_max;
    }
    route_rehome_rollback_success += stats.route_rehome_rollback_success;
    route_rehome_rollback_fail += stats.route_rehome_rollback_fail;
    remote_free_foreign_candidate += stats.remote_free_foreign_candidate;
    remote_free_status_committed += stats.remote_free_status_committed;
    remote_free_status_backpressure += stats.remote_free_status_backpressure;
    remote_free_status_stale += stats.remote_free_status_stale;
    remote_free_status_integrity_failure +=
        stats.remote_free_status_integrity_failure;
    remote_free_status_dispatch_transfer +=
        stats.remote_free_status_dispatch_transfer;
    transfer_reserve_attempt += stats.transfer_reserve_attempt;
    transfer_reserve_success += stats.transfer_reserve_success;
    transfer_reserve_full += stats.transfer_reserve_full;
    transfer_reserve_full_after_state_mutation +=
        stats.transfer_reserve_full_after_state_mutation;
    route_rehome_commit_enter += stats.route_rehome_commit_enter;
    route_rehome_commit_success += stats.route_rehome_commit_success;
    remote_free_backpressure += stats.remote_free_backpressure;
    remote_free_returned_uncommitted +=
        stats.remote_free_returned_uncommitted;
    remote_free_returned_backpressure +=
        stats.remote_free_returned_backpressure;
    remote_free_returned_stale += stats.remote_free_returned_stale;
    remote_free_returned_integrity_failure +=
        stats.remote_free_returned_integrity_failure;
    preload_malloc_transfer_retry_eligible +=
        stats.preload_malloc_transfer_retry_eligible;
    preload_malloc_transfer_retry_stride_skip +=
        stats.preload_malloc_transfer_retry_stride_skip;
    preload_malloc_transfer_retry_attempt +=
        stats.preload_malloc_transfer_retry_attempt;
    preload_malloc_transfer_retry_hit +=
        stats.preload_malloc_transfer_retry_hit;
    remote_pending_enqueue_attempt +=
        stats.remote_pending_enqueue_attempt;
    remote_pending_enqueue_success +=
        stats.remote_pending_enqueue_success;
    remote_pending_enqueue_full += stats.remote_pending_enqueue_full;
    remote_pending_duplicate_claim +=
        stats.remote_pending_duplicate_claim;
    remote_pending_owner_dead += stats.remote_pending_owner_dead;
    remote_pending_publish_fail += stats.remote_pending_publish_fail;
    remote_pending_current += stats.remote_pending_current;
    remote_pending_queued_current += stats.remote_pending_queued_current;
    remote_pending_claimed_current += stats.remote_pending_claimed_current;
    remote_pending_total_current += stats.remote_pending_total_current;
    size_t pending_high_water = stats.remote_pending_high_water;
    if (pending_high_water > remote_pending_high_water) {
      remote_pending_high_water = pending_high_water;
    }
    remote_pending_maintenance_check +=
        stats.remote_pending_maintenance_check;
    remote_pending_maintenance_armed +=
        stats.remote_pending_maintenance_armed;
    remote_pending_maintenance_noop +=
        stats.remote_pending_maintenance_noop;
    remote_pending_maintenance_key_race +=
        stats.remote_pending_maintenance_key_race;
    remote_pending_maintenance_external_miss +=
        stats.remote_pending_maintenance_external_miss;
    remote_pending_maintenance_inline_empty +=
        stats.remote_pending_maintenance_inline_empty;
    remote_pending_maintenance_frontcache_full_stop +=
        stats.remote_pending_maintenance_frontcache_full_stop;
    remote_pending_maintenance_entry_gate_miss +=
        stats.remote_pending_maintenance_entry_gate_miss;
    remote_pending_maintenance_inline_gate_hit +=
        stats.remote_pending_maintenance_inline_gate_hit;
    remote_pending_maintenance_external_gate_hit +=
        stats.remote_pending_maintenance_external_gate_hit;
    remote_pending_maintenance_external_attempt +=
        stats.remote_pending_maintenance_external_attempt;
    remote_pending_maintenance_external_success +=
        stats.remote_pending_maintenance_external_success;
    remote_pending_maintenance_external_only_call +=
        stats.remote_pending_maintenance_external_only_call;
    remote_pending_maintenance_inline_deferred +=
        stats.remote_pending_maintenance_inline_deferred;
    remote_pending_maintenance_inline_policy_skip +=
        stats.remote_pending_maintenance_inline_policy_skip;
    remote_pending_maintenance_inline_pop_attempt +=
        stats.remote_pending_maintenance_inline_pop_attempt;
    remote_pending_maintenance_inline_pop_success +=
        stats.remote_pending_maintenance_inline_pop_success;
    remote_pending_maintenance_route_validate_inline +=
        stats.remote_pending_maintenance_route_validate_inline;
    remote_pending_maintenance_route_validate_external +=
        stats.remote_pending_maintenance_route_validate_external;
    remote_pending_maintenance_frontcache_push_attempt_inline +=
        stats.remote_pending_maintenance_frontcache_push_attempt_inline;
    remote_pending_maintenance_frontcache_push_attempt_external +=
        stats.remote_pending_maintenance_frontcache_push_attempt_external;
    remote_pending_maintenance_frontcache_push_success_inline +=
        stats.remote_pending_maintenance_frontcache_push_success_inline;
    remote_pending_maintenance_frontcache_push_success_external +=
        stats.remote_pending_maintenance_frontcache_push_success_external;
    remote_pending_maintenance_drained_inline +=
        stats.remote_pending_maintenance_drained_inline;
    remote_pending_maintenance_drained_external +=
        stats.remote_pending_maintenance_drained_external;
    remote_pending_maintenance_drained_inline_toy +=
        stats.remote_pending_maintenance_drained_inline_toy;
    remote_pending_maintenance_drained_inline_midpage +=
        stats.remote_pending_maintenance_drained_inline_midpage;
    remote_pending_maintenance_drained_external_toy +=
        stats.remote_pending_maintenance_drained_external_toy;
    remote_pending_maintenance_drained_external_midpage +=
        stats.remote_pending_maintenance_drained_external_midpage;
    remote_pending_external_key_probe +=
        stats.remote_pending_external_key_probe;
    remote_pending_external_key_hit +=
        stats.remote_pending_external_key_hit;
    remote_pending_batch_call += stats.remote_pending_batch_call;
    remote_pending_batch_items += stats.remote_pending_batch_items;
    remote_pending_frontcache_push +=
        stats.remote_pending_frontcache_push;
    remote_pending_frontcache_full +=
        stats.remote_pending_frontcache_full;
    remote_pending_generation_mismatch +=
        stats.remote_pending_generation_mismatch;
    remote_pending_owner_mismatch +=
        stats.remote_pending_owner_mismatch;
    remote_pending_route_mismatch +=
        stats.remote_pending_route_mismatch;
    remote_pending_state_mismatch +=
        stats.remote_pending_state_mismatch;
    remote_free_origin_pending_commit +=
        stats.remote_free_origin_pending_commit;
    remote_free_pending_no_rehome += stats.remote_free_pending_no_rehome;
    remote_free_pending_publish_fail +=
        stats.remote_free_pending_publish_fail;
    remote_pending_owner_inbox_storage_ineligible +=
        stats.remote_pending_owner_inbox_storage_ineligible;
    remote_pending_owner_inbox_descriptor_mismatch +=
        stats.remote_pending_owner_inbox_descriptor_mismatch;
    remote_pending_owner_inbox_owner_dead +=
        stats.remote_pending_owner_inbox_owner_dead;
    remote_pending_owner_inbox_owner_mismatch +=
        stats.remote_pending_owner_inbox_owner_mismatch;
    remote_pending_owner_inbox_enqueue_fail +=
        stats.remote_pending_owner_inbox_enqueue_fail;
    remote_pending_external_ticket_attempt +=
        stats.remote_pending_external_ticket_attempt;
    remote_pending_external_ticket_success +=
        stats.remote_pending_external_ticket_success;
    remote_pending_external_ticket_full +=
        stats.remote_pending_external_ticket_full;
    remote_pending_external_ticket_duplicate +=
        stats.remote_pending_external_ticket_duplicate;
    remote_pending_external_ticket_duplicate_probe_total +=
        stats.remote_pending_external_ticket_duplicate_probe_total;
    if (stats.remote_pending_external_ticket_duplicate_probe_max >
        remote_pending_external_ticket_duplicate_probe_max) {
      remote_pending_external_ticket_duplicate_probe_max =
          stats.remote_pending_external_ticket_duplicate_probe_max;
    }
    remote_pending_external_ticket_duplicate_index_stale +=
        stats.remote_pending_external_ticket_duplicate_index_stale;
    remote_pending_external_ticket_duplicate_scan_skip +=
        stats.remote_pending_external_ticket_duplicate_scan_skip;
    remote_pending_external_ticket_locked_revalidate_fail +=
        stats.remote_pending_external_ticket_locked_revalidate_fail;
    remote_pending_external_ticket_consume +=
        stats.remote_pending_external_ticket_consume;
    remote_pending_external_ticket_current +=
        stats.remote_pending_external_ticket_current;
    size_t external_ticket_high_water =
        stats.remote_pending_external_ticket_high_water;
    if (external_ticket_high_water >
        remote_pending_external_ticket_high_water) {
      remote_pending_external_ticket_high_water =
          external_ticket_high_water;
    }
    remote_pending_external_ticket_consume_empty +=
        stats.remote_pending_external_ticket_consume_empty;
    remote_pending_external_ticket_frontcache_full +=
        stats.remote_pending_external_ticket_frontcache_full;
    remote_pending_external_ticket_route_mismatch +=
        stats.remote_pending_external_ticket_route_mismatch;
    remote_pending_external_ticket_owner_mismatch +=
        stats.remote_pending_external_ticket_owner_mismatch;
    remote_pending_external_ticket_state_mismatch +=
        stats.remote_pending_external_ticket_state_mismatch;
    remote_pending_external_ticket_storage_mismatch +=
        stats.remote_pending_external_ticket_storage_mismatch;
    remote_pending_external_ticket_integrity_abort +=
        stats.remote_pending_external_ticket_integrity_abort;
    remote_pending_inline_accounting_mismatch +=
        stats.remote_pending_inline_accounting_mismatch;
    remote_pending_external_accounting_mismatch +=
        stats.remote_pending_external_accounting_mismatch;
    remote_pending_total_state_count_mismatch +=
        stats.remote_pending_total_state_count_mismatch;
    remote_pending_external_free_list_corruption +=
        stats.remote_pending_external_free_list_corruption;
    remote_pending_external_list_cycle +=
        stats.remote_pending_external_list_cycle;
    remote_pending_external_ticket_multiple_list_membership +=
        stats.remote_pending_external_ticket_multiple_list_membership;
    remote_pending_claimed_current_at_quiescence +=
        stats.remote_pending_claimed_current_at_quiescence;
    remote_pending_external_claimed_at_quiescence +=
        stats.remote_pending_external_claimed_at_quiescence;
    remote_pending_destroy_inline_closeout +=
        stats.remote_pending_destroy_inline_closeout;
    remote_pending_destroy_external_closeout +=
        stats.remote_pending_destroy_external_closeout;
    remote_pending_destroy_external_release_fail +=
        stats.remote_pending_destroy_external_release_fail;
    remote_pending_enqueue_after_owner_dying +=
        stats.remote_pending_enqueue_after_owner_dying;
    remote_pending_ticket_to_dead_owner +=
        stats.remote_pending_ticket_to_dead_owner;
    remote_pending_ticket_to_dead_storage_owner +=
        stats.remote_pending_ticket_to_dead_storage_owner;
    remote_pending_route_removed_while_pending +=
        stats.remote_pending_route_removed_while_pending;
    remote_pending_after_allocator_destroy +=
        stats.remote_pending_after_allocator_destroy;
    remote_pending_key_nonempty_load +=
        stats.remote_pending_key_nonempty_load;
    remote_pending_key_nonempty_hit +=
        stats.remote_pending_key_nonempty_hit;
    pending_same_key_on_frontcache_miss +=
        stats.pending_same_key_on_frontcache_miss;
    pending_same_key_on_front_dispatch +=
        stats.pending_same_key_on_front_dispatch;
    pending_same_key_on_source_alloc +=
        stats.pending_same_key_on_source_alloc;
    source_alloc_with_matching_pending +=
        stats.source_alloc_with_matching_pending;
    pending_same_key_before_maintenance +=
        stats.pending_same_key_before_maintenance;
    pending_same_key_after_maintenance +=
        stats.pending_same_key_after_maintenance;
    pending_maintenance_immediate_reuse_success +=
        stats.pending_maintenance_immediate_reuse_success;
    pending_maintenance_batch_surplus +=
        stats.pending_maintenance_batch_surplus;
    pending_same_key_on_prefill_attempt +=
        stats.pending_same_key_on_prefill_attempt;
    prefill_commit_with_matching_pending +=
        stats.prefill_commit_with_matching_pending;
    prefill_commit_with_inline_pending +=
        stats.prefill_commit_with_inline_pending;
    prefill_commit_with_external_pending +=
        stats.prefill_commit_with_external_pending;
    source_block_commit_with_matching_pending +=
        stats.source_block_commit_with_matching_pending;
    source_block_commit_with_inline_pending +=
        stats.source_block_commit_with_inline_pending;
    source_block_commit_with_external_pending +=
        stats.source_block_commit_with_external_pending;
    direct_source_attempt_with_matching_pending +=
        stats.direct_source_attempt_with_matching_pending;
    direct_source_commit_with_matching_pending +=
        stats.direct_source_commit_with_matching_pending;
    remote_pending_direct_gate_load +=
        stats.remote_pending_direct_gate_load;
    remote_pending_direct_gate_hit += stats.remote_pending_direct_gate_hit;
    remote_pending_direct_claim_attempt +=
        stats.remote_pending_direct_claim_attempt;
    remote_pending_direct_claim_success +=
        stats.remote_pending_direct_claim_success;
    remote_pending_direct_claim_busy +=
        stats.remote_pending_direct_claim_busy;
    remote_pending_direct_claim_empty_after_hint +=
        stats.remote_pending_direct_claim_empty_after_hint;
    remote_pending_direct_activate_success +=
        stats.remote_pending_direct_activate_success;
    remote_pending_direct_integrity_failure +=
        stats.remote_pending_direct_integrity_failure;
    remote_pending_direct_source_boundary_attempt +=
        stats.remote_pending_direct_source_boundary_attempt;
    remote_pending_direct_source_boundary_gate_hit +=
        stats.remote_pending_direct_source_boundary_gate_hit;
    remote_pending_direct_source_boundary_claim_success +=
        stats.remote_pending_direct_source_boundary_claim_success;
    remote_pending_direct_prefill_avoided +=
        stats.remote_pending_direct_prefill_avoided;
    remote_pending_direct_source_alloc_avoided +=
        stats.remote_pending_direct_source_alloc_avoided;
    remote_pending_direct_claim_before_existing_reuse +=
        stats.remote_pending_direct_claim_before_existing_reuse;
    remote_pending_direct_claim_while_frontcache_nonempty +=
        stats.remote_pending_direct_claim_while_frontcache_nonempty;
    remote_pending_direct_claim_while_transfer_nonempty +=
        stats.remote_pending_direct_claim_while_transfer_nonempty;
    remote_pending_direct_claim_success_toy +=
        stats.remote_pending_direct_claim_success_toy;
    remote_pending_direct_claim_success_midpage +=
        stats.remote_pending_direct_claim_success_midpage;
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
      remote_pending_direct_claim_success_by_class[class_id] +=
          stats.remote_pending_direct_claim_success_by_class[class_id];
      remote_pending_maintenance_drained_inline_by_class[class_id] +=
          stats.remote_pending_maintenance_drained_inline_by_class[class_id];
      remote_pending_maintenance_drained_external_by_class[class_id] +=
          stats.remote_pending_maintenance_drained_external_by_class[class_id];
    }
    remote_pending_direct_claim_success_while_transfer_nonempty +=
        stats.remote_pending_direct_claim_success_while_transfer_nonempty;
    remote_pending_direct_claim_success_transfer_toy +=
        stats.remote_pending_direct_claim_success_transfer_toy;
    remote_pending_direct_claim_success_transfer_midpage +=
        stats.remote_pending_direct_claim_success_transfer_midpage;
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
      remote_pending_direct_claim_success_transfer_by_class[class_id] +=
          stats.remote_pending_direct_claim_success_transfer_by_class[class_id];
    }
    remote_free_backpressure_origin_transfer_success +=
        stats.remote_free_backpressure_origin_transfer_success;
    remote_free_backpressure_origin_transfer_fail +=
        stats.remote_free_backpressure_origin_transfer_fail;
    remote_free_backpressure_origin_transfer_stride_skip +=
        stats.remote_free_backpressure_origin_transfer_stride_skip;
    remote_free_backpressure_origin_transfer_validation_fail +=
        stats.remote_free_backpressure_origin_transfer_validation_fail;
    remote_free_backpressure_origin_transfer_full +=
        stats.remote_free_backpressure_origin_transfer_full;
    remote_free_backpressure_origin_full_transfer_count_total +=
        stats.remote_free_backpressure_origin_full_transfer_count_total;
    remote_free_backpressure_origin_full_class_count_total +=
        stats.remote_free_backpressure_origin_full_class_count_total;
    size_t origin_full_class_max =
        stats.remote_free_backpressure_origin_full_class_count_max;
    if (origin_full_class_max >
        remote_free_backpressure_origin_full_class_count_max) {
      remote_free_backpressure_origin_full_class_count_max =
          origin_full_class_max;
    }
    origin_transfer_source_commit_with_same_class_transfer +=
        stats.origin_transfer_source_commit_with_same_class_transfer;
    origin_transfer_source_commit_with_any_transfer +=
        stats.origin_transfer_source_commit_with_any_transfer;
    origin_transfer_source_commit_same_class_count_total +=
        stats.origin_transfer_source_commit_same_class_count_total;
    if (stats.origin_transfer_source_commit_same_class_count_max >
        origin_transfer_source_commit_same_class_count_max) {
      origin_transfer_source_commit_same_class_count_max =
          stats.origin_transfer_source_commit_same_class_count_max;
    }
    origin_transfer_source_commit_transfer_count_total +=
        stats.origin_transfer_source_commit_transfer_count_total;
    if (stats.origin_transfer_source_commit_transfer_count_max >
        origin_transfer_source_commit_transfer_count_max) {
      origin_transfer_source_commit_transfer_count_max =
          stats.origin_transfer_source_commit_transfer_count_max;
    }
    origin_transfer_prefill_commit_with_same_class_transfer +=
        stats.origin_transfer_prefill_commit_with_same_class_transfer;
    origin_transfer_prefill_commit_with_any_transfer +=
        stats.origin_transfer_prefill_commit_with_any_transfer;
    origin_transfer_prefill_commit_same_class_count_total +=
        stats.origin_transfer_prefill_commit_same_class_count_total;
    if (stats.origin_transfer_prefill_commit_same_class_count_max >
        origin_transfer_prefill_commit_same_class_count_max) {
      origin_transfer_prefill_commit_same_class_count_max =
          stats.origin_transfer_prefill_commit_same_class_count_max;
    }
    origin_transfer_prefill_commit_transfer_count_total +=
        stats.origin_transfer_prefill_commit_transfer_count_total;
    if (stats.origin_transfer_prefill_commit_transfer_count_max >
        origin_transfer_prefill_commit_transfer_count_max) {
      origin_transfer_prefill_commit_transfer_count_max =
          stats.origin_transfer_prefill_commit_transfer_count_max;
    }
    origin_transfer_pop_loop_attempt +=
        stats.origin_transfer_pop_loop_attempt;
    origin_transfer_pop_loop_empty += stats.origin_transfer_pop_loop_empty;
    origin_transfer_pop_loop_invalid +=
        stats.origin_transfer_pop_loop_invalid;
    origin_transfer_pop_loop_hit += stats.origin_transfer_pop_loop_hit;
    remote_free_backpressure_origin_drain_attempt +=
        stats.remote_free_backpressure_origin_drain_attempt;
    remote_free_backpressure_origin_drain_success +=
        stats.remote_free_backpressure_origin_drain_success;
    remote_free_backpressure_origin_drain_retry_success +=
        stats.remote_free_backpressure_origin_drain_retry_success;
    remote_free_backpressure_drain_attempt +=
        stats.remote_free_backpressure_drain_attempt;
    remote_free_backpressure_drain_success +=
        stats.remote_free_backpressure_drain_success;
    remote_free_backpressure_retry_success +=
        stats.remote_free_backpressure_retry_success;
    remote_free_backpressure_requeue_fail +=
        stats.remote_free_backpressure_requeue_fail;
    transfer_reserve_full_transfer_count_total +=
        stats.transfer_reserve_full_transfer_count_total;
    transfer_reserve_full_class_count_total +=
        stats.transfer_reserve_full_class_count_total;
    size_t full_class_max = stats.transfer_reserve_full_class_count_max;
    if (full_class_max > transfer_reserve_full_class_count_max) {
      transfer_reserve_full_class_count_max = full_class_max;
    }
    remote_free_overflow_reserve_success +=
        stats.remote_free_overflow_reserve_success;
    remote_free_overflow_reserve_full +=
        stats.remote_free_overflow_reserve_full;
    remote_free_overflow_pop += stats.remote_free_overflow_pop;
    transfer_reuse_rehome_attempt += stats.transfer_reuse_rehome_attempt;
    transfer_reuse_rehome_success += stats.transfer_reuse_rehome_success;
    transfer_reuse_rehome_fail += stats.transfer_reuse_rehome_fail;
