// hz3_small_v2_lane16.inc - Lane16R90 page-remote path (research lane)
// Part of hz3_small_v2.c (single TU split)

#if HZ3_LANE_T16_R90_PAGE_REMOTE

#define HZ3_T16_QSTATE_IDLE 0u
#define HZ3_T16_QSTATE_QUEUED 1u
#define HZ3_T16_QSTATE_PROC 2u

#if HZ3_LANE16_FAILFAST
static inline void hz3_lane16_failfast(const char* where, uint8_t owner, int sc,
                                       void* cur, void* page_base,
                                       size_t cap, size_t seen) {
    fprintf(stderr,
            "[HZ3_LANE16_FAILFAST] where=%s owner=%u sc=%d cur=%p page=%p seen=%zu cap=%zu\n",
            where, owner, sc, cur, page_base, seen, cap);
    abort();
}
#endif

#if HZ3_LANE16_REMOTE_LOCK
static inline void hz3_lane16_lock(_Atomic(uint8_t)* lock) {
    while (atomic_exchange_explicit(lock, 1, memory_order_acquire) != 0) {
        // spin
    }
}

static inline void hz3_lane16_unlock(_Atomic(uint8_t)* lock) {
    atomic_store_explicit(lock, 0, memory_order_release);
}
#endif

#if HZ3_LANE16_DUP_FAILFAST || HZ3_LANE16_DUP_GUARD
static inline void hz3_lane16_dup_fail(const char* where, Hz3SmallV2PageHdr* ph,
                                       void* obj, int sc, size_t obj_size,
                                       size_t cap, size_t idx, uintptr_t off) {
    fprintf(stderr,
            "[HZ3_LANE16_DUP_FAILFAST] where=%s page=%p obj=%p sc=%d obj_size=%zu cap=%zu idx=%zu off=%zu\n",
            (where ? where : "?"), (void*)ph, obj, sc, obj_size, cap, idx, (size_t)off);
    abort();
}

static inline size_t hz3_lane16_page_start_aligned(void) {
    uintptr_t start = (uintptr_t)HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);
    return (size_t)start;
}

static inline size_t hz3_lane16_obj_index(Hz3SmallV2PageHdr* ph, void* obj, int sc,
                                          size_t* cap_out, size_t* obj_size_out, uintptr_t* off_out) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    size_t cap = hz3_small_v2_page_capacity(sc);
    size_t start = hz3_lane16_page_start_aligned();
    uintptr_t off = (uintptr_t)obj - (uintptr_t)ph - (uintptr_t)start;
    if (off_out) {
        *off_out = off;
    }
    if (obj_size_out) {
        *obj_size_out = obj_size;
    }
    if (cap_out) {
        *cap_out = cap;
    }
    if (obj_size == 0 || cap == 0) {
        return (size_t)-1;
    }
    if ((off % obj_size) != 0) {
        return (size_t)-1;
    }
    return (size_t)(off / obj_size);
}

// Returns 1 if the caller should push, 0 if duplicate should be skipped.
static inline int hz3_lane16_dup_mark(Hz3SmallV2PageHdr* ph, void* obj, int sc) {
    if (ph->sc != (uint8_t)sc) {
        hz3_lane16_dup_fail("mark:sc_mismatch", ph, obj, sc, 0, 0, 0, 0);
    }
    size_t cap = 0;
    size_t obj_size = 0;
    uintptr_t off = 0;
    size_t idx = hz3_lane16_obj_index(ph, obj, sc, &cap, &obj_size, &off);
    if (idx == (size_t)-1 || idx >= cap) {
        hz3_lane16_dup_fail("mark:idx_oob", ph, obj, sc, obj_size, cap, idx, off);
    }
    uint32_t word = (uint32_t)(idx >> 6);
    uint64_t mask = 1ULL << (idx & 63u);
    uint64_t old = atomic_fetch_or_explicit(&ph->lane16_dup_bits[word], mask, memory_order_acq_rel);
    if (old & mask) {
        if (HZ3_LANE16_DUP_FAILFAST) {
            hz3_lane16_dup_fail("mark:dup", ph, obj, sc, obj_size, cap, idx, off);
        }
        return 0;  // duplicate: skip push
    }
    return 1;
}

