// hz4_tls_init.c - HZ4 TLS Init Box
#include <stdatomic.h>

#include "hz4_tls_init.h"

#if HZ4_TLS_DIRECT
// 実体定義（extern 対応）
__thread hz4_tls_t g_hz4_tls;
__thread uint8_t g_hz4_tls_inited = 0;
static _Atomic(uint16_t) g_hz4_tid_next = 1;

void hz4_tls_init_slow(void) {
    uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
    hz4_tls_init(&g_hz4_tls, tid);
    g_hz4_tls_inited = 1;
}
#else
// Legacy 実装
__thread hz4_tls_t tls;
static __thread uint8_t g_hz4_tls_inited = 0;
static _Atomic(uint16_t) g_hz4_tid_next = 1;

hz4_tls_t* hz4_tls_get(void) {
    if (!g_hz4_tls_inited) {
        uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
        hz4_tls_init(&tls, tid);
        g_hz4_tls_inited = 1;
    }
    return &tls;
}
#endif
