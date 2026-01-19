// Archived code: S128 RemoteStash Defer-MinRun (flush-side pending table)
// Copied from: hakozuna/hz3/src/hz3_tcache_remote.c
// Note: This is a reference snapshot; not built.

// =============================================================================
// S128: RemoteStash Defer-MinRun stats (atexit one-shot)
// =============================================================================
#if HZ3_S128_STATS
HZ3_DTOR_STATS_BEGIN(S128);
HZ3_DTOR_STAT(g_s128_calls);
HZ3_DTOR_STAT(g_s128_ring_drained_total);
HZ3_DTOR_STAT(g_s128_dispatched_groups_total);
HZ3_DTOR_STAT(g_s128_dispatched_entries_total);
HZ3_DTOR_STAT(g_s128_deferred_groups_total);
HZ3_DTOR_STAT(g_s128_forced_age_groups_total);
HZ3_DTOR_STAT(g_s128_forced_maxkeys_flushes);
HZ3_DTOR_STAT(g_s128_pending_keys_max);
HZ3_DTOR_STAT(g_s128_pending_objs_max);
HZ3_DTOR_ATEXIT_FLAG(g_s128);

static void hz3_s128_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s128_calls);
    uint32_t ring_drained = HZ3_DTOR_STAT_LOAD(g_s128_ring_drained_total);
    uint32_t disp_groups = HZ3_DTOR_STAT_LOAD(g_s128_dispatched_groups_total);
    uint32_t disp_entries = HZ3_DTOR_STAT_LOAD(g_s128_dispatched_entries_total);
    uint32_t defer_groups = HZ3_DTOR_STAT_LOAD(g_s128_deferred_groups_total);
    uint32_t force_age_groups = HZ3_DTOR_STAT_LOAD(g_s128_forced_age_groups_total);
    uint32_t force_maxkeys = HZ3_DTOR_STAT_LOAD(g_s128_forced_maxkeys_flushes);
    uint32_t pend_keys_max = HZ3_DTOR_STAT_LOAD(g_s128_pending_keys_max);
    uint32_t pend_objs_max = HZ3_DTOR_STAT_LOAD(g_s128_pending_objs_max);

    fprintf(stderr,
            "[HZ3_S128_REMOTE] min_run=%u max_age=%u max_keys=%u calls=%u ring_drained=%u "
            "dispatched_groups=%u dispatched_entries=%u deferred_groups=%u forced_age_groups=%u "
            "forced_maxkeys_flushes=%u pending_keys_max=%u pending_objs_max=%u\n",
            (unsigned)HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN,
            (unsigned)HZ3_S128_REMOTE_STASH_DEFER_MAX_AGE,
            (unsigned)HZ3_S128_REMOTE_STASH_DEFER_MAX_KEYS,
            calls, ring_drained, disp_groups, disp_entries, defer_groups,
            force_age_groups, force_maxkeys, pend_keys_max, pend_objs_max);
}

#define S128_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s128, hz3_s128_atexit_dump)
#define S128_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S128_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S128_STAT_MAX(name, val) hz3_stat_update_max_u32(&(name), (uint32_t)(val))
#else
#define S128_STAT_REGISTER() ((void)0)
#define S128_STAT_INC(name) ((void)0)
#define S128_STAT_ADD(name, val) ((void)0)
#define S128_STAT_MAX(name, val) ((void)(val))
#endif  // HZ3_S128_STATS

// =============================================================================
// S128: RemoteStash Defer-MinRun table (TLS)
// =============================================================================
#if HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN
typedef struct {
    uint16_t key;  // (dst<<8)|bin
    uint8_t dst;
    uint8_t bin;
    uint8_t tail_null_set;
    uint8_t age;
    void* head;
    void* tail;
    uint32_t n;
} Hz3S128Bucket;

typedef struct {
    uint16_t map[256];
    Hz3S128Bucket buckets[256];
    uint32_t nb;
    uint32_t pending_objs;
    uint8_t inited;
} Hz3S128Pending;

static __thread Hz3S128Pending t_hz3_s128_pending;

static inline void hz3_s128_pending_reset(Hz3S128Pending* p) {
    p->nb = 0;
    p->pending_objs = 0;
    memset(p->map, 0xFF, sizeof(p->map));
    p->inited = 1;
}

static inline void hz3_s128_pending_init_if_needed(Hz3S128Pending* p) {
    if (!p->inited) {
        hz3_s128_pending_reset(p);
    }
}
#endif  // HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN

