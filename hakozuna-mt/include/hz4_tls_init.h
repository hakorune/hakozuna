// hz4_tls_init.h - HZ4 TLS Init Box
#ifndef HZ4_TLS_INIT_H
#define HZ4_TLS_INIT_H

#include "hz4_config.h"
#include "hz4_tls.h"

#if HZ4_TLS_DIRECT
// Direct TLS access: extern 宣言 + inline getter
extern __thread hz4_tls_t g_hz4_tls;
extern __thread uint8_t g_hz4_tls_inited;

// Slow path: 実際の初期化（separate TU, noinline で hot path 分離）
void hz4_tls_init_slow(void) __attribute__((noinline));

static inline hz4_tls_t* hz4_tls_get(void) {
    if (__builtin_expect(!g_hz4_tls_inited, 0)) {
        hz4_tls_init_slow();
    }
    return &g_hz4_tls;
}
#else
// Legacy: PLT 経由
hz4_tls_t* hz4_tls_get(void);
#endif

#endif // HZ4_TLS_INIT_H
