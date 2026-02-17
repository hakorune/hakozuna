// Extracted from hz4_os.c (move-only): OS stats increment/flush functions.
void hz4_os_stats_decommit_attempt(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_attempt, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_success(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_success, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_uninit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_uninit, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_already(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_already, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_used(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_used, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_cph(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_cph, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_cph_queued(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_cph_queued, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_budget0(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_budget0, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_still_local(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_still_local, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_seal_enter(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_seal_enter, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_grace_block(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_grace_block, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_cph_grace(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_cph_grace, 1, memory_order_relaxed);
}

void hz4_os_stats_cand_requeue_grace(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cand_requeue_grace, 1, memory_order_relaxed);
}

void hz4_os_stats_cand_requeue_cph(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cand_requeue_cph, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_extract_ok(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_extract_ok, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_extract_fail(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_extract_fail, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_extract_fail_state(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_extract_fail_state, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_extract_fail_queued(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_extract_fail_queued, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_extract_fail_notfound(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_extract_fail_notfound, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_hot_push(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_hot_push, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_cold_push(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_cold_push, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_hot_pop(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_hot_pop, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_cold_pop(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_cold_pop, 1, memory_order_relaxed);
}

void hz4_os_stats_cph_hot_full(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_cph_hot_full, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_enqueue(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_enqueue, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_enqueue_already(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_enqueue_already, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_dequeue(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_dequeue, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_due_ready(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_due_ready, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_due_notready(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_due_notready, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_process_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_process_call, 1, memory_order_relaxed);
}

void hz4_os_stats_dq_process_skip_duegate(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_dq_process_skip_duegate, 1, memory_order_relaxed);
}

void hz4_os_stats_used_zero(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_used_zero, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_pending(uint32_t pending) {
    hz4_os_stats_init_once();
    uint64_t cur = atomic_load_explicit(&g_hz4_os_rss_pending_max, memory_order_relaxed);
    while (pending > cur) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_rss_pending_max, &cur,
                                                  pending, memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}

void hz4_os_stats_rss_gate_fire(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_gate_fire, 1, memory_order_relaxed);
}

void hz4_os_stats_decommit_skip_budget0_pending(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_decommit_skip_budget0_pending, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_len(uint32_t len) {
    hz4_os_stats_init_once();
    uint64_t cur = atomic_load_explicit(&g_hz4_os_rss_cool_len_max, memory_order_relaxed);
    while (len > cur) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_rss_cool_len_max, &cur,
                                                  (uint64_t)len,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}

void hz4_os_stats_rss_cool_promote(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_promote, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_drop_used(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_drop_used, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_release_fire(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_release_fire, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_enq(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_enq, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_block_deadline(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_block_deadline, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_sweep_budget_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_sweep_budget_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_cool_skip_already(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_cool_skip_already, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_quarantine_set(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_quarantine_set, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_quarantine_mature(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_quarantine_mature, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_quarantine_drop_used(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_quarantine_drop_used, 1, memory_order_relaxed);
}

void hz4_os_stats_collect_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_collect_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_refill_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_refill_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_inbox_consume_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_inbox_consume_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_inbox_lite_scan_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_scan_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_inbox_lite_shortcut(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_shortcut, 1, memory_order_relaxed);
}

void hz4_os_stats_inbox_stash_len(uint32_t len) {
    hz4_os_stats_init_once();
    uint64_t cur = atomic_load_explicit(&g_hz4_os_inbox_stash_len_max, memory_order_relaxed);
    while ((uint64_t)len > cur) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_inbox_stash_len_max, &cur,
                                                  (uint64_t)len,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}

void hz4_os_stats_carry_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_carry_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_carry_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_carry_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_drain_page_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_drain_page_objs(uint32_t n) {
    if (n == 0) return;
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_objs, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_page_used_dec(uint32_t n) {
    if (n == 0) return;
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_page_used_dec_calls, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_rlen(uint32_t rlen) {
    hz4_os_stats_init_once();
    if (rlen >= HZ4_REMOTE_FLUSH_THRESHOLD) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_ge_threshold, 1, memory_order_relaxed);
        return;
    }
    if (rlen == 1) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_1, 1, memory_order_relaxed);
    } else if (rlen <= 4) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_2_4, 1, memory_order_relaxed);
    } else if (rlen <= 8) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_5_8, 1, memory_order_relaxed);
    } else if (rlen <= 16) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_9_16, 1, memory_order_relaxed);
    } else if (rlen <= 32) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_17_32, 1, memory_order_relaxed);
    } else if (rlen <= 64) {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_33_64, 1, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_65_127, 1, memory_order_relaxed);
    }
}

void hz4_os_stats_remote_flush_fastpath_n1(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_n1, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_fastpath_le4(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_le4, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_probe_overflow(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_probe_overflow, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_compact_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_compact_call, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_compact_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_compact_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_compact_objs(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_compact_objs, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_compact_groups(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_compact_groups, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_compact_fallback(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_compact_fallback, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_direct_index_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_direct_index_call, 1, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_direct_index_objs(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_direct_index_objs, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_remote_flush_direct_index_groups(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_direct_index_groups, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_inbox_push_one(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_one_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_inbox_push_list(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_list_calls, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_try(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_try, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_ok(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_ok, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_fail_guard(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_fail_guard, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_fail_lock(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_fail_lock, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_fail_full(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_fail_full, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_fail_draining(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_fail_draining, 1, memory_order_relaxed);
}

void hz4_os_stats_rbmf_fallback(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbmf_fallback, 1, memory_order_relaxed);
}

void hz4_os_stats_rbuf_notify_transition(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbuf_notify_transition, 1, memory_order_relaxed);
}

void hz4_os_stats_rbuf_notify_already_queued(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rbuf_notify_already_queued, 1, memory_order_relaxed);
}

void hz4_os_stats_rbuf_drain_objs(uint32_t n) {
    hz4_os_stats_init_once();
    if (n == 0) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_0, 1, memory_order_relaxed);
    } else if (n == 1) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_1, 1, memory_order_relaxed);
    } else if (n <= 4) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_2_4, 1, memory_order_relaxed);
    } else if (n <= 8) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_5_8, 1, memory_order_relaxed);
    } else if (n <= 16) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_9_16, 1, memory_order_relaxed);
    } else if (n <= 32) {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_17_32, 1, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_hz4_os_rbuf_drain_33_plus, 1, memory_order_relaxed);
    }
}

void hz4_os_stats_large_rescue_attempt(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_attempt, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_success(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_success, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_fail(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_fail, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_fallback_flush(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_fallback_flush, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_route_batch_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_route_batch_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_route_mmap_retry_ok(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_route_mmap_retry_ok, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_route_mmap_retry_fail(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_route_mmap_retry_fail, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_gate_skip(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_gate_skip, 1, memory_order_relaxed);
}

void hz4_os_stats_large_rescue_gate_force(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_rescue_gate_force, 1, memory_order_relaxed);
}

void hz4_os_stats_large_owner_remote(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_owner_remote, 1, memory_order_relaxed);
}

void hz4_os_stats_large_owner_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_owner_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_owner_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_owner_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_owner_skip_pages(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_owner_skip_pages, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_acq_self_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_acq_self_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_acq_steal_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_acq_steal_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_acq_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_acq_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_rel_self_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_rel_self_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_rel_steal_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_rel_steal_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_rel_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_rel_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_acq_self_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_acq_self_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_acq_steal_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_acq_steal_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_acq_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_acq_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_rel_self_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_rel_self_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_rel_steal_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_rel_steal_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_p2_rel_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_p2_rel_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_hint_acq_try(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_hint_acq_try, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_hint_acq_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_hint_acq_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_hint_rel_try(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_hint_rel_try, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_hint_rel_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_hint_rel_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_steal_probe(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_steal_probe, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_steal_skip_budget(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_steal_skip_budget, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_steal_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_steal_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_mask_skip(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_mask_skip, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_mask_fallback(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_mask_fallback, 1, memory_order_relaxed);
}

void hz4_os_stats_large_lock_shard_mask_fallback_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_lock_shard_mask_fallback_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_acq_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_acq_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_acq_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_acq_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_rel_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_rel_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_rel_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_rel_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_acq_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_acq_call, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_acq_take_list(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_acq_take_list, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_acq_stash_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_acq_stash_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_rel_flush_call(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_rel_flush_call, 1, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_rel_flush_objs(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_rel_flush_objs, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_large_pagebin_b15_rel_cas_retry(uint32_t n) {
    if (n == 0) {
        return;
    }
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_pagebin_b15_rel_cas_retry, (uint64_t)n, memory_order_relaxed);
}

void hz4_os_stats_large_remote_free_seen(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_free_seen, 1, memory_order_relaxed);
}

void hz4_os_stats_large_remote_bypass_spancache(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_bypass_spancache, 1, memory_order_relaxed);
}

void hz4_os_stats_large_remote_bypass_pages1(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_bypass_pages1, 1, memory_order_relaxed);
}

void hz4_os_stats_large_remote_bypass_pages2(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_bypass_pages2, 1, memory_order_relaxed);
}

void hz4_os_stats_large_remote_bypass_cache_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_bypass_cache_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_remote_bypass_cache_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_remote_bypass_cache_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_acq_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_extent_b16_acq_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_acq_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_extent_b16_acq_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_rel_hit(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_extent_b16_rel_hit, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_rel_miss(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_extent_b16_rel_miss, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_rel_drop_cap(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_large_extent_b16_rel_drop_cap, 1, memory_order_relaxed);
}

void hz4_os_stats_large_extent_b16_bytes_peak(size_t bytes) {
    hz4_os_stats_init_once();
    uint64_t old = atomic_load_explicit(&g_hz4_os_large_extent_b16_bytes_peak, memory_order_relaxed);
    while ((uint64_t)bytes > old) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_large_extent_b16_bytes_peak, &old,
                                                  (uint64_t)bytes,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}

#if HZ4_RSSRETURN_PRESSUREGATEBOX
void hz4_os_stats_rss_gate_on_window(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_gate_on_windows, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_gate_seg_acq_delta(uint32_t delta) {
    hz4_os_stats_init_once();
    uint64_t old = atomic_load_explicit(&g_hz4_os_rss_gate_seg_acq_delta_max, memory_order_relaxed);
    while ((uint64_t)delta > old) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_rss_gate_seg_acq_delta_max, &old,
                                                  (uint64_t)delta,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}
#endif

#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
// Include hz4_tls.h for hz4_tls_t and hz4_os_stats_tls_t definitions
#include "hz4_tls.h"

void hz4_os_stats_tls_flush(struct hz4_tls* tls) {
    hz4_os_stats_init_once();
    hz4_os_stats_tls_t* st = &tls->stats_tls;

    atomic_fetch_add_explicit(&g_hz4_os_collect_calls, st->collect_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_refill_calls, st->refill_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_consume_calls, st->inbox_consume_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_scan_calls, st->inbox_lite_scan_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_shortcut, st->inbox_lite_shortcut, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_carry_hit, st->carry_hit, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_carry_miss, st->carry_miss, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_calls, st->drain_page_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_objs, st->drain_page_objs, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_page_used_dec_calls, st->page_used_dec_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_calls, st->remote_flush_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_n1, st->remote_flush_n1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_le4, st->remote_flush_le4, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_probe_overflow, st->remote_flush_probe_ovf, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_one_calls, st->inbox_push_one_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_list_calls, st->inbox_push_list_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_1, st->rf_rlen_1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_2_4, st->rf_rlen_2_4, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_5_8, st->rf_rlen_5_8, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_9_16, st->rf_rlen_9_16, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_17_32, st->rf_rlen_17_32, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_33_64, st->rf_rlen_33_64, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_65_127, st->rf_rlen_65_127, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_ge_threshold, st->rf_rlen_ge_th, memory_order_relaxed);

    // Clear TLS counters after flush
    *st = (hz4_os_stats_tls_t){0};
}
#endif
