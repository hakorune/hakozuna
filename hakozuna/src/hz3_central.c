#define _GNU_SOURCE

#include "hz3_central.h"
#include "hz3_arena.h"
#include "hz3_tcache.h"
#include "hz3_central_shadow.h"
#include "hz3_platform.h"

#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

// Global central pool (zero-initialized)
Hz3CentralBin g_hz3_central[HZ3_NUM_SHARDS][HZ3_NUM_SC];

#if HZ3_S222_CENTRAL_ATOMIC_FAST
typedef struct {
    _Atomic(void*) head;
} Hz3CentralFastBin;

static Hz3CentralFastBin g_hz3_central_fast[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static inline int hz3_s222_sc_ok(int sc) {
    return (sc >= HZ3_S222_SC_MIN && sc <= HZ3_S222_SC_MAX);
}

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
static inline int hz3_s234_sc_ok(int sc) {
    return (sc >= HZ3_S234_SC_MIN && sc <= HZ3_S234_SC_MAX);
}
#endif

static inline void hz3_s222_push_list_to_fast(Hz3CentralFastBin* fast, void* head, void* tail) {
    void* old_head;
    do {
        old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(&fast->head, &old_head, head,
                                                     memory_order_release,
                                                     memory_order_acquire));
}

#if HZ3_S222_STATS && HZ3_S234_CENTRAL_FAST_PARTIAL_POP
static _Atomic uint64_t g_s222_s234_pop_partial_hits = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_objs = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_retry = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_fallback = 0;
#endif

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
// Return value:
//   >0 : popped object count
//    0 : fast head empty
//   -1 : retry budget exhausted, caller should fallback to legacy exchange path
static inline int hz3_s234_fast_pop_partial(Hz3CentralFastBin* fast, void** out, int want) {
    int retries = 0;
    for (;;) {
        void* old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
        if (!old_head) {
            return 0;
        }

        void* cur = old_head;
        void* tail = NULL;
        int got = 0;
        while (got < want && cur) {
            tail = cur;
            cur = hz3_obj_get_next(cur);
            got++;
        }

        if (!tail || got <= 0) {
            return 0;
        }

        // CAS only updates stack head; internal links remain immutable.
        void* expect = old_head;
        if (atomic_compare_exchange_weak_explicit(&fast->head, &expect, cur,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            hz3_obj_set_next(tail, NULL);
            int out_got = 0;
            void* node = old_head;
            while (out_got < got && node) {
                void* next = hz3_obj_get_next(node);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
                hz3_central_shadow_verify_and_remove(node, next);
  #else
                hz3_central_shadow_remove(node);
  #endif
#endif
                out[out_got++] = node;
                node = next;
            }
#if HZ3_S222_STATS
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_hits, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_objs, (uint64_t)out_got, memory_order_relaxed);
            if (retries > 0) {
                atomic_fetch_add_explicit(&g_s222_s234_pop_partial_retry, (uint64_t)retries, memory_order_relaxed);
            }
#endif
            return out_got;
        }

        retries++;
        if (retries >= HZ3_S234_CAS_RETRY_MAX) {
#if HZ3_S222_STATS
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_fallback, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_retry, (uint64_t)retries, memory_order_relaxed);
#endif
            return -1;
        }
    }
}
#endif

#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
typedef struct {
    void* head;
    void* tail;
    uint32_t count;
} Hz3S228LocalRem;

static HZ3_TLS Hz3S228LocalRem t_s228_local_rem[HZ3_NUM_SC];

static inline int hz3_s228_sc_ok(int sc) {
    return (sc >= HZ3_S228_SC_MIN && sc <= HZ3_S228_SC_MAX);
}

