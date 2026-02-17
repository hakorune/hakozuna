// hz4_remote_page_rbuf.inc - RemotePageRbufBox (mimalloc-style per-page remote)
// Box Theory: remote free の「箱」を inbox から分離し、ページ単位でバッチ drain する。
//
// 設計:
// - remote free: page.remote_head[] に直接 push
// - notify: per-owner global page queue (sc bucket で分離)
// - drain: collect 境界で pageq を drain → hz4_drain_page[_list] を再利用
// - stranding 防止: queued=0 の後に remote_head 再チェック（必要なら requeue）

#ifndef HZ4_REMOTE_PAGE_RBUF_INC
#define HZ4_REMOTE_PAGE_RBUF_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif

#if HZ4_REMOTE_PAGE_RBUF

#if !HZ4_PAGE_META_SEPARATE
#error "HZ4_REMOTE_PAGE_RBUF requires HZ4_PAGE_META_SEPARATE=1"
#endif

// Forward declarations from other boxes (defined later in this TU).
static inline uint32_t hz4_drain_page(hz4_tls_t* tls, uint8_t sc, hz4_page_t* page, void** out, uint32_t budget);
#if HZ4_COLLECT_LIST
static inline uint32_t hz4_drain_page_list(hz4_tls_t* tls, uint8_t sc, hz4_page_t* page,
                                           void** head_out, void** tail_out, uint32_t budget);
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_decommit_queue_enqueue(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta);
#endif
#if HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_page_try_decommit(hz4_page_t* page, hz4_page_meta_t* meta);
#endif
#if HZ4_DECOMMIT_REUSE_POOL && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_reuse_pool_push(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta);
#endif

#include "hz4_remote_page_rbuf_types.inc"

#if HZ4_REMOTE_BUMP_FREE_META
static inline bool hz4_remote_bump_free_meta_try_push_idx(hz4_page_t* page,
                                                          hz4_page_meta_t* meta,
                                                          uint16_t idx32,
                                                          uint8_t owner,
                                                          uint8_t sc) {
    hz4_os_stats_rbmf_try();
    if (meta->capacity == 0) {
        hz4_os_stats_rbmf_fail_guard();
        return false;
    }
    if ((meta->bump_off | meta->bump_left) == 0) {
        hz4_os_stats_rbmf_fail_guard();
        return false;
    }
    if (idx32 >= meta->capacity) {
        hz4_os_stats_rbmf_fail_guard();
        return false;
    }
#if HZ4_FAILFAST
    if (idx32 >= meta->bump_off) {
        HZ4_FAIL("remote_bump_free_meta_try_push: idx not yet allocated");
    }
#endif

    bool notify = false;
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    uint32_t shard = (uint32_t)idx32 & (HZ4_REMOTE_BUMP_FREE_META_SHARDS - 1);
#if HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
    if (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock[shard], memory_order_acquire)) {
        hz4_os_stats_rbmf_fail_lock();
        return false;
    }
#else
    while (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock[shard], memory_order_acquire)) {
    }
#endif
    uint16_t n = atomic_load_explicit(&meta->bump_rfree_n[shard], memory_order_relaxed);
    if (n < HZ4_REMOTE_BUMP_FREE_META_CAP) {
        notify = (n == 0);
        meta->bump_rfree_idx[shard][n] = idx32;
        atomic_store_explicit(&meta->bump_rfree_n[shard], (uint16_t)(n + 1), memory_order_relaxed);
    }
    atomic_flag_clear_explicit(&meta->bump_rfree_lock[shard], memory_order_release);

    if (n >= HZ4_REMOTE_BUMP_FREE_META_CAP) {
        hz4_os_stats_rbmf_fail_full();
        return false;
    }
    if (notify) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
    hz4_os_stats_rbmf_ok();
    return true;
