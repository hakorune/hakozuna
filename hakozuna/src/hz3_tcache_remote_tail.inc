// ============================================================================
// Day 4: Outbox operations
// ============================================================================

#if !HZ3_REMOTE_STASH_SPARSE
// Flush outbox to owner's inbox (FIFO order)
void hz3_outbox_flush(uint8_t owner, int sc) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    if (ob->count == 0) return;

    // Build FIFO linked list: slots[0] -> slots[1] -> ... -> slots[count-1]
    void* head = ob->slots[0];
    for (uint8_t i = 0; i < ob->count - 1; i++) {
        hz3_obj_set_next(ob->slots[i], ob->slots[i + 1]);
    }
    hz3_obj_set_next(ob->slots[ob->count - 1], NULL);
    void* tail = ob->slots[ob->count - 1];

    // Push to inbox
    hz3_inbox_push_list(owner, sc, head, tail, ob->count);
    ob->count = 0;
}

// Push to outbox, flush if full
void hz3_outbox_push(uint8_t owner, int sc, void* obj) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    ob->slots[ob->count++] = obj;

    if (ob->count >= HZ3_OUTBOX_SIZE) {
        hz3_outbox_flush(owner, sc);
    }
}
#endif  // !HZ3_REMOTE_STASH_SPARSE

#include "hz3_tcache_remote_stash.inc"
// ============================================================================
// S17: dst/bin direct remote bank flush (event-only, dense bank only)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

