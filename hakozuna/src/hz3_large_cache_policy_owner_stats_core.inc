static _Atomic size_t g_s240_activate = 0;
static _Atomic size_t g_s240_activate_cache = 0;
static _Atomic size_t g_s240_activate_mmap = 0;
static _Atomic size_t g_s240_cache_seed = 0;
static _Atomic size_t g_s240_cache_store = 0;
static _Atomic size_t g_s240_munmap_store = 0;
static _Atomic size_t g_s240_free_seen = 0;
static _Atomic size_t g_s240_free_active = 0;
static _Atomic size_t g_s240_state_mismatch = 0;
static _Atomic size_t g_s240_sc_mismatch = 0;
static _Atomic size_t g_s240_owner_unknown_activate = 0;
static _Atomic size_t g_s240_owner_unknown_free = 0;
static _Atomic size_t g_s240_owner_same_free = 0;
static _Atomic size_t g_s240_owner_remote_free = 0;
#if HZ3_S240_LARGE_FRONT_CACHE
static _Atomic size_t g_s240_front_push = 0;
static _Atomic size_t g_s240_front_pop_hit = 0;
static _Atomic size_t g_s240_front_pop_miss = 0;
static _Atomic size_t g_s240_front_reject_limit = 0;
static _Atomic size_t g_s240_front_flush_calls = 0;
static _Atomic size_t g_s240_front_flush_objs = 0;
static _Atomic size_t g_s240_front_flush_bytes = 0;
static _Atomic size_t g_s240_front_evict_munmap = 0;
static _Atomic size_t g_s240_front_nodes_current = 0;
static _Atomic size_t g_s240_front_nodes_peak = 0;
static _Atomic size_t g_s240_front_bytes_current = 0;
static _Atomic size_t g_s240_front_bytes_peak = 0;
#endif
#if HZ3_S240_LARGE_OWNER_INBOX
static _Atomic size_t g_s240_inbox_remote_push = 0;
static _Atomic size_t g_s240_inbox_reject_owner_dead = 0;
static _Atomic size_t g_s240_inbox_reject_owner_exiting = 0;
static _Atomic size_t g_s240_inbox_reject_shard_collision = 0;
static _Atomic size_t g_s240_inbox_reject_budget = 0;
static _Atomic size_t g_s240_inbox_drain_calls = 0;
static _Atomic size_t g_s240_inbox_drain_nonempty = 0;
static _Atomic size_t g_s240_inbox_drain_objs = 0;
static _Atomic size_t g_s240_inbox_drain_front_reject = 0;
static _Atomic size_t g_s240_inbox_flush_calls = 0;
static _Atomic size_t g_s240_inbox_flush_objs = 0;
static _Atomic size_t g_s240_inbox_flush_bytes = 0;
static _Atomic size_t g_s240_inbox_cas_retries = 0;
static _Atomic size_t g_s240_inbox_nodes_current = 0;
static _Atomic size_t g_s240_inbox_nodes_peak = 0;
static _Atomic size_t g_s240_inbox_bytes_current = 0;
static _Atomic size_t g_s240_inbox_bytes_peak = 0;
static _Atomic size_t g_s240_inbox_pushers_peak = 0;
static _Atomic size_t g_s240_inbox_flush_wait_spins = 0;
#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static _Atomic size_t g_s245_batch_pop_calls = 0;
static _Atomic size_t g_s245_batch_pop_nonempty = 0;
static _Atomic size_t g_s245_batch_pop_objs = 0;
static _Atomic size_t g_s245_batch_pop_cas_retries = 0;
static _Atomic size_t g_s245_batch_pop_max_objs = 0;
#endif
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
static _Atomic size_t g_s246_take_first_calls = 0;
static _Atomic size_t g_s246_take_first_hits = 0;
static _Atomic size_t g_s246_take_first_misses = 0;
static _Atomic size_t g_s246_take_first_direct = 0;
static _Atomic size_t g_s246_take_first_extra_fronted = 0;
static _Atomic size_t g_s246_take_first_extra_munmap = 0;
#endif
#if HZ3_S282_LARGE_OWNER_INBOX_OBS
static _Atomic size_t g_s282_push_attempt = 0;
static _Atomic size_t g_s282_push_ok = 0;
static _Atomic size_t g_s282_push_reject_invalid = 0;
static _Atomic size_t g_s282_push_reject_owner_dead = 0;
static _Atomic size_t g_s282_push_reject_owner_exiting = 0;
static _Atomic size_t g_s282_push_reject_shard_collision = 0;
static _Atomic size_t g_s282_push_reject_budget = 0;
static _Atomic size_t g_s282_push_cas_attempts = 0;
static _Atomic size_t g_s282_push_cas_failures = 0;
static _Atomic size_t g_s282_push_cas_failure_max_per_push = 0;
static _Atomic size_t g_s282_push_depth_0 = 0;
static _Atomic size_t g_s282_push_depth_1 = 0;
static _Atomic size_t g_s282_push_depth_2_3 = 0;
static _Atomic size_t g_s282_push_depth_4_7 = 0;
static _Atomic size_t g_s282_push_depth_8_15 = 0;
static _Atomic size_t g_s282_push_depth_16_plus = 0;
static _Atomic size_t g_s282_pop_cas_attempts = 0;
static _Atomic size_t g_s282_pop_cas_failures = 0;
static _Atomic size_t g_s282_pop_cas_failure_max_per_pop = 0;
static _Atomic size_t g_s282_pop_success = 0;
static _Atomic size_t g_s282_pop_empty = 0;
static _Atomic size_t g_s282_pop_race_empty = 0;
static _Atomic size_t g_s282_take_first_calls = 0;
static _Atomic size_t g_s282_take_first_hit = 0;
static _Atomic size_t g_s282_take_first_miss_empty = 0;
static _Atomic size_t g_s282_take_first_miss_race = 0;
static _Atomic size_t g_s282_take_depth_0 = 0;
static _Atomic size_t g_s282_take_depth_1 = 0;
static _Atomic size_t g_s282_take_depth_2_3 = 0;
static _Atomic size_t g_s282_take_depth_4_7 = 0;
static _Atomic size_t g_s282_take_depth_8_15 = 0;
static _Atomic size_t g_s282_take_depth_16_plus = 0;
static _Atomic size_t g_s282_drain_nodes = 0;
static _Atomic size_t g_s282_drain_return_first_nodes = 0;
static _Atomic size_t g_s282_drain_to_front_nodes = 0;
static _Atomic size_t g_s282_drain_to_munmap_nodes = 0;
#endif
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
static _Atomic size_t g_s251_free_take_to_freeing = 0;
static _Atomic size_t g_s251_free_take_state_fail = 0;
static _Atomic size_t g_s251_front_pop_to_freeing = 0;
static _Atomic size_t g_s251_front_pop_state_fail = 0;
static _Atomic size_t g_s251_inbox_pop_to_freeing = 0;
static _Atomic size_t g_s251_inbox_pop_state_fail = 0;
static _Atomic size_t g_s251_global_pop_to_freeing = 0;
static _Atomic size_t g_s251_global_pop_state_fail = 0;
static _Atomic size_t g_s251_front_push_state_fail = 0;
static _Atomic size_t g_s251_inbox_push_state_fail = 0;
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
static _Atomic size_t g_s276_front_take_try = 0;
static _Atomic size_t g_s276_front_take_hit = 0;
static _Atomic size_t g_s276_front_take_not_owner = 0;
static _Atomic size_t g_s276_front_take_not_eligible = 0;
static _Atomic size_t g_s276_front_take_nonactive = 0;
static _Atomic size_t g_s276_front_take_state_cas_fail = 0;
static _Atomic size_t g_s276_front_push_retained = 0;
static _Atomic size_t g_s276_front_push_reject_clear = 0;
static _Atomic size_t g_s276_front_activate_fast = 0;
static _Atomic size_t g_s276_front_activate_rekey = 0;
static _Atomic size_t g_s276_inbox_take_hit = 0;
static _Atomic size_t g_s276_inbox_push_retained = 0;
static _Atomic size_t g_s276_inbox_push_reject_clear = 0;
static _Atomic size_t g_s276_inbox_activate_fast = 0;
static _Atomic size_t g_s276_direct_clear = 0;
#endif
#if HZ3_S292_HIGHBAND_RETENTION_OBS
static _Atomic size_t g_s292_alloc_highband = 0;
static _Atomic size_t g_s292_alloc_65537 = 0;
static _Atomic size_t g_s292_alloc_lte128k = 0;
static _Atomic size_t g_s292_alloc_gt256k = 0;
static _Atomic size_t g_s292_free_highband = 0;
static _Atomic size_t g_s292_free_65537 = 0;
static _Atomic size_t g_s292_front_pop_calls = 0;
static _Atomic size_t g_s292_front_pop_hit = 0;
static _Atomic size_t g_s292_front_pop_miss = 0;
static _Atomic size_t g_s292_front_push = 0;
static _Atomic size_t g_s292_front_reject = 0;
static _Atomic size_t g_s292_front_nodes_current = 0;
static _Atomic size_t g_s292_front_nodes_peak = 0;
static _Atomic size_t g_s292_front_bytes_current = 0;
static _Atomic size_t g_s292_front_bytes_peak = 0;
static _Atomic size_t g_s292_inbox_push_attempt = 0;
static _Atomic size_t g_s292_inbox_push_ok = 0;
static _Atomic size_t g_s292_inbox_reject_invalid = 0;
static _Atomic size_t g_s292_inbox_reject_owner = 0;
static _Atomic size_t g_s292_inbox_reject_budget = 0;
static _Atomic size_t g_s292_inbox_pop = 0;
static _Atomic size_t g_s292_inbox_flush = 0;
static _Atomic size_t g_s292_inbox_nodes_current = 0;
static _Atomic size_t g_s292_inbox_nodes_peak = 0;
static _Atomic size_t g_s292_inbox_bytes_current = 0;
static _Atomic size_t g_s292_inbox_bytes_peak = 0;
static _Atomic size_t g_s292_global_fallback = 0;
static _Atomic size_t g_s292_global_cache_store = 0;
static _Atomic size_t g_s292_global_munmap = 0;
#endif
#if HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP
static _Atomic size_t g_s293_global_admit_try = 0;
static _Atomic size_t g_s293_global_admit_ok = 0;
static _Atomic size_t g_s293_global_reject_cap = 0;
static _Atomic size_t g_s293_global_reject_nonwindow = 0;
#endif
#if HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP
static _Atomic size_t g_s293d_global_admit_try = 0;
static _Atomic size_t g_s293d_global_admit_ok = 0;
static _Atomic size_t g_s293d_global_admit_soft = 0;
static _Atomic size_t g_s293d_global_reject_soft = 0;
static _Atomic size_t g_s293d_global_reject_hard = 0;
static _Atomic size_t g_s293d_global_reject_nonwindow = 0;
#endif
#if HZ3_S285_LARGE_FIXED_COST_OBS
static _Atomic size_t g_s285_direct_way_loads = 0;
static _Atomic size_t g_s285_direct_key_busy = 0;
static _Atomic size_t g_s285_direct_key_miss = 0;
static _Atomic size_t g_s285_direct_key_match = 0;
static _Atomic size_t g_s285_direct_invalid_hdr = 0;
static _Atomic size_t g_s285_direct_state_nonactive = 0;
static _Atomic size_t g_s285_direct_inuse_bad = 0;
static _Atomic size_t g_s285_direct_not_retainable_same_owner = 0;
static _Atomic size_t g_s285_direct_not_retainable_remote = 0;
static _Atomic size_t g_s285_direct_cas_attempt = 0;
static _Atomic size_t g_s285_direct_cas_fail = 0;
static _Atomic size_t g_s285_direct_hit_front = 0;
static _Atomic size_t g_s285_direct_hit_inbox = 0;
static _Atomic size_t g_s285_activate_check = 0;
static _Atomic size_t g_s285_activate_fail_header = 0;
static _Atomic size_t g_s285_activate_fail_slot = 0;
static _Atomic size_t g_s285_activate_fail_flag = 0;
static _Atomic size_t g_s285_activate_fail_sc = 0;
static _Atomic size_t g_s285_activate_fail_alignment = 0;
static _Atomic size_t g_s285_activate_fail_user_alignment = 0;
static _Atomic size_t g_s285_activate_fail_range = 0;
static _Atomic size_t g_s285_activate_fail_size = 0;
static _Atomic size_t g_s285_activate_success_front = 0;
static _Atomic size_t g_s285_activate_success_inbox = 0;
static _Atomic size_t g_s285_take_first_return_first = 0;
static _Atomic size_t g_s285_take_first_extra_pop_attempt = 0;
static _Atomic size_t g_s285_take_first_extra_pop_hit = 0;
static _Atomic size_t g_s285_take_first_extra_pop_empty = 0;
static _Atomic size_t g_s285_take_first_extra_front_ok = 0;
static _Atomic size_t g_s285_take_first_extra_front_reject = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_1 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_2_3 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_4_7 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_8_plus = 0;
#endif
#if HZ3_S240_LARGE_FLUSH_VALIDATE
static _Atomic size_t g_s240_flush_validate_calls = 0;
static _Atomic size_t g_s240_flush_validate_failures = 0;
static _Atomic size_t g_s240_flush_front_node_fail = 0;
static _Atomic size_t g_s240_flush_inbox_node_fail = 0;
static _Atomic size_t g_s240_flush_drain_node_fail = 0;
static _Atomic size_t g_s240_flush_null_node_fail = 0;
static _Atomic size_t g_s240_flush_bad_state = 0;
static _Atomic size_t g_s240_flush_bad_header = 0;
static _Atomic size_t g_s240_flush_bad_link = 0;
static _Atomic size_t g_s240_flush_sc_mismatch = 0;
static _Atomic size_t g_s240_flush_inbox_count_underflow = 0;
static _Atomic size_t g_s240_flush_inbox_bytes_underflow = 0;
#endif
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
static _Atomic size_t g_s240_flush_quarantine_objs = 0;
static _Atomic size_t g_s240_flush_quarantine_bytes = 0;
#endif