static inline void hz3_lane16_dup_clear(Hz3SmallV2PageHdr* ph, void* obj, int sc) {
    if (ph->sc != (uint8_t)sc) {
        hz3_lane16_dup_fail("clear:sc_mismatch", ph, obj, sc, 0, 0, 0, 0);
    }
    size_t cap = 0;
    size_t obj_size = 0;
    uintptr_t off = 0;
    size_t idx = hz3_lane16_obj_index(ph, obj, sc, &cap, &obj_size, &off);
    if (idx == (size_t)-1 || idx >= cap) {
        hz3_lane16_dup_fail("clear:idx_oob", ph, obj, sc, obj_size, cap, idx, off);
    }
    uint32_t word = (uint32_t)(idx >> 6);
    uint64_t mask = 1ULL << (idx & 63u);
    uint64_t old = atomic_fetch_and_explicit(&ph->lane16_dup_bits[word], ~mask, memory_order_acq_rel);
    if ((old & mask) == 0) {
        if (HZ3_LANE16_DUP_FAILFAST) {
            hz3_lane16_dup_fail("clear:not_set", ph, obj, sc, obj_size, cap, idx, off);
        }
        return;  // guard mode: tolerate double-clear
    }
}
#else
#define hz3_lane16_dup_mark(ph, obj, sc) ((void)(ph), (void)(obj), (void)(sc), 1)
#define hz3_lane16_dup_clear(ph, obj, sc) ((void)(ph), (void)(obj), (void)(sc))
#endif

