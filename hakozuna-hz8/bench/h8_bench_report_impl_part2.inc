          : 0.0;
  printf("medium_remote_class_episode qpush_per_pub=[%.3f,%.3f,%.3f,%.3f]\n",
         medium_qpush_per_pub_8k, medium_qpush_per_pub_16k,
         medium_qpush_per_pub_32k, medium_qpush_per_pub_64k);
  printf("medium_remote_queue push_attempt=%zu push_retry=%zu push_success=%zu\n",
         debug.medium_remote_queue_push_attempt_count,
         debug.medium_remote_queue_push_retry_count,
         debug.medium_remote_queue_push_success_count);
  printf("medium_refill_candidate_shadow install=%zu attempt=%zu hit=%zu owner_mismatch=%zu unusable=%zu\n",
         debug.medium_refill_candidate_install,
         debug.medium_refill_candidate_attempt,
         debug.medium_refill_candidate_hit,
         debug.medium_refill_candidate_owner_mismatch,
         debug.medium_refill_candidate_unusable);
  printf("medium_available_shadow add=%zu remove=%zu head_attempt=%zu head_hit=%zu head_unusable=%zu owner_hit_without_available=%zu duplicate=%zu owner_mismatch=%zu class_mismatch=%zu indexed_full=%zu indexed_detached=%zu indexed_nonactive=%zu active_indexed=%zu exit_nonempty=%zu\n",
         debug.medium_available_add, debug.medium_available_remove,
         debug.medium_available_head_attempt,
         debug.medium_available_head_hit,
         debug.medium_available_head_unusable,
         debug.medium_available_owner_list_hit_without_available,
         debug.medium_available_duplicate_membership,
         debug.medium_available_owner_mismatch,
         debug.medium_available_class_mismatch,
         debug.medium_available_indexed_full,
         debug.medium_available_indexed_detached,
         debug.medium_available_indexed_nonactive,
         debug.medium_available_active_indexed,
         debug.medium_available_exit_nonempty);
  printf("medium_available_hit_cost reuse=%zu lock_ms=%.3f alloc_ms=%.3f active_ms=%.3f collect_ms=%.3f ns_per_reuse=[%.1f,%.1f,%.1f,%.1f]\n",
         debug.medium_available_hit_reuse,
         debug.medium_available_hit_lock_ns / 1000000.0,
         debug.medium_available_hit_alloc_ns / 1000000.0,
         debug.medium_available_hit_active_ns / 1000000.0,
         debug.medium_available_hit_collect_ns / 1000000.0,
         debug.medium_available_hit_reuse
             ? (double)debug.medium_available_hit_lock_ns /
                   (double)debug.medium_available_hit_reuse
             : 0.0,
         debug.medium_available_hit_reuse
             ? (double)debug.medium_available_hit_alloc_ns /
                   (double)debug.medium_available_hit_reuse
             : 0.0,
         debug.medium_available_hit_reuse
             ? (double)debug.medium_available_hit_active_ns /
                   (double)debug.medium_available_hit_reuse
             : 0.0,
         debug.medium_available_hit_reuse
             ? (double)debug.medium_available_hit_collect_ns /
                   (double)debug.medium_available_hit_reuse
             : 0.0);
  printf("medium_alloc_path_cost scaffold=%zu mark_live_ms=%.3f mask_ms=%.3f slot_store_ms=%.3f ptr_ms=%.3f ns_per_scaffold=[%.1f,%.1f,%.1f,%.1f]\n",
         debug.medium_alloc_scaffold_count,
         debug.medium_alloc_mark_live_ns / 1000000.0,
         debug.medium_alloc_mask_ns / 1000000.0,
         debug.medium_alloc_slot_store_ns / 1000000.0,
         debug.medium_alloc_ptr_ns / 1000000.0,
         debug.medium_alloc_scaffold_count
             ? (double)debug.medium_alloc_mark_live_ns /
                   (double)debug.medium_alloc_scaffold_count
             : 0.0,
         debug.medium_alloc_scaffold_count
             ? (double)debug.medium_alloc_mask_ns /
                   (double)debug.medium_alloc_scaffold_count
             : 0.0,
         debug.medium_alloc_scaffold_count
             ? (double)debug.medium_alloc_slot_store_ns /
                   (double)debug.medium_alloc_scaffold_count
             : 0.0,
         debug.medium_alloc_scaffold_count
             ? (double)debug.medium_alloc_ptr_ns /
                   (double)debug.medium_alloc_scaffold_count
             : 0.0);
  printf("medium_active_alloc_path_cost hits=%zu precheck_ms=%.3f mark_live_ms=%.3f mask_ms=%.3f slot_store_ms=%.3f ptr_ms=%.3f ns_per_hit=[%.1f,%.1f,%.1f,%.1f,%.1f]\n",
         debug.medium_active_alloc_hit_count,
         debug.medium_active_alloc_precheck_ns / 1000000.0,
         debug.medium_active_alloc_mark_live_ns / 1000000.0,
         debug.medium_active_alloc_mask_ns / 1000000.0,
         debug.medium_active_alloc_slot_store_ns / 1000000.0,
         debug.medium_active_alloc_ptr_ns / 1000000.0,
         debug.medium_active_alloc_hit_count
             ? (double)debug.medium_active_alloc_precheck_ns /
                   (double)debug.medium_active_alloc_hit_count
             : 0.0,
         debug.medium_active_alloc_hit_count
             ? (double)debug.medium_active_alloc_mark_live_ns /
                   (double)debug.medium_active_alloc_hit_count
             : 0.0,
         debug.medium_active_alloc_hit_count
             ? (double)debug.medium_active_alloc_mask_ns /
                   (double)debug.medium_active_alloc_hit_count
             : 0.0,
         debug.medium_active_alloc_hit_count
             ? (double)debug.medium_active_alloc_slot_store_ns /
                   (double)debug.medium_active_alloc_hit_count
             : 0.0,
         debug.medium_active_alloc_hit_count
             ? (double)debug.medium_active_alloc_ptr_ns /
                   (double)debug.medium_active_alloc_hit_count
             : 0.0);
  printf("medium_mark_live_path_cost calls=%zu lock_ms=%.3f retention_shadow_ms=%.3f warm_shadow_ms=%.3f lazy_shadow_ms=%.3f total_ms=%.3f ns_per_call=[%.1f,%.1f,%.1f,%.1f,%.1f]\n",
         debug.medium_mark_live_call_count,
         debug.medium_mark_live_lock_ns / 1000000.0,
         debug.medium_mark_live_retention_shadow_ns / 1000000.0,
         debug.medium_mark_live_warm_shadow_ns / 1000000.0,
         debug.medium_mark_live_lazy_shadow_ns / 1000000.0,
         debug.medium_alloc_mark_live_ns / 1000000.0,
         debug.medium_mark_live_call_count
             ? (double)debug.medium_mark_live_lock_ns /
                   (double)debug.medium_mark_live_call_count
             : 0.0,
         debug.medium_mark_live_call_count
             ? (double)debug.medium_mark_live_retention_shadow_ns /
                   (double)debug.medium_mark_live_call_count
             : 0.0,
         debug.medium_mark_live_call_count
             ? (double)debug.medium_mark_live_warm_shadow_ns /
                   (double)debug.medium_mark_live_call_count
             : 0.0,
         debug.medium_mark_live_call_count
             ? (double)debug.medium_mark_live_lazy_shadow_ns /
                   (double)debug.medium_mark_live_call_count
             : 0.0,
         debug.medium_mark_live_call_count
             ? (double)debug.medium_alloc_mark_live_ns /
                   (double)debug.medium_mark_live_call_count
             : 0.0);
  printf("medium_free_path_cost scaffold=%zu decode_ms=%.3f state_ms=%.3f pending_ms=%.3f slot_store_ms=%.3f mask_ms=%.3f empty_ms=%.3f ns_per_scaffold=[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f]\n",
         debug.medium_free_scaffold_count,
         debug.medium_free_decode_ns / 1000000.0,
         debug.medium_free_state_ns / 1000000.0,
         debug.medium_free_pending_ns / 1000000.0,
         debug.medium_free_slot_store_ns / 1000000.0,
         debug.medium_free_mask_ns / 1000000.0,
         debug.medium_free_empty_ns / 1000000.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_decode_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_state_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_pending_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_slot_store_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_mask_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0,
         debug.medium_free_scaffold_count
             ? (double)debug.medium_free_empty_ns /
                   (double)debug.medium_free_scaffold_count
             : 0.0);
  printf("medium_local_fast_shadow eligible_free=%zu eligible_alloc=%zu avoid_mark_empty=%zu avoid_mark_live=%zu active_switch_flush=%zu owner_exit_flush=%zu pending_block=%zu not_active_run=%zu reuse_ratio=%.3f\n",
         debug.medium_local_fast_eligible_free,
         debug.medium_local_fast_eligible_alloc,
         debug.medium_local_fast_avoid_mark_empty,
         debug.medium_local_fast_avoid_mark_live,
         debug.medium_local_fast_active_switch_flush,
         debug.medium_local_fast_owner_exit_flush,
         debug.medium_local_fast_pending_block,
         debug.medium_local_fast_not_active_run,
         debug.medium_local_fast_eligible_free
             ? (double)debug.medium_local_fast_eligible_alloc /
                   (double)debug.medium_local_fast_eligible_free
             : 0.0);
  printf("medium_hz9_local_mag_shadow eligible_free=%zu eligible_alloc=%zu avoid_mark_empty=%zu avoid_mark_live=%zu active_switch_flush=%zu owner_exit_flush=%zu pending_block=%zu not_active_run=%zu state_mismatch=%zu reuse_ratio=%.3f\n",
         debug.medium_hz9_local_mag_shadow_eligible_free,
         debug.medium_hz9_local_mag_shadow_eligible_alloc,
         debug.medium_hz9_local_mag_shadow_avoid_mark_empty,
         debug.medium_hz9_local_mag_shadow_avoid_mark_live,
         debug.medium_hz9_local_mag_shadow_active_switch_flush,
         debug.medium_hz9_local_mag_shadow_owner_exit_flush,
         debug.medium_hz9_local_mag_shadow_pending_block,
         debug.medium_hz9_local_mag_shadow_not_active_run,
         debug.medium_hz9_local_mag_shadow_state_mismatch,
         debug.medium_hz9_local_mag_shadow_eligible_free
             ? (double)debug.medium_hz9_local_mag_shadow_eligible_alloc /
                   (double)debug.medium_hz9_local_mag_shadow_eligible_free
             : 0.0);
  printf("medium_post_collect_active_miss total=%zu no_pending=%zu owner_pending=%zu active_pending=%zu active_pending_zero=%zu active_pending_slots=%zu owner_list_hit=%zu owner_hit_pos=[%zu,%zu,%zu,%zu,%zu] create=%zu detached_reuse=%zu\n",
         debug.medium_active_miss_total,
         debug.medium_active_miss_no_pending,
         debug.medium_active_miss_owner_pending,
         debug.medium_active_miss_active_pending,
         debug.medium_active_miss_active_pending_zero,
         debug.medium_active_miss_active_pending_slots,
         debug.medium_active_miss_owner_list_hit,
         debug.medium_active_miss_owner_list_pos1,
         debug.medium_active_miss_owner_list_pos2,
         debug.medium_active_miss_owner_list_pos3,
         debug.medium_active_miss_owner_list_pos4,
         debug.medium_active_miss_owner_list_pos5p,
         debug.medium_active_miss_create,
         debug.medium_active_miss_detached_reuse);
  printf("medium_sizepolicy_runtime active_miss_class=[%zu,%zu,%zu,%zu] active_pending_class=[%zu,%zu,%zu,%zu] owner_list_hit_class=[%zu,%zu,%zu,%zu] active_switch_class=[%zu,%zu,%zu,%zu]\n",
         debug.medium_active_miss_class_8k,
         debug.medium_active_miss_class_16k,
         debug.medium_active_miss_class_32k,
         debug.medium_active_miss_class_64k,
         debug.medium_active_pending_class_8k,
         debug.medium_active_pending_class_16k,
         debug.medium_active_pending_class_32k,
         debug.medium_active_pending_class_64k,
         debug.medium_owner_list_hit_class_8k,
         debug.medium_owner_list_hit_class_16k,
         debug.medium_owner_list_hit_class_32k,
         debug.medium_owner_list_hit_class_64k,
         debug.medium_active_switch_class_8k,
         debug.medium_active_switch_class_16k,
         debug.medium_active_switch_class_32k,
         debug.medium_active_switch_class_64k);
  printf("medium_collect_source periodic_active=[%zu,%zu,%zu] periodic_owner=[%zu,%zu,%zu] capacity=[%zu,%zu,%zu] owner_exit=[%zu,%zu,%zu] demand=[%zu,%zu,%zu] full_to_nonfull=%zu zero_run_call=%zu zero_slot_run=%zu active_run=%zu nonactive_run=%zu\n",
         debug.medium_collect_src_periodic_active_call,
         debug.medium_collect_src_periodic_active_run,
         debug.medium_collect_src_periodic_active_slot,
         debug.medium_collect_src_periodic_owner_call,
         debug.medium_collect_src_periodic_owner_run,
         debug.medium_collect_src_periodic_owner_slot,
         debug.medium_collect_src_capacity_call,
         debug.medium_collect_src_capacity_run,
         debug.medium_collect_src_capacity_slot,
         debug.medium_collect_src_owner_exit_call,
         debug.medium_collect_src_owner_exit_run,
         debug.medium_collect_src_owner_exit_slot,
         debug.medium_collect_src_demand_call,
         debug.medium_collect_src_demand_run,
         debug.medium_collect_src_demand_slot,
         debug.medium_collect_full_to_nonfull_run,
         debug.medium_collect_zero_run_call,
         debug.medium_collect_zero_slot_run,
         debug.medium_collect_active_run,
         debug.medium_collect_nonactive_run);
  printf("medium_sizepolicy_collect full_to_nonfull_class=[%zu,%zu,%zu,%zu] full_to_nonfull_source=[%zu,%zu,%zu,%zu,%zu]\n",
         debug.medium_collect_full_to_nonfull_class_8k,
         debug.medium_collect_full_to_nonfull_class_16k,
         debug.medium_collect_full_to_nonfull_class_32k,
         debug.medium_collect_full_to_nonfull_class_64k,
         debug.medium_collect_full_to_nonfull_periodic_active,
         debug.medium_collect_full_to_nonfull_periodic_owner,
         debug.medium_collect_full_to_nonfull_capacity,
         debug.medium_collect_full_to_nonfull_owner_exit,
         debug.medium_collect_full_to_nonfull_demand);
  printf("medium_collect_credit created=%zu reused=%zu discarded=%zu outstanding_next_collect=%zu underflow=%zu reuse_distance=[%zu,%zu,%zu,%zu,%zu] reused_class=[%zu,%zu,%zu,%zu] quick_class=[%zu,%zu,%zu,%zu]\n",
         debug.medium_collect_credit_created,
         debug.medium_collect_credit_reused,
         debug.medium_collect_credit_discarded,
         debug.medium_collect_credit_outstanding_at_next_collect,
         debug.medium_collect_credit_underflow,
         debug.medium_collect_credit_reuse_d0_1,
         debug.medium_collect_credit_reuse_d2_3,
         debug.medium_collect_credit_reuse_d4_7,
         debug.medium_collect_credit_reuse_d8_31,
         debug.medium_collect_credit_reuse_d32p,
         debug.medium_collect_credit_reused_class_8k,
         debug.medium_collect_credit_reused_class_16k,
         debug.medium_collect_credit_reused_class_32k,
         debug.medium_collect_credit_reused_class_64k,
         debug.medium_collect_credit_quick_class_8k,
         debug.medium_collect_credit_quick_class_16k,
         debug.medium_collect_credit_quick_class_32k,
         debug.medium_collect_credit_quick_class_64k);
  printf("medium_demand64 trigger=%zu qstate_not_queued=%zu processed=[%zu,%zu,%zu] target_opened=%zu retry_hit=%zu target_not_reached=%zu owner_list_fallback=%zu active_slots_created=%zu nonactive_runs_processed=%zu periodic_tick_replaced=%zu\n",
         debug.medium_demand64_trigger,
         debug.medium_demand64_qstate_not_queued,
         debug.medium_demand64_processed_0,
         debug.medium_demand64_processed_1,
         debug.medium_demand64_processed_2,
         debug.medium_demand64_target_opened,
         debug.medium_demand64_retry_hit,
         debug.medium_demand64_target_not_reached,
         debug.medium_demand64_owner_list_fallback,
         debug.medium_demand64_active_slots_created,
         debug.medium_demand64_nonactive_runs_processed,
         debug.medium_demand64_periodic_tick_replaced);
  printf("medium_chunk create=%zu alloc=%zu reserved_bytes=%zu used_bytes=%zu\n",
         debug.medium_chunk_create_count, debug.medium_chunk_alloc_count,
         debug.medium_chunk_reserved_bytes, debug.medium_chunk_used_bytes);

  double medium_remote_pub = (double)debug.medium_remote_publish_count;
  double medium_collect_run = (double)debug.medium_remote_collect_run_count;
  double medium_collect_call = (double)debug.medium_remote_collect_call_count;
  double medium_lease_ns_per_pub =
      medium_remote_pub > 0.0
          ? (double)debug.medium_remote_owner_lease_ns / medium_remote_pub
          : 0.0;
  double medium_claim_ns_per_pub =
      medium_remote_pub > 0.0
          ? (double)debug.medium_remote_pending_claim_ns / medium_remote_pub
          : 0.0;
  double medium_qpush_ns_per_push =
      debug.medium_remote_queue_push_count
          ? (double)debug.medium_remote_queue_push_ns /
                (double)debug.medium_remote_queue_push_count
          : 0.0;
  double medium_collect_ns_per_run =
      medium_collect_run > 0.0
          ? (double)debug.medium_remote_collect_ns / medium_collect_run
          : 0.0;
  double medium_collect_runs_per_call =
      medium_collect_call > 0.0 ? medium_collect_run / medium_collect_call : 0.0;
  double medium_collect_slots_per_run =
      medium_collect_run > 0.0
          ? (double)debug.medium_remote_collect_slot_count / medium_collect_run
          : 0.0;
  printf("medium_residual lease_ns_per_pub=%.1f claim_ns_per_pub=%.1f qpush_ns_per_push=%.1f collect_ns_per_run=%.1f collect_runs_per_call=%.3f collect_slots_per_run=%.3f periodic_fast=%zu periodic_slow=%zu periodic_hit=%zu periodic_miss=%zu periodic_active=%zu periodic_owner=%zu active_refill_hint=%zu\n",
         medium_lease_ns_per_pub, medium_claim_ns_per_pub,
         medium_qpush_ns_per_push, medium_collect_ns_per_run,
         medium_collect_runs_per_call, medium_collect_slots_per_run,
         debug.medium_collect_periodic_fast_skip,
         debug.medium_collect_periodic_slow_enter,
         debug.medium_collect_periodic_pending_hit,
         debug.medium_collect_periodic_pending_miss,
         debug.medium_collect_periodic_from_active,
         debug.medium_collect_periodic_from_owner_list,
         debug.medium_collect_active_refill_hint);
  double medium_attributed_ms =
      (double)(debug.medium_remote_owner_lease_ns +
               debug.medium_remote_pending_claim_ns +
               debug.medium_remote_queue_push_ns + debug.medium_remote_collect_ns +
               debug.medium_global_lock_wait_ns + debug.medium_run_lock_wait_ns +
               debug.medium_alloc_slot_ns + debug.medium_free_slot_ns +
               debug.medium_madvise_ns) /
      1e6;
  double medium_lock_ms =
      (double)(debug.medium_global_lock_wait_ns +
               debug.medium_run_lock_wait_ns +
               debug.medium_remote_run_lock_ns) /
      1e6;
  double medium_slot_ms =
      (double)(debug.medium_alloc_slot_ns + debug.medium_free_slot_ns) / 1e6;
  size_t minor_median =
      h8_percentile_size_t(input->minor_faults, (size_t)opt->runs, 0.50);
  double bench_ops = (double)opt->threads * (double)opt->iters_per_thread;
  double minor_per_op = bench_ops > 0.0 ? (double)minor_median / bench_ops : 0.0;
  printf("medium_residual_budget attributed_ms=%.3f lock_ms=%.3f slot_ms=%.3f qpush_ms=%.3f collect_ms=%.3f lease_ms=%.3f claim_ms=%.3f madvise_ms=%.3f minor_faults_median=%zu minor_faults_per_op=%.6f\n",
         medium_attributed_ms, medium_lock_ms, medium_slot_ms,
         (double)debug.medium_remote_queue_push_ns / 1e6,
         (double)debug.medium_remote_collect_ns / 1e6,
         (double)debug.medium_remote_owner_lease_ns / 1e6,
         (double)debug.medium_remote_pending_claim_ns / 1e6,
         (double)debug.medium_madvise_ns / 1e6, minor_median, minor_per_op);
  size_t lower_median =
      h8_percentile_size_t(input->span_lower_bound, (size_t)opt->runs, 0.50);
  double actual_per_run =
      opt->runs > 0 ? (double)debug.local_span_commit / (double)opt->runs : 0.0;
  double span_excess_ratio =
      lower_median && debug.local_span_commit
          ? actual_per_run / (double)lower_median
          : 0.0;
  printf("span_commit_lower_bound actual_total=%zu actual_per_run=%.1f lower_bound_median=%zu excess_ratio=%.3f\n",
         debug.local_span_commit, actual_per_run, lower_median, span_excess_ratio);
  printf("local_scan_detail hint_null=%zu hint_full=%zu hint_state_blocked=%zu hint_trusted=%zu hint_class_mismatch=%zu hint_owner_mismatch=%zu hint_generation_mismatch=%zu hint_state_mismatch=%zu scan_usable=%zu scan_full=%zu scan_state_blocked=%zu skip_no_pending=%zu\n",
         debug.local_active_hint_null, debug.local_active_hint_full,
         debug.local_active_hint_state_blocked, debug.local_active_hint_trusted,
         debug.local_active_hint_class_mismatch,
         debug.local_active_hint_owner_mismatch,
         debug.local_active_hint_generation_mismatch,
         debug.local_active_hint_state_mismatch,
         debug.local_find_scan_span_usable, debug.local_find_scan_span_full,
         debug.local_find_scan_span_state_blocked,
         debug.local_find_skip_scan_no_pending);
  printf("local_hot live_alloc=%zu live_free=%zu word0=%zu word1=%zu word2_7=%zu word8_31=%zu word32_63=%zu free_head_alloc=%zu free_head_free=%zu pending_alloc=%zu pending_free=%zu used_alloc=%zu used_free=%zu\n",
         debug.local_live_touch_alloc, debug.local_live_touch_free,
         debug.local_live_word_0, debug.local_live_word_1,
         debug.local_live_word_2_7, debug.local_live_word_8_31,
         debug.local_live_word_32_63, debug.local_free_head_touch_alloc,
         debug.local_free_head_touch_free, debug.local_pending_check_alloc,
         debug.local_pending_check_free, debug.local_used_touch_alloc,
         debug.local_used_touch_free);
  printf("used_count_detail load_alloc=%zu store_alloc=%zu load_free=%zu store_free=%zu full_check=%zu underflow=%zu mirror_mismatch=%zu mirror_underflow=%zu\n",
         debug.local_used_count_load_alloc, debug.local_used_count_store_alloc,
         debug.local_used_count_load_free, debug.local_used_count_store_free,
         debug.local_used_count_full_check, debug.local_used_count_underflow,
         debug.local_used_mirror_mismatch, debug.local_used_mirror_underflow);
  printf("used_count_cold active_hint=%zu owner_scan_locked=%zu adoption_locked=%zu owner_exit=%zu verify_quiescent=%zu derived_mismatch=%zu derived_scan=%zu\n",
         debug.local_used_cold_active_hint,
         debug.local_used_cold_owner_scan_locked,
         debug.local_used_cold_adoption_locked, debug.local_used_cold_owner_exit,
         debug.local_used_cold_verify_quiescent, debug.local_used_derived_mismatch,
         debug.local_used_derived_quiescent_scan);
  printf("span_commit_timing total_ms=%.3f meta_ms=%.3f mprotect_ms=%.3f\n",
         (double)debug.span_commit_total_ns / 1e6,
         (double)debug.span_commit_meta_ns / 1e6,
         (double)debug.span_commit_mprotect_ns / 1e6);
  printf("lifecycle_timing owner_exit_total_ms=%.3f owner_exit_collect_ms=%.3f owner_exit_span_walk_ms=%.3f span_retire_count=%zu span_retire_total_ms=%.3f span_retire_lock_wait_ms=%.3f span_retire_madvise_ms=%.3f span_retire_meta_free_ms=%.3f\n",
         (double)debug.owner_exit_total_ns / 1e6,
         (double)debug.owner_exit_collect_ns / 1e6,
         (double)debug.owner_exit_span_walk_ns / 1e6, debug.span_retire_count,
         (double)debug.span_retire_total_ns / 1e6,
         (double)debug.span_retire_lock_wait_ns / 1e6,
         (double)debug.span_retire_madvise_ns / 1e6,
         (double)debug.span_retire_meta_free_ns / 1e6);
  double purge_avg =
      debug.span_purge_run_count
          ? (double)debug.span_purge_run_spans_total /
                (double)debug.span_purge_run_count
          : 0.0;
  printf("span_purge runs=%zu spans=%zu avg_spans_per_run=%.3f max_run=%zu singleton_runs=%zu madvise_calls=%zu madvise_bytes=%zu madvise_ms=%.3f\n",
         debug.span_purge_run_count, debug.span_purge_run_spans_total,
         purge_avg, debug.span_purge_run_max, debug.span_purge_singleton_runs,
         debug.span_purge_madvise_calls, debug.span_purge_madvise_bytes,
         (double)debug.span_purge_madvise_ns / 1e6);
  printf("slot_shadow valid_mismatch=%zu invalid_mismatch=%zu pending_nonallocated=%zu free_unreachable=%zu free_duplicate=%zu free_cycle=%zu bad_next=%zu never_used_below_bump=%zu nonvirgin_above_bump=%zu used_mismatch=%zu reserved_quiescent=%zu\n",
         debug.slot_shadow_valid_mismatch, debug.slot_shadow_invalid_mismatch,
         debug.slot_shadow_pending_nonallocated, debug.slot_shadow_free_unreachable,
         debug.slot_shadow_free_duplicate, debug.slot_shadow_free_cycle,
         debug.slot_shadow_bad_next, debug.slot_shadow_never_used_below_bump,
         debug.slot_shadow_nonvirgin_above_bump, debug.slot_shadow_used_mismatch,
         debug.slot_shadow_reserved_quiescent);
}
