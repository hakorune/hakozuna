// Windows-friendly allocator comparison bench.
// Usage: bench_allocator_compare [threads] [iters_per_thread] [working_set] [min_size] [max_size]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#include <process.h>
#else
#include <pthread.h>
#include <time.h>
#endif

#if defined(HZ_BENCH_USE_HZ6)
#ifndef HZ_BENCH_HZ6_PROFILE
#define HZ_BENCH_HZ6_PROFILE HZ6_PROFILE_STRICT
#endif
#endif

#if defined(HZ_BENCH_USE_HZ5_POLICY)
#ifndef HZ_BENCH_HZ5_ALIGN
#define HZ_BENCH_HZ5_ALIGN 16u
#endif
#endif

typedef struct {
    uint32_t seed;
    size_t iters;
    size_t ws;
    size_t min_size;
    size_t max_size;
    size_t alloc_attempts;
    size_t alloc_successes;
    size_t alloc_failures;
    size_t frees;
#if defined(HZ_BENCH_USE_HZ6)
    Hz6Allocator hz6_allocator;
    Hz6StatsSnapshot hz6_stats_after;
#endif
} ThreadArg;

static inline void bench_thread_setup(ThreadArg* ta) {
#if defined(HZ_BENCH_USE_HZ6)
    hz6_allocator_init_with_profile(&ta->hz6_allocator, HZ_BENCH_HZ6_PROFILE);
#else
    (void)ta;
#endif
}

static inline void bench_thread_teardown(ThreadArg* ta) {
#if defined(HZ_BENCH_USE_HZ6)
    hz6_allocator_destroy(&ta->hz6_allocator);
#else
    (void)ta;
#endif
}

static inline void* bench_alloc(ThreadArg* ta, size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    return hz3_malloc(size);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    return hz4_win_malloc(size);
#elif defined(HZ_BENCH_USE_HZ6)
    return hz6_malloc(&ta->hz6_allocator, size);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    static const Hz5PolicyHooks hooks = {0};
    return hz5_policy_alloc_aligned(size, (size_t)HZ_BENCH_HZ5_ALIGN, &hooks);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    return mi_malloc(size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    return tc_malloc(size);
#else
    (void)ta;
    return malloc(size);
#endif
}

static inline void* bench_realloc(ThreadArg* ta, void* ptr, size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    return hz3_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    return hz4_win_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ6)
    (void)ta;
    (void)ptr;
    (void)size;
    return NULL;
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    (void)ptr;
    (void)size;
    return NULL;
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    return mi_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    return tc_realloc(ptr, size);
#else
    (void)ta;
    return realloc(ptr, size);
#endif
}

static inline void bench_free(ThreadArg* ta, void* ptr) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    hz3_free(ptr);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    hz4_win_free(ptr);
#elif defined(HZ_BENCH_USE_HZ6)
    hz6_free(&ta->hz6_allocator, ptr);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    static const Hz5PolicyHooks hooks = {0};
    (void)hz5_policy_free(ptr, &hooks);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    mi_free(ptr);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    tc_free(ptr);
#else
    (void)ta;
    free(ptr);
#endif
}

#ifndef HZ_BENCH_DISABLE_REALLOC
#define HZ_BENCH_DISABLE_REALLOC 0
#endif

static inline uint32_t lcg_next(uint32_t* state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1) : 1;
    return min_size + (r % span);
}

#if defined(_WIN32)
static uint64_t now_ns(void) {
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    {
        uint64_t whole = (uint64_t)(counter.QuadPart / freq.QuadPart);
        uint64_t rem = (uint64_t)(counter.QuadPart % freq.QuadPart);
        return (whole * 1000000000ULL) + ((rem * 1000000000ULL) / (uint64_t)freq.QuadPart);
    }
}

static size_t peak_working_set_kb(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return 0;
    }
    return (size_t)(pmc.PeakWorkingSetSize / 1024u);
}

static unsigned __stdcall bench_thread(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;
    uint32_t seed = ta->seed;
    size_t ws = ta->ws ? ta->ws : 1;
    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        return 0;
    }
    bench_thread_setup(ta);

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
            bench_free(ta, slots[idx]);
            ++ta->frees;
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        ++ta->alloc_attempts;
        void* p = bench_alloc(ta, size);
        if (!p) {
            ++ta->alloc_failures;
            continue;
        }
        ++ta->alloc_successes;
        if (!HZ_BENCH_DISABLE_REALLOC && ((i & 0x3fu) == 0)) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(ta, p, new_size);
            if (p2) {
                p = p2;
                size = new_size;
            }
        }
        memset(p, 0xA5, size < 64 ? size : 64);
        slots[idx] = p;
    }

    for (size_t i = 0; i < ws; i++) {
        if (slots[i]) {
            bench_free(ta, slots[i]);
            ++ta->frees;
        }
    }
#if defined(HZ_BENCH_USE_HZ6)
    ta->hz6_stats_after = hz6_stats_snapshot(&ta->hz6_allocator);
