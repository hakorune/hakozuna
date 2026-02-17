// hz4_remote_page_rbuf_drain.inc - RemotePageRbufBox drain/consume slice
#ifndef HZ4_REMOTE_PAGE_RBUF_DRAIN_INC
#define HZ4_REMOTE_PAGE_RBUF_DRAIN_INC

static inline hz4_page_t* hz4_remote_page_rbufq_pop_all(uint8_t owner, uint8_t sc) {
    // Fast empty check: avoid atomic_exchange when queue is empty (common case).
    // Safe under races: a concurrent producer may enqueue after this load; it will be consumed
    // by a later collect. No correctness risk (only potential delay).
    _Atomic(hz4_page_t*)* slot = &g_hz4_remote_page_rbufq[owner][sc];
    hz4_page_t* head = atomic_load_explicit(slot, memory_order_acquire);
    if (!head) return NULL;
    return atomic_exchange_explicit(slot, NULL, memory_order_acq_rel);
}

static inline void hz4_remote_page_rbufq_push_list(uint8_t owner, uint8_t sc,
                                                   hz4_page_t* head, hz4_page_t* tail) {
    if (!head) return;
#if HZ4_FAILFAST
    if (owner >= HZ4_NUM_SHARDS) {
        HZ4_FAIL("remote_page_rbufq_push_list: owner out of range");
    }
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("remote_page_rbufq_push_list: sc out of range");
    }
    if (!tail) {
        HZ4_FAIL("remote_page_rbufq_push_list: tail is NULL");
    }
#endif

    hz4_page_t* old = atomic_load_explicit(&g_hz4_remote_page_rbufq[owner][sc], memory_order_acquire);
    hz4_page_meta_t* tail_meta = hz4_page_meta(tail);
    do {
        tail_meta->rbufq_next = old;
    } while (!atomic_compare_exchange_weak_explicit(
        &g_hz4_remote_page_rbufq[owner][sc], &old, head,
        memory_order_release, memory_order_acquire));
}

static inline bool hz4_remote_page_rbuf_any(hz4_page_meta_t* meta) {
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
            return true;
        }
    }
#if HZ4_REMOTE_BUMP_FREE_META
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    for (uint32_t s = 0; s < HZ4_REMOTE_BUMP_FREE_META_SHARDS; s++) {
        if (atomic_load_explicit(&meta->bump_rfree_n[s], memory_order_acquire) != 0) {
            return true;
        }
    }
#else
    if (atomic_load_explicit(&meta->bump_rfree_n, memory_order_acquire) != 0) {
        return true;
    }
#endif
#endif
#if HZ4_REMOTE_FREE_META
    if (atomic_load_explicit(&meta->remote_free_n, memory_order_acquire) != 0) {
        return true;
    }
#endif
    return false;
}

static inline uint32_t hz4_remote_page_rbuf_consume(hz4_tls_t* tls,
                                                    uint8_t owner,
                                                    uint8_t sc,
                                                    void** out,
                                                    uint32_t budget) {
    if (budget == 0) return 0;
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("remote_page_rbuf_consume: sc out of range");
    }
#endif

    hz4_page_t* pagelist = hz4_remote_page_rbufq_pop_all(owner, sc);
    hz4_page_t* requeue_head = NULL;
    hz4_page_t* requeue_tail = NULL;
    uint32_t got = 0;
    uint32_t pages_left = HZ4_REMOTE_PAGE_RBUF_DRAIN_PAGE_BUDGET;
    if (pages_left == 0) pages_left = UINT32_MAX;

    bool msg_first = true;
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ || \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ
    msg_first = (pagelist == NULL);
#endif
    if (msg_first) {
        uint32_t msg_got = hz4_remote_idx_msgq_consume_array(owner, sc, out + got, budget - got);
        if (msg_got > 0) {
            got += msg_got;
            hz4_os_stats_rbuf_drain_objs(msg_got);
        }
    }

    while (pagelist && got < budget && pages_left > 0) {
        pages_left--;
        hz4_page_t* page = pagelist;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->rbufq_next;
        meta->rbufq_next = NULL;
        pagelist = nextp;

#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
#if HZ4_FAILFAST
            HZ4_FAIL("remote_page_rbuf_consume: page is in central pageheap");
#endif
            atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_release);
            hz4_remote_page_rbufq_lazy_clear(meta);
            continue;
        }
