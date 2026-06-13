static inline int hz3_inbox_medium_sc_from_obj(void* obj, int* sc_out) {
#if HZ3_PTAG_DSTBIN_ENABLE
    uint32_t page_idx = 0;
    if (hz3_arena_page_index_fast(obj, &page_idx)) {
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 != 0) {
            uint32_t bin = hz3_pagetag32_bin(tag32);
            if (bin >= (uint32_t)HZ3_MEDIUM_BIN_BASE &&
                bin < (uint32_t)(HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                *sc_out = (int)(bin - (uint32_t)HZ3_MEDIUM_BIN_BASE);
                return 1;
            }
        }
    }
#endif

    Hz3SegMeta* meta = hz3_segmap_get(obj);
    if (!meta) {
        return 0;
    }
    uintptr_t seg_base = (uintptr_t)obj & ~((uintptr_t)HZ3_SEG_SIZE - 1u);
    uint32_t seg_page_idx = (uint32_t)(((uintptr_t)obj - seg_base) >> HZ3_PAGE_SHIFT);
    if (seg_page_idx >= (uint32_t)HZ3_PAGES_PER_SEG) {
        return 0;
    }
    uint16_t tag = meta->sc_tag[seg_page_idx];
    if (hz3_tag_kind(tag) != HZ3_TAG_KIND_SMALL) {
        return 0;
    }
    int sc = hz3_tag_sc(tag);
    if (sc < 0 || sc >= HZ3_NUM_SC) {
        return 0;
    }
    *sc_out = sc;
    return 1;
}

void* hz3_inbox_medium_mixed_drain(uint8_t shard, int sc, Hz3Bin* bin, uint32_t budget_objs) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin || budget_objs == 0) return NULL;

    Hz3InboxMixed* inbox = &g_hz3_inbox_medium_mixed[shard];
    void* first = NULL;
    uint32_t drained_any = 0;

    void* reroute_head[HZ3_NUM_SC];
    void* reroute_tail[HZ3_NUM_SC];
    uint32_t reroute_n[HZ3_NUM_SC];
    memset(reroute_head, 0, sizeof(reroute_head));
    memset(reroute_tail, 0, sizeof(reroute_tail));
    memset(reroute_n, 0, sizeof(reroute_n));

#if HZ3_BIN_LAZY_COUNT && HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
    uint16_t bin_len = hz3_bin_is_empty(bin) ? 0 : HZ3_BIN_HARD_CAP;
#elif HZ3_BIN_LAZY_COUNT
    uint16_t bin_len = hz3_bin_count_walk(bin, HZ3_BIN_HARD_CAP);
#endif

    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        void* head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        if (!head) {
            continue;
        }
        head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }
        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        void* obj = head;
        while (obj && budget_objs > 0) {
            void* next = hz3_obj_get_next(obj);
            hz3_obj_set_next(obj, NULL);
            budget_objs--;

            int obj_sc = sc;
            if (!hz3_inbox_medium_sc_from_obj(obj, &obj_sc)) {
#if HZ3_S206B_MIXED_DECODE_FAILFAST
                fprintf(stderr, "[HZ3_S206B_FAILFAST] mixed_decode_fail ptr=%p shard=%u sc=%d\n",
                        obj, (unsigned)shard, sc);
                abort();
#endif
                obj_sc = sc;
            }

            if (obj_sc == sc) {
                if (!first) {
                    first = obj;
                } else {
#if HZ3_BIN_LAZY_COUNT
                    if (bin_len < HZ3_BIN_HARD_CAP) {
                        hz3_bin_push(bin, obj);
                        bin_len++;
                    } else {
#else
                    if (hz3_bin_count_get(bin) < HZ3_BIN_HARD_CAP) {
                        hz3_bin_push(bin, obj);
                    } else {
#endif
                        if (!reroute_head[sc]) {
                            reroute_head[sc] = obj;
                        } else {
                            hz3_obj_set_next(reroute_tail[sc], obj);
                        }
                        reroute_tail[sc] = obj;
                        reroute_n[sc]++;
                    }
                }
            } else {
                if (!reroute_head[obj_sc]) {
                    reroute_head[obj_sc] = obj;
                } else {
                    hz3_obj_set_next(reroute_tail[obj_sc], obj);
                }
                reroute_tail[obj_sc] = obj;
                reroute_n[obj_sc]++;
            }
            drained_any++;
            obj = next;
        }

        if (obj) {
            void* rem_head = obj;
            void* rem_tail = obj;
            uint32_t rem_n = 1;
            while ((obj = hz3_obj_get_next(obj)) != NULL) {
                rem_tail = obj;
                rem_n++;
            }
            hz3_inbox_medium_mixed_push_list(shard, rem_head, rem_tail, rem_n);
            break;
        }
    }

    for (int s = 0; s < HZ3_NUM_SC; s++) {
        if (!reroute_head[s]) {
            continue;
        }
        if (s == sc) {
            hz3_central_push_list(shard, s, reroute_head[s], reroute_tail[s], reroute_n[s]);
        } else {
            hz3_inbox_push_list(shard, s, reroute_head[s], reroute_tail[s], reroute_n[s]);
        }
    }

    if (drained_any > 0) {
        atomic_fetch_add_explicit(&inbox->drain_seq, 1, memory_order_release);
    }
    return first;
}

uint32_t hz3_inbox_drain_to_central(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t total_n = 0;

    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        void* head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }

        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        void* tail = head;
        uint32_t n = 1;
        void* obj = hz3_obj_get_next(head);
        while (obj) {
            tail = obj;
            n++;
            obj = hz3_obj_get_next(obj);
        }

        hz3_central_push_list(shard, sc, head, tail, n);
        total_n += n;
    }

    hz3_s65_inbox_note_drain(sc, (uint64_t)total_n);
    return total_n;
}