#endif
    bench_thread_teardown(ta);
    free(slots);
    return 0;
}
#else
static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void* bench_thread(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;
    uint32_t seed = ta->seed;
    size_t ws = ta->ws ? ta->ws : 1;
    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        return NULL;
    }
    bench_thread_setup(ta);

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
            bench_free(ta, slots[idx]);
            ++ta->frees;
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        ++ta->alloc_attempts;
        void* p = bench_alloc(ta, size);
        if (!p) {
            ++ta->alloc_failures;
            continue;
        }
        ++ta->alloc_successes;
        if (!HZ_BENCH_DISABLE_REALLOC && ((i & 0x3fu) == 0)) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(ta, p, new_size);
            if (p2) {
                p = p2;
                size = new_size;
            }
        }
        memset(p, 0xA5, size < 64 ? size : 64);
        slots[idx] = p;
    }

    for (size_t i = 0; i < ws; i++) {
        if (slots[i]) {
            bench_free(ta, slots[i]);
            ++ta->frees;
        }
    }
#if defined(HZ_BENCH_USE_HZ6)
    ta->hz6_stats_after = hz6_stats_snapshot(&ta->hz6_allocator);
#endif
    bench_thread_teardown(ta);
    free(slots);
    return NULL;
}
#endif

int main(int argc, char** argv) {
    size_t threads = 4;
    size_t iters = 1000000;
    size_t ws = 8192;
    size_t min_size = 16;
    size_t max_size = 1024;

    if (argc > 1) threads = (size_t)strtoull(argv[1], NULL, 10);
    if (argc > 2) iters = (size_t)strtoull(argv[2], NULL, 10);
    if (argc > 3) ws = (size_t)strtoull(argv[3], NULL, 10);
    if (argc > 4) min_size = (size_t)strtoull(argv[4], NULL, 10);
    if (argc > 5) max_size = (size_t)strtoull(argv[5], NULL, 10);
    if (threads == 0) threads = 1;
    if (ws == 0) ws = 1;
    if (min_size == 0) min_size = 1;
    if (max_size < min_size) max_size = min_size;

    ThreadArg* args = (ThreadArg*)calloc(threads, sizeof(ThreadArg));
    if (!args) {
        fprintf(stderr, "alloc args failed\n");
        return 1;
    }

    uint64_t start = now_ns();

#if defined(_WIN32)
    HANDLE* handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    if (!handles) {
        fprintf(stderr, "alloc handles failed\n");
        free(args);
        return 1;
    }
    for (size_t i = 0; i < threads; i++) {
        args[i].seed = (uint32_t)(1234 + i);
        args[i].iters = iters;
        args[i].ws = ws;
        args[i].min_size = min_size;
        args[i].max_size = max_size;
        uintptr_t h = _beginthreadex(NULL, 0, bench_thread, &args[i], 0, NULL);
        handles[i] = (HANDLE)h;
    }
    WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    for (size_t i = 0; i < threads; i++) {
        CloseHandle(handles[i]);
    }
    free(handles);
#else
    pthread_t* tids = (pthread_t*)calloc(threads, sizeof(pthread_t));
    if (!tids) {
        fprintf(stderr, "alloc tids failed\n");
        free(args);
        return 1;
    }
    for (size_t i = 0; i < threads; i++) {
        args[i].seed = (uint32_t)(1234 + i);
        args[i].iters = iters;
        args[i].ws = ws;
        args[i].min_size = min_size;
        args[i].max_size = max_size;
        pthread_create(&tids[i], NULL, bench_thread, &args[i]);
    }
    for (size_t i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }
    free(tids);
#endif

    uint64_t end = now_ns();
    double sec = (double)(end - start) / 1000000000.0;
    double total_ops = (double)threads * (double)iters;
    double ops_sec = sec > 0.0 ? (total_ops / sec) : 0.0;
    size_t peak_kb = peak_working_set_kb();

    size_t alloc_attempts = 0;
    size_t alloc_successes = 0;
    size_t alloc_failures = 0;
    size_t frees = 0;
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats;
    memset(&hz6_stats, 0, sizeof(hz6_stats));
#endif
    for (size_t i = 0; i < threads; i++) {
        alloc_attempts += args[i].alloc_attempts;
        alloc_successes += args[i].alloc_successes;
        alloc_failures += args[i].alloc_failures;
        frees += args[i].frees;
#if defined(HZ_BENCH_USE_HZ6)
        hz6_stats.route_valid += args[i].hz6_stats_after.route_valid;
        hz6_stats.route_invalid += args[i].hz6_stats_after.route_invalid;
        hz6_stats.route_miss += args[i].hz6_stats_after.route_miss;
        hz6_stats.transfer_push += args[i].hz6_stats_after.transfer_push;
        hz6_stats.transfer_pop += args[i].hz6_stats_after.transfer_pop;
        hz6_stats.source_alloc += args[i].hz6_stats_after.source_alloc;
        hz6_stats.alloc_fail += args[i].hz6_stats_after.alloc_fail;
        hz6_stats.descriptor_exhausted +=
            args[i].hz6_stats_after.descriptor_exhausted;
        hz6_stats.route_register_fail +=
            args[i].hz6_stats_after.route_register_fail;
        hz6_stats.source_block_exhausted +=
            args[i].hz6_stats_after.source_block_exhausted;
        hz6_stats.descriptor_probe_total +=
            args[i].hz6_stats_after.descriptor_probe_total;
        if (args[i].hz6_stats_after.descriptor_probe_max >
            hz6_stats.descriptor_probe_max) {
            hz6_stats.descriptor_probe_max =
                args[i].hz6_stats_after.descriptor_probe_max;
        }
#if HZ6_DIAGNOSTIC_PROBES
#define HZ6_MAX_STAT(field) \
        do { \
            if (args[i].hz6_stats_after.field > hz6_stats.field) { \
                hz6_stats.field = args[i].hz6_stats_after.field; \
            } \
        } while (0)
        HZ6_MAX_STAT(descriptor_fail_active_max);
        HZ6_MAX_STAT(descriptor_fail_local_free_max);
        HZ6_MAX_STAT(descriptor_fail_transfer_free_max);
        HZ6_MAX_STAT(descriptor_fail_remote_pending_max);
        HZ6_MAX_STAT(descriptor_fail_central_free_max);
        HZ6_MAX_STAT(descriptor_fail_released_max);
        HZ6_MAX_STAT(descriptor_fail_orphan_max);
        HZ6_MAX_STAT(descriptor_fail_dead_with_ptr_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_total_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_largest_bin_max);
        HZ6_MAX_STAT(descriptor_fail_frontcache_nonempty_bins_max);
        hz6_stats.descriptor_frontcache_reuse_dryrun_calls +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_dryrun_calls;
        hz6_stats.descriptor_frontcache_reuse_requested_nonempty +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_requested_nonempty;
        hz6_stats.descriptor_frontcache_reuse_requested_total +=
            args[i].hz6_stats_after
                .descriptor_frontcache_reuse_requested_total;
        hz6_stats.descriptor_frontcache_reuse_donor_total +=
            args[i].hz6_stats_after.descriptor_frontcache_reuse_donor_total;
        HZ6_MAX_STAT(descriptor_frontcache_reuse_largest_donor_max);
        HZ6_MAX_STAT(descriptor_frontcache_reuse_donor_bins_max);
        hz6_stats.descriptorless_frontcache_push +=
            args[i].hz6_stats_after.descriptorless_frontcache_push;
        hz6_stats.descriptorless_frontcache_pop +=
            args[i].hz6_stats_after.descriptorless_frontcache_pop;
        hz6_stats.descriptorless_frontcache_descriptor_fail +=
            args[i].hz6_stats_after
                .descriptorless_frontcache_descriptor_fail;
        hz6_stats.descriptorless_frontcache_route_fail +=
            args[i].hz6_stats_after.descriptorless_frontcache_route_fail;
        hz6_stats.descriptorless_frontcache_invalid +=
            args[i].hz6_stats_after.descriptorless_frontcache_invalid;
        hz6_stats.descriptorreserve_frontcache_push +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_push;
        hz6_stats.descriptorreserve_frontcache_pop +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_pop;
        hz6_stats.descriptorreserve_frontcache_missing +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_missing;
        hz6_stats.descriptorreserve_frontcache_invalid +=
            args[i].hz6_stats_after.descriptorreserve_frontcache_invalid;
        hz6_stats.descgov_trigger_descriptor_fail +=
            args[i].hz6_stats_after.descgov_trigger_descriptor_fail;
        hz6_stats.descgov_detach_attempt +=
            args[i].hz6_stats_after.descgov_detach_attempt;
        hz6_stats.descgov_detach_success +=
            args[i].hz6_stats_after.descgov_detach_success;
        hz6_stats.descgov_detach_budget_denied +=
            args[i].hz6_stats_after.descgov_detach_budget_denied;
        hz6_stats.descgov_detach_class_denied +=
            args[i].hz6_stats_after.descgov_detach_class_denied;
        hz6_stats.descgov_materialize_admit +=
            args[i].hz6_stats_after.descgov_materialize_admit;
        hz6_stats.descgov_materialize_block_no_descriptor +=
            args[i].hz6_stats_after.descgov_materialize_block_no_descriptor;
        hz6_stats.descgov_materialize_fail +=
            args[i].hz6_stats_after.descgov_materialize_fail;
        hz6_stats.descgov_detached_current +=
            args[i].hz6_stats_after.descgov_detached_current;
        HZ6_MAX_STAT(descgov_detached_max);
        hz6_stats.frontcache_spill_dryrun_calls +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_calls;
        hz6_stats.frontcache_spill_dryrun_requested_empty +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_requested_empty;
        hz6_stats.frontcache_spill_dryrun_candidate_calls +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_candidate_calls;
        hz6_stats.frontcache_spill_dryrun_reclaimable_total +=
            args[i].hz6_stats_after.frontcache_spill_dryrun_reclaimable_total;
        HZ6_MAX_STAT(frontcache_spill_dryrun_largest_donor_max);
        HZ6_MAX_STAT(frontcache_spill_dryrun_donor_bins_max);
        hz6_stats.frontcache_spill_attempt +=
            args[i].hz6_stats_after.frontcache_spill_attempt;
        hz6_stats.frontcache_spill_success +=
            args[i].hz6_stats_after.frontcache_spill_success;
        hz6_stats.frontcache_spill_no_candidate +=
            args[i].hz6_stats_after.frontcache_spill_no_candidate;
        hz6_stats.frontcache_spill_invalid +=
            args[i].hz6_stats_after.frontcache_spill_invalid;
        hz6_stats.frontcache_spill_retry_success +=
            args[i].hz6_stats_after.frontcache_spill_retry_success;
        hz6_stats.frontcache_borrow_dryrun_calls +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_calls;
        hz6_stats.frontcache_borrow_dryrun_candidate_calls +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_candidate_calls;
        hz6_stats.frontcache_borrow_dryrun_candidate_total +=
            args[i].hz6_stats_after.frontcache_borrow_dryrun_candidate_total;
        HZ6_MAX_STAT(frontcache_borrow_dryrun_largest_candidate_max);
        hz6_stats.frontcache_borrow_attempt +=
            args[i].hz6_stats_after.frontcache_borrow_attempt;
        hz6_stats.frontcache_borrow_success +=
            args[i].hz6_stats_after.frontcache_borrow_success;
        hz6_stats.frontcache_borrow_no_candidate +=
            args[i].hz6_stats_after.frontcache_borrow_no_candidate;
        hz6_stats.frontcache_borrow_invalid +=
            args[i].hz6_stats_after.frontcache_borrow_invalid;
        hz6_stats.frontcache_cap_dryrun_push +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_push;
        hz6_stats.frontcache_cap_dryrun_over_cap +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_over_cap;
        hz6_stats.frontcache_cap_dryrun_would_release +=
            args[i].hz6_stats_after.frontcache_cap_dryrun_would_release;
        HZ6_MAX_STAT(frontcache_cap_dryrun_soft_cap_max);
        HZ6_MAX_STAT(frontcache_cap_dryrun_bin_count_max);
        hz6_stats.frontcache_cap_release +=
            args[i].hz6_stats_after.frontcache_cap_release;
        hz6_stats.source_run_reuse_dryrun_calls +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_calls;
        hz6_stats.source_run_reuse_dryrun_candidate_calls +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_candidate_calls;
        hz6_stats.source_run_reuse_dryrun_candidate_blocks_total +=
            args[i].hz6_stats_after
                .source_run_reuse_dryrun_candidate_blocks_total;
        hz6_stats.source_run_reuse_dryrun_free_slots_total +=
            args[i].hz6_stats_after.source_run_reuse_dryrun_free_slots_total;
        HZ6_MAX_STAT(source_run_reuse_dryrun_largest_free_slots_max);
        hz6_stats.source_run_reuse_attempt +=
            args[i].hz6_stats_after.source_run_reuse_attempt;
        hz6_stats.source_run_reuse_candidate +=
            args[i].hz6_stats_after.source_run_reuse_candidate;
        hz6_stats.source_run_reuse_hit +=
            args[i].hz6_stats_after.source_run_reuse_hit;
        hz6_stats.source_run_reuse_miss_no_block +=
            args[i].hz6_stats_after.source_run_reuse_miss_no_block;
        hz6_stats.source_run_reuse_miss_no_slot +=
            args[i].hz6_stats_after.source_run_reuse_miss_no_slot;
        hz6_stats.source_run_reuse_reserved +=
            args[i].hz6_stats_after.source_run_reuse_reserved;
        hz6_stats.source_run_reuse_slot_fail +=
            args[i].hz6_stats_after.source_run_reuse_slot_fail;
        hz6_stats.source_run_reuse_descriptor_fail +=
            args[i].hz6_stats_after.source_run_reuse_descriptor_fail;
        hz6_stats.source_run_reuse_descriptor_reclaim_attempt +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_attempt;
        hz6_stats.source_run_reuse_descriptor_reclaim_success +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_success;
        hz6_stats.source_run_reuse_descriptor_reclaim_no_candidate +=
            args[i].hz6_stats_after
                .source_run_reuse_descriptor_reclaim_no_candidate;
        hz6_stats.source_run_reuse_same_class_reclaim_attempt +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_attempt;
        hz6_stats.source_run_reuse_same_class_reclaim_success +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_success;
        hz6_stats.source_run_reuse_same_class_reclaim_no_candidate +=
            args[i].hz6_stats_after
                .source_run_reuse_same_class_reclaim_no_candidate;
        hz6_stats.source_run_reuse_route_fail +=
            args[i].hz6_stats_after.source_run_reuse_route_fail;
        hz6_stats.source_run_reuse_prepare_fail +=
            args[i].hz6_stats_after.source_run_reuse_prepare_fail;
        hz6_stats.source_run_reuse_rollback +=
            args[i].hz6_stats_after.source_run_reuse_rollback;
        hz6_stats.source_run_reuse_used_count_mismatch +=
            args[i].hz6_stats_after.source_run_reuse_used_count_mismatch;
        for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT;
             ++class_id) {
            hz6_stats.frontcache_push_by_class[class_id] +=
                args[i].hz6_stats_after.frontcache_push_by_class[class_id];
            hz6_stats.frontcache_pop_empty_by_class[class_id] +=
                args[i].hz6_stats_after
                    .frontcache_pop_empty_by_class[class_id];
        }
#undef HZ6_MAX_STAT
#endif
        hz6_stats.route_lookup_probe_total +=
            args[i].hz6_stats_after.route_lookup_probe_total;
#if HZ6_DIAGNOSTIC_PROBES
        for (size_t bucket = 0; bucket < HZ6_ROUTE_PROBE_BUCKET_COUNT;
             ++bucket) {
            hz6_stats.route_lookup_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_lookup_probe_hist[bucket];
            hz6_stats.route_register_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_register_probe_hist[bucket];
            hz6_stats.route_unregister_probe_hist[bucket] +=
                args[i].hz6_stats_after.route_unregister_probe_hist[bucket];
        }
#endif
        if (args[i].hz6_stats_after.route_lookup_probe_max >
            hz6_stats.route_lookup_probe_max) {
            hz6_stats.route_lookup_probe_max =
                args[i].hz6_stats_after.route_lookup_probe_max;
        }
        hz6_stats.route_register_probe_total +=
            args[i].hz6_stats_after.route_register_probe_total;
        if (args[i].hz6_stats_after.route_register_probe_max >
            hz6_stats.route_register_probe_max) {
            hz6_stats.route_register_probe_max =
                args[i].hz6_stats_after.route_register_probe_max;
        }
        hz6_stats.route_unregister_probe_total +=
            args[i].hz6_stats_after.route_unregister_probe_total;
        if (args[i].hz6_stats_after.route_unregister_probe_max >
            hz6_stats.route_unregister_probe_max) {
            hz6_stats.route_unregister_probe_max =
                args[i].hz6_stats_after.route_unregister_probe_max;
        }
        hz6_stats.source_block_probe_total +=
            args[i].hz6_stats_after.source_block_probe_total;
        if (args[i].hz6_stats_after.source_block_probe_max >
            hz6_stats.source_block_probe_max) {
            hz6_stats.source_block_probe_max =
                args[i].hz6_stats_after.source_block_probe_max;
        }
#if HZ6_DIAGNOSTIC_PROBES
#define HZ6_MAX_STAT(field) \
        do { \
            if (args[i].hz6_stats_after.field > hz6_stats.field) { \
                hz6_stats.field = args[i].hz6_stats_after.field; \
            } \
        } while (0)
        HZ6_MAX_STAT(source_block_fail_active_max);
        HZ6_MAX_STAT(source_block_fail_registered_max);
        HZ6_MAX_STAT(source_block_fail_ref_nonzero_max);
        HZ6_MAX_STAT(source_block_fail_ref_zero_max);
#undef HZ6_MAX_STAT
#endif
        hz6_stats.large_span_central_push +=
            args[i].hz6_stats_after.large_span_central_push;
        hz6_stats.large_span_central_pop +=
            args[i].hz6_stats_after.large_span_central_pop;
        hz6_stats.large_span_source_alloc +=
            args[i].hz6_stats_after.large_span_source_alloc;
#endif
    }

    printf("threads=%zu iters=%zu ws=%zu size=%zu..%zu time=%.3f ops/s=%.3f "
           "alloc_attempts=%zu alloc_success=%zu alloc_fail=%zu frees=%zu",
           threads, iters, ws, min_size, max_size, sec, ops_sec,
           alloc_attempts, alloc_successes, alloc_failures, frees);