#else
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    if (atomic_load_explicit(&meta->bump_rfree_draining, memory_order_relaxed) != 0) {
        hz4_os_stats_rbmf_fail_draining();
        return false;
    }
    uint16_t n = atomic_load_explicit(&meta->bump_rfree_n, memory_order_relaxed);
    for (;;) {
        if (n >= HZ4_REMOTE_BUMP_FREE_META_CAP) {
            hz4_os_stats_rbmf_fail_full();
            return false;
        }
        if (atomic_compare_exchange_weak_explicit(&meta->bump_rfree_n,
                                                  &n,
                                                  (uint16_t)(n + 1),
                                                  memory_order_acq_rel,
                                                  memory_order_relaxed)) {
            break;
        }
    }
    notify = (n == 0);
    meta->bump_rfree_idx[n] = idx32;
    atomic_fetch_add_explicit(&meta->bump_rfree_pub, 1, memory_order_release);
    if (notify) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
    hz4_os_stats_rbmf_ok();
    return true;
#else
#if HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
    if (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
        hz4_os_stats_rbmf_fail_lock();
        return false;
    }
#else
    while (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
    }
#endif
    uint16_t n = atomic_load_explicit(&meta->bump_rfree_n, memory_order_relaxed);
    if (n < HZ4_REMOTE_BUMP_FREE_META_CAP) {
        notify = (n == 0);
        meta->bump_rfree_idx[n] = idx32;
        atomic_store_explicit(&meta->bump_rfree_n, (uint16_t)(n + 1), memory_order_relaxed);
    }
    atomic_flag_clear_explicit(&meta->bump_rfree_lock, memory_order_release);

    if (n >= HZ4_REMOTE_BUMP_FREE_META_CAP) {
        hz4_os_stats_rbmf_fail_full();
        return false;
    }
    if (notify) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
    hz4_os_stats_rbmf_ok();
    return true;
#endif
#endif
}

static inline bool hz4_remote_bump_free_meta_try_push(hz4_page_t* page,
                                                      hz4_page_meta_t* meta,
                                                      void* ptr,
                                                      uint8_t owner,
                                                      uint8_t sc) {
    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t p = (uintptr_t)ptr;
    if (p < start || p >= (uintptr_t)page + HZ4_PAGE_SIZE) {
        hz4_os_stats_rbmf_try();
        hz4_os_stats_rbmf_fail_guard();
        return false;
    }
    uintptr_t off = p - start;
    if ((off % obj_size) != 0) {
        hz4_os_stats_rbmf_try();
        hz4_os_stats_rbmf_fail_guard();
        return false;
    }

    uint32_t idx32 = (uint32_t)(off / obj_size);
    return hz4_remote_bump_free_meta_try_push_idx(page, meta, (uint16_t)idx32, owner, sc);
}

static inline uint16_t hz4_remote_bump_free_meta_drain(hz4_tls_t* tls,
                                                       uint8_t sc,
                                                       hz4_page_t* page,
                                                       hz4_page_meta_t* meta) {
    uint16_t take = 0;
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    for (uint32_t s = 0; s < HZ4_REMOTE_BUMP_FREE_META_SHARDS; s++) {
        if (meta->bump_free_n >= HZ4_BUMP_FREE_META_CAP) {
            break;
        }
        if (atomic_load_explicit(&meta->bump_rfree_n[s], memory_order_acquire) == 0) {
            continue;
        }
#if HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
        if (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock[s], memory_order_acquire)) {
            continue;
        }
#else
        while (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock[s], memory_order_acquire)) {
        }
#endif
        uint16_t n = atomic_load_explicit(&meta->bump_rfree_n[s], memory_order_relaxed);
        if (n > 0 && meta->bump_free_n < HZ4_BUMP_FREE_META_CAP) {
            uint16_t free_cap = (uint16_t)(HZ4_BUMP_FREE_META_CAP - meta->bump_free_n);
            uint16_t take_one = (n < free_cap) ? n : free_cap;
            for (uint16_t i = 0; i < take_one; i++) {
                uint16_t idx = meta->bump_rfree_idx[s][--n];
                meta->bump_free_idx[meta->bump_free_n++] = idx;
            }
            atomic_store_explicit(&meta->bump_rfree_n[s], n, memory_order_relaxed);
            take = (uint16_t)(take + take_one);
        }
        atomic_flag_clear_explicit(&meta->bump_rfree_lock[s], memory_order_release);
    }
