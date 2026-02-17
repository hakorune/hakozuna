// hz4_os.c - HZ4 OS Box (segment acquire/release)
#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hz4_config.h"
#include "hz4_os.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

#if HZ4_RSSRETURN
// PressureGateBox 用常時カウンタ（HZ4_OS_STATSなしならこっちを使う）
atomic_uint_fast64_t g_hz4_os_segs_acquired_always = ATOMIC_VAR_INIT(0);
#endif

#include "hz4_os_seg_registry.inc"

int hz4_os_is_seg_ptr(const void* ptr) {
    return hz4_os_seg_registry_contains_ptr(ptr);
}

#if HZ4_OS_STATS
_Atomic uint64_t g_hz4_os_segs_acquired = ATOMIC_VAR_INIT(0);
static _Atomic(uint64_t) g_hz4_os_segs_released;
static _Atomic(uint64_t) g_hz4_os_pages_acquired;
static _Atomic(uint64_t) g_hz4_os_pages_released;
static _Atomic(uint64_t) g_hz4_os_pages_decommitted;
static _Atomic(uint64_t) g_hz4_os_pages_committed;
static _Atomic(uint64_t) g_hz4_os_large_acquired;
static _Atomic(uint64_t) g_hz4_os_large_released;
static _Atomic(uint64_t) g_hz4_os_decommit_attempt;
static _Atomic(uint64_t) g_hz4_os_decommit_success;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_uninit;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_already;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_used;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_cph;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_cph_queued;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_budget0;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_still_local;
static _Atomic(uint64_t) g_hz4_os_cph_seal_enter;
static _Atomic(uint64_t) g_hz4_os_cph_grace_block;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_cph_grace;
static _Atomic(uint64_t) g_hz4_os_cand_requeue_grace;
static _Atomic(uint64_t) g_hz4_os_cand_requeue_cph;
static _Atomic(uint64_t) g_hz4_os_cph_extract_ok;
static _Atomic(uint64_t) g_hz4_os_cph_extract_fail;
static _Atomic(uint64_t) g_hz4_os_cph_extract_fail_state;
static _Atomic(uint64_t) g_hz4_os_cph_extract_fail_queued;
static _Atomic(uint64_t) g_hz4_os_cph_extract_fail_notfound;
static _Atomic(uint64_t) g_hz4_os_cph_hot_push;
static _Atomic(uint64_t) g_hz4_os_cph_cold_push;
static _Atomic(uint64_t) g_hz4_os_cph_hot_pop;
static _Atomic(uint64_t) g_hz4_os_cph_cold_pop;
static _Atomic(uint64_t) g_hz4_os_cph_hot_full;
static _Atomic(uint64_t) g_hz4_os_dq_enqueue;
static _Atomic(uint64_t) g_hz4_os_dq_enqueue_already;
static _Atomic(uint64_t) g_hz4_os_dq_dequeue;
static _Atomic(uint64_t) g_hz4_os_dq_due_ready;
static _Atomic(uint64_t) g_hz4_os_dq_due_notready;
static _Atomic(uint64_t) g_hz4_os_dq_process_call;
static _Atomic(uint64_t) g_hz4_os_dq_process_skip_duegate;
static _Atomic(uint64_t) g_hz4_os_used_zero;
static _Atomic(uint64_t) g_hz4_os_rss_pending_max;
static _Atomic(uint64_t) g_hz4_os_rss_gate_fire;
static _Atomic(uint64_t) g_hz4_os_decommit_skip_budget0_pending;
static _Atomic(uint64_t) g_hz4_os_rss_cool_len_max;
static _Atomic(uint64_t) g_hz4_os_rss_cool_promote;
static _Atomic(uint64_t) g_hz4_os_rss_cool_drop_used;
static _Atomic(uint64_t) g_hz4_os_rss_release_fire;
static _Atomic(uint64_t) g_hz4_os_rss_cool_enq;
static _Atomic(uint64_t) g_hz4_os_rss_cool_block_deadline;
static _Atomic(uint64_t) g_hz4_os_rss_cool_sweep_budget_hit;
static _Atomic(uint64_t) g_hz4_os_rss_cool_skip_already;
static _Atomic(uint64_t) g_hz4_os_rss_quarantine_set;
static _Atomic(uint64_t) g_hz4_os_rss_quarantine_mature;
static _Atomic(uint64_t) g_hz4_os_rss_quarantine_drop_used;
#if HZ4_RSSRETURN_PRESSUREGATEBOX
static _Atomic(uint64_t) g_hz4_os_rss_gate_on_windows;  // ON だった window 数
static _Atomic(uint64_t) g_hz4_os_rss_gate_seg_acq_delta_max;  // 最大増分
#endif
static _Atomic(uint64_t) g_hz4_os_collect_calls;
static _Atomic(uint64_t) g_hz4_os_refill_calls;
static _Atomic(uint64_t) g_hz4_os_inbox_consume_calls;
static _Atomic(uint64_t) g_hz4_os_inbox_lite_scan_calls;
static _Atomic(uint64_t) g_hz4_os_inbox_lite_shortcut;
static _Atomic(uint64_t) g_hz4_os_inbox_stash_len_max;
static _Atomic(uint64_t) g_hz4_os_carry_hit;
static _Atomic(uint64_t) g_hz4_os_carry_miss;
static _Atomic(uint64_t) g_hz4_os_drain_page_calls;
static _Atomic(uint64_t) g_hz4_os_drain_page_objs;
static _Atomic(uint64_t) g_hz4_os_page_used_dec_calls;
static _Atomic(uint64_t) g_hz4_os_remote_flush_calls;
static _Atomic(uint64_t) g_hz4_os_remote_flush_fastpath_n1;
static _Atomic(uint64_t) g_hz4_os_remote_flush_fastpath_le4;
static _Atomic(uint64_t) g_hz4_os_remote_flush_probe_overflow;
static _Atomic(uint64_t) g_hz4_os_inbox_push_one_calls;
static _Atomic(uint64_t) g_hz4_os_inbox_push_list_calls;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_1;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_2_4;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_5_8;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_9_16;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_17_32;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_33_64;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_65_127;
static _Atomic(uint64_t) g_hz4_os_remote_flush_rlen_ge_threshold;
static _Atomic(uint64_t) g_hz4_os_remote_flush_compact_call;
static _Atomic(uint64_t) g_hz4_os_remote_flush_compact_hit;
static _Atomic(uint64_t) g_hz4_os_remote_flush_compact_objs;
static _Atomic(uint64_t) g_hz4_os_remote_flush_compact_groups;
static _Atomic(uint64_t) g_hz4_os_remote_flush_compact_fallback;
static _Atomic(uint64_t) g_hz4_os_remote_flush_direct_index_call;
static _Atomic(uint64_t) g_hz4_os_remote_flush_direct_index_objs;
static _Atomic(uint64_t) g_hz4_os_remote_flush_direct_index_groups;
static _Atomic(uint64_t) g_hz4_os_rbmf_try;
static _Atomic(uint64_t) g_hz4_os_rbmf_ok;
static _Atomic(uint64_t) g_hz4_os_rbmf_fail_guard;
static _Atomic(uint64_t) g_hz4_os_rbmf_fail_lock;
static _Atomic(uint64_t) g_hz4_os_rbmf_fail_full;
static _Atomic(uint64_t) g_hz4_os_rbmf_fail_draining;
static _Atomic(uint64_t) g_hz4_os_rbmf_fallback;
static _Atomic(uint64_t) g_hz4_os_rbuf_notify_transition;
static _Atomic(uint64_t) g_hz4_os_rbuf_notify_already_queued;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_0;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_1;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_2_4;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_5_8;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_9_16;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_17_32;
static _Atomic(uint64_t) g_hz4_os_rbuf_drain_33_plus;
static _Atomic(uint64_t) g_hz4_os_large_rescue_attempt;
static _Atomic(uint64_t) g_hz4_os_large_rescue_success;
static _Atomic(uint64_t) g_hz4_os_large_rescue_fail;
static _Atomic(uint64_t) g_hz4_os_large_rescue_fallback_flush;
static _Atomic(uint64_t) g_hz4_os_large_rescue_route_batch_hit;
static _Atomic(uint64_t) g_hz4_os_large_rescue_route_mmap_retry_ok;
static _Atomic(uint64_t) g_hz4_os_large_rescue_route_mmap_retry_fail;
static _Atomic(uint64_t) g_hz4_os_large_rescue_gate_skip;
static _Atomic(uint64_t) g_hz4_os_large_rescue_gate_force;
static _Atomic(uint64_t) g_hz4_os_large_owner_remote;
static _Atomic(uint64_t) g_hz4_os_large_owner_hit;
static _Atomic(uint64_t) g_hz4_os_large_owner_miss;
static _Atomic(uint64_t) g_hz4_os_large_owner_skip_pages;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_acq_self_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_acq_steal_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_acq_miss;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_rel_self_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_rel_steal_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_rel_miss;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_acq_self_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_acq_steal_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_acq_miss;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_rel_self_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_rel_steal_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_p2_rel_miss;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_hint_acq_try;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_hint_acq_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_hint_rel_try;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_hint_rel_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_steal_probe;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_steal_skip_budget;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_steal_hit;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_mask_skip;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_mask_fallback;
static _Atomic(uint64_t) g_hz4_os_large_lock_shard_mask_fallback_hit;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_acq_hit;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_acq_miss;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_rel_hit;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_rel_miss;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_acq_call;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_acq_take_list;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_acq_stash_hit;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_rel_flush_call;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_rel_flush_objs;
static _Atomic(uint64_t) g_hz4_os_large_pagebin_b15_rel_cas_retry;
static _Atomic(uint64_t) g_hz4_os_large_remote_free_seen;
static _Atomic(uint64_t) g_hz4_os_large_remote_bypass_spancache;
static _Atomic(uint64_t) g_hz4_os_large_remote_bypass_pages1;
static _Atomic(uint64_t) g_hz4_os_large_remote_bypass_pages2;
static _Atomic(uint64_t) g_hz4_os_large_remote_bypass_cache_hit;
static _Atomic(uint64_t) g_hz4_os_large_remote_bypass_cache_miss;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_acq_hit;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_acq_miss;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_rel_hit;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_rel_miss;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_rel_drop_cap;
static _Atomic(uint64_t) g_hz4_os_large_extent_b16_bytes_peak;
static _Atomic(int) g_hz4_os_stats_atexit_inited;

