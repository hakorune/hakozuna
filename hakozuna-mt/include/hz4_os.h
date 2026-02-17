// hz4_os.h - HZ4 OS Box (segment acquire/release)
#ifndef HZ4_OS_H
#define HZ4_OS_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "hz4_config.h"

void* hz4_os_seg_acquire(void);
void  hz4_os_seg_release(void* seg_base);
// Research helpers (no-op when knobs are OFF)
int   hz4_os_mmap_seg_extra_flags(void);
void  hz4_os_seg_research_advise(void* seg_base);
void* hz4_os_page_acquire(void);
void  hz4_os_page_release(void* page_base);
void  hz4_os_page_decommit(void* page_base);
void  hz4_os_page_commit(void* page_base);
void* hz4_os_large_acquire(size_t size);
void  hz4_os_large_release(void* base, size_t size);
int   hz4_os_is_seg_ptr(const void* ptr);

#if HZ4_RSSRETURN
// PressureGateBox support:
// Access to a seg_acq counter even when HZ4_OS_STATS=0 (stats disabled).
#if HZ4_OS_STATS
extern _Atomic(uint64_t) g_hz4_os_segs_acquired;
#endif
extern atomic_uint_fast64_t g_hz4_os_segs_acquired_always;

static inline uint64_t hz4_os_segs_acquired_relaxed(void) {
#if HZ4_OS_STATS
    return atomic_load_explicit(&g_hz4_os_segs_acquired, memory_order_relaxed);
#else
    return atomic_load_explicit(&g_hz4_os_segs_acquired_always, memory_order_relaxed);
#endif
}
#endif // HZ4_RSSRETURN

