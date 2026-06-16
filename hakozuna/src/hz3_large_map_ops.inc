#if HZ3_S242_LARGE_DIRECT_SIDE_MAP
#define HZ3_S242_DIRECT_SET_COUNT (1u << HZ3_S242_DIRECT_SET_BITS)
#define HZ3_S242_DIRECT_SLOT_NONE UINT32_MAX
#define HZ3_S242_DIRECT_KEY_BUSY UINTPTR_MAX

typedef struct Hz3LargeDirectSlot {
    _Atomic uintptr_t key;
    _Atomic(Hz3LargeHdr*) hdr;
} Hz3LargeDirectSlot;

static Hz3LargeDirectSlot g_hz3_s242_direct[HZ3_S242_DIRECT_SET_COUNT][HZ3_S242_DIRECT_WAYS];

static _Atomic size_t g_hz3_s242_insert_try = 0;
static _Atomic size_t g_hz3_s242_insert_ok = 0;
static _Atomic size_t g_hz3_s242_insert_collision = 0;
static _Atomic size_t g_hz3_s242_lookup_try = 0;
static _Atomic size_t g_hz3_s242_lookup_hit = 0;
static _Atomic size_t g_hz3_s242_lookup_miss_empty = 0;
static _Atomic size_t g_hz3_s242_lookup_miss_key = 0;
static _Atomic size_t g_hz3_s242_lookup_miss_validate = 0;
static _Atomic size_t g_hz3_s242_remove_try = 0;
static _Atomic size_t g_hz3_s242_remove_ok = 0;
static _Atomic size_t g_hz3_s242_remove_miss = 0;
static _Atomic size_t g_hz3_s242_shadow_match = 0;
static _Atomic size_t g_hz3_s242_shadow_mismatch = 0;
static _Atomic size_t g_hz3_s242_shadow_direct_only = 0;
static _Atomic size_t g_hz3_s242_shadow_map_only = 0;
static _Atomic size_t g_hz3_s242_mapless_active = 0;
static _Atomic size_t g_hz3_s242_entries_current = 0;
static _Atomic size_t g_hz3_s242_entries_peak = 0;
static _Atomic size_t g_hz3_s242_take_try = 0;
static _Atomic size_t g_hz3_s242_take_skip_empty = 0;
static _Atomic size_t g_hz3_s242_take_ok = 0;
static _Atomic size_t g_hz3_s242_take_miss = 0;
static _Atomic size_t g_hz3_s242_take_validate_fail = 0;
static _Atomic size_t g_hz3_s242_insert_existing_same = 0;
static _Atomic size_t g_hz3_s242_insert_existing_mismatch = 0;
static _Atomic size_t g_hz3_s242_insert_existing_null_hdr = 0;
static _Atomic size_t g_hz3_s242_insert_empty_key_nonnull_hdr = 0;
static _Atomic size_t g_hz3_s242_take_busy = 0;
static _Atomic size_t g_hz3_s242_take_slot_mismatch = 0;
static _Atomic size_t g_hz3_s242_take_missing_mapless_flag = 0;
static _Atomic size_t g_hz3_s242_entry_underflow = 0;

static inline void hz3_s242_stats_inc(_Atomic size_t* p) {
#if HZ3_S242_DIRECT_STATS
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
#else
    (void)p;
#endif
}