static inline void* hz3_lane16_page_base_from_obj(void* obj) {
    return (void*)((uintptr_t)obj & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
}

static inline Hz3SegHdr* hz3_lane16_seg_hdr_from_page(void* page_base) {
    return (Hz3SegHdr*)((uintptr_t)page_base & ~((uintptr_t)HZ3_SEG_SIZE - 1u));
}

static inline uint32_t hz3_lane16_page_idx(void* page_base) {
    uintptr_t seg_base = (uintptr_t)page_base & ~((uintptr_t)HZ3_SEG_SIZE - 1u);
    return (uint32_t)(((uintptr_t)page_base - seg_base) >> HZ3_PAGE_SHIFT);
}

static inline uint32_t hz3_lane16_select_shard(uintptr_t page_base) {
    uint32_t page_idx = (uint32_t)((page_base >> HZ3_PAGE_SHIFT) & 0xFFFFu);
    return (page_idx + (uint32_t)t_hz3_cache.my_shard) & (HZ3_PAGE_REMOTE_SHARDS - 1u);
}

static inline void hz3_lane16_remote_pending_set(Hz3SegHdr* hdr, uint32_t page_idx) {
    uint32_t word = page_idx >> 6;
    uint64_t mask = 1ULL << (page_idx & 63u);
    atomic_fetch_or_explicit(&hdr->t16_remote_pending[word], mask, memory_order_release);
    uint8_t expect = HZ3_T16_QSTATE_IDLE;
    if (atomic_compare_exchange_strong_explicit(&hdr->t16_qstate, &expect, HZ3_T16_QSTATE_QUEUED,
                                                memory_order_acq_rel, memory_order_acquire)) {
        uint8_t owner = hdr->owner;
        if (owner < HZ3_NUM_SHARDS) {
            Hz3SegHdr* old = atomic_load_explicit(&g_hz3_lane16_segq[owner], memory_order_acquire);
            do {
                hdr->t16_qnext = old;
            } while (!atomic_compare_exchange_weak_explicit(&g_hz3_lane16_segq[owner], &old, hdr,
                                                            memory_order_release, memory_order_acquire));
        } else {
            atomic_store_explicit(&hdr->t16_qstate, HZ3_T16_QSTATE_IDLE, memory_order_release);
        }
    }
}

static inline int hz3_lane16_segment_has_pending(Hz3SegHdr* hdr) {
    for (uint32_t w = 0; w < HZ3_BITMAP_WORDS; w++) {
        if (atomic_load_explicit(&hdr->t16_remote_pending[w], memory_order_acquire) != 0) {
            return 1;
        }
    }
    return 0;
}

static inline void hz3_lane16_segq_push_list(uint8_t owner, Hz3SegHdr* head, Hz3SegHdr* tail) {
    Hz3SegHdr* old = atomic_load_explicit(&g_hz3_lane16_segq[owner], memory_order_acquire);
    do {
        tail->t16_qnext = old;
    } while (!atomic_compare_exchange_weak_explicit(&g_hz3_lane16_segq[owner], &old, head,
                                                    memory_order_release, memory_order_acquire));
}

static inline void hz3_lane16_segq_finish(uint8_t owner, Hz3SegHdr* hdr) {
    if (hz3_lane16_segment_has_pending(hdr)) {
        atomic_store_explicit(&hdr->t16_qstate, HZ3_T16_QSTATE_QUEUED, memory_order_release);
        hz3_lane16_segq_push_list(owner, hdr, hdr);
    } else {
        atomic_store_explicit(&hdr->t16_qstate, HZ3_T16_QSTATE_IDLE, memory_order_release);
    }
}

static inline void hz3_lane16_page_push_list(Hz3SmallV2PageHdr* ph,
                                             void* head, void* tail, uint32_t n) {
    (void)n;
    uint32_t shard = hz3_lane16_select_shard((uintptr_t)ph);
    _Atomic(void*)* slot = &ph->remote_head[shard];
#if HZ3_LANE16_REMOTE_LOCK
    hz3_lane16_lock(&ph->remote_lock[shard]);
    void* old_head = atomic_load_explicit(slot, memory_order_relaxed);
    hz3_obj_set_next(tail, old_head);
    atomic_store_explicit(slot, head, memory_order_release);
    hz3_lane16_unlock(&ph->remote_lock[shard]);
#else
    void* old_head = atomic_load_explicit(slot, memory_order_acquire);
    hz3_obj_set_next(tail, old_head);  // Set ONCE before CAS loop
    for (;;) {
        if (atomic_compare_exchange_strong_explicit(
                slot, &old_head, head, memory_order_release, memory_order_acquire)) {
            break;
        }
        // CAS failed: old_head updated, re-link tail
        hz3_obj_set_next(tail, old_head);
    }
#endif

    Hz3SegHdr* hdr = hz3_lane16_seg_hdr_from_page(ph);
    hz3_lane16_remote_pending_set(hdr, hz3_lane16_page_idx(ph));
}

static inline void hz3_lane16_push_one(Hz3SmallV2PageHdr* ph, void* obj) {
    if (!hz3_lane16_dup_mark(ph, obj, (int)ph->sc)) {
        return;
    }
    hz3_obj_set_next(obj, NULL);
    hz3_lane16_page_push_list(ph, obj, obj, 1);
}

void hz3_lane16_push_remote_list_small(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    (void)owner;
    (void)tail;
    if (!head || n == 0) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }
    if (n == 1) {
        Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)hz3_lane16_page_base_from_obj(head);
        hz3_lane16_push_one(ph, head);
        return;
    }
    if (n <= 4) {
        void* cur = head;
        for (uint32_t i = 0; i < n && cur; i++) {
            void* next = (i + 1 < n) ? hz3_obj_get_next(cur) : NULL;
            hz3_obj_set_next(cur, NULL);
            Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)hz3_lane16_page_base_from_obj(cur);
            hz3_lane16_push_one(ph, cur);
            cur = next;
        }
        return;
    }

    typedef struct {
        uintptr_t page_base;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3Lane16Bucket;

    Hz3Lane16Bucket buckets[HZ3_T16_REMOTE_BUCKET];
    for (uint32_t i = 0; i < HZ3_T16_REMOTE_BUCKET; i++) {
        buckets[i].page_base = 0;
        buckets[i].head = NULL;
        buckets[i].tail = NULL;
        buckets[i].n = 0;
    }

    void* cur = head;
    for (uint32_t i = 0; i < n && cur; i++) {
        void* next = (i + 1 < n) ? hz3_obj_get_next(cur) : NULL;
        hz3_obj_set_next(cur, NULL);
        Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)hz3_lane16_page_base_from_obj(cur);
        uintptr_t page_base = (uintptr_t)ph;
        uint32_t idx = (uint32_t)((page_base >> HZ3_PAGE_SHIFT) & (HZ3_T16_REMOTE_BUCKET - 1u));
        Hz3Lane16Bucket* b = &buckets[idx];

        if (b->page_base == 0) {
            if (!hz3_lane16_dup_mark(ph, cur, sc)) {
                cur = next;
                continue;
            }
            b->page_base = page_base;
            b->head = cur;
            b->tail = cur;
            b->n = 1;
        } else if (b->page_base == page_base) {
            if (!hz3_lane16_dup_mark(ph, cur, sc)) {
                cur = next;
                continue;
            }
            hz3_obj_set_next(b->tail, cur);
            b->tail = cur;
            b->n++;
        } else {
            hz3_lane16_push_one(ph, cur);
        }
        cur = next;
    }

    for (uint32_t i = 0; i < HZ3_T16_REMOTE_BUCKET; i++) {
        Hz3Lane16Bucket* b = &buckets[i];
        if (b->page_base != 0 && b->head) {
            hz3_lane16_page_push_list((Hz3SmallV2PageHdr*)b->page_base, b->head, b->tail, b->n);
        }
    }
}