static void hz4_os_stats_dump_atexit(void) {
    uint64_t sa = atomic_load_explicit(&g_hz4_os_segs_acquired, memory_order_relaxed);
    uint64_t sr = atomic_load_explicit(&g_hz4_os_segs_released, memory_order_relaxed);
    uint64_t pa = atomic_load_explicit(&g_hz4_os_pages_acquired, memory_order_relaxed);
    uint64_t pr = atomic_load_explicit(&g_hz4_os_pages_released, memory_order_relaxed);
    uint64_t pd = atomic_load_explicit(&g_hz4_os_pages_decommitted, memory_order_relaxed);
    uint64_t pc = atomic_load_explicit(&g_hz4_os_pages_committed, memory_order_relaxed);
    uint64_t la = atomic_load_explicit(&g_hz4_os_large_acquired, memory_order_relaxed);
    uint64_t lr = atomic_load_explicit(&g_hz4_os_large_released, memory_order_relaxed);
    uint64_t da = atomic_load_explicit(&g_hz4_os_decommit_attempt, memory_order_relaxed);
    uint64_t ds = atomic_load_explicit(&g_hz4_os_decommit_success, memory_order_relaxed);
    uint64_t dsu = atomic_load_explicit(&g_hz4_os_decommit_skip_uninit, memory_order_relaxed);
    uint64_t dsa = atomic_load_explicit(&g_hz4_os_decommit_skip_already, memory_order_relaxed);
    uint64_t dsused = atomic_load_explicit(&g_hz4_os_decommit_skip_used, memory_order_relaxed);
    uint64_t dscph = atomic_load_explicit(&g_hz4_os_decommit_skip_cph, memory_order_relaxed);
    uint64_t dscph_queued = atomic_load_explicit(&g_hz4_os_decommit_skip_cph_queued, memory_order_relaxed);
    uint64_t dsb0 = atomic_load_explicit(&g_hz4_os_decommit_skip_budget0, memory_order_relaxed);
    uint64_t dslocal = atomic_load_explicit(&g_hz4_os_decommit_skip_still_local, memory_order_relaxed);
    uint64_t cph_seal = atomic_load_explicit(&g_hz4_os_cph_seal_enter, memory_order_relaxed);
    uint64_t cph_grace = atomic_load_explicit(&g_hz4_os_cph_grace_block, memory_order_relaxed);
    uint64_t dscph_grace = atomic_load_explicit(&g_hz4_os_decommit_skip_cph_grace, memory_order_relaxed);
    uint64_t cand_requeue_grace = atomic_load_explicit(&g_hz4_os_cand_requeue_grace, memory_order_relaxed);
    uint64_t cand_requeue_cph = atomic_load_explicit(&g_hz4_os_cand_requeue_cph, memory_order_relaxed);
    uint64_t cph_extract_ok = atomic_load_explicit(&g_hz4_os_cph_extract_ok, memory_order_relaxed);
    uint64_t cph_extract_fail = atomic_load_explicit(&g_hz4_os_cph_extract_fail, memory_order_relaxed);
    uint64_t cph_extract_fail_state = atomic_load_explicit(&g_hz4_os_cph_extract_fail_state, memory_order_relaxed);
    uint64_t cph_extract_fail_queued = atomic_load_explicit(&g_hz4_os_cph_extract_fail_queued, memory_order_relaxed);
    uint64_t cph_extract_fail_notfound = atomic_load_explicit(&g_hz4_os_cph_extract_fail_notfound, memory_order_relaxed);
    uint64_t cph_hot_push = atomic_load_explicit(&g_hz4_os_cph_hot_push, memory_order_relaxed);
    uint64_t cph_cold_push = atomic_load_explicit(&g_hz4_os_cph_cold_push, memory_order_relaxed);
    uint64_t cph_hot_pop = atomic_load_explicit(&g_hz4_os_cph_hot_pop, memory_order_relaxed);
    uint64_t cph_cold_pop = atomic_load_explicit(&g_hz4_os_cph_cold_pop, memory_order_relaxed);
    uint64_t cph_hot_full = atomic_load_explicit(&g_hz4_os_cph_hot_full, memory_order_relaxed);
    uint64_t dq_enq = atomic_load_explicit(&g_hz4_os_dq_enqueue, memory_order_relaxed);
    uint64_t dq_enq_already = atomic_load_explicit(&g_hz4_os_dq_enqueue_already, memory_order_relaxed);
    uint64_t dq_deq = atomic_load_explicit(&g_hz4_os_dq_dequeue, memory_order_relaxed);
    uint64_t dq_due_ready = atomic_load_explicit(&g_hz4_os_dq_due_ready, memory_order_relaxed);
    uint64_t dq_due_notready = atomic_load_explicit(&g_hz4_os_dq_due_notready, memory_order_relaxed);
    uint64_t dq_process_call = atomic_load_explicit(&g_hz4_os_dq_process_call, memory_order_relaxed);
    uint64_t dq_process_skip_duegate = atomic_load_explicit(&g_hz4_os_dq_process_skip_duegate, memory_order_relaxed);
    uint64_t used_zero = atomic_load_explicit(&g_hz4_os_used_zero, memory_order_relaxed);
    uint64_t rss_pending_max = atomic_load_explicit(&g_hz4_os_rss_pending_max, memory_order_relaxed);
    uint64_t gate_fire = atomic_load_explicit(&g_hz4_os_rss_gate_fire, memory_order_relaxed);
    uint64_t budget0_pending = atomic_load_explicit(&g_hz4_os_decommit_skip_budget0_pending, memory_order_relaxed);
    uint64_t cool_len_max = atomic_load_explicit(&g_hz4_os_rss_cool_len_max, memory_order_relaxed);
    uint64_t cool_promote = atomic_load_explicit(&g_hz4_os_rss_cool_promote, memory_order_relaxed);
    uint64_t cool_drop_used = atomic_load_explicit(&g_hz4_os_rss_cool_drop_used, memory_order_relaxed);
    uint64_t release_fire = atomic_load_explicit(&g_hz4_os_rss_release_fire, memory_order_relaxed);
    uint64_t cool_enq = atomic_load_explicit(&g_hz4_os_rss_cool_enq, memory_order_relaxed);
    uint64_t cool_block_deadline = atomic_load_explicit(&g_hz4_os_rss_cool_block_deadline, memory_order_relaxed);
    uint64_t cool_sweep_budget_hit = atomic_load_explicit(&g_hz4_os_rss_cool_sweep_budget_hit, memory_order_relaxed);
    uint64_t cool_skip_already = atomic_load_explicit(&g_hz4_os_rss_cool_skip_already, memory_order_relaxed);
    uint64_t q_set = atomic_load_explicit(&g_hz4_os_rss_quarantine_set, memory_order_relaxed);
    uint64_t q_mature = atomic_load_explicit(&g_hz4_os_rss_quarantine_mature, memory_order_relaxed);
    uint64_t q_drop_used = atomic_load_explicit(&g_hz4_os_rss_quarantine_drop_used, memory_order_relaxed);
#if HZ4_RSSRETURN_PRESSUREGATEBOX
    uint64_t gate_on_windows = atomic_load_explicit(&g_hz4_os_rss_gate_on_windows, memory_order_relaxed);
    uint64_t gate_delta_max = atomic_load_explicit(&g_hz4_os_rss_gate_seg_acq_delta_max, memory_order_relaxed);
#endif
    uint64_t collect_calls = atomic_load_explicit(&g_hz4_os_collect_calls, memory_order_relaxed);
    uint64_t refill_calls = atomic_load_explicit(&g_hz4_os_refill_calls, memory_order_relaxed);
    uint64_t inbox_consume_calls = atomic_load_explicit(&g_hz4_os_inbox_consume_calls, memory_order_relaxed);
    uint64_t inbox_lite_scan_calls = atomic_load_explicit(&g_hz4_os_inbox_lite_scan_calls, memory_order_relaxed);
    uint64_t inbox_lite_shortcut = atomic_load_explicit(&g_hz4_os_inbox_lite_shortcut, memory_order_relaxed);
    uint64_t inbox_stash_len_max = atomic_load_explicit(&g_hz4_os_inbox_stash_len_max, memory_order_relaxed);
    uint64_t carry_hit = atomic_load_explicit(&g_hz4_os_carry_hit, memory_order_relaxed);
    uint64_t carry_miss = atomic_load_explicit(&g_hz4_os_carry_miss, memory_order_relaxed);
    uint64_t drain_page_calls = atomic_load_explicit(&g_hz4_os_drain_page_calls, memory_order_relaxed);
    uint64_t drain_page_objs = atomic_load_explicit(&g_hz4_os_drain_page_objs, memory_order_relaxed);
    uint64_t page_used_dec_calls = atomic_load_explicit(&g_hz4_os_page_used_dec_calls, memory_order_relaxed);
    uint64_t remote_flush_calls = atomic_load_explicit(&g_hz4_os_remote_flush_calls, memory_order_relaxed);
    uint64_t remote_flush_n1 = atomic_load_explicit(&g_hz4_os_remote_flush_fastpath_n1, memory_order_relaxed);
    uint64_t remote_flush_le4 = atomic_load_explicit(&g_hz4_os_remote_flush_fastpath_le4, memory_order_relaxed);
    uint64_t remote_flush_probe_ovf = atomic_load_explicit(&g_hz4_os_remote_flush_probe_overflow, memory_order_relaxed);
    uint64_t inbox_push_one_calls = atomic_load_explicit(&g_hz4_os_inbox_push_one_calls, memory_order_relaxed);
    uint64_t inbox_push_list_calls = atomic_load_explicit(&g_hz4_os_inbox_push_list_calls, memory_order_relaxed);
    uint64_t rf_rlen_1 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_1, memory_order_relaxed);
    uint64_t rf_rlen_2_4 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_2_4, memory_order_relaxed);
    uint64_t rf_rlen_5_8 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_5_8, memory_order_relaxed);
    uint64_t rf_rlen_9_16 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_9_16, memory_order_relaxed);
    uint64_t rf_rlen_17_32 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_17_32, memory_order_relaxed);
    uint64_t rf_rlen_33_64 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_33_64, memory_order_relaxed);
    uint64_t rf_rlen_65_127 = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_65_127, memory_order_relaxed);
    uint64_t rf_rlen_ge_th = atomic_load_explicit(&g_hz4_os_remote_flush_rlen_ge_threshold, memory_order_relaxed);
    uint64_t rf_compact_call =
        atomic_load_explicit(&g_hz4_os_remote_flush_compact_call, memory_order_relaxed);
    uint64_t rf_compact_hit =
        atomic_load_explicit(&g_hz4_os_remote_flush_compact_hit, memory_order_relaxed);
    uint64_t rf_compact_objs =
        atomic_load_explicit(&g_hz4_os_remote_flush_compact_objs, memory_order_relaxed);
    uint64_t rf_compact_groups =
        atomic_load_explicit(&g_hz4_os_remote_flush_compact_groups, memory_order_relaxed);
    uint64_t rf_compact_fallback =
        atomic_load_explicit(&g_hz4_os_remote_flush_compact_fallback, memory_order_relaxed);
    uint64_t rf_direct_idx_call =
        atomic_load_explicit(&g_hz4_os_remote_flush_direct_index_call, memory_order_relaxed);
    uint64_t rf_direct_idx_objs =
        atomic_load_explicit(&g_hz4_os_remote_flush_direct_index_objs, memory_order_relaxed);
    uint64_t rf_direct_idx_groups =
        atomic_load_explicit(&g_hz4_os_remote_flush_direct_index_groups, memory_order_relaxed);
    uint64_t rbmf_try = atomic_load_explicit(&g_hz4_os_rbmf_try, memory_order_relaxed);
    uint64_t rbmf_ok = atomic_load_explicit(&g_hz4_os_rbmf_ok, memory_order_relaxed);
    uint64_t rbmf_fail_guard = atomic_load_explicit(&g_hz4_os_rbmf_fail_guard, memory_order_relaxed);
    uint64_t rbmf_fail_lock = atomic_load_explicit(&g_hz4_os_rbmf_fail_lock, memory_order_relaxed);
    uint64_t rbmf_fail_full = atomic_load_explicit(&g_hz4_os_rbmf_fail_full, memory_order_relaxed);
    uint64_t rbmf_fail_draining = atomic_load_explicit(&g_hz4_os_rbmf_fail_draining, memory_order_relaxed);
    uint64_t rbmf_fallback = atomic_load_explicit(&g_hz4_os_rbmf_fallback, memory_order_relaxed);
    uint64_t rbuf_notify_transition =
        atomic_load_explicit(&g_hz4_os_rbuf_notify_transition, memory_order_relaxed);
    uint64_t rbuf_notify_already_queued =
        atomic_load_explicit(&g_hz4_os_rbuf_notify_already_queued, memory_order_relaxed);
    uint64_t rbuf_drain_0 = atomic_load_explicit(&g_hz4_os_rbuf_drain_0, memory_order_relaxed);
    uint64_t rbuf_drain_1 = atomic_load_explicit(&g_hz4_os_rbuf_drain_1, memory_order_relaxed);
    uint64_t rbuf_drain_2_4 = atomic_load_explicit(&g_hz4_os_rbuf_drain_2_4, memory_order_relaxed);
    uint64_t rbuf_drain_5_8 = atomic_load_explicit(&g_hz4_os_rbuf_drain_5_8, memory_order_relaxed);
    uint64_t rbuf_drain_9_16 = atomic_load_explicit(&g_hz4_os_rbuf_drain_9_16, memory_order_relaxed);
    uint64_t rbuf_drain_17_32 =
        atomic_load_explicit(&g_hz4_os_rbuf_drain_17_32, memory_order_relaxed);
    uint64_t rbuf_drain_33_plus =
        atomic_load_explicit(&g_hz4_os_rbuf_drain_33_plus, memory_order_relaxed);
    uint64_t large_rescue_attempt =
        atomic_load_explicit(&g_hz4_os_large_rescue_attempt, memory_order_relaxed);
    uint64_t large_rescue_success =
        atomic_load_explicit(&g_hz4_os_large_rescue_success, memory_order_relaxed);
    uint64_t large_rescue_fail =
        atomic_load_explicit(&g_hz4_os_large_rescue_fail, memory_order_relaxed);
    uint64_t large_rescue_fallback_flush =
        atomic_load_explicit(&g_hz4_os_large_rescue_fallback_flush, memory_order_relaxed);
    uint64_t large_rescue_route_batch_hit =
        atomic_load_explicit(&g_hz4_os_large_rescue_route_batch_hit, memory_order_relaxed);
    uint64_t large_rescue_route_mmap_retry_ok =
        atomic_load_explicit(&g_hz4_os_large_rescue_route_mmap_retry_ok, memory_order_relaxed);
    uint64_t large_rescue_route_mmap_retry_fail =
        atomic_load_explicit(&g_hz4_os_large_rescue_route_mmap_retry_fail, memory_order_relaxed);
    uint64_t large_rescue_gate_skip =
        atomic_load_explicit(&g_hz4_os_large_rescue_gate_skip, memory_order_relaxed);
    uint64_t large_rescue_gate_force =
        atomic_load_explicit(&g_hz4_os_large_rescue_gate_force, memory_order_relaxed);
    uint64_t large_owner_remote =
        atomic_load_explicit(&g_hz4_os_large_owner_remote, memory_order_relaxed);
    uint64_t large_owner_hit =
        atomic_load_explicit(&g_hz4_os_large_owner_hit, memory_order_relaxed);
    uint64_t large_owner_miss =
        atomic_load_explicit(&g_hz4_os_large_owner_miss, memory_order_relaxed);
    uint64_t large_owner_skip_pages =
        atomic_load_explicit(&g_hz4_os_large_owner_skip_pages, memory_order_relaxed);
    uint64_t large_lock_shard_acq_self_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_acq_self_hit, memory_order_relaxed);
    uint64_t large_lock_shard_acq_steal_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_acq_steal_hit, memory_order_relaxed);
    uint64_t large_lock_shard_acq_miss =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_acq_miss, memory_order_relaxed);
    uint64_t large_lock_shard_rel_self_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_rel_self_hit, memory_order_relaxed);
    uint64_t large_lock_shard_rel_steal_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_rel_steal_hit, memory_order_relaxed);
    uint64_t large_lock_shard_rel_miss =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_rel_miss, memory_order_relaxed);
    uint64_t large_lock_shard_p2_acq_self_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_acq_self_hit, memory_order_relaxed);
    uint64_t large_lock_shard_p2_acq_steal_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_acq_steal_hit, memory_order_relaxed);
    uint64_t large_lock_shard_p2_acq_miss =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_acq_miss, memory_order_relaxed);
    uint64_t large_lock_shard_p2_rel_self_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_rel_self_hit, memory_order_relaxed);
    uint64_t large_lock_shard_p2_rel_steal_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_rel_steal_hit, memory_order_relaxed);
    uint64_t large_lock_shard_p2_rel_miss =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_p2_rel_miss, memory_order_relaxed);
    uint64_t large_lock_shard_hint_acq_try =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_hint_acq_try, memory_order_relaxed);
    uint64_t large_lock_shard_hint_acq_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_hint_acq_hit, memory_order_relaxed);
    uint64_t large_lock_shard_hint_rel_try =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_hint_rel_try, memory_order_relaxed);
    uint64_t large_lock_shard_hint_rel_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_hint_rel_hit, memory_order_relaxed);
    uint64_t large_lock_shard_steal_probe =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_steal_probe, memory_order_relaxed);
    uint64_t large_lock_shard_steal_skip_budget =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_steal_skip_budget, memory_order_relaxed);
    uint64_t large_lock_shard_steal_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_steal_hit, memory_order_relaxed);
    uint64_t large_lock_shard_mask_skip =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_mask_skip, memory_order_relaxed);
    uint64_t large_lock_shard_mask_fallback =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_mask_fallback, memory_order_relaxed);
    uint64_t large_lock_shard_mask_fallback_hit =
        atomic_load_explicit(&g_hz4_os_large_lock_shard_mask_fallback_hit, memory_order_relaxed);
    uint64_t large_pagebin_acq_hit =
        atomic_load_explicit(&g_hz4_os_large_pagebin_acq_hit, memory_order_relaxed);
    uint64_t large_pagebin_acq_miss =
        atomic_load_explicit(&g_hz4_os_large_pagebin_acq_miss, memory_order_relaxed);
    uint64_t large_pagebin_rel_hit =
        atomic_load_explicit(&g_hz4_os_large_pagebin_rel_hit, memory_order_relaxed);
    uint64_t large_pagebin_rel_miss =
        atomic_load_explicit(&g_hz4_os_large_pagebin_rel_miss, memory_order_relaxed);
    uint64_t large_pagebin_b15_acq_call =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_acq_call, memory_order_relaxed);
    uint64_t large_pagebin_b15_acq_take_list =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_acq_take_list, memory_order_relaxed);
    uint64_t large_pagebin_b15_acq_stash_hit =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_acq_stash_hit, memory_order_relaxed);
    uint64_t large_pagebin_b15_rel_flush_call =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_rel_flush_call, memory_order_relaxed);
    uint64_t large_pagebin_b15_rel_flush_objs =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_rel_flush_objs, memory_order_relaxed);
    uint64_t large_pagebin_b15_rel_cas_retry =
        atomic_load_explicit(&g_hz4_os_large_pagebin_b15_rel_cas_retry, memory_order_relaxed);
    uint64_t large_remote_free_seen =
        atomic_load_explicit(&g_hz4_os_large_remote_free_seen, memory_order_relaxed);
    uint64_t large_remote_bypass_spancache =
        atomic_load_explicit(&g_hz4_os_large_remote_bypass_spancache, memory_order_relaxed);
    uint64_t large_remote_bypass_pages1 =
        atomic_load_explicit(&g_hz4_os_large_remote_bypass_pages1, memory_order_relaxed);
    uint64_t large_remote_bypass_pages2 =
        atomic_load_explicit(&g_hz4_os_large_remote_bypass_pages2, memory_order_relaxed);
    uint64_t large_remote_bypass_cache_hit =
        atomic_load_explicit(&g_hz4_os_large_remote_bypass_cache_hit, memory_order_relaxed);
    uint64_t large_remote_bypass_cache_miss =
        atomic_load_explicit(&g_hz4_os_large_remote_bypass_cache_miss, memory_order_relaxed);
    uint64_t large_extent_b16_acq_hit =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_acq_hit, memory_order_relaxed);
    uint64_t large_extent_b16_acq_miss =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_acq_miss, memory_order_relaxed);
    uint64_t large_extent_b16_rel_hit =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_rel_hit, memory_order_relaxed);
    uint64_t large_extent_b16_rel_miss =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_rel_miss, memory_order_relaxed);
    uint64_t large_extent_b16_rel_drop_cap =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_rel_drop_cap, memory_order_relaxed);
    uint64_t large_extent_b16_bytes_peak =
        atomic_load_explicit(&g_hz4_os_large_extent_b16_bytes_peak, memory_order_relaxed);

    // One line SSOT: split into 3 fprintf to avoid argument limit
    fprintf(stderr,
            "[HZ4_OS_STATS] seg_acq=%llu seg_rel=%llu page_acq=%llu page_rel=%llu page_decommit=%llu page_commit=%llu large_acq=%llu large_rel=%llu d_attempt=%llu d_success=%llu d_skip_uninit=%llu d_skip_already=%llu d_skip_used=%llu d_skip_cph=%llu d_skip_cph_queued=%llu d_skip_budget0=%llu d_skip_still_local=%llu",
            (unsigned long long)sa, (unsigned long long)sr,
            (unsigned long long)pa, (unsigned long long)pr,
            (unsigned long long)pd, (unsigned long long)pc,
            (unsigned long long)la, (unsigned long long)lr,
            (unsigned long long)da, (unsigned long long)ds,
            (unsigned long long)dsu, (unsigned long long)dsa,
            (unsigned long long)dsused, (unsigned long long)dscph,
            (unsigned long long)dscph_queued,
            (unsigned long long)dsb0, (unsigned long long)dslocal);
    fprintf(stderr,
            " cph_seal=%llu cph_grace=%llu d_skip_cph_grace=%llu cand_requeue_grace=%llu cand_requeue_cph=%llu cph_extract_ok=%llu cph_extract_fail=%llu cph_extract_fail_state=%llu cph_extract_fail_queued=%llu cph_extract_fail_notfound=%llu cph_hot_push=%llu cph_cold_push=%llu cph_hot_pop=%llu cph_cold_pop=%llu cph_hot_full=%llu dq_enq=%llu dq_enq_already=%llu dq_deq=%llu dq_due_ready=%llu dq_due_notready=%llu used_zero=%llu rss_pending_max=%llu rss_gate_fire=%llu d_skip_budget0_pending=%llu rss_cool_len_max=%llu rss_cool_promote=%llu rss_cool_drop_used=%llu rss_release_fire=%llu rss_cool_enq=%llu rss_cool_block_deadline=%llu rss_cool_sweep_budget_hit=%llu rss_cool_skip_already=%llu rss_q_set=%llu rss_q_mature=%llu rss_q_drop_used=%llu",
            (unsigned long long)cph_seal, (unsigned long long)cph_grace,
            (unsigned long long)dscph_grace, (unsigned long long)cand_requeue_grace,
            (unsigned long long)cand_requeue_cph,
            (unsigned long long)cph_extract_ok, (unsigned long long)cph_extract_fail,
            (unsigned long long)cph_extract_fail_state,
            (unsigned long long)cph_extract_fail_queued,
            (unsigned long long)cph_extract_fail_notfound,
            (unsigned long long)cph_hot_push, (unsigned long long)cph_cold_push,
            (unsigned long long)cph_hot_pop, (unsigned long long)cph_cold_pop,
            (unsigned long long)cph_hot_full,
            (unsigned long long)dq_enq, (unsigned long long)dq_enq_already,
            (unsigned long long)dq_deq, (unsigned long long)dq_due_ready,
            (unsigned long long)dq_due_notready, (unsigned long long)used_zero,
            (unsigned long long)rss_pending_max, (unsigned long long)gate_fire,
            (unsigned long long)budget0_pending,
            (unsigned long long)cool_len_max, (unsigned long long)cool_promote,
            (unsigned long long)cool_drop_used, (unsigned long long)release_fire,
            (unsigned long long)cool_enq, (unsigned long long)cool_block_deadline,
            (unsigned long long)cool_sweep_budget_hit, (unsigned long long)cool_skip_already,
            (unsigned long long)q_set, (unsigned long long)q_mature, (unsigned long long)q_drop_used);
    fprintf(stderr,
            " collect_calls=%llu refill_calls=%llu inbox_consume=%llu inbox_lite_scan=%llu inbox_lite_shortcut=%llu inbox_stash_max=%llu carry_hit=%llu carry_miss=%llu drain_page_calls=%llu drain_page_objs=%llu page_used_dec=%llu remote_flush_calls=%llu rf_n1=%llu rf_le4=%llu rf_probe_ovf=%llu inbox_push_one=%llu inbox_push_list=%llu rlen1=%llu rlen2_4=%llu rlen5_8=%llu rlen9_16=%llu rlen17_32=%llu rlen33_64=%llu rlen65_127=%llu rlen_ge_th=%llu rbmf_try=%llu rbmf_ok=%llu rbmf_fail_guard=%llu rbmf_fail_lock=%llu rbmf_fail_full=%llu rbmf_fail_draining=%llu rbmf_fallback=%llu rbuf_notify_trans=%llu rbuf_notify_already=%llu rbuf_drain0=%llu rbuf_drain1=%llu rbuf_drain2_4=%llu rbuf_drain5_8=%llu rbuf_drain9_16=%llu rbuf_drain17_32=%llu rbuf_drain33_plus=%llu dq_process_call=%llu dq_process_skip_duegate=%llu"
#if HZ4_RSSRETURN_PRESSUREGATEBOX
            " rss_gate_on_windows=%llu rss_gate_seg_acq_delta_max=%llu"
#endif
            "\n",
            (unsigned long long)collect_calls, (unsigned long long)refill_calls,
            (unsigned long long)inbox_consume_calls,
            (unsigned long long)inbox_lite_scan_calls,
            (unsigned long long)inbox_lite_shortcut,
            (unsigned long long)inbox_stash_len_max,
            (unsigned long long)carry_hit, (unsigned long long)carry_miss,
            (unsigned long long)drain_page_calls, (unsigned long long)drain_page_objs,
            (unsigned long long)page_used_dec_calls,
            (unsigned long long)remote_flush_calls,
            (unsigned long long)remote_flush_n1, (unsigned long long)remote_flush_le4,
            (unsigned long long)remote_flush_probe_ovf,
            (unsigned long long)inbox_push_one_calls, (unsigned long long)inbox_push_list_calls,
            (unsigned long long)rf_rlen_1, (unsigned long long)rf_rlen_2_4,
            (unsigned long long)rf_rlen_5_8, (unsigned long long)rf_rlen_9_16,
            (unsigned long long)rf_rlen_17_32, (unsigned long long)rf_rlen_33_64,
            (unsigned long long)rf_rlen_65_127, (unsigned long long)rf_rlen_ge_th,
            (unsigned long long)rbmf_try, (unsigned long long)rbmf_ok,
            (unsigned long long)rbmf_fail_guard, (unsigned long long)rbmf_fail_lock,
            (unsigned long long)rbmf_fail_full, (unsigned long long)rbmf_fail_draining,
            (unsigned long long)rbmf_fallback,
            (unsigned long long)rbuf_notify_transition,
            (unsigned long long)rbuf_notify_already_queued,
            (unsigned long long)rbuf_drain_0, (unsigned long long)rbuf_drain_1,
            (unsigned long long)rbuf_drain_2_4, (unsigned long long)rbuf_drain_5_8,
            (unsigned long long)rbuf_drain_9_16, (unsigned long long)rbuf_drain_17_32,
            (unsigned long long)rbuf_drain_33_plus,
            (unsigned long long)dq_process_call, (unsigned long long)dq_process_skip_duegate
#if HZ4_RSSRETURN_PRESSUREGATEBOX
            , (unsigned long long)gate_on_windows, (unsigned long long)gate_delta_max
#endif
            );
    fprintf(stderr,
            "[HZ4_OS_STATS_B6] large_rescue_attempt=%llu large_rescue_success=%llu large_rescue_fail=%llu large_rescue_fallback_flush=%llu large_rescue_route_batch_hit=%llu large_rescue_route_mmap_retry_ok=%llu large_rescue_route_mmap_retry_fail=%llu\n",
            (unsigned long long)large_rescue_attempt,
            (unsigned long long)large_rescue_success,
            (unsigned long long)large_rescue_fail,
            (unsigned long long)large_rescue_fallback_flush,
            (unsigned long long)large_rescue_route_batch_hit,
            (unsigned long long)large_rescue_route_mmap_retry_ok,
            (unsigned long long)large_rescue_route_mmap_retry_fail);
    fprintf(stderr,
            "[HZ4_OS_STATS_B7] large_rescue_gate_skip=%llu large_rescue_gate_force=%llu\n",
            (unsigned long long)large_rescue_gate_skip,
            (unsigned long long)large_rescue_gate_force);
    fprintf(stderr,
            "[HZ4_OS_STATS_B8] large_owner_remote=%llu large_owner_hit=%llu large_owner_miss=%llu large_owner_skip_pages=%llu\n",
            (unsigned long long)large_owner_remote,
            (unsigned long long)large_owner_hit,
            (unsigned long long)large_owner_miss,
            (unsigned long long)large_owner_skip_pages);
    fprintf(stderr,
            "[HZ4_OS_STATS_B9] ls_acq_self=%llu ls_acq_steal=%llu ls_acq_miss=%llu ls_rel_self=%llu ls_rel_steal=%llu ls_rel_miss=%llu p2_acq_self=%llu p2_acq_steal=%llu p2_acq_miss=%llu p2_rel_self=%llu p2_rel_steal=%llu p2_rel_miss=%llu\n",
            (unsigned long long)large_lock_shard_acq_self_hit,
            (unsigned long long)large_lock_shard_acq_steal_hit,
            (unsigned long long)large_lock_shard_acq_miss,
            (unsigned long long)large_lock_shard_rel_self_hit,
            (unsigned long long)large_lock_shard_rel_steal_hit,
            (unsigned long long)large_lock_shard_rel_miss,
            (unsigned long long)large_lock_shard_p2_acq_self_hit,
            (unsigned long long)large_lock_shard_p2_acq_steal_hit,
            (unsigned long long)large_lock_shard_p2_acq_miss,
            (unsigned long long)large_lock_shard_p2_rel_self_hit,
            (unsigned long long)large_lock_shard_p2_rel_steal_hit,
            (unsigned long long)large_lock_shard_p2_rel_miss);
    fprintf(stderr,
            "[HZ4_OS_STATS_B10] ls_hint_acq_try=%llu ls_hint_acq_hit=%llu ls_hint_rel_try=%llu ls_hint_rel_hit=%llu\n",
            (unsigned long long)large_lock_shard_hint_acq_try,
            (unsigned long long)large_lock_shard_hint_acq_hit,
            (unsigned long long)large_lock_shard_hint_rel_try,
            (unsigned long long)large_lock_shard_hint_rel_hit);
    fprintf(stderr,
            "[HZ4_OS_STATS_B11] ls_steal_probe=%llu ls_steal_skip_budget=%llu ls_steal_hit=%llu\n",
            (unsigned long long)large_lock_shard_steal_probe,
            (unsigned long long)large_lock_shard_steal_skip_budget,
            (unsigned long long)large_lock_shard_steal_hit);
    fprintf(stderr,
            "[HZ4_OS_STATS_B12] ls_mask_skip=%llu ls_mask_fallback=%llu ls_mask_fallback_hit=%llu\n",
            (unsigned long long)large_lock_shard_mask_skip,
            (unsigned long long)large_lock_shard_mask_fallback,
            (unsigned long long)large_lock_shard_mask_fallback_hit);
    fprintf(stderr,
            "[HZ4_OS_STATS_B13] pagebin_acq_hit=%llu pagebin_acq_miss=%llu pagebin_rel_hit=%llu pagebin_rel_miss=%llu\n",
            (unsigned long long)large_pagebin_acq_hit,
            (unsigned long long)large_pagebin_acq_miss,
            (unsigned long long)large_pagebin_rel_hit,
            (unsigned long long)large_pagebin_rel_miss);
    fprintf(stderr,
            "[HZ4_OS_STATS_B15] b15_acq_call=%llu b15_acq_take_list=%llu b15_acq_stash_hit=%llu b15_rel_flush_call=%llu b15_rel_flush_objs=%llu b15_rel_cas_retry=%llu\n",
            (unsigned long long)large_pagebin_b15_acq_call,
            (unsigned long long)large_pagebin_b15_acq_take_list,
            (unsigned long long)large_pagebin_b15_acq_stash_hit,
            (unsigned long long)large_pagebin_b15_rel_flush_call,
            (unsigned long long)large_pagebin_b15_rel_flush_objs,
            (unsigned long long)large_pagebin_b15_rel_cas_retry);
    fprintf(stderr,
            "[HZ4_OS_STATS_B14] remote_free_seen=%llu remote_bypass_spancache=%llu remote_bypass_pages1=%llu remote_bypass_pages2=%llu remote_bypass_cache_hit=%llu remote_bypass_cache_miss=%llu\n",
            (unsigned long long)large_remote_free_seen,
            (unsigned long long)large_remote_bypass_spancache,
            (unsigned long long)large_remote_bypass_pages1,
            (unsigned long long)large_remote_bypass_pages2,
            (unsigned long long)large_remote_bypass_cache_hit,
            (unsigned long long)large_remote_bypass_cache_miss);
    fprintf(stderr,
            "[HZ4_OS_STATS_B16] ext_acq_hit=%llu ext_acq_miss=%llu ext_rel_hit=%llu ext_rel_miss=%llu ext_rel_drop_cap=%llu ext_bytes_peak=%llu\n",
            (unsigned long long)large_extent_b16_acq_hit,
            (unsigned long long)large_extent_b16_acq_miss,
            (unsigned long long)large_extent_b16_rel_hit,
            (unsigned long long)large_extent_b16_rel_miss,
            (unsigned long long)large_extent_b16_rel_drop_cap,
            (unsigned long long)large_extent_b16_bytes_peak);
    fprintf(stderr,
            "[HZ4_OS_STATS_B17] rf_compact_call=%llu rf_compact_hit=%llu rf_compact_objs=%llu rf_compact_groups=%llu rf_compact_fallback=%llu\n",
            (unsigned long long)rf_compact_call,
            (unsigned long long)rf_compact_hit,
            (unsigned long long)rf_compact_objs,
            (unsigned long long)rf_compact_groups,
            (unsigned long long)rf_compact_fallback);
    fprintf(stderr,
            "[HZ4_OS_STATS_B18] rf_direct_idx_call=%llu rf_direct_idx_objs=%llu rf_direct_idx_groups=%llu\n",
            (unsigned long long)rf_direct_idx_call,
            (unsigned long long)rf_direct_idx_objs,
            (unsigned long long)rf_direct_idx_groups);
    fflush(stderr);
}