#else
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    if (atomic_load_explicit(&meta->bump_rfree_pub, memory_order_acquire) == 0) {
        return 0;
    }
    atomic_store_explicit(&meta->bump_rfree_draining, 1, memory_order_release);
    uint16_t n = 0;
    for (;;) {
        n = atomic_load_explicit(&meta->bump_rfree_n, memory_order_acquire);
        uint16_t pub;
        do {
            pub = atomic_load_explicit(&meta->bump_rfree_pub, memory_order_acquire);
        } while (pub < n);
        if (atomic_load_explicit(&meta->bump_rfree_n, memory_order_acquire) == n) {
            break;
        }
    }
    if (n > 0 && meta->bump_free_n < HZ4_BUMP_FREE_META_CAP) {
        uint16_t free_cap = (uint16_t)(HZ4_BUMP_FREE_META_CAP - meta->bump_free_n);
        take = (n < free_cap) ? n : free_cap;
        for (uint16_t i = 0; i < take; i++) {
            uint16_t idx = meta->bump_rfree_idx[--n];
            meta->bump_free_idx[meta->bump_free_n++] = idx;
        }
    }
    atomic_store_explicit(&meta->bump_rfree_n, n, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_pub, n, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_draining, 0, memory_order_release);
#else
    if (atomic_load_explicit(&meta->bump_rfree_n, memory_order_acquire) == 0) {
        return 0;
    }

#if HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
    if (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
        return 0;
    }
#else
    while (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
    }
#endif
    uint16_t n = atomic_load_explicit(&meta->bump_rfree_n, memory_order_relaxed);
    if (n > 0 && meta->bump_free_n < HZ4_BUMP_FREE_META_CAP) {
        uint16_t free_cap = (uint16_t)(HZ4_BUMP_FREE_META_CAP - meta->bump_free_n);
        take = (n < free_cap) ? n : free_cap;
        for (uint16_t i = 0; i < take; i++) {
            uint16_t idx = meta->bump_rfree_idx[--n];
            meta->bump_free_idx[meta->bump_free_n++] = idx;
        }
        atomic_store_explicit(&meta->bump_rfree_n, n, memory_order_relaxed);
    }
    atomic_flag_clear_explicit(&meta->bump_rfree_lock, memory_order_release);
#endif
#endif

    if (take > 0) {
        hz4_page_used_dec_meta(meta, take);
        if (!tls->bins[sc].bump_page) {
            tls->bins[sc].bump_page = page;
        }
    }
    return take;
}
#endif

#if HZ4_REMOTE_FREE_META
static inline bool hz4_remote_free_meta_try_push(hz4_page_t* page,
                                                 hz4_page_meta_t* meta,
                                                 void* ptr,
                                                 uint8_t owner,
                                                 uint8_t sc) {
    if (meta->capacity == 0) {
        return false;
    }

    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t p = (uintptr_t)ptr;
    if (p < start || p >= (uintptr_t)page + HZ4_PAGE_SIZE) {
        return false;
    }
    uintptr_t off = p - start;
    if ((off % obj_size) != 0) {
        return false;
    }

    uint32_t idx32 = (uint32_t)(off / obj_size);
    if (idx32 >= meta->capacity) {
        return false;
    }
#if HZ4_FAILFAST
#if HZ4_POPULATE_BATCH
    if (meta->bump_off != 0 && idx32 >= meta->bump_off) {
        HZ4_FAIL("remote_free_meta_try_push: idx not yet allocated");
    }
#endif
#endif

    bool notify = false;
    while (atomic_flag_test_and_set_explicit(&meta->remote_free_lock, memory_order_acquire)) {
    }
    uint16_t n = atomic_load_explicit(&meta->remote_free_n, memory_order_relaxed);
    if (n < HZ4_REMOTE_FREE_META_CAP) {
        notify = (n == 0);
        meta->remote_free_idx[n] = (uint16_t)idx32;
        atomic_store_explicit(&meta->remote_free_n, (uint16_t)(n + 1), memory_order_relaxed);
    }
    atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_release);

    if (n >= HZ4_REMOTE_FREE_META_CAP) {
        return false;
    }
    if (notify) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
    return true;
}
#endif

