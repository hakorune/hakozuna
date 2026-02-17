// hz4_tls_init.c - HZ4 TLS Init Box
#include <stdatomic.h>

#include "hz4_tls_init.h"
#if HZ4_CENTRAL_PAGEHEAP
#include "hz4_seg.h"  // for hz4_page_meta_t complete definition
#include "hz4_central_pageheap.h"
#endif

#if HZ4_TLS_DIRECT
static _Atomic(uint16_t) g_hz4_tid_next = 1;
__thread hz4_tls_t g_hz4_tls;
__thread uint8_t g_hz4_tls_inited = 0;

#if HZ4_CENTRAL_PAGEHEAP
static _Atomic(uint8_t) g_cph_inited = 0;
#endif

void hz4_tls_init_slow(void) {
#if HZ4_CENTRAL_PAGEHEAP
    // One-time global CPH initialization
    if (atomic_exchange_explicit(&g_cph_inited, 1, memory_order_acq_rel) == 0) {
        hz4_cph_init();
    }
#endif
    uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
    hz4_tls_init(&g_hz4_tls, tid);
    g_hz4_tls_inited = 1;
}

#else
// Legacy 実装 (HZ4_TLS_DIRECT=0)
__thread hz4_tls_t tls;
static __thread uint8_t g_hz4_tls_inited = 0;
static _Atomic(uint16_t) g_hz4_tid_next = 1;

#if HZ4_CENTRAL_PAGEHEAP
static _Atomic(uint8_t) g_cph_inited = 0;
#endif

hz4_tls_t* hz4_tls_get(void) {
    if (!g_hz4_tls_inited) {
#if HZ4_CENTRAL_PAGEHEAP
        // One-time global CPH initialization
        if (atomic_exchange_explicit(&g_cph_inited, 1, memory_order_acq_rel) == 0) {
            hz4_cph_init();
        }
#endif
        uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
        hz4_tls_init(&tls, tid);
        g_hz4_tls_inited = 1;
    }
    return &tls;
}
#endif