static inline void hz3_lane16_push_remainder(uint8_t owner, int sc,
                                             void* head, void* tail, uint32_t n) {
#if HZ3_S44_OWNER_STASH_PUSH && !HZ3_S44_OWNER_STASH_DISABLE
    if (hz3_owner_stash_push_list(owner, sc, head, tail, n)) {
        return;
    }
#endif
    hz3_small_v2_central_init();
    hz3_small_v2_central_push_list(owner, sc, head, tail, n);
}

static inline void hz3_lane16_consume_list(uint8_t owner, int sc,
                                           void* list, void** out,
                                           uint32_t want, uint32_t* got) {
    if (!list) {
        return;
    }

#if HZ3_LANE16_FAILFAST
    void* page_base = hz3_lane16_page_base_from_obj(list);
    size_t cap = hz3_small_v2_page_capacity(sc);
    size_t seen = 0;
#endif

    void* cur = list;
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)hz3_lane16_page_base_from_obj(list);
    while (cur && *got < want) {
#if HZ3_LANE16_FAILFAST
        if (hz3_lane16_page_base_from_obj(cur) != page_base || (cap && ++seen > cap)) {
            hz3_lane16_failfast("consume:walk", owner, sc, cur, page_base, cap, seen);
        }
#endif
        hz3_lane16_dup_clear(ph, cur, sc);
        out[(*got)++] = cur;
        cur = hz3_obj_get_next(cur);
    }

    if (cur) {
#if HZ3_LANE16_FAILFAST
        if (hz3_lane16_page_base_from_obj(cur) != page_base || (cap && ++seen > cap)) {
            hz3_lane16_failfast("consume:remainder_head", owner, sc, cur, page_base, cap, seen);
        }
#endif
        hz3_lane16_dup_clear(ph, cur, sc);
        void* rem_head = cur;
        void* rem_tail = cur;
        uint32_t rem_n = 1;
        cur = hz3_obj_get_next(cur);
        while (cur) {
#if HZ3_LANE16_FAILFAST
            if (hz3_lane16_page_base_from_obj(cur) != page_base || (cap && ++seen > cap)) {
                hz3_lane16_failfast("consume:remainder_walk", owner, sc, cur, page_base, cap, seen);
            }
#endif
            hz3_lane16_dup_clear(ph, cur, sc);
            rem_tail = cur;
            rem_n++;
            cur = hz3_obj_get_next(cur);
        }
        hz3_lane16_push_remainder(owner, sc, rem_head, rem_tail, rem_n);
    }
}

static inline void hz3_lane16_drain_page(uint8_t owner, int sc,
                                         Hz3SmallV2PageHdr* ph,
                                         void** out, uint32_t want, uint32_t* got) {
    if (ph->sc != (uint8_t)sc) {
        return;
    }
    for (uint32_t s = 0; s < HZ3_PAGE_REMOTE_SHARDS; s++) {
#if HZ3_LANE16_REMOTE_LOCK
        hz3_lane16_lock(&ph->remote_lock[s]);
        void* list = atomic_exchange_explicit(&ph->remote_head[s], NULL, memory_order_acquire);
        hz3_lane16_unlock(&ph->remote_lock[s]);
#else
        void* list = atomic_exchange_explicit(&ph->remote_head[s], NULL, memory_order_acquire);
#endif
        if (list) {
            hz3_lane16_consume_list(owner, sc, list, out, want, got);
            if (*got >= want) {
                return;
            }
        }
    }
}