static inline void hz3_s242_stats_update_peak(_Atomic size_t* peak, size_t value) {
#if HZ3_S242_DIRECT_STATS
    size_t old = atomic_load_explicit(peak, memory_order_relaxed);
    while (old < value &&
           !atomic_compare_exchange_weak_explicit(peak,
                                                  &old,
                                                  value,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
#else
    (void)peak;
    (void)value;
#endif
}

static inline void hz3_s242_direct_note_entry_add(void) {
#if HZ3_S242_DIRECT_ENTRY_COUNT
    size_t now = atomic_fetch_add_explicit(&g_hz3_s242_entries_current,
                                           1,
                                           memory_order_release) + 1;
    hz3_s242_stats_update_peak(&g_hz3_s242_entries_peak, now);
#endif
}

static inline void hz3_s242_direct_note_entry_remove(void) {
#if HZ3_S242_DIRECT_ENTRY_COUNT
    size_t old = atomic_load_explicit(&g_hz3_s242_entries_current,
                                      memory_order_acquire);
    while (old > 0) {
        if (atomic_compare_exchange_weak_explicit(&g_hz3_s242_entries_current,
                                                  &old,
                                                  old - 1,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            return;
        }
    }
    hz3_s242_stats_inc(&g_hz3_s242_entry_underflow);
#if HZ3_S242_DIRECT_FAILFAST
    fprintf(stderr, "[HZ3_S242_DIRECT_FAIL] where=entry-remove reason=underflow\n");
    abort();
#endif
#endif
}

static inline int hz3_s242_direct_has_entries(void) {
#if HZ3_S242_DIRECT_ENTRY_COUNT
    return atomic_load_explicit(&g_hz3_s242_entries_current,
                                memory_order_acquire) != 0;
#else
    return 1;
#endif
}

static void hz3_s242_stats_dump_final(void) {
#if HZ3_S242_DIRECT_STATS
    fprintf(stderr,
            "[HZ3_S242_DIRECT] insert_try=%zu insert_ok=%zu "
            "insert_collision=%zu lookup_try=%zu lookup_hit=%zu "
            "lookup_miss_empty=%zu lookup_miss_key=%zu "
            "lookup_miss_validate=%zu remove_try=%zu remove_ok=%zu "
            "remove_miss=%zu shadow_match=%zu shadow_mismatch=%zu "
            "shadow_direct_only=%zu shadow_map_only=%zu mapless_active=%zu "
            "entries_current=%zu entries_peak=%zu take_try=%zu "
            "take_skip_empty=%zu take_ok=%zu take_miss=%zu "
            "take_validate_fail=%zu insert_existing_same=%zu "
            "insert_existing_mismatch=%zu insert_existing_null_hdr=%zu "
            "insert_empty_key_nonnull_hdr=%zu take_busy=%zu "
            "take_slot_mismatch=%zu take_missing_mapless_flag=%zu "
            "entry_underflow=%zu\n",
            atomic_load_explicit(&g_hz3_s242_insert_try, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_ok, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_collision, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_lookup_try, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_lookup_hit, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_lookup_miss_empty, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_lookup_miss_key, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_lookup_miss_validate, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_remove_try, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_remove_ok, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_remove_miss, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_shadow_match, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_shadow_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_shadow_direct_only, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_shadow_map_only, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_mapless_active, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_entries_current, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_entries_peak, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_try, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_skip_empty, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_ok, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_miss, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_validate_fail, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_existing_same, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_existing_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_existing_null_hdr, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_insert_empty_key_nonnull_hdr, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_busy, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_slot_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_take_missing_mapless_flag, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s242_entry_underflow, memory_order_relaxed));
#endif
}

static inline void hz3_s242_stats_register_once(void) {
#if HZ3_S242_DIRECT_STATS
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s242_stats_dump_final);
    }
#endif
}

static inline void hz3_s242_direct_fail(const char* where,
                                        const char* reason,
                                        const void* ptr,
                                        const Hz3LargeHdr* hdr,
                                        uint32_t set,
                                        uint32_t way) {
#if HZ3_S242_DIRECT_VALIDATE
    fprintf(stderr,
            "[HZ3_S242_DIRECT_FAIL] where=%s reason=%s ptr=%p hdr=%p "
            "set=%u way=%u slot=%u hdr_slot=%u flags=0x%x state=%u "
            "user_ptr=%p map_base=%p map_size=%zu\n",
            where,
            reason,
            ptr,
            (const void*)hdr,
            set,
            way,
            set * (uint32_t)HZ3_S242_DIRECT_WAYS + way,
            hdr ? hdr->direct_slot : HZ3_S242_DIRECT_SLOT_NONE,
            hdr ? (unsigned)hdr->flags : 0u,
            hdr ? atomic_load_explicit(&hdr->state, memory_order_acquire) : 0u,
            hdr ? hdr->user_ptr : NULL,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0u);
#if HZ3_S242_DIRECT_FAILFAST
    abort();
#endif
#else
    (void)where;
    (void)reason;
    (void)ptr;
    (void)hdr;
    (void)set;
    (void)way;
#endif
}

static inline uint32_t hz3_s242_direct_hash(const void* ptr) {
    uint64_t v = (uint64_t)(uintptr_t)ptr;
    v >>= 4;
    v ^= v >> 33;
    v *= UINT64_C(0xff51afd7ed558ccd);
    v ^= v >> 33;
    v *= UINT64_C(0xc4ceb9fe1a85ec53);
    v ^= v >> 33;
    return (uint32_t)v & (HZ3_S242_DIRECT_SET_COUNT - 1u);
}

static inline uint32_t hz3_s242_direct_slot_id(uint32_t set, uint32_t way) {
    return set * (uint32_t)HZ3_S242_DIRECT_WAYS + way;
}

static inline int hz3_s242_direct_mapless_eligible(const Hz3LargeHdr* hdr) {
#if HZ3_S242_DIRECT_MAPLESS
    if (!hdr || hdr->map_size == 0) {
        return 0;
    }
    if (hdr->map_size <= (size_t)HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES) {
        return 1;
    }
#if HZ3_S291_HIGHBAND_OWNER_WINDOW
    if (hz3_s291_highband_req_eligible(hdr->req_size)) {
        return 1;
    }
#endif
    return 0;
#else
    (void)hdr;
    return 0;
#endif
}

static inline int hz3_s242_direct_insert(Hz3LargeHdr* hdr) {
    hz3_s242_stats_register_once();
    hz3_s242_stats_inc(&g_hz3_s242_insert_try);

    if (!hdr || !hdr->user_ptr) {
        hz3_s242_stats_inc(&g_hz3_s242_insert_collision);
        return 0;
    }

    uintptr_t key = (uintptr_t)hdr->user_ptr;
    uint32_t set = hz3_s242_direct_hash(hdr->user_ptr);
    for (uint32_t way = 0; way < (uint32_t)HZ3_S242_DIRECT_WAYS; way++) {
        Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
        uintptr_t old = atomic_load_explicit(&slot->key, memory_order_acquire);
        if (old == HZ3_S242_DIRECT_KEY_BUSY) {
            continue;
        }
        if (old == key) {
            Hz3LargeHdr* old_hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (old_hdr == hdr) {
                hz3_s242_stats_inc(&g_hz3_s242_insert_existing_same);
            } else if (old_hdr == NULL) {
                hz3_s242_stats_inc(&g_hz3_s242_insert_existing_null_hdr);
                hz3_s242_direct_fail("insert",
                                     "existing-key-null-hdr",
                                     hdr->user_ptr,
                                     hdr,
                                     set,
                                     way);
            } else {
                hz3_s242_stats_inc(&g_hz3_s242_insert_existing_mismatch);
                hz3_s242_direct_fail("insert",
                                     "existing-key-hdr-mismatch",
                                     hdr->user_ptr,
                                     old_hdr,
                                     set,
                                     way);
                return 0;
            }
            atomic_store_explicit(&slot->hdr, hdr, memory_order_release);
            hdr->direct_slot = hz3_s242_direct_slot_id(set, way);
            hz3_s242_stats_inc(&g_hz3_s242_insert_ok);
            return 1;
        }
        if (old == 0) {
            uintptr_t empty = 0;
            if (!atomic_compare_exchange_strong_explicit(&slot->key,
                                                         &empty,
                                                         HZ3_S242_DIRECT_KEY_BUSY,
                                                         memory_order_acq_rel,
                                                         memory_order_acquire)) {
                continue;
            }
            Hz3LargeHdr* old_hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (old_hdr != NULL) {
                hz3_s242_stats_inc(&g_hz3_s242_insert_empty_key_nonnull_hdr);
                hz3_s242_direct_fail("insert",
                                     "empty-key-nonnull-hdr",
                                     hdr->user_ptr,
                                     old_hdr,
                                     set,
                                     way);
                atomic_store_explicit(&slot->key, 0, memory_order_release);
                continue;
            }
            atomic_store_explicit(&slot->hdr, hdr, memory_order_release);
            hdr->direct_slot = hz3_s242_direct_slot_id(set, way);
            hz3_s242_direct_note_entry_add();
            hz3_s242_stats_inc(&g_hz3_s242_insert_ok);
            atomic_store_explicit(&slot->key, key, memory_order_release);
            return 1;
        }
    }

    hdr->direct_slot = HZ3_S242_DIRECT_SLOT_NONE;
    hz3_s242_stats_inc(&g_hz3_s242_insert_collision);
    return 0;
}

static inline Hz3LargeHdr* hz3_s242_direct_lookup(const void* ptr) {
    hz3_s242_stats_register_once();
    hz3_s242_stats_inc(&g_hz3_s242_lookup_try);

    uintptr_t key = (uintptr_t)ptr;
    uint32_t set = hz3_s242_direct_hash(ptr);
    int saw_empty = 0;
    int saw_other_key = 0;

    for (uint32_t way = 0; way < (uint32_t)HZ3_S242_DIRECT_WAYS; way++) {
        Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
        uintptr_t observed = atomic_load_explicit(&slot->key, memory_order_acquire);
        if (observed == key) {
            Hz3LargeHdr* hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (hdr) {
                hz3_s242_stats_inc(&g_hz3_s242_lookup_hit);
                return hdr;
            }
            hz3_s242_stats_inc(&g_hz3_s242_lookup_miss_validate);
            return NULL;
        }
        if (observed == 0) {
            saw_empty = 1;
        } else {
            saw_other_key = 1;
        }
    }

    if (saw_other_key) {
        hz3_s242_stats_inc(&g_hz3_s242_lookup_miss_key);
    } else if (saw_empty) {
        hz3_s242_stats_inc(&g_hz3_s242_lookup_miss_empty);
    } else {
        hz3_s242_stats_inc(&g_hz3_s242_lookup_miss_key);
    }
    return NULL;
}

static inline void hz3_s242_direct_remove(const void* ptr, Hz3LargeHdr* hdr) {
    hz3_s242_stats_register_once();
    hz3_s242_stats_inc(&g_hz3_s242_remove_try);

    uintptr_t key = (uintptr_t)ptr;
    uint32_t set = hz3_s242_direct_hash(ptr);
    uint32_t first_way = 0;
    uint32_t ways = (uint32_t)HZ3_S242_DIRECT_WAYS;

    if (hdr && hdr->direct_slot != HZ3_S242_DIRECT_SLOT_NONE) {
        uint32_t slot_set = hdr->direct_slot / (uint32_t)HZ3_S242_DIRECT_WAYS;
        uint32_t slot_way = hdr->direct_slot % (uint32_t)HZ3_S242_DIRECT_WAYS;
        if (slot_set == set && slot_way < ways) {
            first_way = slot_way;
        }
    }

    for (uint32_t pass = 0; pass < 2; pass++) {
        uint32_t begin = (pass == 0) ? first_way : 0;
        uint32_t end = (pass == 0) ? (first_way + 1u) : ways;
        for (uint32_t way = begin; way < end; way++) {
            if (pass == 1 && way == first_way) {
                continue;
            }
            Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
            uintptr_t observed = atomic_load_explicit(&slot->key, memory_order_acquire);
            if (observed != key) {
                continue;
            }
            Hz3LargeHdr* slot_hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (hdr && slot_hdr != hdr) {
                continue;
            }
            atomic_store_explicit(&slot->hdr, NULL, memory_order_release);
            atomic_store_explicit(&slot->key, 0, memory_order_release);
            if (hdr) {
                hdr->direct_slot = HZ3_S242_DIRECT_SLOT_NONE;
            }
            hz3_s242_direct_note_entry_remove();
            hz3_s242_stats_inc(&g_hz3_s242_remove_ok);
            return;
        }
    }

    if (hdr) {
        hdr->direct_slot = HZ3_S242_DIRECT_SLOT_NONE;
    }
    hz3_s242_stats_inc(&g_hz3_s242_remove_miss);
}

static inline int hz3_s242_direct_validate_active(const void* ptr, Hz3LargeHdr* hdr) {
    if (!hdr ||
        hdr->magic != HZ3_LARGE_MAGIC ||
        hdr->user_ptr != ptr ||
        hdr->map_base == NULL ||
        hdr->map_size == 0) {
        return 0;
    }
#if HZ3_LARGE_CACHE_ENABLE
    if (hdr->in_use != 1) {
        return 0;
    }
#endif
    return atomic_load_explicit(&hdr->state, memory_order_acquire) == HZ3_LARGE_STATE_ACTIVE;
}

static inline Hz3LargeHdr* hz3_s242_direct_take(const void* ptr) {
    hz3_s242_stats_register_once();
    hz3_s242_stats_inc(&g_hz3_s242_take_try);

    uintptr_t key = (uintptr_t)ptr;
    uint32_t set = hz3_s242_direct_hash(ptr);

    for (uint32_t attempt = 0; attempt <= (uint32_t)HZ3_S242_DIRECT_TAKE_BUSY_RETRIES; attempt++) {
        int saw_busy = 0;
        for (uint32_t way = 0; way < (uint32_t)HZ3_S242_DIRECT_WAYS; way++) {
            Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
            uintptr_t observed = atomic_load_explicit(&slot->key, memory_order_acquire);
            if (observed == HZ3_S242_DIRECT_KEY_BUSY) {
                saw_busy = 1;
                hz3_s242_stats_inc(&g_hz3_s242_take_busy);
                continue;
            }
            if (observed != key) {
                continue;
            }

            Hz3LargeHdr* hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (!hz3_s242_direct_validate_active(ptr, hdr)) {
                hz3_s242_stats_inc(&g_hz3_s242_take_validate_fail);
                hz3_s242_direct_fail("take",
                                     "validate-active-failed",
                                     ptr,
                                     hdr,
                                     set,
                                     way);
                return NULL;
            }
            if (hdr->direct_slot != hz3_s242_direct_slot_id(set, way)) {
                hz3_s242_stats_inc(&g_hz3_s242_take_slot_mismatch);
                hz3_s242_direct_fail("take",
                                     "direct-slot-mismatch",
                                     ptr,
                                     hdr,
                                     set,
                                     way);
                return NULL;
            }
            if ((hdr->flags & HZ3_LARGE_F_DIRECT_MAPLESS) == 0) {
                hz3_s242_stats_inc(&g_hz3_s242_take_missing_mapless_flag);
                hz3_s242_direct_fail("take",
                                     "missing-mapless-flag",
                                     ptr,
                                     hdr,
                                     set,
                                     way);
                return NULL;
            }

            uintptr_t expected = key;
            if (!atomic_compare_exchange_strong_explicit(&slot->key,
                                                         &expected,
                                                         HZ3_S242_DIRECT_KEY_BUSY,
                                                         memory_order_acq_rel,
                                                         memory_order_acquire)) {
                if (expected == HZ3_S242_DIRECT_KEY_BUSY) {
                    saw_busy = 1;
                    hz3_s242_stats_inc(&g_hz3_s242_take_busy);
                    continue;
                }
                hz3_s242_stats_inc(&g_hz3_s242_take_miss);
                return NULL;
            }
            atomic_store_explicit(&slot->hdr, NULL, memory_order_release);
            atomic_store_explicit(&slot->key, 0, memory_order_release);
            hdr->direct_slot = HZ3_S242_DIRECT_SLOT_NONE;
            hdr->flags &= (uint16_t)~HZ3_LARGE_F_DIRECT_MAPLESS;
            hz3_s242_direct_note_entry_remove();
            hz3_s242_stats_inc(&g_hz3_s242_take_ok);
            return hdr;
        }

        if (!saw_busy) {
            break;
        }
        atomic_signal_fence(memory_order_seq_cst);
    }

    hz3_s242_stats_inc(&g_hz3_s242_take_miss);
    return NULL;
}

#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline Hz3LargeHdr* hz3_s276_direct_try_retain_front(const void* ptr,
                                                            int* stop_fallback) {
    if (stop_fallback) {
        *stop_fallback = 0;
    }
    hz3_large_s276_note_front_take_try();

    uintptr_t key = (uintptr_t)ptr;
    uint32_t set = hz3_s242_direct_hash(ptr);

    for (uint32_t attempt = 0; attempt <= (uint32_t)HZ3_S242_DIRECT_TAKE_BUSY_RETRIES; attempt++) {
        int saw_busy = 0;
        for (uint32_t way = 0; way < (uint32_t)HZ3_S242_DIRECT_WAYS; way++) {
            Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
            hz3_s285_on_direct_way_load();
            uintptr_t observed = atomic_load_explicit(&slot->key, memory_order_acquire);
            if (observed == HZ3_S242_DIRECT_KEY_BUSY) {
                saw_busy = 1;
                hz3_s285_on_direct_key_busy();
                continue;
            }
            if (observed != key) {
                hz3_s285_on_direct_key_miss();
                continue;
            }
            hz3_s285_on_direct_key_match();

            Hz3LargeHdr* hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
            if (!hdr ||
                hdr->magic != HZ3_LARGE_MAGIC ||
                hdr->user_ptr != ptr ||
                hdr->map_base == NULL ||
                hdr->map_size == 0 ||
                hdr->direct_slot != hz3_s242_direct_slot_id(set, way) ||
                (hdr->flags & HZ3_LARGE_F_DIRECT_MAPLESS) == 0) {
                hz3_large_s276_note_front_take_not_eligible();
                hz3_s285_on_direct_invalid_hdr();
                return NULL;
            }

            uint32_t state = atomic_load_explicit(&hdr->state, memory_order_acquire);
            if (state != HZ3_LARGE_STATE_ACTIVE) {
                hz3_large_s276_note_front_take_nonactive();
                hz3_s285_on_direct_state_nonactive();
                if (stop_fallback) {
                    *stop_fallback = 1;
                }
                return NULL;
            }
#if HZ3_LARGE_CACHE_ENABLE
            if (hdr->in_use != 1) {
                hz3_large_s276_note_front_take_not_eligible();
                hz3_s285_on_direct_inuse_bad();
                return NULL;
            }
#endif
            uint8_t cur = hz3_large_s240_current_owner();
            int sc = hz3_large_sc(hdr->map_size);
            int retain_for_front =
                cur != UINT8_MAX &&
                hdr->owner_shard != UINT8_MAX &&
                hdr->owner_shard == cur &&
                hz3_large_s240_front_eligible_for_req(sc, hdr->map_size, hdr->req_size);
            int retain_for_inbox = 0;
#if HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            retain_for_inbox =
                cur != UINT8_MAX &&
                hdr->owner_shard != UINT8_MAX &&
                hdr->owner_shard != cur &&
                hz3_large_s240_inbox_eligible(sc, hdr->map_size, hdr->req_size);
#endif
            if (!retain_for_front && !retain_for_inbox) {
                if (cur != UINT8_MAX && hdr->owner_shard != UINT8_MAX &&
                    hdr->owner_shard == cur) {
                    hz3_large_s276_note_front_take_not_eligible();
                    hz3_s285_on_direct_not_retainable(1);
                } else {
                    hz3_large_s276_note_front_take_not_owner();
                    hz3_s285_on_direct_not_retainable(0);
                }
                return NULL;
            }

            if (retain_for_front && !HZ3_S276_LARGE_DIRECT_RETAIN_FRONT) {
                hz3_large_s276_note_front_take_not_owner();
                hz3_s285_on_direct_not_retainable(1);
                return NULL;
            }

            uint32_t expected = HZ3_LARGE_STATE_ACTIVE;
            hz3_s285_on_direct_cas_attempt();
            if (!atomic_compare_exchange_strong_explicit(&hdr->state,
                                                         &expected,
                                                         HZ3_LARGE_STATE_FREEING,
                                                         memory_order_acq_rel,
                                                         memory_order_acquire)) {
                if (expected != HZ3_LARGE_STATE_ACTIVE && stop_fallback) {
                    *stop_fallback = 1;
                }
                hz3_large_s276_note_front_take_state_cas_fail();
                hz3_s285_on_direct_cas_fail();
                return NULL;
            }

            hdr->flags |= HZ3_LARGE_F_DIRECT_RETAINED;
            hz3_s285_on_direct_hit(retain_for_inbox);
            if (retain_for_inbox) {
                hz3_large_s276_note_inbox_take_hit();
            } else {
                hz3_large_s276_note_front_take_hit();
            }
            return hdr;
        }

        if (!saw_busy) {
            break;
        }
        atomic_signal_fence(memory_order_seq_cst);
    }

    return NULL;
}

