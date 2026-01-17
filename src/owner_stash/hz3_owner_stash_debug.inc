// =============================================================================
// HZ3_STASH_DEBUG: fail-fast list integrity checks
// =============================================================================
#ifndef HZ3_STASH_DEBUG
#define HZ3_STASH_DEBUG 0
#endif

#if HZ3_STASH_DEBUG

static inline int hz3_stash_ptr_valid(void* p) {
    if (p == NULL) return 1;  // NULL is valid
    // Check if pointer is in arena range (basic sanity)
    uintptr_t addr = (uintptr_t)p;
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_relaxed);
    void* end = atomic_load_explicit(&g_hz3_arena_end, memory_order_relaxed);
    if (!base || !end) return 1;  // arena not initialized yet
    return (addr >= (uintptr_t)base && addr < (uintptr_t)end);
}

static inline void hz3_stash_check_list(const char* where, void* head, void* tail, uint32_t expected_n) {
    if (!head) {
        if (expected_n != 0) {
            fprintf(stderr, "[HZ3_STASH_DEBUG] %s: head=NULL but n=%u\n", where, expected_n);
            abort();
        }
        return;
    }

    // Walk list, count and check validity
    void* cur = head;
    void* last = NULL;
    uint32_t actual_n = 0;
    uint32_t max_walk = expected_n + 1000;  // detect infinite loop

    while (cur && actual_n < max_walk) {
        if (!hz3_stash_ptr_valid(cur)) {
            fprintf(stderr, "[HZ3_STASH_DEBUG] %s: invalid ptr %p at pos %u\n", where, cur, actual_n);
            abort();
        }
        last = cur;
        cur = hz3_obj_get_next(cur);
        actual_n++;
    }

    if (actual_n >= max_walk) {
        fprintf(stderr, "[HZ3_STASH_DEBUG] %s: cycle detected or list too long (walked %u)\n", where, actual_n);
        abort();
    }

    if (tail && last != tail) {
        fprintf(stderr, "[HZ3_STASH_DEBUG] %s: tail mismatch (expected %p, got %p)\n", where, tail, last);
        abort();
    }

    if (expected_n > 0 && actual_n != expected_n) {
        fprintf(stderr, "[HZ3_STASH_DEBUG] %s: count mismatch (expected %u, got %u)\n", where, expected_n, actual_n);
        abort();
    }
}

#define STASH_CHECK_LIST(where, head, tail, n) hz3_stash_check_list(where, head, tail, n)
#define STASH_CHECK_PTR(where, p) do { \
    if (!hz3_stash_ptr_valid(p)) { \
        fprintf(stderr, "[HZ3_STASH_DEBUG] %s: invalid ptr %p\n", where, (void*)(p)); \
        abort(); \
    } \
} while(0)

#else
#define STASH_CHECK_LIST(where, head, tail, n) ((void)0)
#define STASH_CHECK_PTR(where, p) ((void)0)
#endif

#if HZ3_S72_BOUNDARY_DEBUG
#define STASH_BOUNDARY_CHECK_OUT(where, owner, sc, out, got) do { \
    for (int _i = 0; _i < (got); _i++) { \
        hz3_small_v2_boundary_check_obj((where), (owner), (sc), (out)[_i]); \
    } \
} while (0)
#else
#define STASH_BOUNDARY_CHECK_OUT(where, owner, sc, out, got) ((void)0)
#endif
