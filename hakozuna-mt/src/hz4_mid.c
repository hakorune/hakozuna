// hz4_mid.c - HZ4 Mid Box (2KB..~64KB, 1 page)
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hz4_config.h"
#include "hz4_types.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#include "hz4_mid.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

#if HZ4_MID_LOCK_SHARDS < 1
#error "HZ4_MID_LOCK_SHARDS must be >= 1"
#endif
#if (HZ4_MID_LOCK_SHARDS & (HZ4_MID_LOCK_SHARDS - 1)) != 0
#error "HZ4_MID_LOCK_SHARDS must be power-of-two"
#endif

#if HZ4_MID_LOCK_ELIDE
static inline void hz4_mid_sc_lock_acquire(uint16_t sc) {
    (void)sc;
}
static inline void hz4_mid_sc_lock_release(uint16_t sc) {
    (void)sc;
}
static inline void hz4_mid_seg_lock_acquire(void) {}
static inline void hz4_mid_seg_lock_release(void) {}
#elif HZ4_MID_LOCK_SPIN
static pthread_spinlock_t g_hz4_mid_sc_locks[HZ4_MID_LOCK_SHARDS];
static pthread_spinlock_t g_hz4_mid_seg_lock;
static pthread_once_t g_hz4_mid_lock_once = PTHREAD_ONCE_INIT;

static void hz4_mid_lock_init(void) {
    for (uint32_t i = 0; i < HZ4_MID_LOCK_SHARDS; i++) {
        pthread_spin_init(&g_hz4_mid_sc_locks[i], PTHREAD_PROCESS_PRIVATE);
    }
    pthread_spin_init(&g_hz4_mid_seg_lock, PTHREAD_PROCESS_PRIVATE);
}

static inline pthread_spinlock_t* hz4_mid_sc_lock_for(uint16_t sc) {
    return &g_hz4_mid_sc_locks[((uint32_t)sc) & (HZ4_MID_LOCK_SHARDS - 1u)];
}

static inline void hz4_mid_sc_lock_acquire(uint16_t sc) {
    pthread_once(&g_hz4_mid_lock_once, hz4_mid_lock_init);
    pthread_spin_lock(hz4_mid_sc_lock_for(sc));
}

static inline void hz4_mid_sc_lock_release(uint16_t sc) {
    pthread_spin_unlock(hz4_mid_sc_lock_for(sc));
}

static inline void hz4_mid_seg_lock_acquire(void) {
    pthread_once(&g_hz4_mid_lock_once, hz4_mid_lock_init);
    pthread_spin_lock(&g_hz4_mid_seg_lock);
}

static inline void hz4_mid_seg_lock_release(void) {
    pthread_spin_unlock(&g_hz4_mid_seg_lock);
}
#else
static pthread_mutex_t g_hz4_mid_sc_locks[HZ4_MID_LOCK_SHARDS];
static pthread_mutex_t g_hz4_mid_seg_lock;
static pthread_once_t g_hz4_mid_lock_once = PTHREAD_ONCE_INIT;

static void hz4_mid_lock_init(void) {
    for (uint32_t i = 0; i < HZ4_MID_LOCK_SHARDS; i++) {
        pthread_mutex_init(&g_hz4_mid_sc_locks[i], NULL);
    }
    pthread_mutex_init(&g_hz4_mid_seg_lock, NULL);
}

static inline pthread_mutex_t* hz4_mid_sc_lock_for(uint16_t sc) {
    return &g_hz4_mid_sc_locks[((uint32_t)sc) & (HZ4_MID_LOCK_SHARDS - 1u)];
}

static inline void hz4_mid_sc_lock_acquire(uint16_t sc) {
    pthread_once(&g_hz4_mid_lock_once, hz4_mid_lock_init);
    pthread_mutex_lock(hz4_mid_sc_lock_for(sc));
}

static inline void hz4_mid_sc_lock_release(uint16_t sc) {
    pthread_mutex_unlock(hz4_mid_sc_lock_for(sc));
}

static inline void hz4_mid_seg_lock_acquire(void) {
    pthread_once(&g_hz4_mid_lock_once, hz4_mid_lock_init);
    pthread_mutex_lock(&g_hz4_mid_seg_lock);
}

