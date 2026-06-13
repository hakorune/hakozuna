void hz3_central_push(int shard, int sc, void* run) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!run) return;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Capture boundary return address at non-inlined function entry.
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

#if HZ3_S90_CENTRAL_GUARD
    {
        uint32_t page_idx = 0;
        if (hz3_arena_page_index_fast(run, &page_idx) && g_hz3_page_tag32) {
            uint32_t tag32 = hz3_pagetag32_load(page_idx);
            uint32_t have_bin = (tag32 != 0) ? hz3_pagetag32_bin(tag32) : 0;
            uint8_t have_dst = (tag32 != 0) ? hz3_pagetag32_dst(tag32) : 0;
            uint32_t want_bin = (uint32_t)(HZ3_MEDIUM_BIN_BASE + (uint32_t)sc);
            uint8_t want_dst = (uint8_t)shard;
            if (tag32 == 0 || have_bin != want_bin || have_dst != want_dst) {
                static _Atomic int g_s90_shot = 0;
                if (!HZ3_S90_CENTRAL_GUARD_SHOT ||
                    atomic_exchange_explicit(&g_s90_shot, 1, memory_order_acq_rel) == 0) {
                    // Slow path: compute base/off only on failure.
                    uintptr_t so_base = 0;
                    {
                        FILE* f = fopen("/proc/self/maps", "r");
                        if (f) {
                            char line[512];
                            while (fgets(line, sizeof(line), f)) {
                                if (strstr(line, "libhakozuna_hz3") == NULL) continue;
                                char* dash = strchr(line, '-');
                                if (!dash) continue;
                                *dash = '\0';
                                so_base = (uintptr_t)strtoull(line, NULL, 16);
                                break;
                            }
                            fclose(f);
                        }
                    }
                    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
                    unsigned long off = (so_base && ra0) ? (unsigned long)((uintptr_t)ra0 - so_base) : 0;
                    fprintf(stderr,
                            "[HZ3_S90_CENTRAL_GUARD] where=push ptr=%p shard=%d sc=%d page_idx=%u "
                            "tag32=0x%08x have_bin=%u have_dst=%u want_bin=%u want_dst=%u "
                            "ra=%p base=%p off=0x%lx\n",
                            run, shard, sc, page_idx,
                            tag32, have_bin, (unsigned)have_dst, want_bin, (unsigned)want_dst,
                            ra0, (void*)so_base, off);
                }
                if (HZ3_S90_CENTRAL_GUARD_FAILFAST) {
                    abort();
                }
            }
        }
    }
#endif

    hz3_lock_acquire(&bin->lock);

    // Intrusive list: store next at run[0]
    hz3_obj_set_next(run, bin->head);
    bin->head = run;
    bin->count++;
    hz3_s65_central_note_push(sc, 1);

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Record (ptr,next) as stored in central after linking.
    hz3_central_shadow_record(run, hz3_obj_get_next(run), s86_ra);
#endif
    hz3_lock_release(&bin->lock);
}

#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
void hz3_central_aligned_push(int shard, int sc, void* run) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!run) return;

    Hz3CentralBin* bin = &g_hz3_central_aligned[shard][sc];
    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(run, bin->head);
    bin->head = run;
    bin->count++;
    hz3_lock_release(&bin->lock);
}

void hz3_central_aligned_push_list(int shard, int sc, void* head, void* tail, uint32_t n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

    Hz3CentralBin* bin = &g_hz3_central_aligned[shard][sc];
    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);
    bin->head = head;
    bin->count += n;
    hz3_lock_release(&bin->lock);
}

void* hz3_central_aligned_pop(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;

    Hz3CentralBin* bin = &g_hz3_central_aligned[shard][sc];
    hz3_lock_acquire(&bin->lock);
    void* run = bin->head;
    if (run) {
        bin->head = hz3_obj_get_next(run);
        bin->count--;
    }
    hz3_lock_release(&bin->lock);
    return run;
}

int hz3_central_aligned_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    if (!out || want <= 0) return 0;

    Hz3CentralBin* bin = &g_hz3_central_aligned[shard][sc];
    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* run = bin->head;
        bin->head = hz3_obj_get_next(run);
        bin->count--;
        out[got++] = run;
    }
    hz3_lock_release(&bin->lock);
    return got;
}

