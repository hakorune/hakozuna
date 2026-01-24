#include "hz3_watch_ptr.h"

#if HZ3_WATCH_PTR_BOX
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

enum {
    HZ3_WATCH_STATE_INIT = 0,
    HZ3_WATCH_STATE_ALLOC = 1,
    HZ3_WATCH_STATE_FREE = 2,
};

static _Atomic(uintptr_t) g_hz3_watch_ptr = 0;
static _Atomic(uintptr_t) g_hz3_watch_ptr_mask = ~(uintptr_t)0;
static _Atomic(int) g_hz3_watch_ptr_inited = 0;
static _Atomic(int) g_hz3_watch_ptr_shot = 0;
static _Atomic(int) g_hz3_watch_ptr_state = HZ3_WATCH_STATE_INIT;
static int g_hz3_watch_ptr_auto = 0;
static int g_hz3_watch_ptr_sc = -1;
static int g_hz3_watch_ptr_owner = -1;

static const char* hz3_watch_ptr_state_name(int state) {
    switch (state) {
        case HZ3_WATCH_STATE_INIT:
            return "init";
        case HZ3_WATCH_STATE_ALLOC:
            return "alloc";
        case HZ3_WATCH_STATE_FREE:
            return "free";
        default:
            return "unknown";
    }
}

static unsigned long long hz3_watch_ptr_gettid(void) {
    return (unsigned long long)(uintptr_t)pthread_self();
}

static uintptr_t hz3_watch_ptr_parse(const char* s) {
    if (!s || !*s) {
        return 0;
    }
    errno = 0;
    char* end = NULL;
    unsigned long long val = strtoull(s, &end, 0);
    if (errno != 0 || end == s) {
        return 0;
    }
    return (uintptr_t)val;
}

static int hz3_watch_ptr_parse_int(const char* s, int defval) {
    if (!s || !*s) {
        return defval;
    }
    errno = 0;
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (errno != 0 || end == s) {
        return defval;
    }
    return (int)v;
}

void hz3_watch_ptr_init_once(void) {
    if (atomic_exchange_explicit(&g_hz3_watch_ptr_inited, 1, memory_order_acq_rel) != 0) {
        return;
    }
    const char* s = getenv("HZ3_WATCH_PTR");
    uintptr_t ptr = hz3_watch_ptr_parse(s);
    if (ptr == 0) {
        g_hz3_watch_ptr_auto = hz3_watch_ptr_parse_int(getenv("HZ3_WATCH_PTR_AUTO"), 0);
        g_hz3_watch_ptr_sc = hz3_watch_ptr_parse_int(getenv("HZ3_WATCH_PTR_SC"), -1);
        g_hz3_watch_ptr_owner = hz3_watch_ptr_parse_int(getenv("HZ3_WATCH_PTR_OWNER"), -1);
        return;
    }
    const char* mask_s = getenv("HZ3_WATCH_PTR_MASK");
    if (mask_s && *mask_s) {
        uintptr_t mask = hz3_watch_ptr_parse(mask_s);
        atomic_store_explicit(&g_hz3_watch_ptr_mask, mask, memory_order_release);
    }
    atomic_store_explicit(&g_hz3_watch_ptr, ptr, memory_order_release);
    fprintf(stderr, "[HZ3_WATCH_PTR] enabled ptr=%p mask=%p\n",
            (void*)ptr, (void*)atomic_load_explicit(&g_hz3_watch_ptr_mask, memory_order_acquire));
}

static void hz3_watch_ptr_hit(const char* where, void* ptr, int sc, int owner, int event) {
    if (ptr == NULL) {
        return;
    }
    if (atomic_load_explicit(&g_hz3_watch_ptr_inited, memory_order_acquire) == 0) {
        hz3_watch_ptr_init_once();
    }
    uintptr_t watch = atomic_load_explicit(&g_hz3_watch_ptr, memory_order_acquire);
    if (watch == 0 && g_hz3_watch_ptr_auto && event == HZ3_WATCH_STATE_ALLOC) {
        if ((g_hz3_watch_ptr_sc < 0 || g_hz3_watch_ptr_sc == sc) &&
            (g_hz3_watch_ptr_owner < 0 || g_hz3_watch_ptr_owner == owner)) {
            atomic_store_explicit(&g_hz3_watch_ptr, (uintptr_t)ptr, memory_order_release);
            fprintf(stderr, "[HZ3_WATCH_PTR] auto ptr=%p sc=%d owner=%d\n", ptr, sc, owner);
            watch = (uintptr_t)ptr;
        }
    }
    uintptr_t mask = atomic_load_explicit(&g_hz3_watch_ptr_mask, memory_order_acquire);
    if (watch == 0 || (((uintptr_t)ptr & mask) != (watch & mask))) {
        return;
    }

    int next_state = (event == HZ3_WATCH_STATE_ALLOC) ? HZ3_WATCH_STATE_ALLOC : HZ3_WATCH_STATE_FREE;
    int prev_state = atomic_exchange_explicit(&g_hz3_watch_ptr_state, next_state, memory_order_acq_rel);

    int violation = 0;
    if (event == HZ3_WATCH_STATE_ALLOC) {
        if (prev_state == HZ3_WATCH_STATE_ALLOC) {
            violation = 1;
        }
    } else {
        if (prev_state != HZ3_WATCH_STATE_ALLOC) {
            violation = 1;
        }
    }

    if (HZ3_WATCH_PTR_SHOT && !violation) {
        if (atomic_exchange_explicit(&g_hz3_watch_ptr_shot, 1, memory_order_acq_rel) != 0) {
            return;
        }
    }

    unsigned long long tid = hz3_watch_ptr_gettid();
    fprintf(stderr,
            "[HZ3_WATCH_PTR] tid=%llu where=%s ptr=%p sc=%d owner=%d event=%s prev=%s%s\n",
            tid,
            (where ? where : "?"),
            ptr,
            sc,
            owner,
            (event == HZ3_WATCH_STATE_ALLOC) ? "alloc" : "free",
            hz3_watch_ptr_state_name(prev_state),
            violation ? " violation=1" : "");

    if (violation && HZ3_WATCH_PTR_FAILFAST) {
        abort();
    }
}

void hz3_watch_ptr_on_alloc(const char* where, void* ptr, int sc, int owner) {
    hz3_watch_ptr_hit(where, ptr, sc, owner, HZ3_WATCH_STATE_ALLOC);
}

void hz3_watch_ptr_on_free(const char* where, void* ptr, int sc, int owner) {
    hz3_watch_ptr_hit(where, ptr, sc, owner, HZ3_WATCH_STATE_FREE);
}
#endif