static inline void hz3_large_s240_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_large_s240_add(_Atomic size_t* p, size_t v) {
    atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static inline void hz3_large_s240_sub(_Atomic size_t* p, size_t v) {
    atomic_fetch_sub_explicit(p, v, memory_order_relaxed);
}

static inline void hz3_large_s240_update_max(_Atomic size_t* p, size_t value) {
    size_t cur = atomic_load_explicit(p, memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                p, &cur, value, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

#if HZ3_S292_HIGHBAND_RETENTION_OBS
static inline void hz3_s292_update_bytes(_Atomic size_t* current,
                                         _Atomic size_t* peak,
                                         size_t bytes,
                                         int add) {
    if (add) {
        size_t total = atomic_fetch_add_explicit(current,
                                                 bytes,
                                                 memory_order_relaxed) + bytes;
        hz3_large_s240_update_max(peak, total);
    } else {
        hz3_large_s240_sub(current, bytes);
    }
}

static inline void hz3_s292_update_nodes(_Atomic size_t* current,
                                         _Atomic size_t* peak,
                                         int add) {
    if (add) {
        size_t total = atomic_fetch_add_explicit(current,
                                                 1,
                                                 memory_order_relaxed) + 1;
        hz3_large_s240_update_max(peak, total);
    } else {
        hz3_large_s240_sub(current, 1);
    }
}

static inline int hz3_s292_is_highband_req(size_t req_size) {
    return req_size > (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES &&
           req_size <= (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES;
}

static inline void hz3_s292_on_alloc_req(size_t req_size) {
    if (hz3_s292_is_highband_req(req_size)) {
        hz3_large_s240_inc(&g_s292_alloc_highband);
    } else if (req_size == 65537u) {
        hz3_large_s240_inc(&g_s292_alloc_65537);
    } else if (req_size <= (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES) {
        hz3_large_s240_inc(&g_s292_alloc_lte128k);
    } else if (req_size > (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES) {
        hz3_large_s240_inc(&g_s292_alloc_gt256k);
    }
}

static inline void hz3_s292_on_free_hdr(const Hz3LargeHdr* hdr) {
    if (!hdr) {
        return;
    }
    if (hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_free_highband);
    } else if (hdr->req_size == 65537u) {
        hz3_large_s240_inc(&g_s292_free_65537);
    }
}

static inline void hz3_s292_on_front_pop(size_t req_size, int hit, size_t bytes) {
    if (!hz3_s292_is_highband_req(req_size)) {
        return;
    }
    hz3_large_s240_inc(&g_s292_front_pop_calls);
    if (hit) {
        hz3_large_s240_inc(&g_s292_front_pop_hit);
        hz3_s292_update_nodes(&g_s292_front_nodes_current,
                              &g_s292_front_nodes_peak,
                              0);
        hz3_s292_update_bytes(&g_s292_front_bytes_current,
                              &g_s292_front_bytes_peak,
                              bytes,
                              0);
    } else {
        hz3_large_s240_inc(&g_s292_front_pop_miss);
    }
}

static inline void hz3_s292_on_front_push_result(const Hz3LargeHdr* hdr, int success) {
    if (!hdr || !hz3_s292_is_highband_req(hdr->req_size)) {
        return;
    }
    if (!success) {
        hz3_large_s240_inc(&g_s292_front_reject);
        return;
    }
    hz3_large_s240_inc(&g_s292_front_push);
    hz3_s292_update_nodes(&g_s292_front_nodes_current,
                          &g_s292_front_nodes_peak,
                          1);
    hz3_s292_update_bytes(&g_s292_front_bytes_current,
                          &g_s292_front_bytes_peak,
                          hdr->map_size,
                          1);
}

static inline void hz3_s292_on_front_flush_node(const Hz3LargeHdr* hdr) {
    if (!hdr || !hz3_s292_is_highband_req(hdr->req_size)) {
        return;
    }
    hz3_s292_update_nodes(&g_s292_front_nodes_current,
                          &g_s292_front_nodes_peak,
                          0);
    hz3_s292_update_bytes(&g_s292_front_bytes_current,
                          &g_s292_front_bytes_peak,
                          hdr->map_size,
                          0);
}

static inline void hz3_s292_on_inbox_push_attempt(const Hz3LargeHdr* hdr) {
    if (hdr && hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_inbox_push_attempt);
    }
}

static inline void hz3_s292_on_inbox_push_result(const Hz3LargeHdr* hdr,
                                                 int ok,
                                                 int reject_owner,
                                                 int reject_budget,
                                                 int reject_invalid) {
    if (!hdr || !hz3_s292_is_highband_req(hdr->req_size)) {
        return;
    }
    if (ok) {
        hz3_large_s240_inc(&g_s292_inbox_push_ok);
        hz3_s292_update_nodes(&g_s292_inbox_nodes_current,
                              &g_s292_inbox_nodes_peak,
                              1);
        hz3_s292_update_bytes(&g_s292_inbox_bytes_current,
                              &g_s292_inbox_bytes_peak,
                              hdr->map_size,
                              1);
        return;
    }
    if (reject_owner) {
        hz3_large_s240_inc(&g_s292_inbox_reject_owner);
    }
    if (reject_budget) {
        hz3_large_s240_inc(&g_s292_inbox_reject_budget);
    }
    if (reject_invalid) {
        hz3_large_s240_inc(&g_s292_inbox_reject_invalid);
    }
}

static inline void hz3_s292_on_inbox_pop(const Hz3LargeHdr* hdr, int from_flush) {
    if (!hdr || !hz3_s292_is_highband_req(hdr->req_size)) {
        return;
    }
    if (from_flush) {
        hz3_large_s240_inc(&g_s292_inbox_flush);
    } else {
        hz3_large_s240_inc(&g_s292_inbox_pop);
    }
    hz3_s292_update_nodes(&g_s292_inbox_nodes_current,
                          &g_s292_inbox_nodes_peak,
                          0);
    hz3_s292_update_bytes(&g_s292_inbox_bytes_current,
                          &g_s292_inbox_bytes_peak,
                          hdr->map_size,
                          0);
}

static inline void hz3_s292_on_inbox_flush_seen(const Hz3LargeHdr* hdr) {
    if (hdr && hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_inbox_flush);
    }
}

static inline void hz3_s292_on_global_fallback(const Hz3LargeHdr* hdr) {
    if (hdr && hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_global_fallback);
    }
}

static inline void hz3_s292_on_global_store(const Hz3LargeHdr* hdr) {
    if (hdr && hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_global_cache_store);
    }
}

static inline void hz3_s292_on_global_munmap(const Hz3LargeHdr* hdr) {
    if (hdr && hz3_s292_is_highband_req(hdr->req_size)) {
        hz3_large_s240_inc(&g_s292_global_munmap);
    }
}
#else
static inline void hz3_s292_on_alloc_req(size_t req_size) {
    (void)req_size;
}
static inline void hz3_s292_on_free_hdr(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_front_pop(size_t req_size, int hit, size_t bytes) {
    (void)req_size;
    (void)hit;
    (void)bytes;
}
static inline void hz3_s292_on_front_push_result(const Hz3LargeHdr* hdr, int success) {
    (void)hdr;
    (void)success;
}
static inline void hz3_s292_on_front_flush_node(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_inbox_push_attempt(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_inbox_push_result(const Hz3LargeHdr* hdr,
                                                 int ok,
                                                 int reject_owner,
                                                 int reject_budget,
                                                 int reject_invalid) {
    (void)hdr;
    (void)ok;
    (void)reject_owner;
    (void)reject_budget;
    (void)reject_invalid;
}
static inline void hz3_s292_on_inbox_pop(const Hz3LargeHdr* hdr, int from_flush) {
    (void)hdr;
    (void)from_flush;
}
static inline void hz3_s292_on_inbox_flush_seen(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_fallback(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_store(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_munmap(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
#endif

#if HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP
static inline int hz3_s293_highband_global_req(const Hz3LargeHdr* hdr) {
    return hdr &&
           hdr->req_size > (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES &&
           hdr->req_size <= (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES;
}

static inline int hz3_s293_global_admit_under_cap_locked(int sc,
                                                         const Hz3LargeHdr* hdr) {
    if (!hz3_s293_highband_global_req(hdr)) {
        hz3_large_s240_inc(&g_s293_global_reject_nonwindow);
        return 1;
    }
    hz3_large_s240_inc(&g_s293_global_admit_try);
    hz3_large_sc_lock_acquire(sc);
    uint32_t nodes = g_sc_nodes[sc];
    hz3_large_sc_lock_release(sc);
    if (nodes >= (uint32_t)HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC) {
        hz3_large_s240_inc(&g_s293_global_reject_cap);
        return 0;
    }
    hz3_large_s240_inc(&g_s293_global_admit_ok);
    return 1;
}
#else
static inline int hz3_s293_global_admit_under_cap_locked(int sc,
                                                         const Hz3LargeHdr* hdr) {
    (void)sc;
    (void)hdr;
    return 1;
}
#endif

#if HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP
static inline int hz3_s293d_highband_global_req(const Hz3LargeHdr* hdr) {
    return hdr &&
           hdr->req_size > (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES &&
           hdr->req_size <= (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES;
}

static inline int hz3_s293d_global_admit_under_soft_cap_locked(
    int sc,
    const Hz3LargeHdr* hdr) {
    if (!hz3_s293d_highband_global_req(hdr)) {
        hz3_large_s240_inc(&g_s293d_global_reject_nonwindow);
        return 1;
    }
    hz3_large_s240_inc(&g_s293d_global_admit_try);
    hz3_large_sc_lock_acquire(sc);
    uint32_t nodes = g_sc_nodes[sc];
    hz3_large_sc_lock_release(sc);
    if (nodes < (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_SOFT_NODES_PER_SC) {
        hz3_large_s240_inc(&g_s293d_global_admit_ok);
        return 1;
    }
    if (nodes >= (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_HARD_NODES_PER_SC) {
        hz3_large_s240_inc(&g_s293d_global_reject_hard);
        return 0;
    }
    uint32_t epoch = atomic_fetch_add_explicit(
        &g_s293d_global_admit_epoch[sc],
        1,
        memory_order_relaxed);
    if ((epoch % (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_ADMIT_STRIDE) == 0) {
        hz3_large_s240_inc(&g_s293d_global_admit_ok);
        hz3_large_s240_inc(&g_s293d_global_admit_soft);
        return 1;
    }
    hz3_large_s240_inc(&g_s293d_global_reject_soft);
    return 0;
}
#else
static inline int hz3_s293d_global_admit_under_soft_cap_locked(
    int sc,
    const Hz3LargeHdr* hdr) {
    (void)sc;
    (void)hdr;
    return 1;
}
#endif