static inline void hz3_s276_direct_clear_retained(Hz3LargeHdr* hdr) {
    if (!hdr || (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        return;
    }
    if (hdr->direct_slot != HZ3_S242_DIRECT_SLOT_NONE && hdr->user_ptr != NULL) {
        hz3_s242_direct_remove(hdr->user_ptr, hdr);
    } else {
        hdr->direct_slot = HZ3_S242_DIRECT_SLOT_NONE;
    }
    hdr->flags &= (uint16_t)~(HZ3_LARGE_F_DIRECT_RETAINED | HZ3_LARGE_F_DIRECT_MAPLESS);
    hz3_large_s276_note_direct_clear();
}
#endif

static inline Hz3LargeHdr* hz3_s242_direct_peek(const void* ptr) {
    uintptr_t key = (uintptr_t)ptr;
    uint32_t set = hz3_s242_direct_hash(ptr);
    for (uint32_t way = 0; way < (uint32_t)HZ3_S242_DIRECT_WAYS; way++) {
        Hz3LargeDirectSlot* slot = &g_hz3_s242_direct[set][way];
        uintptr_t observed = atomic_load_explicit(&slot->key, memory_order_acquire);
        if (observed != key) {
            continue;
        }
        Hz3LargeHdr* hdr = atomic_load_explicit(&slot->hdr, memory_order_acquire);
        return hz3_s242_direct_validate_active(ptr, hdr) ? hdr : NULL;
    }
    return NULL;
}

static inline void hz3_s242_direct_shadow_compare(Hz3LargeHdr* direct_hdr,
                                                  Hz3LargeHdr* map_hdr) {
    if (direct_hdr && map_hdr) {
        if (direct_hdr == map_hdr) {
            hz3_s242_stats_inc(&g_hz3_s242_shadow_match);
        } else {
            hz3_s242_stats_inc(&g_hz3_s242_shadow_mismatch);
        }
    } else if (direct_hdr) {
        hz3_s242_stats_inc(&g_hz3_s242_shadow_direct_only);
    } else if (map_hdr) {
        hz3_s242_stats_inc(&g_hz3_s242_shadow_map_only);
    }
}
#else
static inline int hz3_s242_direct_insert(Hz3LargeHdr* hdr) {
    (void)hdr;
    return 0;
}

static inline Hz3LargeHdr* hz3_s242_direct_lookup(const void* ptr) {
    (void)ptr;
    return NULL;
}

static inline void hz3_s242_direct_remove(const void* ptr, Hz3LargeHdr* hdr) {
    (void)ptr;
    (void)hdr;
}

static inline Hz3LargeHdr* hz3_s242_direct_take(const void* ptr) {
    (void)ptr;
    return NULL;
}

static inline Hz3LargeHdr* hz3_s242_direct_peek(const void* ptr) {
    (void)ptr;
    return NULL;
}

static inline void hz3_s242_direct_shadow_compare(Hz3LargeHdr* direct_hdr,
                                                  Hz3LargeHdr* map_hdr) {
    (void)direct_hdr;
    (void)map_hdr;
}
#endif

static void hz3_large_insert(Hz3LargeHdr* hdr) {
#if HZ3_S242_DIRECT_MAPLESS
    if (hz3_s242_direct_mapless_eligible(hdr)) {
        size_t s275_direct_insert_start_ns = hz3_s275_activation_timing_start();
        int s242_direct_inserted = hz3_s242_direct_insert(hdr);
        hz3_s275_on_insert_direct_mapless_elapsed(
            s275_direct_insert_start_ns, s242_direct_inserted);
        if (s242_direct_inserted) {
            hdr->flags |= HZ3_LARGE_F_DIRECT_MAPLESS;
            hz3_s242_stats_inc(&g_hz3_s242_mapless_active);
            return;
        }
    }
#endif
    size_t obs_start_ns = hz3_large_aligned_obs_timing_start();
    size_t s275_active_map_start_ns = hz3_s275_activation_timing_start();
    uint32_t idx = hz3_large_hash_ptr(hdr->user_ptr);
    hz3_lock_t* map_lock = hz3_large_map_lock_for_idx(idx);
    hz3_lock_acquire(map_lock);
    hdr->next = g_hz3_large_buckets[idx];
    g_hz3_large_buckets[idx] = hdr;
    hz3_large_debug_on_bucket_insert_locked(hdr);
    hz3_lock_release(map_lock);
#if !HZ3_S242_DIRECT_MAPLESS
    size_t s275_direct_shadow_start_ns = hz3_s275_activation_timing_start();
    hz3_s242_direct_insert(hdr);
    hz3_s275_on_insert_direct_shadow_elapsed(s275_direct_shadow_start_ns);
#endif
    hz3_large_aligned_obs_on_map_insert_elapsed(obs_start_ns);
    hz3_s275_on_insert_active_map_elapsed(s275_active_map_start_ns);
}

#include "hz3_large_map_ops_tail.inc"
