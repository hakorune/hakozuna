// =============================================================================
// S46: Flush owner stash to central with budget (for pressure box)
// =============================================================================
void hz3_owner_stash_flush_to_central_budget(uint8_t owner, uint32_t budget_per_sc) {
    if (owner >= HZ3_NUM_SHARDS) {
        return;
    }

    hz3_owner_stash_init();

#if HZ3_S74_LANE_BATCH
    void* batch[HZ3_S74_FLUSH_BATCH];
    const int batch_cap = HZ3_S74_FLUSH_BATCH;
#else
    void* batch[64];
    const int batch_cap = 64;
#endif
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        uint32_t total = 0;
        while (total < budget_per_sc) {
            int want = batch_cap;
            if (total + (uint32_t)want > budget_per_sc) {
                want = (int)(budget_per_sc - total);
            }
            int got = hz3_owner_stash_pop_batch(owner, sc, batch, want);
            if (got == 0) {
                break;  // empty for this sc
            }
            // Build linked list from batch array and push to central
            for (int i = 0; i < got - 1; i++) {
                hz3_obj_set_next(batch[i], batch[i + 1]);
            }
            hz3_obj_set_next(batch[got - 1], NULL);
            hz3_small_v2_central_push_list(owner, sc, batch[0], batch[got - 1], (uint32_t)got);
#if HZ3_S74_STATS
            hz3_s74_stats_flush((uint32_t)got);
#endif
            total += (uint32_t)got;
        }
    }
}

// =============================================================================
// S71: Check if stash has pending objects for this owner/sc
// Used by retire scan to skip SC if objects are in transit
// =============================================================================
int hz3_owner_stash_has_pending(uint8_t owner, int sc) {
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    hz3_owner_stash_init();

#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Check all instances for pending objects
    for (int i = 0; i < HZ3_OWNER_STASH_INSTANCES; i++) {
        void* head = atomic_load_explicit(
            &g_hz3_owner_stash[owner][sc][i].head, memory_order_relaxed);
        if (head != NULL) return 1;
    }
    return 0;
#else
    Hz3OwnerStashBin* bin = &g_hz3_owner_stash[owner][sc];
    // Relaxed load OK - approximate check for retire gate
    void* head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    return (head != NULL);
#endif
}
