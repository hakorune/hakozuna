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
static inline int hz3_s234_fast_pop_partial(Hz3CentralFastBin* fast, int sc, void** out, int want) {
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
            hz3_s65_central_note_pop(sc, (uint64_t)out_got);
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
    hz3_s65_local_rem_note_pop(sc, (uint64_t)got);
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