uint32_t hz3_central_aligned_count_snapshot(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3CentralBin* bin = &g_hz3_central_aligned[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
}
#endif

void* hz3_central_pop(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

    hz3_lock_acquire(&bin->lock);

    void* run = bin->head;
    if (run) {
        void* next = hz3_obj_get_next(run);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        hz3_central_shadow_verify_and_remove(run, next);
  #else
        hz3_central_shadow_remove(run);
  #endif
#endif
        bin->head = next;
        bin->count--;
        hz3_s65_central_note_pop(sc, 1);
    }

    hz3_lock_release(&bin->lock);

    return run;
}

// ============================================================================
// Day 5: Batch API
// ============================================================================

// Push a linked list to central (head→...→tail in forward order)
// tail->next will be set to old head (caller doesn't need to set it)
void hz3_central_push_list(int shard, int sc, void* head, void* tail, uint32_t n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Capture return address from push boundary (caller of push_list).
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

#if HZ3_S90_CENTRAL_GUARD
    {
        uint32_t want_bin = (uint32_t)(HZ3_MEDIUM_BIN_BASE + (uint32_t)sc);
        uint8_t want_dst = (uint8_t)shard;
        void* cur = head;
        for (uint32_t i = 0; i < n && cur; i++) {
            uint32_t page_idx = 0;
            if (hz3_arena_page_index_fast(cur, &page_idx) && g_hz3_page_tag32) {
                uint32_t tag32 = hz3_pagetag32_load(page_idx);
                uint32_t have_bin = (tag32 != 0) ? hz3_pagetag32_bin(tag32) : 0;
                uint8_t have_dst = (tag32 != 0) ? hz3_pagetag32_dst(tag32) : 0;
                if (tag32 == 0 || have_bin != want_bin || have_dst != want_dst) {
                    static _Atomic int g_s90_list_shot = 0;
                    if (!HZ3_S90_CENTRAL_GUARD_SHOT ||
                        atomic_exchange_explicit(&g_s90_list_shot, 1, memory_order_acq_rel) == 0) {
                        uintptr_t so_base = 0;
                        {
                            FILE* f = fopen("/proc/self/maps", "r");
                            if (f) {
                                char line[512];
                                while (fgets(line, sizeof(line), f)) {
                                    if (strstr(line, "libhakozuna_hz3") == NULL) continue;
                                    char* dash = strchr(line, '-');
                                    if (!dash) continue;
                                    *dash = '\0';
                                    so_base = (uintptr_t)strtoull(line, NULL, 16);
                                    break;
                                }
                                fclose(f);
                            }
                        }
                        void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
                        unsigned long off = (so_base && ra0) ? (unsigned long)((uintptr_t)ra0 - so_base) : 0;
                        fprintf(stderr,
                                "[HZ3_S90_CENTRAL_GUARD] where=push_list ptr=%p shard=%d sc=%d page_idx=%u "
                                "tag32=0x%08x have_bin=%u have_dst=%u want_bin=%u want_dst=%u "
                                "ra=%p base=%p off=0x%lx\n",
                                cur, shard, sc, page_idx,
                                tag32, have_bin, (unsigned)have_dst, want_bin, (unsigned)want_dst,
                                ra0, (void*)so_base, off);
                    }
                    if (HZ3_S90_CENTRAL_GUARD_FAILFAST) {
                        abort();
                    }
                    break;
                }
            }
            if (cur == tail) break;
            cur = hz3_obj_get_next(cur);
        }
    }
#endif

#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        void* old_head;
        do {
            old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
            hz3_obj_set_next(tail, old_head);
        } while (!atomic_compare_exchange_weak_explicit(&fast->head, &old_head, head,
                                                         memory_order_release,
                                                         memory_order_acquire));
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
        {
            void* cur = head;
            while (cur) {
                void* next = hz3_obj_get_next(cur);
                hz3_central_shadow_record(cur, next, s86_ra);
                if (cur == tail) break;
                cur = next;
            }
        }
#endif
        HZ3_S222_STAT_INC(push_calls);
        HZ3_S222_STAT_ADD(push_objs, n);
        hz3_s65_central_note_push(sc, n);
        return;
    }
#endif

    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);  // tail->next = old head
    bin->head = head;
    bin->count += n;
    hz3_s65_central_note_push(sc, n);

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Record each node after linking (tail->next overwritten on concat).
    {
        void* cur = head;
        while (cur) {
            void* next = hz3_obj_get_next(cur);
            hz3_central_shadow_record(cur, next, s86_ra);
            if (cur == tail) break;
            cur = next;
        }
    }
#endif
    hz3_lock_release(&bin->lock);
}

// Pop up to 'want' objects into out array (returns actual count)
int hz3_central_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    if (!out || want <= 0) return 0;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        HZ3_S222_STAT_INC(pop_calls);
        int pre_got = 0;

