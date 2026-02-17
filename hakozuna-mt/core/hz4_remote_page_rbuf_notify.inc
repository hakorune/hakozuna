// hz4_remote_page_rbuf_notify.inc - RemotePageRbufBox notify/lazy slice
#ifndef HZ4_REMOTE_PAGE_RBUF_NOTIFY_INC
#define HZ4_REMOTE_PAGE_RBUF_NOTIFY_INC

#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
static inline void hz4_remote_page_rbufq_lazy_refresh(hz4_page_meta_t* meta) {
    atomic_store_explicit(&meta->rbufq_lazy_left,
                          (uint8_t)(HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL - 1),
                          memory_order_relaxed);
}

static inline void hz4_remote_page_rbufq_lazy_clear(hz4_page_meta_t* meta) {
    atomic_store_explicit(&meta->rbufq_lazy_left, 0, memory_order_relaxed);
}

static inline bool hz4_remote_page_rbufq_lazy_should_hold(hz4_page_meta_t* meta) {
    uint8_t left = atomic_load_explicit(&meta->rbufq_lazy_left, memory_order_relaxed);
    if (left == 0) {
        return false;
    }
    atomic_store_explicit(&meta->rbufq_lazy_left, (uint8_t)(left - 1), memory_order_relaxed);
    return true;
}

static inline void hz4_remote_page_rbufq_empty_streak_reset(hz4_page_meta_t* meta) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX > 0
    atomic_store_explicit(&meta->rbufq_empty_streak, 0, memory_order_relaxed);
#else
    (void)meta;
#endif
}

static inline void hz4_remote_page_rbufq_empty_streak_bump(hz4_page_meta_t* meta) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX > 0
    uint8_t cur = atomic_load_explicit(&meta->rbufq_empty_streak, memory_order_relaxed);
    if (cur < 255) {
        atomic_store_explicit(&meta->rbufq_empty_streak, (uint8_t)(cur + 1), memory_order_relaxed);
    }
#else
    (void)meta;
#endif
}

static inline void hz4_remote_page_rbufq_lazy_fb_on_try(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0
    uint32_t tries = ++tls->rbufq_lazy_fb_tries;
    if ((tries & (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES - 1)) != 0) {
        return;
    }

    uint32_t total = tries;
    uint32_t fallback = tls->rbufq_lazy_fb_fallback;
    tls->rbufq_lazy_fb_tries = 0;
    tls->rbufq_lazy_fb_fallback = 0;

    if (tls->rbufq_lazy_fb_hold_left > 0) {
        tls->rbufq_lazy_fb_hold_left--;
        return;
    }

    if (fallback * 100 >= HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT * total &&
        !tls->rbufq_lazy_fb_on) {
        tls->rbufq_lazy_fb_on = 1;
        tls->rbufq_lazy_fb_hold_left = HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS;
    } else if (fallback * 100 <= HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT * total &&
               tls->rbufq_lazy_fb_on) {
        tls->rbufq_lazy_fb_on = 0;
        tls->rbufq_lazy_fb_hold_left = HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS;
    }
#else
    (void)tls;
#endif
}

static inline void hz4_remote_page_rbufq_lazy_fb_on_fallback(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0
    if (tls->rbufq_lazy_fb_fallback < UINT32_MAX) {
        tls->rbufq_lazy_fb_fallback++;
    }
#else
    (void)tls;
#endif
}

static inline void hz4_remote_page_rbufq_lazy_drain0_on_try(hz4_tls_t* tls, bool drain0) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0
    if (drain0 && tls->rbufq_lazy_drain0_hits < UINT32_MAX) {
        tls->rbufq_lazy_drain0_hits++;
    }
    uint32_t tries = ++tls->rbufq_lazy_drain0_tries;
    if ((tries & (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES - 1)) != 0) {
        return;
    }

    uint32_t total = tries;
    uint32_t hits = tls->rbufq_lazy_drain0_hits;
    tls->rbufq_lazy_drain0_tries = 0;
    tls->rbufq_lazy_drain0_hits = 0;

    if (tls->rbufq_lazy_drain0_hold_left > 0) {
        tls->rbufq_lazy_drain0_hold_left--;
        return;
    }

    if (((uint64_t)hits * 100u) >= ((uint64_t)HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT * total) &&
        !tls->rbufq_lazy_drain0_on) {
        tls->rbufq_lazy_drain0_on = 1;
        tls->rbufq_lazy_drain0_hold_left = HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS;
    } else if (((uint64_t)hits * 100u) <=
                   ((uint64_t)HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT * total) &&
               tls->rbufq_lazy_drain0_on) {
        tls->rbufq_lazy_drain0_on = 0;
        tls->rbufq_lazy_drain0_hold_left = HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS;
    }