static inline void hz4_os_stats_init_once(void) {
    if (atomic_load_explicit(&g_hz4_os_stats_atexit_inited, memory_order_relaxed) != 0) {
        return;
    }
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&g_hz4_os_stats_atexit_inited, &expected, 1,
                                               memory_order_relaxed, memory_order_relaxed)) {
        atexit(hz4_os_stats_dump_atexit);
    }
}


#include "hz4_os_stats_funcs.inc"

#endif

static void* hz4_os_mmap_aligned(size_t size, size_t align, int extra_flags) {
    size_t len = size + align;
    void* base = mmap(NULL, len, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | extra_flags, -1, 0);
    if (base == MAP_FAILED) {
        return NULL;
    }

    uintptr_t addr = (uintptr_t)base;
    uintptr_t aligned = (addr + (align - 1)) & ~(align - 1);

    size_t prefix = (size_t)(aligned - addr);
    size_t suffix = (size_t)((addr + len) - (aligned + size));

    if (prefix) {
        munmap((void*)addr, prefix);
    }
    if (suffix) {
        munmap((void*)(aligned + size), suffix);
    }

    return (void*)aligned;
}

void* hz4_os_seg_acquire(void) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
    int extra = hz4_os_mmap_seg_extra_flags();
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_SEG_SIZE, (size_t)HZ4_SEG_SIZE, extra);
    if (!mem) {
        abort();
    }