static inline void hz4_mid_seg_lock_release(void) {
    pthread_mutex_unlock(&g_hz4_mid_seg_lock);
}
#endif

#ifndef HZ4_MID_SC_COUNT
#define HZ4_MID_SC_COUNT (HZ4_PAGE_SIZE / HZ4_MID_ALIGN)
#endif

#if HZ4_MID_NO_CLEAR_NEXT || HZ4_ST_MID_NO_CLEAR_NEXT
#define HZ4_MID_CLEAR_NEXT(obj) ((void)0)
#else
#define HZ4_MID_CLEAR_NEXT(obj) hz4_obj_set_next((obj), NULL)
#endif

#if HZ4_ST_MID_OBJ_CACHE
#define HZ4_MID_OBJ_CACHE_EFF 1
#define HZ4_MID_OBJ_CACHE_SLOTS_EFF HZ4_ST_MID_OBJ_CACHE_SLOTS
#define HZ4_MID_OBJ_CACHE_BATCH_EFF HZ4_ST_MID_OBJ_CACHE_BATCH
#define HZ4_MID_OBJ_CACHE_BATCH_N_EFF HZ4_ST_MID_OBJ_CACHE_BATCH_N
#else
#define HZ4_MID_OBJ_CACHE_EFF HZ4_MID_OBJ_CACHE
#define HZ4_MID_OBJ_CACHE_SLOTS_EFF HZ4_MID_OBJ_CACHE_SLOTS
#define HZ4_MID_OBJ_CACHE_BATCH_EFF HZ4_MID_OBJ_CACHE_BATCH
#define HZ4_MID_OBJ_CACHE_BATCH_N_EFF HZ4_MID_OBJ_CACHE_BATCH_N
#endif

// TLS state boundary: keep per-thread mid state declarations grouped.
#if HZ4_MID_TLS_CACHE
static __thread hz4_mid_page_t* g_mid_tls_pages[HZ4_MID_SC_COUNT][HZ4_MID_TLS_CACHE_DEPTH];
static __thread uint8_t g_mid_tls_n[HZ4_MID_SC_COUNT];
#endif

#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
static __thread hz4_mid_page_t* g_mid_prefetched_bin_head[HZ4_MID_SC_COUNT];
#endif

#if HZ4_ST_MID_LOCAL_STACK_BOX && HZ4_TLS_SINGLE
static __thread void* g_mid_st_local_stack[HZ4_MID_SC_COUNT][HZ4_ST_MID_LOCAL_STACK_SLOTS];
static __thread uint8_t g_mid_st_local_stack_n[HZ4_MID_SC_COUNT];
#endif

#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX
static __thread void* g_mid_owner_local_stack[HZ4_MID_SC_COUNT][HZ4_MID_OWNER_LOCAL_STACK_SLOTS];
static __thread uint8_t g_mid_owner_local_stack_n[HZ4_MID_SC_COUNT];
static __thread int g_hz4_mid_owner_local_stack_tls_registered;
#endif

#if HZ4_MID_OBJ_CACHE_EFF
static __thread void* g_mid_obj_cache[(HZ4_PAGE_SIZE / HZ4_MID_ALIGN)][HZ4_MID_OBJ_CACHE_SLOTS_EFF];
static __thread uint8_t g_mid_obj_cache_n[(HZ4_PAGE_SIZE / HZ4_MID_ALIGN)];
#endif

#if HZ4_MID_BUMP_INIT
#define HZ4_MID_PAGE_HAS_FREE(p) ((p)->free_count > 0)
#elif HZ4_MID_OBJ_CACHE_EFF
#define HZ4_MID_PAGE_HAS_FREE(p) ((p)->free != NULL)
#elif HZ4_MID_FREECOUNT
#define HZ4_MID_PAGE_HAS_FREE(p) ((p)->free_count > 0)
#else
#define HZ4_MID_PAGE_HAS_FREE(p) ((p)->free != NULL)
#endif

#include "hz4_mid_freelist_boundary.inc"