static inline uint32_t hz3_lane16_drain_segment(uint8_t owner, int sc,
                                                Hz3SegHdr* hdr,
                                                void** out, uint32_t want) {
    uint32_t got = 0;
    for (uint32_t w = 0; w < HZ3_BITMAP_WORDS && got < want; w++) {
        uint64_t pending = atomic_load_explicit(&hdr->t16_remote_pending[w], memory_order_acquire);
        while (pending && got < want) {
            uint32_t bit = (uint32_t)__builtin_ctzll(pending);
            uint64_t mask = 1ULL << bit;
            uint32_t page_idx = (w << 6) | bit;
            void* page_base = (void*)((uintptr_t)hdr + ((uintptr_t)page_idx << HZ3_PAGE_SHIFT));
            Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;

            if (ph->magic != HZ3_PAGE_MAGIC || ph->sc != (uint8_t)sc) {
                pending &= ~mask;
                continue;
            }
            uint64_t old = atomic_fetch_and_explicit(&hdr->t16_remote_pending[w], ~mask,
                                                     memory_order_acq_rel);
            if ((old & mask) == 0) {
                pending &= ~mask;
                continue;
            }
            hz3_lane16_drain_page(owner, sc, ph, out, want, &got);
            pending &= ~mask;
        }
    }
    hz3_lane16_segq_finish(owner, hdr);
    return got;
}

uint32_t hz3_lane16_pop_batch_small(uint8_t owner, int sc, void** out, uint32_t want) {
    if (want == 0 || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    if (!t_hz3_cache.initialized) {
        return 0;
    }

    uint32_t got = 0;
    Hz3SegHdr* qlist = atomic_exchange_explicit(&g_hz3_lane16_segq[owner], NULL, memory_order_acq_rel);
    if (qlist) {
        Hz3SegHdr* tail = qlist;
        while (tail->t16_qnext) {
            tail = tail->t16_qnext;
        }
        Hz3SegHdr* cur = qlist;
        while (cur) {
            Hz3SegHdr* next = cur->t16_qnext;
            cur->t16_qnext = NULL;
            if (got < want) {
                atomic_store_explicit(&cur->t16_qstate, HZ3_T16_QSTATE_PROC, memory_order_release);
                got += hz3_lane16_drain_segment(owner, sc, cur, out + got, want - got);
            } else {
                hz3_lane16_segq_push_list(owner, cur, tail);
                break;
            }
            cur = next;
        }
    }
    if (got >= want) {
        return got;
    }

    uint16_t seg_count = t_hz3_cache.t16_small_seg_count;
    if (seg_count == 0) {
        return got;
    }

    uint16_t cursor = t_hz3_cache.t16_small_seg_cursor;
    uint16_t scanned = 0;
    uint16_t budget = (uint16_t)HZ3_T16_COLLECT_SEG_BUDGET;
    if (budget > 0) {
        if (want <= 16) {
            budget = 1;
        } else if (want <= 32) {
            budget = (budget > 2 ? 2 : budget);
        } else if (want <= 64) {
            budget = (budget > 3 ? 3 : budget);
        }
        if (budget == 0) {
            budget = 1;
        }
    }

    void* arena_base = hz3_arena_get_base();
    if (!arena_base) {
        return got;
    }

    while (got < want && scanned < seg_count && budget > 0) {
        uint16_t slot = (uint16_t)(cursor % seg_count);
        uint32_t seg_idx = t_hz3_cache.t16_small_seg_ids[slot];
        cursor++;
        scanned++;
        budget--;

        if (!hz3_arena_slot_used(seg_idx)) {
            continue;
        }

        void* seg_base = (void*)((uintptr_t)arena_base + ((uintptr_t)seg_idx << HZ3_SEG_SHIFT));
        Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
        if (hdr->magic != HZ3_SEG_HDR_MAGIC || hdr->kind != HZ3_SEG_KIND_SMALL) {
            continue;
        }
        uint8_t expect = HZ3_T16_QSTATE_IDLE;
        if (!atomic_compare_exchange_strong_explicit(&hdr->t16_qstate, &expect, HZ3_T16_QSTATE_PROC,
                                                     memory_order_acq_rel, memory_order_acquire)) {
            continue;
        }
        got += hz3_lane16_drain_segment(owner, sc, hdr, out + got, want - got);
    }

    t_hz3_cache.t16_small_seg_cursor = cursor;
    return got;
}

#endif  // HZ3_LANE_T16_R90_PAGE_REMOTE
