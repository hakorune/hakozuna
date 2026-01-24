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

// Day 5: hz3_once for thread-safe initialization
static hz3_once_t g_hz3_central_once = HZ3_ONCE_INIT;

static void hz3_central_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3CentralBin* bin = &g_hz3_central[shard][sc];
            hz3_lock_init(&bin->lock);
            bin->head = NULL;
            bin->count = 0;

        }
    }
}

void hz3_central_init(void) {
    hz3_once(&g_hz3_central_once, hz3_central_do_init);
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