#if HZ4_OS_STATS
// Observation hooks (cold path only)
void hz4_os_stats_decommit_attempt(void);
void hz4_os_stats_decommit_success(void);
void hz4_os_stats_decommit_skip_uninit(void);
void hz4_os_stats_decommit_skip_already(void);
void hz4_os_stats_decommit_skip_used(void);
void hz4_os_stats_decommit_skip_cph(void);
void hz4_os_stats_decommit_skip_cph_queued(void);
void hz4_os_stats_decommit_skip_budget0(void);
void hz4_os_stats_decommit_skip_still_local(void);
void hz4_os_stats_cph_seal_enter(void);
void hz4_os_stats_cph_grace_block(void);
void hz4_os_stats_decommit_skip_cph_grace(void);
void hz4_os_stats_cand_requeue_grace(void);
void hz4_os_stats_cand_requeue_cph(void);
void hz4_os_stats_cph_extract_ok(void);
void hz4_os_stats_cph_extract_fail(void);
void hz4_os_stats_cph_extract_fail_state(void);
void hz4_os_stats_cph_extract_fail_queued(void);
void hz4_os_stats_cph_extract_fail_notfound(void);
void hz4_os_stats_cph_hot_push(void);
void hz4_os_stats_cph_cold_push(void);
void hz4_os_stats_cph_hot_pop(void);
void hz4_os_stats_cph_cold_pop(void);
void hz4_os_stats_cph_hot_full(void);
void hz4_os_stats_dq_enqueue(void);
void hz4_os_stats_dq_enqueue_already(void);
void hz4_os_stats_dq_dequeue(void);
void hz4_os_stats_dq_due_ready(void);
void hz4_os_stats_dq_due_notready(void);
void hz4_os_stats_dq_process_call(void);
void hz4_os_stats_dq_process_skip_duegate(void);
void hz4_os_stats_used_zero(void);
void hz4_os_stats_rss_pending(uint32_t pending);
void hz4_os_stats_rss_gate_fire(void);
void hz4_os_stats_decommit_skip_budget0_pending(void);
void hz4_os_stats_rss_cool_len(uint32_t len);
void hz4_os_stats_rss_cool_promote(void);
void hz4_os_stats_rss_cool_drop_used(void);
void hz4_os_stats_rss_release_fire(void);
void hz4_os_stats_rss_cool_enq(void);
void hz4_os_stats_rss_cool_block_deadline(void);
void hz4_os_stats_rss_cool_sweep_budget_hit(void);
void hz4_os_stats_rss_cool_skip_already(void);
void hz4_os_stats_rss_quarantine_set(void);
void hz4_os_stats_rss_quarantine_mature(void);
void hz4_os_stats_rss_quarantine_drop_used(void);
void hz4_os_stats_collect_call(void);
void hz4_os_stats_refill_call(void);
void hz4_os_stats_inbox_consume_call(void);
void hz4_os_stats_inbox_lite_scan_call(void);
void hz4_os_stats_inbox_lite_shortcut(void);
void hz4_os_stats_inbox_stash_len(uint32_t len);
void hz4_os_stats_carry_hit(void);
void hz4_os_stats_carry_miss(void);
void hz4_os_stats_drain_page_call(void);
void hz4_os_stats_drain_page_objs(uint32_t n);
void hz4_os_stats_page_used_dec(uint32_t n);
void hz4_os_stats_remote_flush_call(void);
void hz4_os_stats_remote_flush_rlen(uint32_t rlen);
void hz4_os_stats_remote_flush_fastpath_n1(void);
void hz4_os_stats_remote_flush_fastpath_le4(void);
void hz4_os_stats_remote_flush_probe_overflow(void);
void hz4_os_stats_remote_flush_compact_call(void);
void hz4_os_stats_remote_flush_compact_hit(void);
void hz4_os_stats_remote_flush_compact_objs(uint32_t n);
void hz4_os_stats_remote_flush_compact_groups(uint32_t n);
void hz4_os_stats_remote_flush_compact_fallback(uint32_t n);
void hz4_os_stats_remote_flush_direct_index_call(void);
void hz4_os_stats_remote_flush_direct_index_objs(uint32_t n);
void hz4_os_stats_remote_flush_direct_index_groups(uint32_t n);
void hz4_os_stats_inbox_push_one(void);
void hz4_os_stats_inbox_push_list(void);
void hz4_os_stats_rbmf_try(void);
void hz4_os_stats_rbmf_ok(void);
void hz4_os_stats_rbmf_fail_guard(void);
void hz4_os_stats_rbmf_fail_lock(void);
void hz4_os_stats_rbmf_fail_full(void);
void hz4_os_stats_rbmf_fail_draining(void);
void hz4_os_stats_rbmf_fallback(void);
void hz4_os_stats_rbuf_notify_transition(void);
void hz4_os_stats_rbuf_notify_already_queued(void);
void hz4_os_stats_rbuf_drain_objs(uint32_t n);
void hz4_os_stats_large_rescue_attempt(void);
void hz4_os_stats_large_rescue_success(void);
void hz4_os_stats_large_rescue_fail(void);
void hz4_os_stats_large_rescue_fallback_flush(void);
void hz4_os_stats_large_rescue_route_batch_hit(void);
void hz4_os_stats_large_rescue_route_mmap_retry_ok(void);
void hz4_os_stats_large_rescue_route_mmap_retry_fail(void);
void hz4_os_stats_large_rescue_gate_skip(void);
void hz4_os_stats_large_rescue_gate_force(void);
void hz4_os_stats_large_owner_remote(void);
void hz4_os_stats_large_owner_hit(void);
void hz4_os_stats_large_owner_miss(void);
void hz4_os_stats_large_owner_skip_pages(void);
void hz4_os_stats_large_lock_shard_acq_self_hit(void);
void hz4_os_stats_large_lock_shard_acq_steal_hit(void);
void hz4_os_stats_large_lock_shard_acq_miss(void);
void hz4_os_stats_large_lock_shard_rel_self_hit(void);
void hz4_os_stats_large_lock_shard_rel_steal_hit(void);
void hz4_os_stats_large_lock_shard_rel_miss(void);
void hz4_os_stats_large_lock_shard_p2_acq_self_hit(void);
void hz4_os_stats_large_lock_shard_p2_acq_steal_hit(void);
void hz4_os_stats_large_lock_shard_p2_acq_miss(void);
void hz4_os_stats_large_lock_shard_p2_rel_self_hit(void);
void hz4_os_stats_large_lock_shard_p2_rel_steal_hit(void);
void hz4_os_stats_large_lock_shard_p2_rel_miss(void);
void hz4_os_stats_large_lock_shard_hint_acq_try(void);
void hz4_os_stats_large_lock_shard_hint_acq_hit(void);
void hz4_os_stats_large_lock_shard_hint_rel_try(void);
void hz4_os_stats_large_lock_shard_hint_rel_hit(void);
void hz4_os_stats_large_lock_shard_steal_probe(void);
void hz4_os_stats_large_lock_shard_steal_skip_budget(void);
void hz4_os_stats_large_lock_shard_steal_hit(void);
void hz4_os_stats_large_lock_shard_mask_skip(void);
void hz4_os_stats_large_lock_shard_mask_fallback(void);
void hz4_os_stats_large_lock_shard_mask_fallback_hit(void);
void hz4_os_stats_large_pagebin_acq_hit(void);
void hz4_os_stats_large_pagebin_acq_miss(void);
void hz4_os_stats_large_pagebin_rel_hit(void);
void hz4_os_stats_large_pagebin_rel_miss(void);
void hz4_os_stats_large_pagebin_b15_acq_call(void);
void hz4_os_stats_large_pagebin_b15_acq_take_list(void);
void hz4_os_stats_large_pagebin_b15_acq_stash_hit(void);
void hz4_os_stats_large_pagebin_b15_rel_flush_call(void);
void hz4_os_stats_large_pagebin_b15_rel_flush_objs(uint32_t n);
void hz4_os_stats_large_pagebin_b15_rel_cas_retry(uint32_t n);
void hz4_os_stats_large_remote_free_seen(void);
void hz4_os_stats_large_remote_bypass_spancache(void);
void hz4_os_stats_large_remote_bypass_pages1(void);
void hz4_os_stats_large_remote_bypass_pages2(void);
void hz4_os_stats_large_remote_bypass_cache_hit(void);
void hz4_os_stats_large_remote_bypass_cache_miss(void);
void hz4_os_stats_large_extent_b16_acq_hit(void);
void hz4_os_stats_large_extent_b16_acq_miss(void);
void hz4_os_stats_large_extent_b16_rel_hit(void);
void hz4_os_stats_large_extent_b16_rel_miss(void);
void hz4_os_stats_large_extent_b16_rel_drop_cap(void);
void hz4_os_stats_large_extent_b16_bytes_peak(size_t bytes);