#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
        if (hz3_s228_sc_ok(sc)) {
            int allow_local = 1;
#if HZ3_S228_ONLY_IF_LIVECOUNT_1
            if (hz3_shard_live_count((uint8_t)shard) > 1) {
                allow_local = 0;
            }
#endif
            if (!allow_local) {
#if HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1
                Hz3S228LocalRem* rem_flush = &t_s228_local_rem[sc];
                if (rem_flush->head) {
                    uint32_t rem_n = rem_flush->count;
                    hz3_s222_push_list_to_fast(fast, rem_flush->head, rem_flush->tail);
                    hz3_s65_local_rem_note_pop(sc, (uint64_t)rem_n);
                    hz3_s65_central_note_push(sc, (uint64_t)rem_n);
                    rem_flush->head = NULL;
                    rem_flush->tail = NULL;
                    rem_flush->count = 0;
                }
#endif
            } else {
                int local_got = hz3_s228_consume_local(sc, out, want);
                if (local_got >= want) {
                    HZ3_S222_STAT_INC(pop_hits);
                    HZ3_S222_STAT_ADD(pop_objs, local_got);
                    return local_got;
                }
                if (local_got > 0) {
                    pre_got = local_got;
                    out += local_got;
                    want -= local_got;
                }
            }
        }
#endif

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
        if (hz3_s234_sc_ok(sc)) {
            int part_got = hz3_s234_fast_pop_partial(fast, sc, out, want);
            if (part_got > 0) {
                HZ3_S222_STAT_INC(pop_hits);
                HZ3_S222_STAT_ADD(pop_objs, part_got + pre_got);
                return part_got + pre_got;
            }
            if (part_got == 0 && pre_got > 0) {
                HZ3_S222_STAT_INC(pop_hits);
                HZ3_S222_STAT_ADD(pop_objs, pre_got);
                return pre_got;
            }
            // part_got == -1 -> fallback to exchange path below.
        }
#endif

        void* list = atomic_exchange_explicit(&fast->head, NULL, memory_order_acq_rel);
        if (list) {
            int got = 0;
            void* cur = list;
            while (got < want && cur) {
                void* next = hz3_obj_get_next(cur);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
                hz3_central_shadow_verify_and_remove(cur, next);
  #else
                hz3_central_shadow_remove(cur);
  #endif
#endif
                out[got++] = cur;
                cur = next;
            }

            if (cur) {
                int keep_local = 0;
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
                if (hz3_s228_sc_ok(sc)) {
                    keep_local = 1;
#if HZ3_S228_ONLY_IF_LIVECOUNT_1
                    if (hz3_shard_live_count((uint8_t)shard) > 1) {
                        keep_local = 0;
                    }
#endif
                }
#endif
                if (keep_local) {
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
                    Hz3S228LocalRem* rem_keep = &t_s228_local_rem[sc];
                    void* rem_head = cur;
                    void* rem_tail = cur;
                    uint32_t rem_count = 1;
                    while (hz3_obj_get_next(rem_tail)) {
                        rem_tail = hz3_obj_get_next(rem_tail);
                        rem_count++;
                    }
                    if (rem_keep->head) {
                        hz3_obj_set_next(rem_tail, rem_keep->head);
                        rem_keep->head = rem_head;
                        if (!rem_keep->tail) {
                            rem_keep->tail = rem_tail;
                        }
                        rem_keep->count += rem_count;
                    } else {
                        rem_keep->head = rem_head;
                        rem_keep->tail = rem_tail;
                        rem_keep->count = rem_count;
                    }
                    hz3_s65_central_note_pop(sc, (uint64_t)got + (uint64_t)rem_count);
                    hz3_s65_local_rem_note_add(sc, (uint64_t)rem_count);
#endif
                } else {
                    // Re-publish remaining list back to fast head.
                    void* rem_head = cur;
                    void* rem_tail = cur;
                    while (hz3_obj_get_next(rem_tail)) {
                        rem_tail = hz3_obj_get_next(rem_tail);
                    }
                    hz3_s222_push_list_to_fast(fast, rem_head, rem_tail);
                    HZ3_S222_STAT_INC(pop_repush);
                    hz3_s65_central_note_pop(sc, (uint64_t)got);
                }
            } else {
                hz3_s65_central_note_pop(sc, (uint64_t)got);
            }

            HZ3_S222_STAT_INC(pop_hits);
            HZ3_S222_STAT_ADD(pop_objs, got + pre_got);
            return got + pre_got;
        }
        if (pre_got > 0) {
            HZ3_S222_STAT_INC(pop_hits);
            HZ3_S222_STAT_ADD(pop_objs, pre_got);
            return pre_got;
        }
    }
#endif

    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* cur = bin->head;
        void* next = hz3_obj_get_next(cur);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        hz3_central_shadow_verify_and_remove(cur, next);
  #else
        hz3_central_shadow_remove(cur);
  #endif
#endif
        out[got++] = cur;
        bin->head = next;
        bin->count--;
    }
    hz3_s65_central_note_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);

    return got;
}

int hz3_central_has_supply(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        return atomic_load_explicit(&fast->head, memory_order_acquire) != NULL;
    }
#endif
    // Locked central bin supply check is expensive; return optimistic hint.
    return 1;
}

uint32_t hz3_central_count_snapshot(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
}
