#if HZ4_RSSRETURN_PRESSUREGATEBOX
void hz4_os_stats_rss_gate_on_window(void) {
    hz4_os_stats_init_once();
    atomic_fetch_add_explicit(&g_hz4_os_rss_gate_on_windows, 1, memory_order_relaxed);
}

void hz4_os_stats_rss_gate_seg_acq_delta(uint32_t delta) {
    hz4_os_stats_init_once();
    uint64_t old = atomic_load_explicit(&g_hz4_os_rss_gate_seg_acq_delta_max, memory_order_relaxed);
    while ((uint64_t)delta > old) {
        if (atomic_compare_exchange_weak_explicit(&g_hz4_os_rss_gate_seg_acq_delta_max, &old,
                                                  (uint64_t)delta,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}
#endif

#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
// Include hz4_tls.h for hz4_tls_t and hz4_os_stats_tls_t definitions
#include "hz4_tls.h"

void hz4_os_stats_tls_flush(struct hz4_tls* tls) {
    hz4_os_stats_init_once();
    hz4_os_stats_tls_t* st = &tls->stats_tls;

    atomic_fetch_add_explicit(&g_hz4_os_collect_calls, st->collect_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_refill_calls, st->refill_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_consume_calls, st->inbox_consume_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_scan_calls, st->inbox_lite_scan_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_lite_shortcut, st->inbox_lite_shortcut, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_carry_hit, st->carry_hit, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_carry_miss, st->carry_miss, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_calls, st->drain_page_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_drain_page_objs, st->drain_page_objs, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_page_used_dec_calls, st->page_used_dec_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_calls, st->remote_flush_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_n1, st->remote_flush_n1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_fastpath_le4, st->remote_flush_le4, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_probe_overflow, st->remote_flush_probe_ovf, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_one_calls, st->inbox_push_one_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_inbox_push_list_calls, st->inbox_push_list_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_1, st->rf_rlen_1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_2_4, st->rf_rlen_2_4, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_5_8, st->rf_rlen_5_8, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_9_16, st->rf_rlen_9_16, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_17_32, st->rf_rlen_17_32, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_33_64, st->rf_rlen_33_64, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_65_127, st->rf_rlen_65_127, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz4_os_remote_flush_rlen_ge_threshold, st->rf_rlen_ge_th, memory_order_relaxed);

    // Clear TLS counters after flush
    *st = (hz4_os_stats_tls_t){0};
}
#endif