#if HZ3_TCACHE_SOA_BANK
// S40: SoA version - detach from bank_head/bank_count
static void hz3_dstbin_detach_soa(uint8_t dst, int bin_idx, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = t_hz3_cache.bank_head[dst][bin_idx];
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    t_hz3_cache.bank_head[dst][bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
    t_hz3_cache.bank_count[dst][bin_idx] = 0;
#endif
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S40: SoA version - flush one bin
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    if (!t_hz3_cache.bank_head[dst][bin]) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach_soa(dst, bin, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
    } else if (hz3_bin_is_medium_aligned((uint32_t)bin)) {
        int sc = hz3_bin_medium_aligned_sc((uint32_t)bin);
        hz3_inbox_aligned_push_list(dst, sc, head, tail, n);
#endif
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}

#else  // !HZ3_TCACHE_SOA_BANK

static void hz3_dstbin_detach(Hz3Bin* bin, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = hz3_bin_head(bin);
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    hz3_bin_clear(bin);
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S24: Helper to flush one bin (shared by budget/all)
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
    if (hz3_bin_is_empty(b)) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach(b, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
    } else if (hz3_bin_is_medium_aligned((uint32_t)bin)) {
        int sc = hz3_bin_medium_aligned_sc((uint32_t)bin);
        hz3_inbox_aligned_push_list(dst, sc, head, tail, n);
#endif
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}
#endif  // HZ3_TCACHE_SOA_BANK

#endif  // HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

// ============================================================================
// S24-1/S41: Remote flush entry points (work for both sparse and dense)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE
// S190-v3: targeted remote flush by medium size-class.
void hz3_dstbin_flush_remote_sc_budget(uint32_t sc, uint32_t budget_bins) {
    if (budget_bins == 0) return;

    uint32_t target_bin32 = (uint32_t)HZ3_MEDIUM_BIN_BASE + sc;
    if (target_bin32 >= (uint32_t)HZ3_BIN_TOTAL) {
        hz3_dstbin_flush_remote_budget(budget_bins);
        return;
    }

    uint8_t target_bin = (uint8_t)target_bin32;
#if HZ3_REMOTE_STASH_SPARSE
    // Sparse mode keeps a ring; only fire budget flush when target bin is observed.
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;
    uint32_t scanned = 0;
    int found_target = 0;
    while (t != h && scanned < budget_bins) {
        if (ring->ring[t].bin == target_bin) {
            found_target = 1;
            break;
        }
        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        scanned++;
    }
    if (!found_target) {
        return;
    }
    hz3_remote_stash_flush_budget_impl(budget_bins);
#if HZ3_S173_DSTBIN_DEMAND_GATE
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense mode: scan only owner shards for the selected bin.
    uint8_t my_shard = t_hz3_cache.my_shard;
    uint8_t dst = t_hz3_cache.remote_dst_cursor;
    uint8_t start_dst = dst;
    uint32_t scanned = 0;
    int wrapped = 0;

    while (scanned < budget_bins && !wrapped) {
        if (dst == my_shard) {
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            if (dst == start_dst) break;
            continue;
        }

#if HZ3_TCACHE_SOA_BANK
        void* head = t_hz3_cache.bank_head[dst][target_bin];
#else
        Hz3Bin* b = hz3_tcache_get_bank_bin(dst, (int)target_bin);
        void* head = b->head;
#endif
        scanned++;
        if (head) {
            hz3_dstbin_flush_one(dst, (int)target_bin);
            t_hz3_cache.remote_hint = 1;
        }

        dst = (dst + 1) % HZ3_NUM_SHARDS;
        if (dst == start_dst) wrapped = 1;
    }

    t_hz3_cache.remote_dst_cursor = dst;
#endif
}

// S24-1: Budgeted flush with round-robin cursor
void hz3_dstbin_flush_remote_budget(uint32_t budget_bins) {
#if HZ3_REMOTE_STASH_SPARSE
    // S41: For sparse ring, budget_bins means "number of ring entries"
    //      (not "number of bins to scan" as in dense bank)
    hz3_remote_stash_flush_budget_impl(budget_bins);
#if HZ3_S173_DSTBIN_DEMAND_GATE
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    uint8_t dst = t_hz3_cache.remote_dst_cursor;
    uint16_t bin = t_hz3_cache.remote_bin_cursor;
    uint32_t scanned = 0;
    uint8_t start_dst = dst;
    int wrapped = 0;

    while (scanned < budget_bins && !wrapped) {
        // Skip my_shard
        if (dst == my_shard) {
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            bin = 0;
            if (dst == start_dst) break;
            continue;
        }

#if HZ3_TCACHE_SOA_BANK
        void* head = t_hz3_cache.bank_head[dst][bin];
#else
        Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
        void* head = b->head;
#endif
        scanned++;
        if (head) {
            hz3_dstbin_flush_one(dst, bin);
            // Found remote -> set hint (budgeted can only raise, never lower)
            t_hz3_cache.remote_hint = 1;
        }

        // Advance cursor
        bin++;
        if (bin >= HZ3_BIN_TOTAL) {
            bin = 0;
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            // Check for wrap-around
            if (dst == start_dst) wrapped = 1;
        }
    }

    // Save cursor for next call
    t_hz3_cache.remote_dst_cursor = dst;
    t_hz3_cache.remote_bin_cursor = bin;
#endif
}

// S24-1: Full flush (all bins, for epoch/destructor)
void hz3_dstbin_flush_remote_all(void) {
#if HZ3_REMOTE_STASH_SPARSE
    hz3_remote_stash_flush_all_impl();
#if HZ3_S173_DSTBIN_DEMAND_GATE
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    int found_any = 0;

    for (uint8_t dst = 0; dst < HZ3_NUM_SHARDS; dst++) {
        if (dst == my_shard) continue;
        for (int bin = 0; bin < HZ3_BIN_TOTAL; bin++) {
#if HZ3_TCACHE_SOA_BANK
            if (t_hz3_cache.bank_head[dst][bin]) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#else
            Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
            if (b->head) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#endif
        }
    }

    // Full flush confirms hint: 0 if nothing found, 1 if any found
    t_hz3_cache.remote_hint = found_any ? 1 : 0;
    // Reset cursor after full flush
    t_hz3_cache.remote_dst_cursor = 0;
    t_hz3_cache.remote_bin_cursor = 0;
#endif
}
#endif  // HZ3_PTAG_DSTBIN_ENABLE
