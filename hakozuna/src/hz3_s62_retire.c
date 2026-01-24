#define _GNU_SOURCE

#include "hz3_s62_retire.h"

#if HZ3_S62_RETIRE

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_small_v2.h"
#include "hz3_small.h"
#include "hz3_dtor_stats.h"
#include "hz3_release_boundary.h"
#include "hz3_arena.h"
#include <string.h>
#if HZ3_S62_RETIRE_MPROTECT
#include <errno.h>
#include <sys/mman.h>
#endif

// ============================================================================
// S62-1: PageRetireBox - retire fully-empty small pages to free_bits
// ============================================================================
//
// Algorithm (O(n)):
// 1. For each size class, pop objects from central bin in batches
// 2. Group objects per-page in a hash table (count + list head/tail)
// 3. When a page reaches full capacity, retire it immediately:
//    - mark free_bits/free_pages
//    - drop the page's object list (do not push back to central)
// 4. After draining central, push remaining (non-retired) page lists back to central
//
// Safety checks:
// - Verify Hz3SegHdr magic/kind/owner before touching free_bits
// - Skip page 0 (header page)
//
// Scope: small_v2 only (sub4k deferred to S62-1b)

extern HZ3_TLS Hz3TCache t_hz3_cache;

// Remote path detection: when remote paths (xfer/stash) are enabled, retiring pages
// based only on "central has capacity objects" is unsafe unless guarded by a
// global predicate (e.g., S69 live_count via S64 retire_scan).
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif
#ifndef HZ3_REMOTE_ENABLED
#define HZ3_REMOTE_ENABLED \
    ((HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE) || \
     (HZ3_S44_OWNER_STASH && !HZ3_S44_OWNER_STASH_DISABLE))
#endif

// Stats (atomic for multi-thread safety)
HZ3_DTOR_STAT(g_s62r_dtor_calls);
HZ3_DTOR_STAT(g_s62r_pages_retired);
HZ3_DTOR_STAT(g_s62r_objs_processed);

HZ3_DTOR_ATEXIT_FLAG(g_s62r);

// Page tracking: stack-local for concurrent dtor safety
#define S62R_PAGE_TABLE_SIZE 4096
#define S62R_BATCH_SIZE      256

typedef struct {
    uintptr_t page_base;
    uint16_t  count;
    uint8_t   retired;  // 1 = retired, 0 = not retired
    void*     head;     // per-page object list head
    void*     tail;     // per-page object list tail
} S62RPageEntry;

#if HZ3_S62_RETIRE_FAILFAST
typedef struct {
    uint32_t magic;
    uint8_t  owner;
    uint8_t  sc;
    uint8_t  flags;
    uint8_t  reserved;
} Hz3SmallV2PageHdrLocal;
#endif

static void hz3_s62r_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s62r_dtor_calls);
    uint32_t retired = HZ3_DTOR_STAT_LOAD(g_s62r_pages_retired);
    uint32_t processed = HZ3_DTOR_STAT_LOAD(g_s62r_objs_processed);
    if (calls > 0 || retired > 0) {
        fprintf(stderr, "[HZ3_S62_RETIRE] dtor_calls=%u pages_retired=%u objs_processed=%u\n",
                calls, retired, processed);
    }
}

// Find page_base in page_table using linear probe
static inline size_t s62r_page_table_find(S62RPageEntry* table, uintptr_t page_base) {
    size_t idx = (page_base >> HZ3_PAGE_SHIFT) % S62R_PAGE_TABLE_SIZE;
    while (table[idx].page_base != 0 && table[idx].page_base != page_base) {
        idx = (idx + 1) % S62R_PAGE_TABLE_SIZE;
    }
    return idx;
}

