// hz4_collect_page_drain.inc - PageDrainBox (Page-level Object Collection)
// Box Theory: CollectBox の下層（page単位の drain / inbox accounting）
//
// 重要: ここは「挙動を変えない refactor-only」前提で、元の hz4_collect.inc 実装をそのまま移したもの。
// - hz4_inbox_account_* は owner-thread のみ
// - hz4_drain_page / hz4_drain_page_list は remote_head[] → (out[] or list) 変換の境界
// - carry slot への退避や pushback の挙動は元実装と同一

#ifndef HZ4_COLLECT_PAGE_DRAIN_INC
#define HZ4_COLLECT_PAGE_DRAIN_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_os.h"
#include "hz4_sizeclass.h"

// Forward declarations (DecommitQueueBox)
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_decommit_queue_enqueue(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta);
#endif

// ============================================================================
// Inbox accounting (owner thread): dec used_count, optionally enqueue for decommit
// ============================================================================
#if HZ4_PAGE_META_SEPARATE
static inline void hz4_inbox_account_array(hz4_tls_t* tls, void** out, uint32_t n) {
    (void)tls;
    for (uint32_t i = 0; i < n; i++) {
        hz4_page_t* page = hz4_page_from_ptr(out[i]);
        uint16_t used;
#if HZ4_INBOX_ACCOUNT_FASTDEC
        hz4_page_meta_t* meta = hz4_page_meta(page);
        used = hz4_page_used_dec_meta(meta, 1);
#else
        hz4_page_used_dec(page, 1);
        used = hz4_page_used_count(page);
#endif
#if HZ4_PAGE_DECOMMIT
        if (used == 0) {
            hz4_os_stats_used_zero();
#if !HZ4_INBOX_ACCOUNT_FASTDEC
            hz4_page_meta_t* meta = hz4_page_meta(page);
#endif
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            atomic_store_explicit(&meta->cph_state, HZ4_CPH_SEALING, memory_order_relaxed);
            meta->seal_epoch = (uint32_t)tls->collect_count;
            hz4_os_stats_cph_seal_enter();
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_DQ_ENQUEUE_ON_INBOX_ZERO
            hz4_decommit_queue_enqueue(tls, page, meta);
#endif
            hz4_pageq_notify(page);
        }
#endif
        (void)used;
    }
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.page_used_dec_calls += n;
#else
    hz4_os_stats_page_used_dec(n);
#endif
#endif
}

static inline void hz4_inbox_account_list(hz4_tls_t* tls, void* head, uint32_t n) {
    (void)tls;
    void* cur = head;
    uint32_t i = 0;
    while (cur && i < n) {
        void* next = hz4_obj_get_next(cur);
        hz4_page_t* page = hz4_page_from_ptr(cur);
        uint16_t used;
#if HZ4_INBOX_ACCOUNT_FASTDEC
        hz4_page_meta_t* meta = hz4_page_meta(page);
        used = hz4_page_used_dec_meta(meta, 1);
#else
        hz4_page_used_dec(page, 1);
        used = hz4_page_used_count(page);
#endif
#if HZ4_PAGE_DECOMMIT
        if (used == 0) {
            hz4_os_stats_used_zero();
#if !HZ4_INBOX_ACCOUNT_FASTDEC
            hz4_page_meta_t* meta = hz4_page_meta(page);
#endif
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            atomic_store_explicit(&meta->cph_state, HZ4_CPH_SEALING, memory_order_relaxed);
            meta->seal_epoch = (uint32_t)tls->collect_count;
            hz4_os_stats_cph_seal_enter();
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_DQ_ENQUEUE_ON_INBOX_ZERO
            hz4_decommit_queue_enqueue(tls, page, meta);
#endif
            hz4_pageq_notify(page);
        }
#endif
        (void)used;
        cur = next;
        i++;
    }
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.page_used_dec_calls += i;
#else
    hz4_os_stats_page_used_dec(i);
#endif
#endif
}
#endif // HZ4_PAGE_META_SEPARATE

#if HZ4_REMOTE_FREE_META
static inline uint32_t hz4_remote_free_meta_drain_array(uint8_t sc,
                                                        hz4_page_t* page,
                                                        hz4_page_meta_t* meta,
                                                        void** out,
                                                        uint32_t budget) {
    if (budget == 0) return 0;
    if (atomic_load_explicit(&meta->remote_free_n, memory_order_acquire) == 0) {
        return 0;
    }

    while (atomic_flag_test_and_set_explicit(&meta->remote_free_lock, memory_order_acquire)) {
    }
    uint16_t n = atomic_load_explicit(&meta->remote_free_n, memory_order_relaxed);
    uint32_t take = (n < budget) ? n : budget;
    if (take == 0) {
        atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_release);
        return 0;
    }

    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    for (uint32_t i = 0; i < take; i++) {
        uint16_t idx = meta->remote_free_idx[--n];
#if HZ4_FAILFAST
        if (idx >= meta->capacity) {
            HZ4_FAIL("remote_free_meta_drain_array: idx out of range");
        }
#endif
        out[i] = (void*)(start + ((uintptr_t)idx * obj_size));
    }
    atomic_store_explicit(&meta->remote_free_n, n, memory_order_relaxed);
    atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_release);
    return take;
}