#if HZ4_ST_MID_LOCAL_STACK_BOX && HZ4_TLS_SINGLE
static inline void* hz4_mid_st_local_stack_pop(uint16_t sc) {
    uint8_t n = g_mid_st_local_stack_n[sc];
    if (n == 0) {
        return NULL;
    }
    n--;
    void* obj = g_mid_st_local_stack[sc][n];
    g_mid_st_local_stack[sc][n] = NULL;
    g_mid_st_local_stack_n[sc] = n;
    if (!obj) {
        HZ4_FAIL("hz4_mid_st_local_stack_pop: null object");
        abort();
    }
    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
    if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
        HZ4_FAIL("hz4_mid_st_local_stack_pop: page/sc mismatch");
        abort();
    }
    hz4_mid_page_freelist_pop_count_dec(page);
    HZ4_MID_CLEAR_NEXT(obj);
    return obj;
}

static inline int hz4_mid_st_local_stack_push(uint16_t sc, void* obj) {
    uint8_t n = g_mid_st_local_stack_n[sc];
    if (n >= (uint8_t)HZ4_ST_MID_LOCAL_STACK_SLOTS) {
        return 0;
    }
    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
    if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
        HZ4_FAIL("hz4_mid_st_local_stack_push: page/sc mismatch");
        abort();
    }
    hz4_mid_page_freelist_push_count(page, 1);
    g_mid_st_local_stack[sc][n] = obj;
    g_mid_st_local_stack_n[sc] = (uint8_t)(n + 1u);
    return 1;
}
#else
static inline void* hz4_mid_st_local_stack_pop(uint16_t sc) {
    (void)sc;
    return NULL;
}
static inline int hz4_mid_st_local_stack_push(uint16_t sc, void* obj) {
    (void)sc;
    (void)obj;
    return 0;
}
#endif

#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX
static pthread_key_t g_hz4_mid_owner_local_stack_tls_key;
static pthread_once_t g_hz4_mid_owner_local_stack_tls_once = PTHREAD_ONCE_INIT;
#endif

#if HZ4_MID_OBJ_CACHE_EFF
static inline void* hz4_mid_obj_cache_pop(uint16_t sc) {
    uint8_t n = g_mid_obj_cache_n[sc];
    if (n == 0) {
        return NULL;
    }
    n--;
    g_mid_obj_cache_n[sc] = n;
    void* obj = g_mid_obj_cache[sc][n];
    g_mid_obj_cache[sc][n] = NULL;
    return obj;
}

static inline int hz4_mid_obj_cache_push(uint16_t sc, void* obj) {
    uint8_t n = g_mid_obj_cache_n[sc];
    if (n >= HZ4_MID_OBJ_CACHE_SLOTS_EFF) {
        return 0;
    }
    g_mid_obj_cache[sc][n] = obj;
    g_mid_obj_cache_n[sc] = (uint8_t)(n + 1);
    return 1;
}
#if HZ4_MID_OBJ_CACHE_BATCH_EFF
static inline void* hz4_mid_obj_cache_refill_from_page(uint16_t sc, hz4_mid_page_t* page) {
    if (!page) {
        return NULL;
    }
    uint8_t n = g_mid_obj_cache_n[sc];
    if (n >= HZ4_MID_OBJ_CACHE_SLOTS_EFF) {
        return NULL;
    }
    uint8_t avail = (uint8_t)(HZ4_MID_OBJ_CACHE_SLOTS_EFF - n);
    uint8_t take = HZ4_MID_OBJ_CACHE_BATCH_N_EFF;
    if (take > avail) {
        take = avail;
    }
    if (take == 0) {
        return NULL;
    }
    void* ret = NULL;
    while (take && HZ4_MID_PAGE_HAS_FREE(page)) {
        void* obj = hz4_mid_page_pop(page);
        if (!obj) {
            break;
        }
        if (!ret) {
            ret = obj;
        } else {
            g_mid_obj_cache[sc][n++] = obj;
        }
        take--;
    }
    g_mid_obj_cache_n[sc] = n;
    return ret;
}
#endif
#endif
static hz4_mid_page_t* g_hz4_mid_bins[(HZ4_PAGE_SIZE / HZ4_MID_ALIGN)];

// Mid bin insert helper (sc lock must be held).
// Prevent duplicate insertion, especially head self-link (page->next == page).
static inline void hz4_mid_bin_prepend_unique_locked(uint16_t sc, hz4_mid_page_t* page) {
    if (!page) {
        return;
    }
    if (page->in_bin) {
        return;
    }
    page->next = g_hz4_mid_bins[sc];
    g_hz4_mid_bins[sc] = page;
    page->in_bin = 1;
}

