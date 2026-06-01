#ifndef HZ_BENCH_MODERN_ALLOCATOR_ADAPTER_H
#define HZ_BENCH_MODERN_ALLOCATOR_ADAPTER_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(HZ_BENCH_USE_HZ3)
#include "hz3.h"
#elif defined(HZ_BENCH_USE_HZ4)
#include "hz4_win_api.h"
#elif defined(HZ_BENCH_USE_HZ6)
#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_profiles.h"
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
#include "hz5_policy.h"
#elif defined(HZ_BENCH_USE_MIMALLOC)
#include <mimalloc.h>
#elif defined(HZ_BENCH_USE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#endif

#if defined(HZ_BENCH_USE_HZ6)
#ifndef HZ_BENCH_HZ6_PROFILE
#define HZ_BENCH_HZ6_PROFILE HZ6_PROFILE_STRICT
#endif

__declspec(thread) static Hz6Allocator hz_bench_tls_hz6_allocator;
__declspec(thread) static int hz_bench_tls_hz6_initialized;
#endif

#if defined(HZ_BENCH_USE_HZ5_POLICY)
#ifndef HZ_BENCH_HZ5_ALIGN
#define HZ_BENCH_HZ5_ALIGN 16u
#endif
#endif

static inline void hz_bench_allocator_thread_setup(void) {
#if defined(HZ_BENCH_USE_HZ6)
    if (!hz_bench_tls_hz6_initialized) {
        hz6_allocator_init_with_profile(&hz_bench_tls_hz6_allocator, HZ_BENCH_HZ6_PROFILE);
        hz_bench_tls_hz6_initialized = 1;
    }
#endif
}

static inline void hz_bench_allocator_thread_teardown(void) {
#if defined(HZ_BENCH_USE_HZ6)
    if (hz_bench_tls_hz6_initialized) {
        hz6_allocator_destroy(&hz_bench_tls_hz6_allocator);
        hz_bench_tls_hz6_initialized = 0;
    }
#endif
}

