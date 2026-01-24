#define _GNU_SOURCE

#include "hz3_stash_guard.h"

#if HZ3_S82_STASH_GUARD

#include "hz3_arena.h"
#include "hz3_tcache.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static _Atomic int g_s82_stash_inited = 0;
static _Atomic int g_s82_stash_enabled = HZ3_S82_STASH_GUARD_DEFAULT;
static _Atomic int g_s82_stash_shot = 0;
static _Atomic int g_s82_stash_logged = 0;

static inline int hz3_s82_parse_bool(const char* s, int defval) {
    if (!s || !*s) {
        return defval;
    }
    if (s[0] == '0' && s[1] == '\0') {
        return 0;
    }
    return 1;
}

static inline int hz3_s82_guard_enabled(void) {
    if (atomic_load_explicit(&g_s82_stash_inited, memory_order_acquire) == 0) {
        int enabled = hz3_s82_parse_bool(getenv("HZ3_S82_STASH_GUARD"),
                                         HZ3_S82_STASH_GUARD_DEFAULT);
        atomic_store_explicit(&g_s82_stash_enabled, enabled, memory_order_release);
        atomic_store_explicit(&g_s82_stash_inited, 1, memory_order_release);
#if HZ3_S82_STASH_GUARD_LOG
        if (!enabled &&
            atomic_exchange_explicit(&g_s82_stash_logged, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr, "[HZ3_S82_STASH_GUARD] disabled via env\n");
        }
#endif
    }
    return atomic_load_explicit(&g_s82_stash_enabled, memory_order_acquire);
}

static inline void hz3_s82_guard_report(const char* where,
                                        const char* why,
                                        void* ptr,
                                        uint32_t expect_bin,
                                        uint32_t page_idx,
                                        uint32_t tag32,
                                        uint32_t bin,
                                        uint8_t dst) {
    if (HZ3_S82_STASH_GUARD_SHOT &&
        atomic_exchange_explicit(&g_s82_stash_shot, 1, memory_order_acq_rel) != 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_S82_STASH_GUARD] where=%s why=%s ptr=%p expect_bin=%u page_idx=%u "
            "tag32=0x%08x bin=%u dst=%u\n",
            (where ? where : "?"), why, ptr, expect_bin, page_idx, tag32, bin, (unsigned)dst);
    if (HZ3_S82_STASH_GUARD_FAILFAST) {
        abort();
    }
}

void hz3_s82_stash_guard_one(const char* where, void* ptr, uint32_t expect_bin) {
    if (!ptr) {
        return;
    }
    if (!hz3_s82_guard_enabled()) {
        return;
    }
    if (!g_hz3_page_tag32) {
        return;
    }

    uint32_t page_idx = 0;
    if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
        return;
    }

    uint32_t tag32 = hz3_pagetag32_load(page_idx);
    if (tag32 == 0) {
        hz3_s82_guard_report(where, "tag32_zero", ptr, expect_bin,
                             page_idx, tag32, 0, 0);
        return;
    }
    uint32_t bin = hz3_pagetag32_bin(tag32);
    uint8_t dst = hz3_pagetag32_dst(tag32);
    if (bin != expect_bin) {
        hz3_s82_guard_report(where, "bin_mismatch", ptr, expect_bin,
                             page_idx, tag32, bin, dst);
    }
}

void hz3_s82_stash_guard_list(const char* where, void* head, uint32_t expect_bin, uint32_t max_walk) {
    if (!head || max_walk == 0) {
        return;
    }
    if (!hz3_s82_guard_enabled()) {
        return;
    }
    void* cur = head;
    uint32_t walked = 0;
    while (cur && walked < max_walk) {
        hz3_s82_stash_guard_one(where, cur, expect_bin);
        cur = hz3_obj_get_next(cur);
        walked++;
    }
}

#endif  // HZ3_S82_STASH_GUARD