// Mid bin remove helper (sc lock must be held).
static inline void hz4_mid_bin_remove_locked(uint16_t sc, hz4_mid_page_t* page, hz4_mid_page_t* prev) {
    if (!page || !page->in_bin) {
        return;
    }
    if (prev) {
        if (prev->next != page) {
            HZ4_FAIL("hz4_mid_bin_remove_locked: prev linkage mismatch");
            abort();
        }
        prev->next = page->next;
    } else if (g_hz4_mid_bins[sc] == page) {
        g_hz4_mid_bins[sc] = page->next;
    } else {
        HZ4_FAIL("hz4_mid_bin_remove_locked: head mismatch");
        abort();
    }
    page->next = NULL;
    page->in_bin = 0;
#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    // Clear hint if we're removing the hinted page
    if (g_mid_prefetched_bin_head[sc] == page) {
        g_mid_prefetched_bin_head[sc] = NULL;
    }
#endif
}

// Include-order boundary:
// keep tls_cache before stats/owner/page_supply/batch_cache composition units.
#include "hz4_mid_tls_cache.inc"
#include "hz4_mid_stats.inc"

static inline uint16_t hz4_mid_size_to_sc(size_t size) {
    size = hz4_align_up(size, HZ4_MID_ALIGN);
    if (size == 0 || size > HZ4_PAGE_SIZE) {
        return 0xFFFFu;
    }
    return (uint16_t)((size / HZ4_MID_ALIGN) - 1);
}

static inline size_t hz4_mid_sc_to_size(uint16_t sc) {
    return (size_t)(sc + 1) * HZ4_MID_ALIGN;
}

#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX
static inline void hz4_mid_owner_local_stack_flush_sc_locked(uint16_t sc, uintptr_t self_token);
#endif

// Route/building blocks used by malloc/free lock-path.
#include "hz4_mid_owner_remote.inc"
#include "hz4_mid_page_supply.inc"
#include "hz4_mid_batch_cache.inc"

#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX
static void hz4_mid_owner_local_stack_tls_destructor(void* value) {
    (void)value;
    uintptr_t self = hz4_mid_owner_token();
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint16_t sc = (uint16_t)i;
        if (g_mid_owner_local_stack_n[sc] == 0) {
            continue;
        }
        hz4_mid_sc_lock_acquire(sc);
        hz4_mid_owner_local_stack_flush_sc_locked(sc, self);
        hz4_mid_sc_lock_release(sc);
    }
}

static void hz4_mid_owner_local_stack_tls_init(void) {
    (void)pthread_key_create(&g_hz4_mid_owner_local_stack_tls_key,
                             hz4_mid_owner_local_stack_tls_destructor);
}

static inline void hz4_mid_owner_local_stack_tls_register_once(void) {
    if (g_hz4_mid_owner_local_stack_tls_registered) {
        return;
    }
    pthread_once(&g_hz4_mid_owner_local_stack_tls_once, hz4_mid_owner_local_stack_tls_init);
    (void)pthread_setspecific(g_hz4_mid_owner_local_stack_tls_key, (void*)1);
    g_hz4_mid_owner_local_stack_tls_registered = 1;
}

static inline int hz4_mid_owner_local_stack_push(uint16_t sc, hz4_mid_page_t* page, void* obj) {
    if (sc >= HZ4_MID_OWNER_LOCAL_STACK_SC_MAX) {
        return 0;
    }
    uint8_t n = g_mid_owner_local_stack_n[sc];
    if (n >= (uint8_t)HZ4_MID_OWNER_LOCAL_STACK_SLOTS) {
        return 0;
    }
    uintptr_t self = hz4_mid_owner_token();
    if (hz4_mid_owner_tag_load(page) != self) {
        return 0;
    }
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    // B58: Gate on remote pressure - fall back to traditional freelist
    if (atomic_load_explicit(&page->remote_count, memory_order_acquire) > 0) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_free_owner_local_stack_push_gated_remote);
#endif
        return 0;
    }
#endif
    hz4_mid_owner_local_stack_tls_register_once();
    hz4_mid_page_freelist_push_count(page, 1);
    g_mid_owner_local_stack[sc][n] = obj;
    g_mid_owner_local_stack_n[sc] = (uint8_t)(n + 1u);
    return 1;
}