#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_free_meta_drain_list(uint8_t sc,
                                                       hz4_page_t* page,
                                                       hz4_page_meta_t* meta,
                                                       void** head_out,
                                                       void** tail_out,
                                                       uint32_t budget) {
    *head_out = NULL;
    *tail_out = NULL;
    if (budget == 0) return 0;
    if (atomic_load_explicit(&meta->remote_free_n, memory_order_acquire) == 0) {
        return 0;
    }

    while (atomic_flag_test_and_set_explicit(&meta->remote_free_lock, memory_order_acquire)) {
    }
    uint16_t n = atomic_load_explicit(&meta->remote_free_n, memory_order_relaxed);
    uint32_t take = (n < budget) ? n : budget;
    if (take == 0) {
        atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_release);
        return 0;
    }

    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    void* head = NULL;
    void* tail = NULL;
#if HZ4_REMOTE_FREE_META_DRAIN_REVERSE
    for (uint32_t i = 0; i < take; i++) {
        uint16_t idx = meta->remote_free_idx[--n];
#if HZ4_FAILFAST
        if (idx >= meta->capacity) {
            HZ4_FAIL("remote_free_meta_drain_list: idx out of range");
        }
#endif
        void* obj = (void*)(start + ((uintptr_t)idx * obj_size));
        if (!tail) {
            tail = obj;
        }
        hz4_obj_set_next(obj, head);
        head = obj;
    }
#else
    for (uint32_t i = 0; i < take; i++) {
        uint16_t idx = meta->remote_free_idx[--n];
#if HZ4_FAILFAST
        if (idx >= meta->capacity) {
            HZ4_FAIL("remote_free_meta_drain_list: idx out of range");
        }
#endif
        void* obj = (void*)(start + ((uintptr_t)idx * obj_size));
        hz4_obj_set_next(obj, NULL);
        if (!head) {
            head = obj;
            tail = obj;
        } else {
            hz4_obj_set_next(tail, obj);
            tail = obj;
        }
    }
#endif
    atomic_store_explicit(&meta->remote_free_n, n, memory_order_relaxed);
    atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_release);

    *head_out = head;
    *tail_out = tail;
    return take;
}
#endif // HZ4_COLLECT_LIST
#endif // HZ4_REMOTE_FREE_META

// ============================================================================
// Page drain: remote_head[] から objects を回収 (owner のみ)
// ============================================================================
static inline uint32_t hz4_drain_page(hz4_tls_t* tls, uint8_t sc,
                                      hz4_page_t* page,
                                      void** out, uint32_t budget) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.drain_page_calls++;
#else
    hz4_os_stats_drain_page_call();
#endif
#endif
    uint32_t got = 0;

#if HZ4_REMOTE_FREE_META
    if (got < budget) {
        hz4_page_meta_t* meta = hz4_page_meta(page);
        uint32_t meta_got = hz4_remote_free_meta_drain_array(sc, page, meta, out + got, budget - got);
        got += meta_got;
        if (got >= budget) {
            goto done;
        }
    }
#endif

    uint64_t mask = hz4_page_remote_mask(page);

    // Drain shards with pending hint only (when enabled)
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS && got < budget; s++) {
#if HZ4_REMOTE_MASK
        if ((mask & (1ULL << s)) == 0) {
            continue;
        }
#else
        (void)mask;
#endif
        void* list = hz4_page_drain_remote(page, s);

        // Walk list with bounded count (n 本固定)
        // Note: 本来は list の長さを知るべきだが、ここでは budget で制限
        uint32_t count = 0;
        while (list && got < budget && count < 1024) {  // safety bound
            void* next = hz4_obj_get_next(list);
            out[got++] = list;
            list = next;
            count++;
        }

        if (!list) {
#if HZ4_REMOTE_MASK
            hz4_page_clear_remote_hint(page, s);
            if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                hz4_page_mark_remote(page, s);
            }
#endif
            continue;
        }

        // If list has remainder, try to push to carry slots
        if (list) {
            hz4_carry_t* c = &tls->carry[sc];

            // Try to push to carry slot (if room)
            if (sc < HZ4_SC_MAX && c->n < HZ4_CARRY_SLOTS) {
#if HZ4_FAILFAST
                if (c->n >= HZ4_CARRY_SLOTS) {
                    HZ4_FAIL("carry push: slots full but called");
                }
#endif
                hz4_carry_slot_t* slot = &c->slot[c->n];
                slot->head = list;
                slot->page = page;
                c->n++;
                goto done;
            }

            // Fallback: pushback only if carry slots are full
            void* tail = list;
            uint32_t rem = 1;
            void* cur = hz4_obj_get_next(list);
            while (cur && rem < 1024) {
                tail = cur;
                cur = hz4_obj_get_next(cur);
                rem++;
            }
            hz4_obj_set_next(tail, NULL);
            hz4_page_push_remote_list_tid(page, list, tail, rem, tls->tid);
#if !HZ4_PAGEQ_ENABLE
            // Pending bits path (pageq disabled)
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_pending_set(seg, hz4_page_idx(page));
#endif
        }
    }