static inline int hz3_s228_consume_local(int sc, void** out, int max_take) {
    Hz3S228LocalRem* rem = &t_s228_local_rem[sc];
    if (!rem->head || max_take <= 0) {
        return 0;
    }
    int got = 0;
    void* cur = rem->head;
    while (got < max_take && cur) {
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
    rem->head = cur;
    if (!cur) {
        rem->tail = NULL;
    }
    if ((uint32_t)got >= rem->count) {
        rem->count = 0;
    } else {
        rem->count -= (uint32_t)got;
    }
    return got;
}
#endif

#if HZ3_S222_STATS
static _Atomic uint64_t g_s222_push_calls = 0;
static _Atomic uint64_t g_s222_push_objs = 0;
static _Atomic uint64_t g_s222_pop_calls = 0;
static _Atomic uint64_t g_s222_pop_hits = 0;
static _Atomic uint64_t g_s222_pop_objs = 0;
static _Atomic uint64_t g_s222_pop_repush = 0;
static hz3_once_t g_s222_stats_once = HZ3_ONCE_INIT;

static void hz3_s222_dump(void) {
#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
    fprintf(stderr,
            "[HZ3_S222] push_calls=%lu push_objs=%lu pop_calls=%lu pop_hits=%lu pop_objs=%lu pop_repush=%lu "
            "s234_hits=%lu s234_objs=%lu s234_retry=%lu s234_fallback=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s222_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_repush, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_retry, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_fallback, memory_order_relaxed));
#else
    fprintf(stderr,
            "[HZ3_S222] push_calls=%lu push_objs=%lu pop_calls=%lu pop_hits=%lu pop_objs=%lu pop_repush=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s222_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_repush, memory_order_relaxed));
#endif
}

static void hz3_s222_register_atexit(void) {
    atexit(hz3_s222_dump);
}

#define HZ3_S222_STAT_INC(name) \
    atomic_fetch_add_explicit(&g_s222_##name, 1, memory_order_relaxed)
#define HZ3_S222_STAT_ADD(name, v) \
    atomic_fetch_add_explicit(&g_s222_##name, (uint64_t)(v), memory_order_relaxed)
#else
#define HZ3_S222_STAT_INC(name) ((void)0)
#define HZ3_S222_STAT_ADD(name, v) ((void)(v))
#endif
#endif

#if HZ3_S189_MEDIUM_TRANSFERCACHE
typedef struct {
    hz3_lock_t lock;
    void* head;
    uint32_t count;
} Hz3CentralXferBin;

static Hz3CentralXferBin g_hz3_central_xfer[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic uint64_t g_s189_xfer_push_calls = 0;
static _Atomic uint64_t g_s189_xfer_push_objs = 0;
static _Atomic uint64_t g_s189_xfer_push_drop = 0;
static _Atomic uint64_t g_s189_xfer_pop_calls = 0;
static _Atomic uint64_t g_s189_xfer_pop_hits = 0;
static _Atomic uint64_t g_s189_xfer_pop_objs = 0;
static hz3_once_t g_s189_xfer_atexit_once = HZ3_ONCE_INIT;

static inline int hz3_s189_sc_ok(int sc) {
    return (sc >= HZ3_S189_SC_MIN && sc <= HZ3_S189_SC_MAX);
}

static void hz3_s189_xfer_dump(void) {
    fprintf(stderr,
            "[HZ3_S189_XFER] push_calls=%lu push_objs=%lu push_drop=%lu "
            "pop_calls=%lu pop_hits=%lu pop_objs=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_drop, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_objs, memory_order_relaxed));
}

static void hz3_s189_xfer_register_atexit(void) {
    atexit(hz3_s189_xfer_dump);
}
#endif

// Day 5: hz3_once for thread-safe initialization
static hz3_once_t g_hz3_central_once = HZ3_ONCE_INIT;

static void hz3_central_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3CentralBin* bin = &g_hz3_central[shard][sc];
            hz3_lock_init(&bin->lock);
            bin->head = NULL;
            bin->count = 0;
#if HZ3_S222_CENTRAL_ATOMIC_FAST
            if (hz3_s222_sc_ok(sc)) {
                atomic_store_explicit(&g_hz3_central_fast[shard][sc].head, NULL, memory_order_relaxed);
            }
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
            if (hz3_s189_sc_ok(sc)) {
                Hz3CentralXferBin* xfer = &g_hz3_central_xfer[shard][sc];
                hz3_lock_init(&xfer->lock);
                xfer->head = NULL;
                xfer->count = 0;
            }
#endif
        }
    }
}

void hz3_central_init(void) {
    hz3_once(&g_hz3_central_once, hz3_central_do_init);
#if HZ3_S222_CENTRAL_ATOMIC_FAST && HZ3_S222_STATS
    hz3_once(&g_s222_stats_once, hz3_s222_register_atexit);
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    hz3_once(&g_s189_xfer_atexit_once, hz3_s189_xfer_register_atexit);
#endif
}

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

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Record (ptr,next) as stored in central after linking.
    hz3_central_shadow_record(run, hz3_obj_get_next(run), s86_ra);
#endif
    hz3_lock_release(&bin->lock);
}

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
        return;
    }
#endif

    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);  // tail->next = old head
    bin->head = head;
    bin->count += n;

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
                    hz3_s222_push_list_to_fast(fast, rem_flush->head, rem_flush->tail);
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
            int part_got = hz3_s234_fast_pop_partial(fast, out, want);
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
                }
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

#if HZ3_S189_MEDIUM_TRANSFERCACHE
int hz3_central_xfer_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!out || want <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_pop_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* run = bin->head;
        bin->head = hz3_obj_get_next(run);
        bin->count--;
        out[got++] = run;
    }
    hz3_lock_release(&bin->lock);

    if (got > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_pop_hits, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s189_xfer_pop_objs, (uint64_t)got, memory_order_relaxed);
    }
    return got;
}

int hz3_central_xfer_push_array(int shard, int sc, void** arr, int n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!arr || n <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_push_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int avail = (int)HZ3_S189_MAX_OBJS_PER_BIN - (int)bin->count;
    if (avail < 0) avail = 0;
    int keep = (n < avail) ? n : avail;
    int pushed = 0;
    for (int i = 0; i < keep; i++) {
        void* run = arr[i];
        if (!run) continue;
        hz3_obj_set_next(run, bin->head);
        bin->head = run;
        bin->count++;
        pushed++;
    }
    hz3_lock_release(&bin->lock);

    if (pushed > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_objs, (uint64_t)pushed, memory_order_relaxed);
    }
    if (n > pushed) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_drop, (uint64_t)(n - pushed), memory_order_relaxed);
    }
    return pushed;
}
#endif