static inline void hz4_remote_page_rbufq_push(uint8_t owner, uint8_t sc,
                                              hz4_page_t* page, hz4_page_meta_t* meta) {
#if HZ4_FAILFAST
    if (owner >= HZ4_NUM_SHARDS) {
        HZ4_FAIL("remote_page_rbufq_push: owner out of range");
    }
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("remote_page_rbufq_push: sc out of range");
    }
#endif

    hz4_page_t* old = atomic_load_explicit(&g_hz4_remote_page_rbufq[owner][sc], memory_order_acquire);
    do {
        meta->rbufq_next = old;
    } while (!atomic_compare_exchange_weak_explicit(
        &g_hz4_remote_page_rbufq[owner][sc], &old, page,
        memory_order_release, memory_order_acquire));
}

#include "hz4_remote_page_rbuf_notify.inc"

#if HZ4_REMOTE_PAGE_STAGING
static inline uint32_t hz4_remote_page_staging_idx(hz4_page_t* page) {
    uintptr_t paddr = (uintptr_t)page;
    uint32_t mix = (uint32_t)(paddr >> HZ4_PAGE_SHIFT);
    mix ^= (uint32_t)(paddr >> 12);
    return mix & (HZ4_REMOTE_PAGE_STAGING_CACHE_N - 1);
}

#if HZ4_REMOTE_PAGE_STAGING_META
static inline bool hz4_remote_page_staging_calc_idx(hz4_page_t* page,
                                                    hz4_page_meta_t* meta,
                                                    void* ptr,
                                                    uint8_t sc,
                                                    uint16_t* out_idx) {
    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t p = (uintptr_t)ptr;
    if (p < start || p >= (uintptr_t)page + HZ4_PAGE_SIZE) {
        return false;
    }
    uintptr_t off = p - start;
    if ((off % obj_size) != 0) {
        return false;
    }
    uint32_t idx32 = (uint32_t)(off / obj_size);
    if (idx32 >= meta->capacity) {
        return false;
    }
#if HZ4_FAILFAST
    if (idx32 >= meta->bump_off) {
        HZ4_FAIL("remote_page_staging_calc_idx: idx not yet allocated");
    }
#endif
    *out_idx = (uint16_t)idx32;
    return true;
}
#endif

static inline void hz4_remote_page_staging_reset_ent(hz4_remote_page_stash_ent_t* ent) {
    ent->page = NULL;
#if HZ4_REMOTE_PAGE_STAGING_META
    ent->count = 0;
    ent->defer_retries = 0;
#else
    ent->head = NULL;
    ent->tail = NULL;
    ent->count = 0;
#endif
}

#if HZ4_REMOTE_PAGE_STAGING_META
static inline uint16_t hz4_remote_bump_free_meta_try_push_idx_batch(hz4_page_t* page,
                                                                     hz4_page_meta_t* meta,
                                                                     const uint16_t* idxs,
                                                                     uint16_t n,
                                                                     uint8_t owner,
                                                                     uint8_t sc) {
#if HZ4_REMOTE_PAGE_STAGING_META_BATCH_PUSH && (HZ4_REMOTE_BUMP_FREE_META_SHARDS == 1)
    if (n == 0) {
        return 0;
    }

    for (uint16_t i = 0; i < n; i++) {
        hz4_os_stats_rbmf_try();
    }

#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    if (atomic_load_explicit(&meta->bump_rfree_draining, memory_order_relaxed) != 0) {
        hz4_os_stats_rbmf_fail_draining();
        return 0;
    }

    uint16_t cur = atomic_load_explicit(&meta->bump_rfree_n, memory_order_relaxed);
    uint16_t take = 0;
    bool notify = false;
    for (;;) {
        if (cur >= HZ4_REMOTE_BUMP_FREE_META_CAP) {
            hz4_os_stats_rbmf_fail_full();
            return 0;
        }
        uint16_t room = (uint16_t)(HZ4_REMOTE_BUMP_FREE_META_CAP - cur);
        take = (n < room) ? n : room;
        uint16_t next = (uint16_t)(cur + take);
        if (atomic_compare_exchange_weak_explicit(&meta->bump_rfree_n,
                                                  &cur,
                                                  next,
                                                  memory_order_acq_rel,
                                                  memory_order_relaxed)) {
            notify = (cur == 0 && take > 0);
            break;
        }
    }

    for (uint16_t i = 0; i < take; i++) {
        meta->bump_rfree_idx[(uint16_t)(cur + i)] = idxs[i];
    }
    atomic_fetch_add_explicit(&meta->bump_rfree_pub, take, memory_order_release);

    for (uint16_t i = 0; i < take; i++) {
        hz4_os_stats_rbmf_ok();
    }
    if (notify) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
    if (take < n) {
        hz4_os_stats_rbmf_fail_full();
    }
    return take;