#if defined(HZ_BENCH_USE_HZ6)
    printf(" hz6_route_valid=%zu hz6_route_invalid=%zu hz6_route_miss=%zu "
           "hz6_transfer_push=%zu hz6_transfer_pop=%zu "
           "hz6_source_alloc=%zu hz6_alloc_fail=%zu "
           "hz6_descriptor_exhausted=%zu hz6_route_register_fail=%zu "
           "hz6_source_block_exhausted=%zu hz6_descriptor_probe_total=%zu "
           "hz6_descriptor_probe_max=%zu "
           "hz6_descriptor_fail_active_max=%zu "
           "hz6_descriptor_fail_local_free_max=%zu "
           "hz6_descriptor_fail_transfer_free_max=%zu "
           "hz6_descriptor_fail_remote_pending_max=%zu "
           "hz6_descriptor_fail_central_free_max=%zu "
           "hz6_descriptor_fail_released_max=%zu "
           "hz6_descriptor_fail_orphan_max=%zu "
           "hz6_descriptor_fail_dead_with_ptr_max=%zu "
           "hz6_descriptor_fail_frontcache_total_max=%zu "
           "hz6_descriptor_fail_frontcache_largest_bin_max=%zu "
           "hz6_descriptor_fail_frontcache_nonempty_bins_max=%zu "
           "hz6_descriptor_frontcache_reuse_dryrun_calls=%zu "
           "hz6_descriptor_frontcache_reuse_requested_nonempty=%zu "
           "hz6_descriptor_frontcache_reuse_requested_total=%zu "
           "hz6_descriptor_frontcache_reuse_donor_total=%zu "
           "hz6_descriptor_frontcache_reuse_largest_donor_max=%zu "
           "hz6_descriptor_frontcache_reuse_donor_bins_max=%zu "
           "hz6_descriptorless_frontcache_push=%zu "
           "hz6_descriptorless_frontcache_pop=%zu "
           "hz6_descriptorless_frontcache_descriptor_fail=%zu "
           "hz6_descriptorless_frontcache_route_fail=%zu "
           "hz6_descriptorless_frontcache_invalid=%zu "
           "hz6_descriptorreserve_frontcache_push=%zu "
           "hz6_descriptorreserve_frontcache_pop=%zu "
           "hz6_descriptorreserve_frontcache_missing=%zu "
           "hz6_descriptorreserve_frontcache_invalid=%zu "
           "hz6_descgov_trigger_descriptor_fail=%zu "
           "hz6_descgov_detach_attempt=%zu "
           "hz6_descgov_detach_success=%zu "
           "hz6_descgov_detach_budget_denied=%zu "
           "hz6_descgov_detach_class_denied=%zu "
           "hz6_descgov_materialize_admit=%zu "
           "hz6_descgov_materialize_block_no_descriptor=%zu "
           "hz6_descgov_materialize_fail=%zu "
           "hz6_descgov_detached_current=%zu "
           "hz6_descgov_detached_max=%zu "
           "hz6_frontcache_spill_dryrun_calls=%zu "
           "hz6_frontcache_spill_dryrun_requested_empty=%zu "
           "hz6_frontcache_spill_dryrun_candidate_calls=%zu "
           "hz6_frontcache_spill_dryrun_reclaimable_total=%zu "
           "hz6_frontcache_spill_dryrun_largest_donor_max=%zu "
           "hz6_frontcache_spill_dryrun_donor_bins_max=%zu "
           "hz6_frontcache_spill_attempt=%zu "
           "hz6_frontcache_spill_success=%zu "
           "hz6_frontcache_spill_no_candidate=%zu "
           "hz6_frontcache_spill_invalid=%zu "
           "hz6_frontcache_spill_retry_success=%zu "
           "hz6_frontcache_borrow_dryrun_calls=%zu "
           "hz6_frontcache_borrow_dryrun_candidate_calls=%zu "
           "hz6_frontcache_borrow_dryrun_candidate_total=%zu "
           "hz6_frontcache_borrow_dryrun_largest_candidate_max=%zu "
           "hz6_frontcache_borrow_attempt=%zu "
           "hz6_frontcache_borrow_success=%zu "
           "hz6_frontcache_borrow_no_candidate=%zu "
           "hz6_frontcache_borrow_invalid=%zu "
           "hz6_frontcache_cap_dryrun_push=%zu "
           "hz6_frontcache_cap_dryrun_over_cap=%zu "
           "hz6_frontcache_cap_dryrun_would_release=%zu "
           "hz6_frontcache_cap_dryrun_soft_cap_max=%zu "
           "hz6_frontcache_cap_dryrun_bin_count_max=%zu "
           "hz6_frontcache_cap_release=%zu "
           "hz6_source_run_reuse_dryrun_calls=%zu "
           "hz6_source_run_reuse_dryrun_candidate_calls=%zu "
           "hz6_source_run_reuse_dryrun_candidate_blocks_total=%zu "
           "hz6_source_run_reuse_dryrun_free_slots_total=%zu "
           "hz6_source_run_reuse_dryrun_largest_free_slots_max=%zu "
           "hz6_source_run_reuse_attempt=%zu "
           "hz6_source_run_reuse_candidate=%zu "
           "hz6_source_run_reuse_hit=%zu "
           "hz6_source_run_reuse_miss_no_block=%zu "
           "hz6_source_run_reuse_miss_no_slot=%zu "
           "hz6_source_run_reuse_reserved=%zu "
           "hz6_source_run_reuse_slot_fail=%zu "
           "hz6_source_run_reuse_descriptor_fail=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_attempt=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_success=%zu "
           "hz6_source_run_reuse_descriptor_reclaim_no_candidate=%zu "
           "hz6_source_run_reuse_same_class_reclaim_attempt=%zu "
           "hz6_source_run_reuse_same_class_reclaim_success=%zu "
           "hz6_source_run_reuse_same_class_reclaim_no_candidate=%zu "
           "hz6_source_run_reuse_route_fail=%zu "
           "hz6_source_run_reuse_prepare_fail=%zu "
           "hz6_source_run_reuse_rollback=%zu "
           "hz6_source_run_reuse_used_count_mismatch=%zu "
           "hz6_route_lookup_probe_total=%zu "
           "hz6_route_lookup_probe_max=%zu "
           "hz6_route_register_probe_total=%zu "
           "hz6_route_register_probe_max=%zu "
           "hz6_route_unregister_probe_total=%zu "
           "hz6_route_unregister_probe_max=%zu "
           "hz6_source_block_probe_total=%zu "
           "hz6_source_block_probe_max=%zu "
           "hz6_source_block_fail_active_max=%zu "
           "hz6_source_block_fail_registered_max=%zu "
           "hz6_source_block_fail_ref_nonzero_max=%zu "
           "hz6_source_block_fail_ref_zero_max=%zu "
           "hz6_large_central_push=%zu hz6_large_central_pop=%zu "
           "hz6_large_source_alloc=%zu",
           hz6_stats.route_valid, hz6_stats.route_invalid,
           hz6_stats.route_miss, hz6_stats.transfer_push,
           hz6_stats.transfer_pop, hz6_stats.source_alloc,
           hz6_stats.alloc_fail, hz6_stats.descriptor_exhausted,
           hz6_stats.route_register_fail, hz6_stats.source_block_exhausted,
           hz6_stats.descriptor_probe_total, hz6_stats.descriptor_probe_max,
           hz6_stats.descriptor_fail_active_max,
           hz6_stats.descriptor_fail_local_free_max,
           hz6_stats.descriptor_fail_transfer_free_max,
           hz6_stats.descriptor_fail_remote_pending_max,
           hz6_stats.descriptor_fail_central_free_max,
           hz6_stats.descriptor_fail_released_max,
           hz6_stats.descriptor_fail_orphan_max,
           hz6_stats.descriptor_fail_dead_with_ptr_max,
           hz6_stats.descriptor_fail_frontcache_total_max,
           hz6_stats.descriptor_fail_frontcache_largest_bin_max,
           hz6_stats.descriptor_fail_frontcache_nonempty_bins_max,
           hz6_stats.descriptor_frontcache_reuse_dryrun_calls,
           hz6_stats.descriptor_frontcache_reuse_requested_nonempty,
           hz6_stats.descriptor_frontcache_reuse_requested_total,
           hz6_stats.descriptor_frontcache_reuse_donor_total,
           hz6_stats.descriptor_frontcache_reuse_largest_donor_max,
           hz6_stats.descriptor_frontcache_reuse_donor_bins_max,
           hz6_stats.descriptorless_frontcache_push,
           hz6_stats.descriptorless_frontcache_pop,
           hz6_stats.descriptorless_frontcache_descriptor_fail,
           hz6_stats.descriptorless_frontcache_route_fail,
           hz6_stats.descriptorless_frontcache_invalid,
           hz6_stats.descriptorreserve_frontcache_push,
           hz6_stats.descriptorreserve_frontcache_pop,
           hz6_stats.descriptorreserve_frontcache_missing,
           hz6_stats.descriptorreserve_frontcache_invalid,
           hz6_stats.descgov_trigger_descriptor_fail,
           hz6_stats.descgov_detach_attempt,
           hz6_stats.descgov_detach_success,
           hz6_stats.descgov_detach_budget_denied,
           hz6_stats.descgov_detach_class_denied,
           hz6_stats.descgov_materialize_admit,
           hz6_stats.descgov_materialize_block_no_descriptor,
           hz6_stats.descgov_materialize_fail,
           hz6_stats.descgov_detached_current,
           hz6_stats.descgov_detached_max,
           hz6_stats.frontcache_spill_dryrun_calls,
           hz6_stats.frontcache_spill_dryrun_requested_empty,
           hz6_stats.frontcache_spill_dryrun_candidate_calls,
           hz6_stats.frontcache_spill_dryrun_reclaimable_total,
           hz6_stats.frontcache_spill_dryrun_largest_donor_max,
           hz6_stats.frontcache_spill_dryrun_donor_bins_max,
           hz6_stats.frontcache_spill_attempt,
           hz6_stats.frontcache_spill_success,
           hz6_stats.frontcache_spill_no_candidate,
           hz6_stats.frontcache_spill_invalid,
           hz6_stats.frontcache_spill_retry_success,
           hz6_stats.frontcache_borrow_dryrun_calls,
           hz6_stats.frontcache_borrow_dryrun_candidate_calls,
           hz6_stats.frontcache_borrow_dryrun_candidate_total,
           hz6_stats.frontcache_borrow_dryrun_largest_candidate_max,
           hz6_stats.frontcache_borrow_attempt,
           hz6_stats.frontcache_borrow_success,
           hz6_stats.frontcache_borrow_no_candidate,
           hz6_stats.frontcache_borrow_invalid,
           hz6_stats.frontcache_cap_dryrun_push,
           hz6_stats.frontcache_cap_dryrun_over_cap,
           hz6_stats.frontcache_cap_dryrun_would_release,
           hz6_stats.frontcache_cap_dryrun_soft_cap_max,
           hz6_stats.frontcache_cap_dryrun_bin_count_max,
           hz6_stats.frontcache_cap_release,
           hz6_stats.source_run_reuse_dryrun_calls,
           hz6_stats.source_run_reuse_dryrun_candidate_calls,
           hz6_stats.source_run_reuse_dryrun_candidate_blocks_total,
           hz6_stats.source_run_reuse_dryrun_free_slots_total,
           hz6_stats.source_run_reuse_dryrun_largest_free_slots_max,
           hz6_stats.source_run_reuse_attempt,
           hz6_stats.source_run_reuse_candidate,
           hz6_stats.source_run_reuse_hit,
           hz6_stats.source_run_reuse_miss_no_block,
           hz6_stats.source_run_reuse_miss_no_slot,
           hz6_stats.source_run_reuse_reserved,
           hz6_stats.source_run_reuse_slot_fail,
           hz6_stats.source_run_reuse_descriptor_fail,
           hz6_stats.source_run_reuse_descriptor_reclaim_attempt,
           hz6_stats.source_run_reuse_descriptor_reclaim_success,
           hz6_stats.source_run_reuse_descriptor_reclaim_no_candidate,
           hz6_stats.source_run_reuse_same_class_reclaim_attempt,
           hz6_stats.source_run_reuse_same_class_reclaim_success,
           hz6_stats.source_run_reuse_same_class_reclaim_no_candidate,
           hz6_stats.source_run_reuse_route_fail,
           hz6_stats.source_run_reuse_prepare_fail,
           hz6_stats.source_run_reuse_rollback,
           hz6_stats.source_run_reuse_used_count_mismatch,
           hz6_stats.route_lookup_probe_total,
           hz6_stats.route_lookup_probe_max,
           hz6_stats.route_register_probe_total,
           hz6_stats.route_register_probe_max,
           hz6_stats.route_unregister_probe_total,
           hz6_stats.route_unregister_probe_max,
           hz6_stats.source_block_probe_total,
           hz6_stats.source_block_probe_max,
           hz6_stats.source_block_fail_active_max,
           hz6_stats.source_block_fail_registered_max,
           hz6_stats.source_block_fail_ref_nonzero_max,
           hz6_stats.source_block_fail_ref_zero_max,
           hz6_stats.large_span_central_push,
           hz6_stats.large_span_central_pop,
           hz6_stats.large_span_source_alloc);