done:
#if HZ4_PAGE_META_SEPARATE
    if (got > 0) {
#if HZ4_DECOMMIT_OBSERVE
        hz4_page_meta_t* drain_meta = hz4_page_meta(page);
        uint16_t before = drain_meta->used_count;
#endif
        hz4_page_used_dec(page, got);
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.page_used_dec_calls += got;
#else
        hz4_os_stats_page_used_dec(got);
#endif
#endif
  #if HZ4_DECOMMIT_OBSERVE
        fprintf(stderr, "[HZ4_DRAIN] page=%p got=%u used_count %u->%u\n",
                (void*)page, got, before, drain_meta->used_count);
  #endif
    }
#endif
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.drain_page_objs += got;
#else
    hz4_os_stats_drain_page_objs(got);
#endif
#endif
    return got;
}

// ============================================================================
// List-mode helpers (P4.1): carry/inbox/page drain list
// ============================================================================
#if HZ4_COLLECT_LIST

// hz4_drain_page_list: page の remote_head[] から list を直接取得
// Note: hz4_list_splice() は CollectBox 側（list-mode）で定義される。
static inline uint32_t hz4_drain_page_list(hz4_tls_t* tls, uint8_t sc,
                                           hz4_page_t* page,
                                           void** head_out, void** tail_out,
                                           uint32_t budget) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.drain_page_calls++;
#else
    hz4_os_stats_drain_page_call();
#endif
#endif
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

#if HZ4_REMOTE_FREE_META
    if (got < budget) {
        hz4_page_meta_t* meta = hz4_page_meta(page);
        void* mh = NULL;
        void* mt = NULL;
        uint32_t meta_got = hz4_remote_free_meta_drain_list(sc, page, meta, &mh, &mt, budget - got);
        if (meta_got > 0) {
            head = mh;
            tail = mt;
            got = meta_got;
            if (got >= budget) {
                *head_out = head;
                *tail_out = tail;
                goto done_list;
            }
        }
    }
#endif

    uint64_t mask = hz4_page_remote_mask(page);

    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS && got < budget; s++) {
#if HZ4_REMOTE_MASK
        if ((mask & (1ULL << s)) == 0) continue;
#else
        (void)mask;
#endif
        void* list = hz4_page_drain_remote(page, s);
        if (!list) {
#if HZ4_REMOTE_MASK
            hz4_page_clear_remote_hint(page, s);
            if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                hz4_page_mark_remote(page, s);
            }
#endif
            continue;
        }

        // Walk list to extract prefix (up to remaining budget)
        void* cur = list;
        void* prefix_tail = NULL;
        uint32_t count = 0;
        uint32_t remaining = budget - got;
        while (cur && count < remaining && count < 1024) {
            prefix_tail = cur;
            void* next = hz4_obj_get_next(cur);
#if HZ4_COLLECT_PREFETCH
            if (next) {
                __builtin_prefetch(next, 0, 3);
            }
#endif
            cur = next;
            count++;
        }

        // Splice prefix to result
        if (prefix_tail) {
            hz4_obj_set_next(prefix_tail, NULL);  // Cut prefix
            hz4_list_splice(&head, &tail, &got, list, prefix_tail, count);
        }

        // Handle remainder (cur points to remainder)
        if (cur) {
            hz4_carry_t* c = &tls->carry[sc];
            if (sc < HZ4_SC_MAX && c->n < HZ4_CARRY_SLOTS) {
                // Push to carry
                hz4_carry_slot_t* slot = &c->slot[c->n];
                slot->head = cur;
                slot->page = page;
                c->n++;
            } else {
                // Pushback to page
                void* rem_tail = cur;
                uint32_t rem = 1;
                void* next = hz4_obj_get_next(cur);
                while (next && rem < 1024) {
                    rem_tail = next;
                    next = hz4_obj_get_next(next);
                    rem++;
                }
                hz4_obj_set_next(rem_tail, NULL);
                hz4_page_push_remote_list_tid(page, cur, rem_tail, rem, tls->tid);
            }
        }
    }

    *head_out = head;
    *tail_out = tail;

#if HZ4_REMOTE_FREE_META
done_list:
#endif
#if HZ4_PAGE_META_SEPARATE
    if (got > 0) {
        hz4_page_used_dec(page, got);
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.page_used_dec_calls += got;
#else
        hz4_os_stats_page_used_dec(got);
#endif
#endif
    }
#endif
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.drain_page_objs += got;
#else
    hz4_os_stats_drain_page_objs(got);
#endif
#endif
    return got;
}

#endif // HZ4_COLLECT_LIST

#endif // HZ4_COLLECT_PAGE_DRAIN_INC