#else
#if HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
    if (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
        hz4_os_stats_rbmf_fail_lock();
        return 0;
    }
#else
    while (atomic_flag_test_and_set_explicit(&meta->bump_rfree_lock, memory_order_acquire)) {
    }
#endif

    uint16_t cur = atomic_load_explicit(&meta->bump_rfree_n, memory_order_relaxed);
    uint16_t take = 0;
    bool notify = false;
    if (cur < HZ4_REMOTE_BUMP_FREE_META_CAP) {
        uint16_t room = (uint16_t)(HZ4_REMOTE_BUMP_FREE_META_CAP - cur);
        take = (n < room) ? n : room;
        notify = (cur == 0 && take > 0);
        for (uint16_t i = 0; i < take; i++) {
            meta->bump_rfree_idx[(uint16_t)(cur + i)] = idxs[i];
        }
        atomic_store_explicit(&meta->bump_rfree_n, (uint16_t)(cur + take), memory_order_relaxed);
    }

    atomic_flag_clear_explicit(&meta->bump_rfree_lock, memory_order_release);

    if (take > 0) {
        for (uint16_t i = 0; i < take; i++) {
            hz4_os_stats_rbmf_ok();
        }
        if (notify) {
            hz4_remote_page_rbufq_notify(owner, sc, page, meta);
        }
    }
    if (take < n) {
        hz4_os_stats_rbmf_fail_full();
    }
    return take;
#endif
#else
    uint16_t i = 0;
    for (; i < n; i++) {
        if (!hz4_remote_bump_free_meta_try_push_idx(page, meta, idxs[i], owner, sc)) {
            break;
        }
    }
    return i;
#endif
}

// spill publish helper is defined in hz4_remote_page_rbuf_spill.inc.
static inline bool hz4_remote_page_staging_msgpass_try_publish(hz4_tls_t* tls,
                                                                uint8_t owner,
                                                                uint8_t sc,
                                                                hz4_page_t* page,
                                                                const uint16_t* idxs,
                                                                uint16_t begin,
                                                                uint16_t n);
static inline bool hz4_remote_page_staging_spill_msg_try_publish(uint8_t owner,
                                                                  uint8_t sc,
                                                                  hz4_page_t* page,
                                                                  const uint16_t* idxs,
                                                                  uint16_t begin,
                                                                  uint16_t n);

static inline void hz4_remote_page_staging_fallback_publish_idxs(hz4_tls_t* tls,
                                                                  hz4_page_t* page,
                                                                  hz4_page_meta_t* meta,
                                                                  uint8_t owner,
                                                                  uint8_t sc,
                                                                  const uint16_t* idxs,
                                                                  uint16_t begin,
                                                                  uint16_t n) {
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
    if (hz4_remote_page_staging_msgpass_try_publish(tls, owner, sc, page, idxs, begin, n)) {
        return;
    }
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
    if (hz4_remote_page_staging_spill_msg_try_publish(owner, sc, page, idxs, begin, n)) {
        return;
    }
#endif
    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    void* head = NULL;
    void* tail = NULL;
    uint32_t list_n = 0;
    for (uint16_t j = begin; j < n; j++) {
        void* obj = (void*)(start + (uintptr_t)idxs[j] * obj_size);
#if !HZ4_REMOTE_PAGE_STAGING_NO_CLEAR_NEXT
        hz4_obj_set_next(obj, NULL);
#endif
        if (!head) {
            head = obj;
            tail = obj;
        } else {
            hz4_obj_set_next(tail, obj);
            tail = obj;
        }
        list_n++;
    }

#if HZ4_CENTRAL_PAGEHEAP && HZ4_REMOTE_INBOX
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        hz4_inbox_push_list(tls, owner, sc, head, tail);
        return;
    }