static inline void* hz_bench_alloc(size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    return hz3_malloc(size);
#elif defined(HZ_BENCH_USE_HZ4)
    return hz4_win_malloc(size);
#elif defined(HZ_BENCH_USE_HZ6)
    hz_bench_allocator_thread_setup();
    return hz6_malloc(&hz_bench_tls_hz6_allocator, size);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    static const Hz5PolicyHooks hooks = {0};
    return hz5_policy_alloc_aligned(size, (size_t)HZ_BENCH_HZ5_ALIGN, &hooks);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    return mi_malloc(size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    return tc_malloc(size);
#else
    return malloc(size);
#endif
}

static inline void hz_bench_free(void* ptr) {
#if defined(HZ_BENCH_USE_HZ3)
    hz3_free(ptr);
#elif defined(HZ_BENCH_USE_HZ4)
    hz4_win_free(ptr);
#elif defined(HZ_BENCH_USE_HZ6)
    hz_bench_allocator_thread_setup();
    hz6_free(&hz_bench_tls_hz6_allocator, ptr);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    static const Hz5PolicyHooks hooks = {0};
    (void)hz5_policy_free(ptr, &hooks);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    mi_free(ptr);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    tc_free(ptr);
#else
    free(ptr);
#endif
}

static inline void hz_bench_dump_stats(FILE* out, const char* label) {
#if defined(HZ_BENCH_USE_HZ6)
    if (hz_bench_tls_hz6_initialized) {
        Hz6StatsSnapshot s = hz6_stats_snapshot(&hz_bench_tls_hz6_allocator);
#if HZ6_DIAGNOSTIC_PROBES
        fprintf(out,
                "[HZ6_STATS] label=%s route_valid=%zu route_invalid=%zu route_miss=%zu "
                "route_visibility_lookup=%zu route_visibility_hit=%zu "
                "route_visibility_hit_local_owner=%zu "
                "route_visibility_hit_foreign_owner=%zu "
                "route_visibility_miss=%zu route_visibility_probe_total=%zu "
                "route_visibility_probe_max=%zu "
                "transfer_push=%zu transfer_pop=%zu transfer_current=%zu "
                "transfer_current_max=%zu remote_free_attempt=%zu "
                "remote_free_strict_owner_block=%zu "
                "remote_free_transfer_fail=%zu "
                "route_rehome_attempt=%zu route_rehome_success=%zu "
                "route_rehome_fail=%zu source_owned_prepare=%zu "
                "source_owned_route_hit_local_owner=%zu "
                "source_owned_visibility_hit_local_owner=%zu "
                "source_owned_visibility_hit_foreign_owner=%zu "
                "source_owned_remote_free_attempt=%zu "
                "source_owned_release=%zu source_alloc=%zu alloc_fail=%zu "
                "descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu "
                "route_active_current=%zu route_active_max=%zu "
                "descriptor_probe_total=%zu descriptor_probe_max=%zu "
                "descriptor_fail_active_max=%zu "
                "descriptor_fail_local_free_max=%zu "
                "descriptor_fail_transfer_free_max=%zu "
                "descriptor_fail_remote_pending_max=%zu "
                "descriptor_fail_central_free_max=%zu "
                "descriptor_fail_released_max=%zu "
                "descriptor_fail_orphan_max=%zu "
                "descriptor_fail_dead_with_ptr_max=%zu "
                "descriptor_fail_frontcache_total_max=%zu "
                "descriptor_fail_frontcache_largest_bin_max=%zu "
                "descriptor_fail_frontcache_nonempty_bins_max=%zu "
                "frontcache_spill_dryrun_calls=%zu "
                "frontcache_spill_dryrun_requested_empty=%zu "
                "frontcache_spill_dryrun_candidate_calls=%zu "
                "frontcache_spill_dryrun_reclaimable_total=%zu "
                "frontcache_spill_dryrun_largest_donor_max=%zu "
                "frontcache_spill_dryrun_donor_bins_max=%zu "
                "frontcache_spill_attempt=%zu "
                "frontcache_spill_success=%zu "
                "frontcache_spill_no_candidate=%zu "
                "frontcache_spill_invalid=%zu "
                "frontcache_spill_retry_success=%zu "
                "frontcache_borrow_dryrun_calls=%zu "
                "frontcache_borrow_dryrun_candidate_calls=%zu "
                "frontcache_borrow_dryrun_candidate_total=%zu "
                "frontcache_borrow_dryrun_largest_candidate_max=%zu "
                "frontcache_borrow_attempt=%zu "
                "frontcache_borrow_success=%zu "
                "frontcache_borrow_no_candidate=%zu "
                "frontcache_borrow_invalid=%zu "
                "frontcache_cap_dryrun_push=%zu "
                "frontcache_cap_dryrun_over_cap=%zu "
                "frontcache_cap_dryrun_would_release=%zu "
                "frontcache_cap_dryrun_soft_cap_max=%zu "
                "frontcache_cap_dryrun_bin_count_max=%zu "
                "frontcache_cap_release=%zu "
                "source_run_reuse_dryrun_calls=%zu "
                "source_run_reuse_dryrun_candidate_calls=%zu "
                "source_run_reuse_dryrun_candidate_blocks_total=%zu "
                "source_run_reuse_dryrun_free_slots_total=%zu "
                "source_run_reuse_dryrun_largest_free_slots_max=%zu "
                "route_lookup_probe_total=%zu route_lookup_probe_max=%zu "
                "route_register_probe_total=%zu route_register_probe_max=%zu "
                "route_unregister_probe_total=%zu route_unregister_probe_max=%zu "
                "source_block_probe_total=%zu source_block_probe_max=%zu "
                "source_block_fail_active_max=%zu "
                "source_block_fail_registered_max=%zu "
                "source_block_fail_ref_nonzero_max=%zu "
                "source_block_fail_ref_zero_max=%zu "
                "large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
                label ? label : "unknown",
                s.route_valid,
                s.route_invalid,
                s.route_miss,
                s.route_visibility_lookup,
                s.route_visibility_hit,
                s.route_visibility_hit_local_owner,
                s.route_visibility_hit_foreign_owner,
                s.route_visibility_miss,
                s.route_visibility_probe_total,
                s.route_visibility_probe_max,
                s.transfer_push,
                s.transfer_pop,
                s.transfer_current,
                s.transfer_current_max,
                s.remote_free_attempt,
                s.remote_free_strict_owner_block,
                s.remote_free_transfer_fail,
                s.route_rehome_attempt,
                s.route_rehome_success,
                s.route_rehome_fail,
                s.source_owned_prepare,
                s.source_owned_route_hit_local_owner,
                s.source_owned_visibility_hit_local_owner,
                s.source_owned_visibility_hit_foreign_owner,
                s.source_owned_remote_free_attempt,
                s.source_owned_release,
                s.source_alloc,
                s.alloc_fail,
                s.descriptor_exhausted,
                s.route_register_fail,
                s.source_block_exhausted,
                s.route_active_current,
                s.route_active_max,
                s.descriptor_probe_total,
                s.descriptor_probe_max,
                s.descriptor_fail_active_max,
                s.descriptor_fail_local_free_max,
                s.descriptor_fail_transfer_free_max,
                s.descriptor_fail_remote_pending_max,
                s.descriptor_fail_central_free_max,
                s.descriptor_fail_released_max,
                s.descriptor_fail_orphan_max,
                s.descriptor_fail_dead_with_ptr_max,
                s.descriptor_fail_frontcache_total_max,
                s.descriptor_fail_frontcache_largest_bin_max,
                s.descriptor_fail_frontcache_nonempty_bins_max,
                s.frontcache_spill_dryrun_calls,
                s.frontcache_spill_dryrun_requested_empty,
                s.frontcache_spill_dryrun_candidate_calls,
                s.frontcache_spill_dryrun_reclaimable_total,
                s.frontcache_spill_dryrun_largest_donor_max,
                s.frontcache_spill_dryrun_donor_bins_max,
                s.frontcache_spill_attempt,
                s.frontcache_spill_success,
                s.frontcache_spill_no_candidate,
                s.frontcache_spill_invalid,
                s.frontcache_spill_retry_success,
                s.frontcache_borrow_dryrun_calls,
                s.frontcache_borrow_dryrun_candidate_calls,
                s.frontcache_borrow_dryrun_candidate_total,
                s.frontcache_borrow_dryrun_largest_candidate_max,
                s.frontcache_borrow_attempt,
                s.frontcache_borrow_success,
                s.frontcache_borrow_no_candidate,
                s.frontcache_borrow_invalid,
                s.frontcache_cap_dryrun_push,
                s.frontcache_cap_dryrun_over_cap,
                s.frontcache_cap_dryrun_would_release,
                s.frontcache_cap_dryrun_soft_cap_max,
                s.frontcache_cap_dryrun_bin_count_max,
                s.frontcache_cap_release,
                s.source_run_reuse_dryrun_calls,
                s.source_run_reuse_dryrun_candidate_calls,
                s.source_run_reuse_dryrun_candidate_blocks_total,
                s.source_run_reuse_dryrun_free_slots_total,
                s.source_run_reuse_dryrun_largest_free_slots_max,
                s.route_lookup_probe_total,
                s.route_lookup_probe_max,
                s.route_register_probe_total,
                s.route_register_probe_max,
                s.route_unregister_probe_total,
                s.route_unregister_probe_max,
                s.source_block_probe_total,
                s.source_block_probe_max,
                s.source_block_fail_active_max,
                s.source_block_fail_registered_max,
                s.source_block_fail_ref_nonzero_max,
                s.source_block_fail_ref_zero_max,
                s.large_span_central_push,
                s.large_span_central_pop,
                s.large_span_source_alloc);
#else
        fprintf(out,
                "[HZ6_STATS] label=%s route_valid=%zu route_invalid=%zu route_miss=%zu "
                "route_visibility_lookup=%zu route_visibility_hit=%zu "
                "route_visibility_hit_local_owner=%zu "
                "route_visibility_hit_foreign_owner=%zu "
                "route_visibility_miss=%zu route_visibility_probe_total=%zu "
                "route_visibility_probe_max=%zu "
                "transfer_push=%zu transfer_pop=%zu transfer_current=%zu "
                "transfer_current_max=%zu remote_free_attempt=%zu "
                "remote_free_strict_owner_block=%zu "
                "remote_free_transfer_fail=%zu "
                "route_rehome_attempt=%zu route_rehome_success=%zu "
                "route_rehome_fail=%zu source_owned_prepare=%zu "
                "source_owned_route_hit_local_owner=%zu "
                "source_owned_visibility_hit_local_owner=%zu "
                "source_owned_visibility_hit_foreign_owner=%zu "
                "source_owned_remote_free_attempt=%zu "
                "source_owned_release=%zu source_alloc=%zu alloc_fail=%zu "
                "descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu "
                "large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
                label ? label : "unknown",
                s.route_valid,
                s.route_invalid,
                s.route_miss,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                s.transfer_push,
                s.transfer_pop,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                (size_t)0,
                s.source_alloc,
                s.alloc_fail,
                s.descriptor_exhausted,
                s.route_register_fail,
                s.source_block_exhausted,
                s.large_span_central_push,
                s.large_span_central_pop,
                s.large_span_source_alloc);
#endif
    }
#else
    (void)out;
    (void)label;
#endif
}

#endif
