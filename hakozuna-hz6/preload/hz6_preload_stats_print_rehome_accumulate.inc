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
    size_t pending_high_water = stats.remote_pending_high_water;
    if (pending_high_water > remote_pending_high_water) {
      remote_pending_high_water = pending_high_water;
    }
    remote_pending_maintenance_check +=
        stats.remote_pending_maintenance_check;
    remote_pending_maintenance_armed +=
        stats.remote_pending_maintenance_armed;
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