#endif

        uint32_t drained = 0;
        if (hz4_page_valid(page) && meta->sc == sc) {
#if HZ4_REMOTE_BUMP_FREE_META
            hz4_remote_bump_free_meta_drain(tls, sc, page, meta);
#endif
            drained = hz4_drain_page(tls, sc, page, out + got, budget - got);
            got += drained;
        }
        hz4_os_stats_rbuf_drain_objs(drained);
        hz4_remote_page_rbufq_lazy_drain0_on_try(tls, drained == 0);

        bool still = hz4_remote_page_rbuf_any(meta);
        if (drained > 0) {
            hz4_remote_page_rbufq_empty_streak_reset(meta);
        } else if (!still) {
            hz4_remote_page_rbufq_empty_streak_bump(meta);
        }
        if (still) {
            // Keep queued=1 and requeue.
            if (!requeue_head) {
                requeue_head = page;
                requeue_tail = page;
            } else {
                hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                tail_meta->rbufq_next = page;
                requeue_tail = page;
            }
            continue;
        }

        if (hz4_remote_page_rbufq_lazy_hold_enabled(tls, meta, drained) &&
            hz4_remote_page_rbufq_lazy_should_hold(meta)) {
            if (!requeue_head) {
                requeue_head = page;
                requeue_tail = page;
            } else {
                hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                tail_meta->rbufq_next = page;
                requeue_tail = page;
            }
            continue;
        }

        // Clear queued first, then re-check to avoid stranding.
        atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_release);
        hz4_remote_page_rbufq_lazy_clear(meta);
        bool raced = hz4_remote_page_rbuf_any(meta);
        if (raced) {
            // Only requeue if we actually transitioned 0->1 (avoid double enqueue).
            if (atomic_exchange_explicit(&meta->rbufq_queued, 1, memory_order_acq_rel) == 0) {
                hz4_remote_page_rbufq_lazy_refresh(meta);
                if (!requeue_head) {
                    requeue_head = page;
                    requeue_tail = page;
                } else {
                    hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                    tail_meta->rbufq_next = page;
                    requeue_tail = page;
                }
            }
            continue;
        }

#if HZ4_PAGE_DECOMMIT
#if HZ4_DECOMMIT_DELAY_QUEUE
        if (meta->used_count == 0) {
            hz4_decommit_queue_enqueue(tls, page, meta);
        }
#else
        hz4_page_try_decommit(page, meta);
#if HZ4_DECOMMIT_REUSE_POOL
        if (meta->decommitted) {
            hz4_reuse_pool_push(tls, page, meta);
        }
#endif
#endif
#endif
    }

    bool allow_msg_drain = true;
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY || \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY
    allow_msg_drain = (got == 0);
#endif
    if (!msg_first && allow_msg_drain && got < budget) {
        uint32_t msg_got = hz4_remote_idx_msgq_consume_array(owner, sc, out + got, budget - got);
        if (msg_got > 0) {
            got += msg_got;
            hz4_os_stats_rbuf_drain_objs(msg_got);
        }
    }

    // Budget ran out: requeue remaining pages (keep queued=1).
    while (pagelist) {
        hz4_page_t* page = pagelist;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->rbufq_next;
        meta->rbufq_next = NULL;
        pagelist = nextp;
        atomic_store_explicit(&meta->rbufq_queued, 1, memory_order_release);
        hz4_remote_page_rbufq_lazy_refresh(meta);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
            tail_meta->rbufq_next = page;
            requeue_tail = page;
        }
    }

    if (requeue_head) {
        hz4_remote_page_rbufq_push_list(owner, sc, requeue_head, requeue_tail);
    }

    return got;
}

#if HZ4_COLLECT_LIST
static inline void hz4_remote_page_rbuf_list_splice(void** head, void** tail, uint32_t* n,
                                                    void* h2, void* t2, uint32_t n2) {
    if (!h2) return;
    if (!*head) {
        *head = h2;
        *tail = t2;
    } else {
        hz4_obj_set_next(*tail, h2);
        *tail = t2;
    }
    *n += n2;
}

static inline uint32_t hz4_remote_page_rbuf_consume_list(hz4_tls_t* tls,
                                                         uint8_t owner,
                                                         uint8_t sc,
                                                         void** head_out,
                                                         void** tail_out,
                                                         uint32_t budget) {
    if (budget == 0) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("remote_page_rbuf_consume_list: sc out of range");
    }
#endif

    hz4_page_t* pagelist = hz4_remote_page_rbufq_pop_all(owner, sc);
    hz4_page_t* requeue_head = NULL;
    hz4_page_t* requeue_tail = NULL;
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;
    uint32_t pages_left = HZ4_REMOTE_PAGE_RBUF_DRAIN_PAGE_BUDGET;
    if (pages_left == 0) pages_left = UINT32_MAX;

    bool msg_first = true;
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ || \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ
    msg_first = (pagelist == NULL);