// =============================================================================
// S128: flush_budget_impl() branch (reference snapshot)
// =============================================================================
#if HZ3_S97_REMOTE_STASH_BUCKET
#if HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN
    // S128: Defer small (dst,bin) groups across flush calls.
    // Dispatch only when n>=MIN_RUN, or age>=MAX_AGE forces.
    Hz3S128Pending* pend = &t_hz3_s128_pending;
    hz3_s128_pending_init_if_needed(pend);

    // Age pending buckets (bounded by u8).
    for (uint32_t i = 0; i < pend->nb; i++) {
        Hz3S128Bucket* b = &pend->buckets[i];
        if (b->n && b->age < 255u) {
            b->age++;
        }
    }

    enum { S128_HASH_SIZE = 256 };  // must be power-of-two
    enum { S128_HASH_MASK = S128_HASH_SIZE - 1 };

    uint32_t ring_drained = 0;
    while (t != h && ring_drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        ring_drained++;
        drained++;

        uint16_t key = (uint16_t)(((uint16_t)dst << 8) | (uint16_t)bin);
        uint32_t idx = ((uint32_t)key * 2654435761u) & (uint32_t)S128_HASH_MASK;

        for (uint32_t probe = 0; probe < (uint32_t)S128_HASH_SIZE; probe++) {
            uint16_t mi = pend->map[idx];
            if (mi == 0xFFFFu) {
                if (pend->nb >= (uint32_t)HZ3_S128_REMOTE_STASH_DEFER_MAX_KEYS) {
                    // Too many distinct keys accumulated: force dispatch all pending and reset.
                    for (uint32_t j = 0; j < pend->nb; j++) {
                        Hz3S128Bucket* fb = &pend->buckets[j];
                        if (fb->n == 0) continue;
                        if (fb->n > 1) {
                            if (!fb->tail_null_set) {
                                hz3_obj_set_next(fb->tail, NULL);
                                fb->tail_null_set = 1;
                            }
                        } else {
                            if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) {
                                hz3_obj_set_next(fb->tail, NULL);
                            }
                        }
                        S97_LOC_GROUPS_INC();
                        S97_LOC_NMAX(fb->n);
                        S97_LOC_NGT1(fb->n);
                        S97_LOC_SEEN(fb->dst, fb->bin);
                        S97_LOC_CAT(fb->bin);
                        S97_LOC_SELFDST(fb->dst);
                        hz3_remote_stash_dispatch_list(fb->dst, (uint32_t)fb->bin, fb->head, fb->tail, fb->n);
                        pend->pending_objs -= fb->n;
                        fb->head = NULL;
                        fb->tail = NULL;
                        fb->n = 0;
                        fb->tail_null_set = 0;
                        fb->age = 0;
                    }
                    S128_STAT_INC(g_s128_forced_maxkeys_flushes);
                    hz3_s128_pending_reset(pend);
                }

                pend->map[idx] = (uint16_t)pend->nb;
                Hz3S128Bucket* b = &pend->buckets[pend->nb++];
                b->key = key;
                b->dst = dst;
                b->bin = bin;
                b->tail_null_set = 0;
                b->age = 0;
                b->head = ptr;
                b->tail = ptr;
                b->n = 1;
                pend->pending_objs++;
                break;
            }

            Hz3S128Bucket* b = &pend->buckets[mi];
            if (b->key == key) {
                if (b->n == 0) {
                    b->dst = dst;
                    b->bin = bin;
                    b->tail_null_set = 0;
                    b->age = 0;
                    b->head = ptr;
                    b->tail = ptr;
                    b->n = 1;
                    pend->pending_objs++;
                    break;
                }

                if (!b->tail_null_set) {
                    hz3_obj_set_next(b->tail, NULL);
                    b->tail_null_set = 1;
                }
                hz3_obj_set_next(ptr, b->head);
                b->head = ptr;
                b->n++;
                b->age = 0;
                pend->pending_objs++;
                break;
            }

            idx = (idx + 1u) & (uint32_t)S128_HASH_MASK;
        }
    }

    S128_STAT_MAX(g_s128_pending_keys_max, pend->nb);
    S128_STAT_MAX(g_s128_pending_objs_max, pend->pending_objs);

    uint32_t dispatched_entries = 0;
    uint32_t dispatched_groups = 0;
    uint32_t deferred_groups = 0;
    uint32_t forced_age_groups = 0;

    for (uint32_t i = 0; i < pend->nb; i++) {
        Hz3S128Bucket* b = &pend->buckets[i];
        uint32_t n = b->n;
        if (n == 0) continue;

        if (n < (uint32_t)HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN &&
            b->age < (uint8_t)HZ3_S128_REMOTE_STASH_DEFER_MAX_AGE) {
            deferred_groups++;
            continue;
        }

        if (n < (uint32_t)HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN) {
            forced_age_groups++;
        }

        if (n > 1) {
            if (!b->tail_null_set) {
                hz3_obj_set_next(b->tail, NULL);
                b->tail_null_set = 1;
            }
        } else {
            if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) {
                hz3_obj_set_next(b->tail, NULL);
            }
        }

        S97_LOC_GROUPS_INC();
        S97_LOC_NMAX(n);
        S97_LOC_NGT1(n);
        S97_LOC_SEEN(b->dst, b->bin);
        S97_LOC_CAT(b->bin);
        S97_LOC_SELFDST(b->dst);
        hz3_remote_stash_dispatch_list(b->dst, (uint32_t)b->bin, b->head, b->tail, n);

        dispatched_entries += n;
        dispatched_groups++;
        pend->pending_objs -= n;

        b->head = NULL;
        b->tail = NULL;
        b->n = 0;
        b->tail_null_set = 0;
        b->age = 0;
    }

    // If no pending objs remain, reset table to avoid key accumulation across bursts.
    if (pend->pending_objs == 0) {
        hz3_s128_pending_reset(pend);
    }

    ring->tail = t;
    // Withholding is allowed, but must stay discoverable: keep remote_hint up while pending exists.
    t_hz3_cache.remote_hint = (ring->tail != ring->head) || (pend->pending_objs != 0);

    S128_STAT_ADD(g_s128_ring_drained_total, ring_drained);
    S128_STAT_ADD(g_s128_dispatched_groups_total, dispatched_groups);
    S128_STAT_ADD(g_s128_dispatched_entries_total, dispatched_entries);
    S128_STAT_ADD(g_s128_deferred_groups_total, deferred_groups);
    S128_STAT_ADD(g_s128_forced_age_groups_total, forced_age_groups);

    return;
#endif
#endif