#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
    hz4_seg_registry_insert((uintptr_t)mem);
#endif

#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_segs_acquired, 1, memory_order_relaxed);
#endif
#if HZ4_RSSRETURN
    // PressureGateBox 用常時カウンタ（STATSなしならこっちだけ）
    atomic_fetch_add_explicit(&g_hz4_os_segs_acquired_always, 1, memory_order_relaxed);
#endif

#if HZ4_PAGE_TAG_TABLE
    // Initialize page tag table on first segment acquire
    // Uses this segment's base as arena_base
    if (!g_hz4_arena_base) {
        hz4_pagetag_init(mem);
    }
#endif

    return mem;
}

void hz4_os_seg_release(void* seg_base) {
    if (!seg_base) {
        return;
    }
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
    hz4_seg_registry_remove((uintptr_t)seg_base);
#endif
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_segs_released, 1, memory_order_relaxed);
#endif
    munmap(seg_base, (size_t)HZ4_SEG_SIZE);
}

void* hz4_os_page_acquire(void) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_PAGE_SIZE, (size_t)HZ4_PAGE_SIZE, 0);
    if (!mem) {
        abort();
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_acquired, 1, memory_order_relaxed);
#endif
    return mem;
}

void hz4_os_page_release(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_released, 1, memory_order_relaxed);
#endif
    munmap(page_base, (size_t)HZ4_PAGE_SIZE);
}

void hz4_os_page_decommit(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_decommitted, 1, memory_order_relaxed);
#endif
    madvise(page_base, (size_t)HZ4_PAGE_SIZE, MADV_DONTNEED);
}

void hz4_os_page_commit(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_committed, 1, memory_order_relaxed);
#endif
#ifdef MADV_WILLNEED
    madvise(page_base, (size_t)HZ4_PAGE_SIZE, MADV_WILLNEED);
#endif
}

void* hz4_os_large_acquire(size_t size) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
#if HZ4_S220_LARGE_MMAP_NOALIGN
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        HZ4_FAIL("hz4_os_large_acquire: mmap failed");
        return NULL;
    }
#else
    void* mem = hz4_os_mmap_aligned(size, (size_t)HZ4_PAGE_SIZE, 0);
    if (!mem) {
        HZ4_FAIL("hz4_os_large_acquire: mmap failed");
        return NULL;
    }
#endif
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_large_acquired, (uint64_t)size, memory_order_relaxed);
#endif
    return mem;
}

void hz4_os_large_release(void* base, size_t size) {
    if (!base || size == 0) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_large_released, (uint64_t)size, memory_order_relaxed);
#endif
    munmap(base, size);
}