#endif
    if (msg_first) {
        uint32_t msg_got = hz4_remote_idx_msgq_consume_list(owner, sc, &head, &tail, budget - got);
        if (msg_got > 0) {
            got += msg_got;
            hz4_os_stats_rbuf_drain_objs(msg_got);
        }
    }

    while (pagelist && got < budget && pages_left > 0) {
        pages_left--;
        hz4_page_t* page = pagelist;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->rbufq_next;
        meta->rbufq_next = NULL;
        pagelist = nextp;

#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
#if HZ4_FAILFAST
            HZ4_FAIL("remote_page_rbuf_consume_list: page is in central pageheap");
#endif
            atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_release);
            hz4_remote_page_rbufq_lazy_clear(meta);
            continue;
        }
#endif

        uint32_t drained = 0;
        if (hz4_page_valid(page) && meta->sc == sc) {
            void* ph = NULL;
            void* pt = NULL;
#if HZ4_REMOTE_BUMP_FREE_META
            hz4_remote_bump_free_meta_drain(tls, sc, page, meta);
#endif
            drained = hz4_drain_page_list(tls, sc, page, &ph, &pt, budget - got);
            if (drained > 0) {
                hz4_remote_page_rbuf_list_splice(&head, &tail, &got, ph, pt, drained);
            }
        }
        hz4_os_stats_rbuf_drain_objs(drained);
        hz4_remote_page_rbufq_lazy_drain0_on_try(tls, drained == 0);

        bool still = hz4_remote_page_rbuf_any(meta);
        if (drained > 0) {
            hz4_remote_page_rbufq_empty_streak_reset(meta);
        } else if (!still) {
            hz4_remote_page_rbufq_empty_streak_bump(meta);
        }
        if (still) {
            if (!requeue_head) {
                requeue_head = page;
                requeue_tail = page;
            } else {
                hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                tail_meta->rbufq_next = page;
                requeue_tail = page;
            }
            continue;
        }

        if (hz4_remote_page_rbufq_lazy_hold_enabled(tls, meta, drained) &&
            hz4_remote_page_rbufq_lazy_should_hold(meta)) {
            if (!requeue_head) {
                requeue_head = page;
                requeue_tail = page;
            } else {
                hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                tail_meta->rbufq_next = page;
                requeue_tail = page;
            }
            continue;
        }

        atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_release);
        hz4_remote_page_rbufq_lazy_clear(meta);
        bool raced = hz4_remote_page_rbuf_any(meta);
        if (raced) {
            if (atomic_exchange_explicit(&meta->rbufq_queued, 1, memory_order_acq_rel) == 0) {
                hz4_remote_page_rbufq_lazy_refresh(meta);
                if (!requeue_head) {
                    requeue_head = page;
                    requeue_tail = page;
                } else {
                    hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
                    tail_meta->rbufq_next = page;
                    requeue_tail = page;
                }
            }
            continue;
        }

#if HZ4_PAGE_DECOMMIT
#if HZ4_DECOMMIT_DELAY_QUEUE
        if (meta->used_count == 0) {
            hz4_decommit_queue_enqueue(tls, page, meta);
        }
#else
        hz4_page_try_decommit(page, meta);
#if HZ4_DECOMMIT_REUSE_POOL
        if (meta->decommitted) {
            hz4_reuse_pool_push(tls, page, meta);
        }
#endif
#endif
#endif
    }

    bool allow_msg_drain = true;
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY || \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY
    allow_msg_drain = (got == 0);
#endif
    if (!msg_first && allow_msg_drain && got < budget) {
        void* mh = NULL;
        void* mt = NULL;
        uint32_t msg_got = hz4_remote_idx_msgq_consume_list(owner, sc, &mh, &mt, budget - got);
        if (msg_got > 0) {
            hz4_remote_page_rbuf_list_splice(&head, &tail, &got, mh, mt, msg_got);
            hz4_os_stats_rbuf_drain_objs(msg_got);
        }
    }

    while (pagelist) {
        hz4_page_t* page = pagelist;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->rbufq_next;
        meta->rbufq_next = NULL;
        pagelist = nextp;
        atomic_store_explicit(&meta->rbufq_queued, 1, memory_order_release);
        hz4_remote_page_rbufq_lazy_refresh(meta);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            hz4_page_meta_t* tail_meta = hz4_page_meta(requeue_tail);
            tail_meta->rbufq_next = page;
            requeue_tail = page;
        }
    }

    if (requeue_head) {
        hz4_remote_page_rbufq_push_list(owner, sc, requeue_head, requeue_tail);
    }

    *head_out = head;
    *tail_out = tail;
    return got;
}
#endif // HZ4_COLLECT_LIST

#endif // HZ4_REMOTE_PAGE_RBUF_DRAIN_INC