void hz3_s62_retire(void) {
#if HZ3_S69_LIVECOUNT
    // S69: S62 central-drain retire is disabled.
    // Retirement happens via S64 retire_scan with live_count predicate.
    return;
#endif

#if HZ3_S62_RETIRE_SKIP_WHEN_REMOTE && HZ3_REMOTE_ENABLED
    // Without S69, S62 retire can race with objects still in transit (remote free)
    // or live on other threads. Default to safe no-op under remote.
    return;
#endif

    if (!t_hz3_cache.initialized) {
        return;
    }

    HZ3_DTOR_STAT_INC(g_s62r_dtor_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s62r, hz3_s62r_atexit_dump);

    uint8_t my_shard = t_hz3_cache.my_shard;
    uint32_t total_processed = 0;
    uint32_t pages_retired = 0;
#if HZ3_S62_RETIRE_MPROTECT
    uint32_t pages_protected = 0;
#endif

    // Stack-local buffers (no static/global to avoid race with concurrent dtors).
    void* batch_buf[S62R_BATCH_SIZE];
    S62RPageEntry page_table[S62R_PAGE_TABLE_SIZE];

    // Process each size class
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        // 1. Initialize page table
        memset(page_table, 0, sizeof(page_table));
        size_t capacity = hz3_small_v2_page_capacity(sc);
        if (capacity == 0) {
            continue;
        }

        // 2. Drain central in batches, build per-page lists in the page table
        uint32_t got;
        while ((got = hz3_small_v2_central_pop_batch_ext(my_shard, sc, batch_buf, S62R_BATCH_SIZE)) > 0) {
            total_processed += got;

            for (uint32_t i = 0; i < got; i++) {
                void* obj = batch_buf[i];
#if HZ3_S62_RETIRE_FAILFAST
                uint32_t obj_page_idx = 0;
                if (!hz3_arena_page_index_fast(obj, &obj_page_idx)) {
                    fprintf(stderr, "[HZ3_S62_RETIRE_FATAL] obj out of arena: obj=%p sc=%d\n",
                            obj, sc);
                    abort();
                }
#endif
                uintptr_t page_base = (uintptr_t)obj & ~(HZ3_PAGE_SIZE - 1);
#if HZ3_S62_RETIRE_FAILFAST
                Hz3SmallV2PageHdrLocal* ph = (Hz3SmallV2PageHdrLocal*)page_base;
                if (ph->magic != HZ3_PAGE_MAGIC) {
                    fprintf(stderr,
                            "[HZ3_S62_RETIRE_FATAL] bad page magic: obj=%p page=%p sc=%d magic=0x%x\n",
                            obj, (void*)page_base, sc, (unsigned)ph->magic);
                    abort();
                }
                if ((int)ph->sc != sc) {
                    fprintf(stderr,
                            "[HZ3_S62_RETIRE_FATAL] sc mismatch: obj=%p page=%p want_sc=%d page_sc=%u\n",
                            obj, (void*)page_base, sc, (unsigned)ph->sc);
                    abort();
                }
                if (ph->owner != my_shard) {
                    fprintf(stderr,
                            "[HZ3_S62_RETIRE_FATAL] owner mismatch: obj=%p page=%p shard=%u page_owner=%u\n",
                            obj, (void*)page_base, (unsigned)my_shard, (unsigned)ph->owner);
                    abort();
                }
#endif

                size_t idx = s62r_page_table_find(page_table, page_base);
                S62RPageEntry* ent = &page_table[idx];

                if (ent->page_base == 0) {
                    ent->page_base = page_base;
                    ent->count = 0;
                    ent->retired = 0;
                    ent->head = NULL;
                    ent->tail = NULL;
                }

                if (ent->retired) {
                    // Page already retired in this pass: drop the object.
                    // (Seeing more objects for a retired page indicates corruption/double-free,
                    // but in non-failfast mode we just drop to avoid reintroducing stale frees.)
                    continue;
                }

                // Append to per-page list
                hz3_obj_set_next(obj, NULL);
                if (!ent->head) {
                    ent->head = obj;
                    ent->tail = obj;
                } else {
                    hz3_obj_set_next(ent->tail, obj);
                    ent->tail = obj;
                }

                ent->count++;
                if (ent->count == capacity) {
                    // Retire immediately: prove we have all objects for this page.
                    uintptr_t seg_base = page_base & ~(HZ3_SEG_SIZE - 1);
                    Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
                    size_t page_idx = (page_base - seg_base) >> HZ3_PAGE_SHIFT;

                    // Safety: skip header page and verify segment header/owner.
                    if (page_idx != 0 &&
                        hdr->magic == HZ3_SEG_HDR_MAGIC &&
                        hdr->kind == HZ3_SEG_KIND_SMALL &&
                        hdr->owner == my_shard) {
#if HZ3_S65_RELEASE_BOUNDARY
                        // S65: Use unified boundary API for free_bits updates.
                        int ret = hz3_release_range_definitely_free(hdr, (uint32_t)page_idx, 1,
                                                                     HZ3_RELEASE_SMALL_PAGE_RETIRE);
                        if (ret == 0) {
                            pages_retired++;
                            ent->retired = 1;
                        }
#else
                        hz3_bitmap_mark_free(hdr->free_bits, page_idx, 1);
                        hdr->free_pages++;
#if HZ3_S110_META_ENABLE
                        // S110: clear page_bin_plus1 for the retired small page
                        atomic_store_explicit(&hdr->page_bin_plus1[page_idx], 0, memory_order_release);
#endif
                        pages_retired++;
                        ent->retired = 1;
#endif
#if HZ3_S62_RETIRE_MPROTECT
                        if (ent->retired) {
                            if (mprotect((void*)page_base, HZ3_PAGE_SIZE, PROT_NONE) != 0) {
                                fprintf(stderr,
                                        "[HZ3_S62_RETIRE_PROTECT] mprotect(PROT_NONE) failed: "
                                        "page=%p errno=%d\n",
                                        (void*)page_base, errno);
                                abort();
                            }
                            pages_protected++;
                        }
#endif
                    }

                    // Drop the page list either way (retired or fail-safe skip).
                    // If we skipped retirement (page_idx==0 or bad hdr), we must keep objects.
                    if (!ent->retired) {
                        // Keep list for push-back.
                    } else {
                        ent->head = NULL;
                        ent->tail = NULL;
                    }
                }
            }
        }

        // 3. Push back all non-retired page lists
        for (size_t i = 0; i < S62R_PAGE_TABLE_SIZE; i++) {
            S62RPageEntry* ent = &page_table[i];
            if (!ent->page_base || ent->retired || !ent->head) {
                continue;
            }
            hz3_small_v2_central_push_list(my_shard, sc, ent->head, ent->tail, ent->count);
        }
    }

    // Aggregate stats
    if (total_processed > 0) {
        HZ3_DTOR_STAT_ADD(g_s62r_objs_processed, total_processed);
    }
    if (pages_retired > 0) {
        HZ3_DTOR_STAT_ADD(g_s62r_pages_retired, pages_retired);
    }
#if HZ3_S62_RETIRE_MPROTECT
    (void)pages_protected;
#endif
}

#endif  // HZ3_S62_RETIRE
