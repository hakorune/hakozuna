#include "hz4_seg.h"  // for hz4_page_meta_t complete definition
#include "hz4_central_pageheap.h"
#include <stdatomic.h>

#if HZ4_CENTRAL_PAGEHEAP

hz4_cph_sc_t g_cph_sc[HZ4_SC_MAX];
#if HZ4_CPH_2TIER
hz4_cph_sc_t g_cph_hot_sc[HZ4_SC_MAX];
hz4_cph_sc_t g_cph_cold_sc[HZ4_SC_MAX];
_Atomic(uint32_t) g_cph_hot_count[HZ4_SC_MAX];
#endif

void hz4_cph_init(void) {
    for (int i = 0; i < HZ4_SC_MAX; i++) {
        atomic_flag_clear_explicit(&g_cph_sc[i].lock, memory_order_relaxed);
        g_cph_sc[i].top = NULL;
#if HZ4_CPH_2TIER
        atomic_flag_clear_explicit(&g_cph_hot_sc[i].lock, memory_order_relaxed);
        g_cph_hot_sc[i].top = NULL;
        atomic_flag_clear_explicit(&g_cph_cold_sc[i].lock, memory_order_relaxed);
        g_cph_cold_sc[i].top = NULL;
        atomic_store_explicit(&g_cph_hot_count[i], 0, memory_order_relaxed);
#endif
    }
}

void hz4_cph_cleanup(void) {
    // shutdown時の必要なクリーンアップがあれば
}

#endif