#if HZ4_RSSRETURN_PRESSUREGATEBOX
void hz4_os_stats_rss_gate_on_window(void);
void hz4_os_stats_rss_gate_seg_acq_delta(uint32_t delta);
#endif
#else
static inline void hz4_os_stats_decommit_attempt(void) {}
static inline void hz4_os_stats_decommit_success(void) {}
static inline void hz4_os_stats_decommit_skip_uninit(void) {}
static inline void hz4_os_stats_decommit_skip_already(void) {}
static inline void hz4_os_stats_decommit_skip_used(void) {}
static inline void hz4_os_stats_decommit_skip_cph(void) {}
static inline void hz4_os_stats_decommit_skip_cph_queued(void) {}
static inline void hz4_os_stats_decommit_skip_budget0(void) {}
static inline void hz4_os_stats_decommit_skip_still_local(void) {}
static inline void hz4_os_stats_cph_seal_enter(void) {}
static inline void hz4_os_stats_cph_grace_block(void) {}
static inline void hz4_os_stats_decommit_skip_cph_grace(void) {}
static inline void hz4_os_stats_cand_requeue_grace(void) {}
static inline void hz4_os_stats_cand_requeue_cph(void) {}
static inline void hz4_os_stats_cph_extract_ok(void) {}
static inline void hz4_os_stats_cph_extract_fail(void) {}
static inline void hz4_os_stats_cph_extract_fail_state(void) {}
static inline void hz4_os_stats_cph_extract_fail_queued(void) {}
static inline void hz4_os_stats_cph_extract_fail_notfound(void) {}
static inline void hz4_os_stats_cph_hot_push(void) {}
static inline void hz4_os_stats_cph_cold_push(void) {}
static inline void hz4_os_stats_cph_hot_pop(void) {}
static inline void hz4_os_stats_cph_cold_pop(void) {}
static inline void hz4_os_stats_cph_hot_full(void) {}
static inline void hz4_os_stats_dq_enqueue(void) {}
static inline void hz4_os_stats_dq_enqueue_already(void) {}
static inline void hz4_os_stats_dq_dequeue(void) {}
static inline void hz4_os_stats_dq_due_ready(void) {}
static inline void hz4_os_stats_dq_due_notready(void) {}
static inline void hz4_os_stats_dq_process_call(void) {}
static inline void hz4_os_stats_dq_process_skip_duegate(void) {}
static inline void hz4_os_stats_used_zero(void) {}
static inline void hz4_os_stats_rss_pending(uint32_t pending) { (void)pending; }
static inline void hz4_os_stats_rss_gate_fire(void) {}
static inline void hz4_os_stats_decommit_skip_budget0_pending(void) {}
static inline void hz4_os_stats_rss_cool_len(uint32_t len) { (void)len; }
static inline void hz4_os_stats_rss_cool_promote(void) {}
static inline void hz4_os_stats_rss_cool_drop_used(void) {}
static inline void hz4_os_stats_rss_release_fire(void) {}
static inline void hz4_os_stats_rss_cool_enq(void) {}
static inline void hz4_os_stats_rss_cool_block_deadline(void) {}
static inline void hz4_os_stats_rss_cool_sweep_budget_hit(void) {}
static inline void hz4_os_stats_rss_cool_skip_already(void) {}
static inline void hz4_os_stats_rss_quarantine_set(void) {}
static inline void hz4_os_stats_rss_quarantine_mature(void) {}
static inline void hz4_os_stats_rss_quarantine_drop_used(void) {}
static inline void hz4_os_stats_collect_call(void) {}
static inline void hz4_os_stats_refill_call(void) {}
static inline void hz4_os_stats_inbox_consume_call(void) {}
static inline void hz4_os_stats_inbox_lite_scan_call(void) {}
static inline void hz4_os_stats_inbox_lite_shortcut(void) {}
static inline void hz4_os_stats_inbox_stash_len(uint32_t len) { (void)len; }
static inline void hz4_os_stats_carry_hit(void) {}
static inline void hz4_os_stats_carry_miss(void) {}
static inline void hz4_os_stats_drain_page_call(void) {}
static inline void hz4_os_stats_drain_page_objs(uint32_t n) { (void)n; }
static inline void hz4_os_stats_page_used_dec(uint32_t n) { (void)n; }
static inline void hz4_os_stats_remote_flush_call(void) {}
static inline void hz4_os_stats_remote_flush_rlen(uint32_t rlen) { (void)rlen; }
static inline void hz4_os_stats_remote_flush_fastpath_n1(void) {}
static inline void hz4_os_stats_remote_flush_fastpath_le4(void) {}
static inline void hz4_os_stats_remote_flush_probe_overflow(void) {}
static inline void hz4_os_stats_remote_flush_compact_call(void) {}
static inline void hz4_os_stats_remote_flush_compact_hit(void) {}
static inline void hz4_os_stats_remote_flush_compact_objs(uint32_t n) { (void)n; }
static inline void hz4_os_stats_remote_flush_compact_groups(uint32_t n) { (void)n; }
static inline void hz4_os_stats_remote_flush_compact_fallback(uint32_t n) { (void)n; }
static inline void hz4_os_stats_remote_flush_direct_index_call(void) {}
static inline void hz4_os_stats_remote_flush_direct_index_objs(uint32_t n) { (void)n; }
static inline void hz4_os_stats_remote_flush_direct_index_groups(uint32_t n) { (void)n; }
static inline void hz4_os_stats_inbox_push_one(void) {}
static inline void hz4_os_stats_inbox_push_list(void) {}
static inline void hz4_os_stats_rbmf_try(void) {}
static inline void hz4_os_stats_rbmf_ok(void) {}
static inline void hz4_os_stats_rbmf_fail_guard(void) {}
static inline void hz4_os_stats_rbmf_fail_lock(void) {}
static inline void hz4_os_stats_rbmf_fail_full(void) {}
static inline void hz4_os_stats_rbmf_fail_draining(void) {}
static inline void hz4_os_stats_rbmf_fallback(void) {}
static inline void hz4_os_stats_rbuf_notify_transition(void) {}
static inline void hz4_os_stats_rbuf_notify_already_queued(void) {}
static inline void hz4_os_stats_rbuf_drain_objs(uint32_t n) { (void)n; }
static inline void hz4_os_stats_large_rescue_attempt(void) {}
static inline void hz4_os_stats_large_rescue_success(void) {}
static inline void hz4_os_stats_large_rescue_fail(void) {}
static inline void hz4_os_stats_large_rescue_fallback_flush(void) {}
static inline void hz4_os_stats_large_rescue_route_batch_hit(void) {}
static inline void hz4_os_stats_large_rescue_route_mmap_retry_ok(void) {}
static inline void hz4_os_stats_large_rescue_route_mmap_retry_fail(void) {}
static inline void hz4_os_stats_large_rescue_gate_skip(void) {}
static inline void hz4_os_stats_large_rescue_gate_force(void) {}
static inline void hz4_os_stats_large_owner_remote(void) {}
static inline void hz4_os_stats_large_owner_hit(void) {}
static inline void hz4_os_stats_large_owner_miss(void) {}
static inline void hz4_os_stats_large_owner_skip_pages(void) {}
static inline void hz4_os_stats_large_lock_shard_acq_self_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_acq_steal_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_acq_miss(void) {}
static inline void hz4_os_stats_large_lock_shard_rel_self_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_rel_steal_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_rel_miss(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_acq_self_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_acq_steal_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_acq_miss(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_rel_self_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_rel_steal_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_p2_rel_miss(void) {}
static inline void hz4_os_stats_large_lock_shard_hint_acq_try(void) {}
static inline void hz4_os_stats_large_lock_shard_hint_acq_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_hint_rel_try(void) {}
static inline void hz4_os_stats_large_lock_shard_hint_rel_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_steal_probe(void) {}
static inline void hz4_os_stats_large_lock_shard_steal_skip_budget(void) {}
static inline void hz4_os_stats_large_lock_shard_steal_hit(void) {}
static inline void hz4_os_stats_large_lock_shard_mask_skip(void) {}
static inline void hz4_os_stats_large_lock_shard_mask_fallback(void) {}
static inline void hz4_os_stats_large_lock_shard_mask_fallback_hit(void) {}
static inline void hz4_os_stats_large_pagebin_acq_hit(void) {}
static inline void hz4_os_stats_large_pagebin_acq_miss(void) {}
static inline void hz4_os_stats_large_pagebin_rel_hit(void) {}
static inline void hz4_os_stats_large_pagebin_rel_miss(void) {}
static inline void hz4_os_stats_large_pagebin_b15_acq_call(void) {}
static inline void hz4_os_stats_large_pagebin_b15_acq_take_list(void) {}
static inline void hz4_os_stats_large_pagebin_b15_acq_stash_hit(void) {}
static inline void hz4_os_stats_large_pagebin_b15_rel_flush_call(void) {}
static inline void hz4_os_stats_large_pagebin_b15_rel_flush_objs(uint32_t n) { (void)n; }
static inline void hz4_os_stats_large_pagebin_b15_rel_cas_retry(uint32_t n) { (void)n; }
static inline void hz4_os_stats_large_remote_free_seen(void) {}
static inline void hz4_os_stats_large_remote_bypass_spancache(void) {}
static inline void hz4_os_stats_large_remote_bypass_pages1(void) {}
static inline void hz4_os_stats_large_remote_bypass_pages2(void) {}
static inline void hz4_os_stats_large_remote_bypass_cache_hit(void) {}
static inline void hz4_os_stats_large_remote_bypass_cache_miss(void) {}
static inline void hz4_os_stats_large_extent_b16_acq_hit(void) {}
static inline void hz4_os_stats_large_extent_b16_acq_miss(void) {}
static inline void hz4_os_stats_large_extent_b16_rel_hit(void) {}
static inline void hz4_os_stats_large_extent_b16_rel_miss(void) {}
static inline void hz4_os_stats_large_extent_b16_rel_drop_cap(void) {}
static inline void hz4_os_stats_large_extent_b16_bytes_peak(size_t bytes) { (void)bytes; }

static inline void hz4_os_stats_rss_gate_on_window(void) {}
static inline void hz4_os_stats_rss_gate_seg_acq_delta(uint32_t delta) { (void)delta; }
#endif

#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
struct hz4_tls;
void hz4_os_stats_tls_flush(struct hz4_tls* tls);
#endif

#endif // HZ4_OS_H