#endif

    if (hz4_page_push_remote_list_tid_transitioned(page, head, tail, list_n, tls->tid)) {
        hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    }
}

static inline bool hz4_remote_page_staging_try_defer_idxs(hz4_remote_page_stash_ent_t* ent,
                                                           hz4_page_t* page,
                                                           const uint16_t* idxs,
                                                           uint16_t begin,
                                                           uint16_t n,
                                                           uint8_t prev_retries) {
#if HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK
    if (begin >= n) {
        return false;
    }
    uint16_t remain = (uint16_t)(n - begin);
    if (remain >= HZ4_REMOTE_PAGE_STAGING_MAX) {
        return false;
    }
    if (prev_retries >= HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY) {
        return false;
    }
    ent->page = page;
    ent->count = remain;
    ent->defer_retries = (uint8_t)(prev_retries + 1);
    for (uint16_t i = 0; i < remain; i++) {
        ent->idx[i] = idxs[(uint16_t)(begin + i)];
    }
    return true;
#else
    (void)ent;
    (void)page;
    (void)idxs;
    (void)begin;
    (void)n;
    (void)prev_retries;
    return false;
#endif
}
#endif

static inline void hz4_remote_page_staging_flush_ent(hz4_tls_t* tls,
                                                     hz4_remote_page_stash_ent_t* ent) {
    if (ent->count == 0) return;

#if HZ4_REMOTE_PAGE_STAGING_META
    hz4_page_t* page = ent->page;
    uint16_t n = ent->count;
    uint8_t prev_retries = ent->defer_retries;
    uint16_t idxs[HZ4_REMOTE_PAGE_STAGING_MAX];
    for (uint16_t i = 0; i < n; i++) {
        idxs[i] = ent->idx[i];
    }

    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
    uint8_t sc = (uint8_t)meta->sc;

    uint16_t pushed = hz4_remote_bump_free_meta_try_push_idx_batch(page, meta, idxs, n, owner, sc);
    if (pushed < n) {
        if (hz4_remote_page_staging_try_defer_idxs(ent, page, idxs, pushed, n, prev_retries)) {
            return;
        }
        hz4_remote_page_staging_reset_ent(ent);
        hz4_remote_page_staging_fallback_publish_idxs(tls, page, meta, owner, sc, idxs, pushed, n);
        return;
    }
    hz4_remote_page_staging_reset_ent(ent);
    return;
#else
    hz4_page_t* page = ent->page;
    void* head = ent->head;
    void* tail = ent->tail;
    uint32_t n = (uint32_t)ent->count;
    hz4_remote_page_staging_reset_ent(ent);

    hz4_page_meta_t* meta = hz4_page_meta(page);

#if HZ4_CENTRAL_PAGEHEAP && HZ4_REMOTE_INBOX
    // Disjointness: do not track CPH-managed pages in the rbufq.
    // Fallback to inbox (safe) to preserve correctness.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
        uint8_t sc = (uint8_t)meta->sc;
        hz4_inbox_push_list(tls, owner, sc, head, tail);
        return;
    }
#endif

    // Publish list to page.remote_head (1 CAS/list) and notify only on transition.
    bool transitioned = hz4_page_push_remote_list_tid_transitioned(page, head, tail, n, tls->tid);
    if (!transitioned) {
        return;
    }

    uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
    uint8_t sc = (uint8_t)meta->sc;
    hz4_remote_page_rbufq_notify(owner, sc, page, meta);
#endif
}

static inline void hz4_remote_page_staging_flush_all(hz4_tls_t* tls) {
    for (uint32_t i = 0; i < HZ4_REMOTE_PAGE_STAGING_CACHE_N; i++) {
        hz4_remote_page_staging_flush_ent(tls, &tls->remote_page_stash[i]);
    }
}
#endif