static inline void* hz4_mid_owner_local_stack_pop(uint16_t sc) {
    if (sc >= HZ4_MID_OWNER_LOCAL_STACK_SC_MAX) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_local_stack_sc_skip);
#endif
        return NULL;
    }
    uint8_t n = g_mid_owner_local_stack_n[sc];
    if (n == 0) {
        return NULL;
    }
    uintptr_t self = hz4_mid_owner_token();
    while (n > 0) {
        n--;
        void* obj = g_mid_owner_local_stack[sc][n];
        g_mid_owner_local_stack[sc][n] = NULL;
        g_mid_owner_local_stack_n[sc] = n;
        if (!obj) {
            HZ4_FAIL("hz4_mid_owner_local_stack_pop: null object");
            abort();
        }

        hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
        if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
            HZ4_FAIL("hz4_mid_owner_local_stack_pop: page/sc mismatch");
            abort();
        }

        uintptr_t owner = hz4_mid_owner_tag_load(page);

#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
        // B58: Gate on remote pressure - return to page freelist and continue
        if (owner == self && atomic_load_explicit(&page->remote_count, memory_order_acquire) > 0) {
            hz4_mid_page_freelist_push_raw(page, obj);
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_local_stack_pop_gated_remote);
#endif
            continue;
        }
#endif

        if (owner == self) {
            hz4_mid_page_freelist_pop_count_dec(page);
            HZ4_MID_CLEAR_NEXT(obj);
            return obj;
        }

        if (owner != 0) {
            // Move to remote inbox and keep free_count consistent.
            hz4_mid_page_freelist_pop_count_dec(page);
            hz4_mid_owner_remote_push(page, obj);
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_local_stack_stale_remote);
#endif
            continue;
        }

        // Owner released: relink to page freelist without incrementing count.
        hz4_mid_sc_lock_acquire(sc);
        if (hz4_mid_page_freelist_empty(page)) {
            hz4_mid_bin_prepend_unique_locked(sc, page);
        }
        hz4_mid_page_freelist_push_raw(page, obj);
        hz4_mid_sc_lock_release(sc);
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_local_stack_stale_unowned);
#endif
    }
    return NULL;
}

static inline void hz4_mid_owner_local_stack_flush_sc_locked(uint16_t sc, uintptr_t self_token) {
    uint8_t n = g_mid_owner_local_stack_n[sc];
    while (n > 0) {
        n--;
        void* obj = g_mid_owner_local_stack[sc][n];
        g_mid_owner_local_stack[sc][n] = NULL;
        if (!obj) {
            HZ4_FAIL("hz4_mid_owner_local_stack_flush_sc_locked: null object");
            abort();
        }
        hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
        if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
            HZ4_FAIL("hz4_mid_owner_local_stack_flush_sc_locked: page/sc mismatch");
            abort();
        }
        uintptr_t owner = hz4_mid_owner_tag_load(page);
        if (owner == self_token || owner == 0) {
            if (hz4_mid_page_freelist_empty(page)) {
                hz4_mid_bin_prepend_unique_locked(sc, page);
            }
            hz4_mid_page_freelist_push_raw(page, obj);
            continue;
        }
        hz4_mid_page_freelist_pop_count_dec(page);
        hz4_mid_owner_remote_push(page, obj);
    }
    g_mid_owner_local_stack_n[sc] = 0;
}
#else
static inline int hz4_mid_owner_local_stack_push(uint16_t sc, hz4_mid_page_t* page, void* obj) {
    (void)sc;
    (void)page;
    (void)obj;
    return 0;
}

static inline void* hz4_mid_owner_local_stack_pop(uint16_t sc) {
    (void)sc;
    return NULL;
}
#endif

size_t hz4_mid_max_size(void) {
    return hz4_mid_max_size_inline();
}

size_t hz4_mid_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1));
    if (page->magic != HZ4_MID_MAGIC) {
        HZ4_FAIL("hz4_mid_usable_size: invalid page");
        abort();
    }
    return (size_t)page->obj_size;
}

// Terminal entry boundaries:
// keep public operation order stable (malloc first, then free).
#include "hz4_mid_malloc.inc"
#include "hz4_mid_free.inc"