#else
    (void)tls;
    (void)drain0;
#endif
}

static inline bool hz4_remote_page_rbufq_lazy_fb_blocked(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0
    return tls->rbufq_lazy_fb_on != 0;
#else
    (void)tls;
    return false;
#endif
}

static inline bool hz4_remote_page_rbufq_lazy_drain0_blocked(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0
    return tls->rbufq_lazy_drain0_on != 0;
#else
    (void)tls;
    return false;
#endif
}

static inline bool hz4_remote_page_rbufq_lazy_hold_enabled(hz4_tls_t* tls, hz4_page_meta_t* meta, uint32_t drained) {
    if (drained < HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_MIN_DRAIN) {
        return false;
    }
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX > 0
    if (atomic_load_explicit(&meta->rbufq_empty_streak, memory_order_relaxed) >=
        HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX) {
        return false;
    }
#else
    (void)meta;
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0
    if (hz4_remote_page_rbufq_lazy_fb_blocked(tls)) {
        return false;
    }
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0
    if (hz4_remote_page_rbufq_lazy_drain0_blocked(tls)) {
        return false;
    }
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS > 0
    if (!tls->rbuf_gate_on) {
        return false;
    }
    if (tls->rbuf_gate_on_windows < HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS) {
        return false;
    }
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ONLY
    if (!tls->rbuf_gate_on) {
        return false;
    }
#else
    (void)tls;
#endif
    return true;
}
#else
static inline void hz4_remote_page_rbufq_lazy_refresh(hz4_page_meta_t* meta) {
    (void)meta;
}

static inline void hz4_remote_page_rbufq_lazy_clear(hz4_page_meta_t* meta) {
    (void)meta;
}

static inline bool hz4_remote_page_rbufq_lazy_should_hold(hz4_page_meta_t* meta) {
    (void)meta;
    return false;
}

static inline void hz4_remote_page_rbufq_empty_streak_reset(hz4_page_meta_t* meta) {
    (void)meta;
}

static inline void hz4_remote_page_rbufq_empty_streak_bump(hz4_page_meta_t* meta) {
    (void)meta;
}

static inline void hz4_remote_page_rbufq_lazy_fb_on_try(hz4_tls_t* tls) {
    (void)tls;
}

static inline void hz4_remote_page_rbufq_lazy_fb_on_fallback(hz4_tls_t* tls) {
    (void)tls;
}

static inline bool hz4_remote_page_rbufq_lazy_fb_blocked(hz4_tls_t* tls) {
    (void)tls;
    return false;
}

static inline void hz4_remote_page_rbufq_lazy_drain0_on_try(hz4_tls_t* tls, bool drain0) {
    (void)tls;
    (void)drain0;
}

static inline bool hz4_remote_page_rbufq_lazy_drain0_blocked(hz4_tls_t* tls) {
    (void)tls;
    return false;
}

static inline bool hz4_remote_page_rbufq_lazy_hold_enabled(hz4_tls_t* tls, hz4_page_meta_t* meta, uint32_t drained) {
    (void)tls;
    (void)meta;
    (void)drained;
    return false;
}
#endif

static inline void hz4_remote_page_rbufq_notify(uint8_t owner, uint8_t sc,
                                                hz4_page_t* page, hz4_page_meta_t* meta) {
    if (atomic_exchange_explicit(&meta->rbufq_queued, 1, memory_order_acq_rel) != 0) {
        hz4_os_stats_rbuf_notify_already_queued();
        return;
    }
    hz4_os_stats_rbuf_notify_transition();
    hz4_remote_page_rbufq_push(owner, sc, page, meta);
    hz4_remote_page_rbufq_lazy_refresh(meta);
}

#endif // HZ4_REMOTE_PAGE_RBUF_NOTIFY_INC