// Producer side: push directly to page.remote_head[] + notify (if needed).
static inline bool hz4_remote_page_rbuf_try_push(hz4_tls_t* tls,
                                                 hz4_page_t* page,
                                                 hz4_page_meta_t* meta,
                                                 void* ptr,
                                                 uint8_t owner,
                                                 uint8_t sc) {
#if HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // Gate OFF -> skip RBUF, fall through to next path (inbox/legacy)
    if (!tls->rbuf_gate_on) {
        return false;
    }
#endif

#if HZ4_CENTRAL_PAGEHEAP
    // Disjointness: do not track CPH-managed pages in this queue.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        return false;
    }
#endif

#if HZ4_REMOTE_BUMP_FREE_META
    hz4_remote_page_rbufq_lazy_fb_on_try(tls);
    if (hz4_remote_bump_free_meta_try_push(page, meta, ptr, owner, sc)) {
        return true;
    }
    hz4_os_stats_rbmf_fallback();
    hz4_remote_page_rbufq_lazy_fb_on_fallback(tls);
#endif
#if HZ4_REMOTE_FREE_META
    if (hz4_remote_free_meta_try_push(page, meta, ptr, owner, sc)) {
        return true;
    }
#endif

#if HZ4_REMOTE_PAGE_STAGING
#if !HZ4_REMOTE_BUMP_FREE_META
    (void)owner;
    (void)sc;
    (void)meta;
#endif

    uint32_t idx = hz4_remote_page_staging_idx(page);
    hz4_remote_page_stash_ent_t* ent = &tls->remote_page_stash[idx];

#if HZ4_REMOTE_PAGE_STAGING_META
    uint16_t obj_idx = 0;
    if (!hz4_remote_page_staging_calc_idx(page, meta, ptr, sc, &obj_idx)) {
        return false;
    }
    if (ent->count == 0) {
        ent->page = page;
        ent->idx[0] = obj_idx;
        ent->count = 1;
        ent->defer_retries = 0;
    } else if (ent->page == page) {
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
            if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
                return false;
            }
            if (ent->count == 0) {
                ent->page = page;
                ent->idx[0] = obj_idx;
                ent->count = 1;
                ent->defer_retries = 0;
                if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
                    hz4_remote_page_staging_flush_all(tls);
                }
                return true;
            }
        }
        ent->idx[ent->count] = obj_idx;
        ent->count++;
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
        }
    } else {
        // Evict: flush old entry first, then install new.
        hz4_remote_page_staging_flush_ent(tls, ent);
        if (ent->count != 0) {
            // Keep deferred old page entry; fall back current pointer to non-staging path.
            return false;
        }
        ent->page = page;
        ent->idx[0] = obj_idx;
        ent->count = 1;
        ent->defer_retries = 0;
    }

    if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
        hz4_remote_page_staging_flush_all(tls);
    }
    return true;
#else
    // Ensure obj is a proper list node.
#if !HZ4_REMOTE_PAGE_STAGING_NO_CLEAR_NEXT
    hz4_obj_set_next(ptr, NULL);
#endif

    if (ent->count == 0) {
        ent->page = page;
        ent->head = ptr;
        ent->tail = ptr;
        ent->count = 1;
    } else if (ent->page == page) {
        hz4_obj_set_next(ent->tail, ptr);
        ent->tail = ptr;
        ent->count++;
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
        }
    } else {
        // Evict: flush old entry first, then install new.
        hz4_remote_page_staging_flush_ent(tls, ent);
        ent->page = page;
        ent->head = ptr;
        ent->tail = ptr;
        ent->count = 1;
    }

    if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
        hz4_remote_page_staging_flush_all(tls);
    }
    return true;
#endif
#else
    // NOTE: transitioned は shard 単位。page 既に queued の場合は notify が抑止される。
    bool transitioned = hz4_page_push_remote_one_tid_transitioned(page, ptr, tls->tid);
    if (!transitioned) {
        return true;
    }
    hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    return true;
#endif
}

#include "hz4_remote_page_rbuf_spill.inc"
#include "hz4_remote_page_rbuf_drain.inc"

#endif // HZ4_REMOTE_PAGE_RBUF

#endif // HZ4_REMOTE_PAGE_RBUF_INC