#endif
    printf(" peak_kb=%zu\n", peak_kb);
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
        size_t push = hz6_stats.frontcache_push_by_class[class_id];
        size_t pop_empty = hz6_stats.frontcache_pop_empty_by_class[class_id];
        if (push == 0 && pop_empty == 0) {
            continue;
        }
        printf("[HZ6_FRONTCACHE_CLASS] class=%zu push=%zu pop_empty=%zu\n",
               class_id, push, pop_empty);
    }
#endif
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    printf("[HZ6_ROUTE_PROBE_SHAPE] kind=lookup b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu\n",
           hz6_stats.route_lookup_probe_hist[0],
           hz6_stats.route_lookup_probe_hist[1],
           hz6_stats.route_lookup_probe_hist[2],
           hz6_stats.route_lookup_probe_hist[3],
           hz6_stats.route_lookup_probe_hist[4],
           hz6_stats.route_lookup_probe_hist[5]);
    printf("[HZ6_ROUTE_PROBE_SHAPE] kind=register b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu\n",
           hz6_stats.route_register_probe_hist[0],
           hz6_stats.route_register_probe_hist[1],
           hz6_stats.route_register_probe_hist[2],
           hz6_stats.route_register_probe_hist[3],
           hz6_stats.route_register_probe_hist[4],
           hz6_stats.route_register_probe_hist[5]);
    printf("[HZ6_ROUTE_PROBE_SHAPE] kind=unregister b1=%zu b2_4=%zu b5_8=%zu "
           "b9_16=%zu b17_64=%zu b65p=%zu\n",
           hz6_stats.route_unregister_probe_hist[0],
           hz6_stats.route_unregister_probe_hist[1],
           hz6_stats.route_unregister_probe_hist[2],
           hz6_stats.route_unregister_probe_hist[3],
           hz6_stats.route_unregister_probe_hist[4],
           hz6_stats.route_unregister_probe_hist[5]);
#endif

    free(args);
    return 0;
}
