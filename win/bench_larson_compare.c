// Windows-native Larson-style allocator comparison bench.
// Usage: bench_larson_compare [runtime_sec] [min_size] [max_size] [chunks_per_thread] [rounds] [seed] [threads] [warmup_mode]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench_modern_allocator_adapter.h"
#if defined(HZ_BENCH_USE_HZ6)
#include "bench_larson_hz6_summary.h"
#endif
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
#include "bench_larson_hz6_diag.h"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

typedef struct LarsonThreadData {
    void** blocks;
    size_t* sizes;
    size_t thread_index;
    size_t live_count;
    size_t iterations_per_check;
    size_t min_size;
    size_t max_size;
    uint32_t seed;
    int warmup_in_worker;
    uint64_t alloc_count;
    uint64_t free_count;
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats_after;
#endif
} LarsonThreadData;

static volatile LONG g_stop_flag = 0;
static volatile LONG g_start_flag = 1;
static volatile LONG g_ready_count = 0;
static volatile LONG g_worker_warmup_fail_count = 0;
static volatile LONG g_worker_warmup_fail_thread = -1;
static volatile LONG g_worker_warmup_fail_slot = -1;
static volatile LONG g_worker_warmup_fail_size = 0;

static LONG WINAPI bench_unhandled_exception_filter(
    struct _EXCEPTION_POINTERS* exception_info) {
    DWORD code = 0;
    void* address = NULL;
    if (exception_info && exception_info->ExceptionRecord) {
        code = exception_info->ExceptionRecord->ExceptionCode;
        address = exception_info->ExceptionRecord->ExceptionAddress;
    }
    fprintf(stderr,
            "bench_larson_compare: unhandled exception code=0x%08lx address=%p worker_fail_count=%ld first_thread=%ld first_slot=%ld first_size=%ld\n",
            (unsigned long)code,
            address,
            InterlockedCompareExchange(&g_worker_warmup_fail_count, 0, 0),
            InterlockedCompareExchange(&g_worker_warmup_fail_thread, 0, 0),
            InterlockedCompareExchange(&g_worker_warmup_fail_slot, 0, 0),
            InterlockedCompareExchange(&g_worker_warmup_fail_size, 0, 0));
    hz_bench_dump_stats(stderr, "larson_unhandled_exception");
    fflush(stderr);
    ExitProcess(3);
    return EXCEPTION_EXECUTE_HANDLER;
}

static uint32_t parse_u32(const char* s, uint32_t def) {
    if (!s) {
        return def;
    }
    {
        char* end = NULL;
        unsigned long v = strtoul(s, &end, 10);
        if (end == s || (end && *end != '\0')) {
            return def;
        }
        if (v > 0xFFFFFFFFu) {
            return 0xFFFFFFFFu;
        }
        return (uint32_t)v;
    }
}

static inline uint32_t rng_next(uint32_t* state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1u) : 1u;
    return min_size + (r % span);
}

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

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
static size_t hz6_larson_next_pow2_capacity(size_t value) {
    size_t cap = 1u;
    if (value == 0u) {
        return 0u;
    }
    while (cap < value && cap <= ((size_t)-1) / 2u) {
        cap *= 2u;
    }
    return cap;
}

static size_t hz6_larson_headroom_capacity(size_t max_used) {
    if (max_used == 0u) {
        return 0u;
    }
    return hz6_larson_next_pow2_capacity(max_used * 2u);
}

static size_t hz6_larson_project_table_bytes(size_t current_bytes,
                                             size_t current_capacity,
                                             size_t projected_capacity) {
    if (current_bytes == 0u || current_capacity == 0u ||
        projected_capacity == 0u) {
        return 0u;
    }
    return (current_bytes / current_capacity) * projected_capacity;
}

static size_t hz6_larson_overflow_count(size_t used, size_t local_capacity) {
    return (used > local_capacity) ? (used - local_capacity) : 0u;
}

static size_t hz6_larson_descriptor_used_from_stats(
    const Hz6StatsSnapshot* stats) {
    if (!stats) {
        return 0u;
    }
    return stats->memory_active_descriptors +
           stats->memory_local_free_descriptors +
           stats->memory_transfer_free_descriptors +
           stats->memory_remote_pending_descriptors +
           stats->memory_dead_with_ptr_descriptors;
}
#endif

static inline void* bench_alloc(size_t size) {
    return hz_bench_alloc(size);
}

static inline void bench_free(void* ptr) {
    hz_bench_free(ptr);
}

static inline void touch_allocation(void* ptr, size_t size, uint32_t tag) {
    unsigned char* p = (unsigned char*)ptr;
    if (!p || size == 0) {
        return;
    }
    p[0] = (unsigned char)(tag & 0xFFu);
    if (size > 1) {
    p[size - 1] = (unsigned char)((tag >> 8) & 0xFFu);
    }
}

static int warmup_thread_blocks(LarsonThreadData* td) {
    uint32_t local_seed = td->seed ^ 0x9E3779B9u;
    size_t i;

    for (i = 0; i < td->live_count; ++i) {
        size_t size = pick_size(rng_next(&local_seed), td->min_size, td->max_size);
        void* p = bench_alloc(size);
        if (!p) {
            if (InterlockedCompareExchange(&g_worker_warmup_fail_count, 1, 0) == 0) {
                InterlockedExchange(&g_worker_warmup_fail_thread, (LONG)td->thread_index);
                InterlockedExchange(&g_worker_warmup_fail_slot, (LONG)i);
                InterlockedExchange(&g_worker_warmup_fail_size, (LONG)size);
            }
            fprintf(stderr,
                    "bench_larson_compare: worker warmup alloc failed at thread=%zu slot=%zu size=%zu\n",
                    td->thread_index, i, size);
            hz_bench_dump_stats(stderr, "larson_worker_warmup_alloc_fail");
            fflush(stderr);
            return 0;
        }
        touch_allocation(p, size, local_seed);
        td->blocks[i] = p;
        td->sizes[i] = size;
    }
    return 1;
}

static void free_thread_blocks(LarsonThreadData* td) {
    size_t i;

    if (!td || !td->blocks) {
        return;
    }
    for (i = 0; i < td->live_count; ++i) {
        if (td->blocks[i]) {
            bench_free(td->blocks[i]);
            td->blocks[i] = NULL;
            td->sizes[i] = 0;
        }
    }
}

static unsigned __stdcall larson_thread(void* arg) {
    LarsonThreadData* td = (LarsonThreadData*)arg;
    uint32_t seed = td->seed;

    hz_bench_allocator_thread_setup();

    if (td->warmup_in_worker) {
        if (!warmup_thread_blocks(td)) {
            InterlockedExchange(&g_stop_flag, 1);
        }
        InterlockedIncrement(&g_ready_count);
        while (InterlockedCompareExchange(&g_start_flag, 0, 0) == 0 &&
               InterlockedCompareExchange(&g_stop_flag, 0, 0) == 0) {
            Sleep(0);
        }
    }

    while (InterlockedCompareExchange(&g_stop_flag, 0, 0) == 0) {
        size_t i;
        for (i = 0; i < td->iterations_per_check; ++i) {
            size_t victim;
            uint32_t r;
            size_t new_size;
            void* next;

            if (InterlockedCompareExchange(&g_stop_flag, 0, 0) != 0) {
                break;
            }

            r = rng_next(&seed);
            victim = (size_t)(r % (uint32_t)td->live_count);
            if (td->blocks[victim]) {
                bench_free(td->blocks[victim]);
                td->blocks[victim] = NULL;
                td->sizes[victim] = 0;
                td->free_count++;
            }

            new_size = pick_size(rng_next(&seed), td->min_size, td->max_size);
            next = bench_alloc(new_size);
            if (!next) {
                hz_bench_dump_stats(stderr, "larson_thread_alloc_fail");
                continue;
            }
            touch_allocation(next, new_size, r);
            td->blocks[victim] = next;
            td->sizes[victim] = new_size;
            td->alloc_count++;
        }
    }

#if defined(HZ_BENCH_USE_HZ6)
    td->hz6_stats_after = hz6_stats_snapshot(hz_bench_tls_hz6_initialized
                                                 ? &hz_bench_tls_hz6_allocator
                                                 : NULL);
#endif
    /*
     * Cleanup is intentionally after the measured snapshot: main-warmup rows
     * can rehome objects to worker-local allocators during the timed phase, so
     * each worker must release its live set before tearing that allocator down.
     */
    free_thread_blocks(td);
    hz_bench_allocator_thread_teardown();
    return 0;
}

int main(int argc, char** argv) {
    uint32_t runtime_sec = (argc > 1) ? parse_u32(argv[1], 10u) : 10u;
    uint32_t min_size = (argc > 2) ? parse_u32(argv[2], 8u) : 8u;
    uint32_t max_size = (argc > 3) ? parse_u32(argv[3], 1024u) : 1024u;
    uint32_t chunks_per_thread = (argc > 4) ? parse_u32(argv[4], 10000u) : 10000u;
    uint32_t rounds = (argc > 5) ? parse_u32(argv[5], 1u) : 1u;
    uint32_t seed = (argc > 6) ? parse_u32(argv[6], 12345u) : 12345u;
    uint32_t threads = (argc > 7) ? parse_u32(argv[7], 4u) : 4u;
    uint32_t warmup_mode = (argc > 8) ? parse_u32(argv[8], 0u) : 0u;
    int worker_warmup = (warmup_mode != 0u);

    SetUnhandledExceptionFilter(bench_unhandled_exception_filter);

    LarsonThreadData* tds = NULL;
    HANDLE* handles = NULL;
    void** all_blocks = NULL;
    size_t* all_sizes = NULL;
    size_t total_slots;
    uint64_t start_ns;
    uint64_t end_ns;
    double duration_sec;
    uint64_t total_allocs = 0;
    uint64_t total_frees = 0;
    size_t t;
    int exit_code = 0;
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats;
    memset(&hz6_stats, 0, sizeof(hz6_stats));
#if HZ6_DIAGNOSTIC_PROBES
    size_t hz6_descriptor_used_max_allocator = 0;
    size_t hz6_route_used_max_allocator = 0;
    size_t hz6_route_occupied_max_allocator = 0;
    size_t hz6_source_block_used_max_allocator = 0;
    size_t hz6_frontcache_used_max_allocator = 0;
    size_t hz6_transfer_used_max_allocator = 0;
    Hz6StatsSnapshot hz6_main_warmup_stats;
    int hz6_has_main_warmup_stats = 0;
    memset(&hz6_main_warmup_stats, 0, sizeof(hz6_main_warmup_stats));
#endif
#endif

    if (runtime_sec == 0 || chunks_per_thread == 0 || rounds == 0 || threads == 0 || min_size == 0 || max_size < min_size) {
        fprintf(stderr, "bench_larson_compare: invalid args\n");
        fprintf(stderr, "usage: %s <runtime_sec> <min> <max> <chunks_per_thread> <rounds> <seed> <threads> <warmup_mode>\n", argv[0]);
        return 2;
    }

    total_slots = (size_t)threads * (size_t)chunks_per_thread;

    printf("[BENCH_ARGS] runtime=%u min=%u max=%u chunks=%u rounds=%u seed=%u threads=%u warmup=%s\n",
           runtime_sec, min_size, max_size, chunks_per_thread, rounds, seed,
           threads, worker_warmup ? "worker" : "main");
    fflush(stdout);

    hz_bench_allocator_thread_setup();

    tds = (LarsonThreadData*)calloc(threads, sizeof(LarsonThreadData));
    handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    all_blocks = (void**)calloc(total_slots, sizeof(void*));
    all_sizes = (size_t*)calloc(total_slots, sizeof(size_t));
    if (!tds || !handles || !all_blocks || !all_sizes) {
        fprintf(stderr, "bench_larson_compare: setup allocation failed\n");
        free(all_sizes);
        free(all_blocks);
        free(handles);
        free(tds);
        return 1;
    }

    for (t = 0; t < threads; ++t) {
        size_t base = t * (size_t)chunks_per_thread;
        size_t i;
        uint32_t local_seed = seed + (uint32_t)(t * 977u);
        if (!worker_warmup) {
            for (i = 0; i < chunks_per_thread; ++i) {
                size_t slot = base + i;
                size_t size = pick_size(rng_next(&local_seed), min_size, max_size);
                void* p = bench_alloc(size);
                if (!p) {
                    fprintf(stderr, "bench_larson_compare: warmup alloc failed at thread=%zu slot=%zu size=%zu\n", t, i, size);
                    hz_bench_dump_stats(stderr, "larson_warmup_alloc_fail");
                    exit_code = 1;
                    goto cleanup;
                }
                touch_allocation(p, size, local_seed);
                all_blocks[slot] = p;
                all_sizes[slot] = size;
            }
        }

        tds[t].blocks = all_blocks + base;
        tds[t].sizes = all_sizes + base;
        tds[t].thread_index = t;
        tds[t].live_count = (size_t)chunks_per_thread;
        tds[t].iterations_per_check = (size_t)chunks_per_thread * (size_t)rounds;
        tds[t].min_size = min_size;
        tds[t].max_size = max_size;
        tds[t].seed = seed + (uint32_t)(t * 131u) + 1u;
        tds[t].warmup_in_worker = worker_warmup;
        tds[t].alloc_count = 0;
        tds[t].free_count = 0;
    }

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    if (!worker_warmup && hz_bench_tls_hz6_initialized) {
        hz6_main_warmup_stats =
            hz6_stats_snapshot(&hz_bench_tls_hz6_allocator);
        hz6_has_main_warmup_stats = 1;
    }
#endif

    InterlockedExchange(&g_stop_flag, 0);
    InterlockedExchange(&g_ready_count, 0);
    InterlockedExchange(&g_start_flag, worker_warmup ? 0 : 1);
    InterlockedExchange(&g_worker_warmup_fail_count, 0);
    InterlockedExchange(&g_worker_warmup_fail_thread, -1);
    InterlockedExchange(&g_worker_warmup_fail_slot, -1);
    InterlockedExchange(&g_worker_warmup_fail_size, 0);

    for (t = 0; t < threads; ++t) {
        uintptr_t h = _beginthreadex(NULL, 0, larson_thread, &tds[t], 0, NULL);
        if (h == 0) {
            fprintf(stderr, "bench_larson_compare: thread start failed at thread=%zu\n", t);
            InterlockedExchange(&g_stop_flag, 1);
            InterlockedExchange(&g_start_flag, 1);
            for (size_t j = 0; j < t; ++j) {
                WaitForSingleObject(handles[j], INFINITE);
                CloseHandle(handles[j]);
                handles[j] = NULL;
            }
            exit_code = 1;
            goto cleanup;
        }
        handles[t] = (HANDLE)h;
    }

    if (worker_warmup) {
        while (InterlockedCompareExchange(&g_ready_count, 0, 0) < (LONG)threads &&
               InterlockedCompareExchange(&g_stop_flag, 0, 0) == 0) {
            Sleep(1);
        }
        if (InterlockedCompareExchange(&g_stop_flag, 0, 0) != 0) {
            InterlockedExchange(&g_start_flag, 1);
            WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
            for (size_t j = 0; j < threads; ++j) {
                if (handles[j]) {
                    CloseHandle(handles[j]);
                    handles[j] = NULL;
                }
            }
            fprintf(stderr,
                    "bench_larson_compare: worker warmup failed count=%ld first_thread=%ld first_slot=%ld first_size=%ld\n",
                    InterlockedCompareExchange(&g_worker_warmup_fail_count, 0, 0),
                    InterlockedCompareExchange(&g_worker_warmup_fail_thread, 0, 0),
                    InterlockedCompareExchange(&g_worker_warmup_fail_slot, 0, 0),
                    InterlockedCompareExchange(&g_worker_warmup_fail_size, 0, 0));
            fflush(stderr);
            exit_code = 1;
            goto cleanup;
        }
    }

    start_ns = now_ns();
    InterlockedExchange(&g_start_flag, 1);
    Sleep(runtime_sec * 1000u);
    InterlockedExchange(&g_stop_flag, 1);

    WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    end_ns = now_ns();

    for (t = 0; t < threads; ++t) {
        total_allocs += tds[t].alloc_count;
        total_frees += tds[t].free_count;
#include "bench_larson_hz6_stats_accumulate.inc"
        CloseHandle(handles[t]);
        handles[t] = NULL;
    }

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    hz6_stats.memory_static_table_bytes =
        hz6_stats.memory_descriptor_table_bytes +
        hz6_stats.memory_route_table_bytes +
        hz6_stats.memory_source_block_table_bytes +
        hz6_stats.memory_source_block_depot_bytes +
        hz6_stats.memory_frontcache_table_bytes +
        hz6_stats.memory_transfer_table_bytes +
        hz6_stats.memory_ownerlocality_index_bytes;
    hz6_stats.memory_static_plus_payload_bytes =
        hz6_stats.memory_static_table_bytes +
        hz6_stats.memory_source_block_committed_estimate;
#endif

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    const size_t hz6_allocator_count = (size_t)threads;
    const size_t hz6_descriptor_capacity =
        HZ6_OBJECT_DESCRIPTOR_CAPACITY * hz6_allocator_count;
    const size_t hz6_descriptor_used =
        hz6_larson_descriptor_used_from_stats(&hz6_stats);
    const size_t hz6_descriptor_unused =
        (hz6_descriptor_capacity > hz6_descriptor_used)
            ? (hz6_descriptor_capacity - hz6_descriptor_used)
            : 0u;
    const size_t hz6_route_capacity =
        HZ6_ROUTE_TABLE_CAPACITY * hz6_allocator_count;
    const size_t hz6_route_used = hz6_stats.route_active_current;
    const size_t hz6_route_occupied =
        hz6_stats.route_active_current + hz6_stats.route_tombstone_current;
    const size_t hz6_route_unused =
        (hz6_route_capacity > hz6_route_occupied)
            ? (hz6_route_capacity - hz6_route_occupied)
            : 0u;
    const size_t hz6_source_block_capacity =
        HZ6_SOURCE_BLOCK_CAPACITY * hz6_allocator_count;
    const size_t hz6_source_block_used =
        hz6_stats.memory_active_source_blocks;
    const size_t hz6_source_block_unused =
        (hz6_source_block_capacity > hz6_source_block_used)
            ? (hz6_source_block_capacity - hz6_source_block_used)
            : 0u;
    const size_t hz6_frontcache_capacity =
        HZ6_FRONT_CACHE_CLASS_COUNT * HZ6_FRONT_CACHE_BIN_CAPACITY *
        hz6_allocator_count;
    const size_t hz6_frontcache_used = hz6_stats.memory_frontcache_total;
    const size_t hz6_frontcache_unused =
        (hz6_frontcache_capacity > hz6_frontcache_used)
            ? (hz6_frontcache_capacity - hz6_frontcache_used)
            : 0u;
    const size_t hz6_transfer_capacity =
        HZ6_TRANSFER_CACHE_CAPACITY * hz6_allocator_count;
    const size_t hz6_transfer_used = hz6_stats.transfer_current;
    const size_t hz6_transfer_unused =
        (hz6_transfer_capacity > hz6_transfer_used)
            ? (hz6_transfer_capacity - hz6_transfer_used)
            : 0u;
    const size_t hz6_descriptor_local_cap_2x =
        hz6_larson_headroom_capacity(hz6_descriptor_used_max_allocator);
    const size_t hz6_route_local_cap_2x =
        hz6_larson_headroom_capacity(hz6_route_occupied_max_allocator);
    const size_t hz6_source_block_local_cap_2x =
        hz6_larson_headroom_capacity(hz6_source_block_used_max_allocator);
    const size_t hz6_frontcache_local_cap_2x =
        hz6_larson_headroom_capacity(hz6_frontcache_used_max_allocator);
    const size_t hz6_transfer_local_cap_2x =
        hz6_larson_headroom_capacity(hz6_transfer_used_max_allocator);
    const size_t hz6_descriptor_projected_capacity =
        hz6_descriptor_local_cap_2x * hz6_allocator_count;
    const size_t hz6_route_projected_capacity =
        hz6_route_local_cap_2x * hz6_allocator_count;
    const size_t hz6_source_block_projected_capacity =
        hz6_source_block_local_cap_2x * hz6_allocator_count;
    const size_t hz6_frontcache_projected_capacity =
        hz6_frontcache_local_cap_2x * hz6_allocator_count;
    const size_t hz6_transfer_projected_capacity =
        hz6_transfer_local_cap_2x * hz6_allocator_count;
    const size_t hz6_descriptor_projected_bytes =
        hz6_larson_project_table_bytes(
            hz6_stats.memory_descriptor_table_bytes,
            hz6_descriptor_capacity,
            hz6_descriptor_projected_capacity);
    const size_t hz6_route_projected_bytes =
        hz6_larson_project_table_bytes(
            hz6_stats.memory_route_table_bytes,
            hz6_route_capacity,
            hz6_route_projected_capacity);
    const size_t hz6_source_block_projected_bytes =
        hz6_larson_project_table_bytes(
            hz6_stats.memory_source_block_table_bytes,
            hz6_source_block_capacity,
            hz6_source_block_projected_capacity);
    const size_t hz6_frontcache_projected_bytes =
        hz6_larson_project_table_bytes(
            hz6_stats.memory_frontcache_table_bytes,
            hz6_frontcache_capacity,
            hz6_frontcache_projected_capacity);
    const size_t hz6_transfer_projected_bytes =
        hz6_larson_project_table_bytes(
            hz6_stats.memory_transfer_table_bytes,
            hz6_transfer_capacity,
            hz6_transfer_projected_capacity);
    const size_t hz6_elastic_projected_static_bytes =
        hz6_descriptor_projected_bytes +
        hz6_route_projected_bytes +
        hz6_source_block_projected_bytes +
        hz6_frontcache_projected_bytes +
        hz6_transfer_projected_bytes +
        hz6_stats.memory_ownerlocality_index_bytes;
    const size_t hz6_elastic_projected_static_plus_payload_bytes =
        hz6_elastic_projected_static_bytes +
        hz6_stats.memory_source_block_committed_estimate;
    const size_t hz6_elastic_projected_savings_bytes =
        (hz6_stats.memory_static_table_bytes >
         hz6_elastic_projected_static_bytes)
            ? (hz6_stats.memory_static_table_bytes -
               hz6_elastic_projected_static_bytes)
            : 0u;
    size_t hz6_overflow_descriptor_count = 0u;
    size_t hz6_overflow_route_count = 0u;
    size_t hz6_overflow_source_block_count = 0u;
    size_t hz6_overflow_frontcache_count = 0u;
    size_t hz6_overflow_transfer_count = 0u;
    size_t hz6_overflow_descriptor_bytes = 0u;
    size_t hz6_overflow_route_bytes = 0u;
    size_t hz6_overflow_source_block_bytes = 0u;
    size_t hz6_overflow_frontcache_bytes = 0u;
    size_t hz6_overflow_transfer_bytes = 0u;
    size_t hz6_overflow_total_bytes = 0u;
    if (hz6_has_main_warmup_stats) {
        const size_t warm_descriptor_used =
            hz6_larson_descriptor_used_from_stats(&hz6_main_warmup_stats);
        const size_t warm_route_occupied =
            hz6_main_warmup_stats.route_active_current +
            hz6_main_warmup_stats.route_tombstone_current;
        const size_t warm_source_block_used =
            hz6_main_warmup_stats.memory_active_source_blocks;
        const size_t warm_frontcache_used =
            hz6_main_warmup_stats.memory_frontcache_total;
        const size_t warm_transfer_used =
            hz6_main_warmup_stats.transfer_current_max;
        hz6_overflow_descriptor_count =
            hz6_larson_overflow_count(warm_descriptor_used,
                                      hz6_descriptor_local_cap_2x);
        hz6_overflow_route_count =
            hz6_larson_overflow_count(warm_route_occupied,
                                      hz6_route_local_cap_2x);
        hz6_overflow_source_block_count =
            hz6_larson_overflow_count(warm_source_block_used,
                                      hz6_source_block_local_cap_2x);
        hz6_overflow_frontcache_count =
            hz6_larson_overflow_count(warm_frontcache_used,
                                      hz6_frontcache_local_cap_2x);
        hz6_overflow_transfer_count =
            hz6_larson_overflow_count(warm_transfer_used,
                                      hz6_transfer_local_cap_2x);
        hz6_overflow_descriptor_bytes =
            hz6_larson_project_table_bytes(
                hz6_stats.memory_descriptor_table_bytes,
                hz6_descriptor_capacity,
                hz6_overflow_descriptor_count);
        hz6_overflow_route_bytes =
            hz6_larson_project_table_bytes(
                hz6_stats.memory_route_table_bytes,
                hz6_route_capacity,
                hz6_overflow_route_count);
        hz6_overflow_source_block_bytes =
            hz6_larson_project_table_bytes(
                hz6_stats.memory_source_block_table_bytes,
                hz6_source_block_capacity,
                hz6_overflow_source_block_count);
        hz6_overflow_frontcache_bytes =
            hz6_larson_project_table_bytes(
                hz6_stats.memory_frontcache_table_bytes,
                hz6_frontcache_capacity,
                hz6_overflow_frontcache_count);
        hz6_overflow_transfer_bytes =
            hz6_larson_project_table_bytes(
                hz6_stats.memory_transfer_table_bytes,
                hz6_transfer_capacity,
                hz6_overflow_transfer_count);
        hz6_overflow_total_bytes =
            hz6_overflow_descriptor_bytes +
            hz6_overflow_route_bytes +
            hz6_overflow_source_block_bytes +
            hz6_overflow_frontcache_bytes +
            hz6_overflow_transfer_bytes;
    }
    printf("[HZ6_RSS_RESIDUAL] "
           "static_table_bytes=%zu static_plus_payload_bytes=%zu "
           "descriptor_table_bytes=%zu route_table_bytes=%zu "
           "shared_route_directory_bytes=%zu owner_locality_index_bytes=%zu "
           "source_block_table_bytes=%zu source_block_depot_bytes=%zu "
           "frontcache_table_bytes=%zu transfer_table_bytes=%zu "
           "source_block_committed_estimate=%zu active_source_blocks=%zu "
           "registered_source_blocks=%zu ref_nonzero_source_blocks=%zu "
           "ref_zero_source_blocks=%zu route_active_current=%zu "
           "route_tombstone_current=%zu frontcache_total=%zu\n",
           hz6_stats.memory_static_table_bytes,
           hz6_stats.memory_static_plus_payload_bytes,
           hz6_stats.memory_descriptor_table_bytes,
           hz6_stats.memory_route_table_bytes,
           hz6_stats.memory_shared_route_directory_bytes,
           hz6_stats.memory_ownerlocality_index_bytes,
           hz6_stats.memory_source_block_table_bytes,
           hz6_stats.memory_source_block_depot_bytes,
           hz6_stats.memory_frontcache_table_bytes,
           hz6_stats.memory_transfer_table_bytes,
           hz6_stats.memory_source_block_committed_estimate,
           hz6_stats.memory_active_source_blocks,
           hz6_stats.memory_registered_source_blocks,
           hz6_stats.memory_ref_nonzero_source_blocks,
           hz6_stats.memory_ref_zero_source_blocks,
           hz6_stats.route_active_current,
           hz6_stats.route_tombstone_current,
           hz6_stats.memory_frontcache_total);
    printf("[HZ6_CAPACITY_UTIL] "
           "allocator_count=%zu "
           "descriptor_capacity=%zu descriptor_used=%zu descriptor_unused=%zu "
           "route_capacity=%zu route_used=%zu route_occupied=%zu "
           "route_unused=%zu source_block_capacity=%zu "
           "source_block_used=%zu source_block_unused=%zu "
           "frontcache_capacity=%zu frontcache_used=%zu "
           "frontcache_unused=%zu transfer_capacity=%zu "
           "transfer_used=%zu transfer_unused=%zu "
           "descriptor_live_max=%zu source_block_active_max=%zu "
           "frontcache_total_max=%zu "
           "descriptor_used_max_allocator=%zu descriptor_local_cap_2x=%zu "
           "route_used_max_allocator=%zu "
           "route_occupied_max_allocator=%zu route_local_cap_2x=%zu "
           "source_block_used_max_allocator=%zu "
           "source_block_local_cap_2x=%zu "
           "frontcache_used_max_allocator=%zu frontcache_local_cap_2x=%zu "
           "transfer_used_max_allocator=%zu transfer_local_cap_2x=%zu\n",
           hz6_allocator_count,
           hz6_descriptor_capacity,
           hz6_descriptor_used,
           hz6_descriptor_unused,
           hz6_route_capacity,
           hz6_route_used,
           hz6_route_occupied,
           hz6_route_unused,
           hz6_source_block_capacity,
           hz6_source_block_used,
           hz6_source_block_unused,
           hz6_frontcache_capacity,
           hz6_frontcache_used,
           hz6_frontcache_unused,
           hz6_transfer_capacity,
           hz6_transfer_used,
           hz6_transfer_unused,
           hz6_stats.descriptor_live_max,
           hz6_stats.source_block_active_max,
           hz6_stats.frontcache_total_max,
           hz6_descriptor_used_max_allocator,
           hz6_descriptor_local_cap_2x,
           hz6_route_used_max_allocator,
           hz6_route_occupied_max_allocator,
           hz6_route_local_cap_2x,
           hz6_source_block_used_max_allocator,
           hz6_source_block_local_cap_2x,
           hz6_frontcache_used_max_allocator,
           hz6_frontcache_local_cap_2x,
           hz6_transfer_used_max_allocator,
           hz6_transfer_local_cap_2x);
    printf("[HZ6_ELASTIC_PROJECTION] "
           "allocator_count=%zu "
           "descriptor_projected_capacity=%zu "
           "descriptor_projected_table_bytes=%zu "
           "route_projected_capacity=%zu route_projected_table_bytes=%zu "
           "source_block_projected_capacity=%zu "
           "source_block_projected_table_bytes=%zu "
           "frontcache_projected_capacity=%zu "
           "frontcache_projected_table_bytes=%zu "
           "transfer_projected_capacity=%zu "
           "transfer_projected_table_bytes=%zu "
           "projected_static_table_bytes=%zu "
           "projected_static_plus_payload_bytes=%zu "
           "projected_static_savings_bytes=%zu\n",
           hz6_allocator_count,
           hz6_descriptor_projected_capacity,
           hz6_descriptor_projected_bytes,
           hz6_route_projected_capacity,
           hz6_route_projected_bytes,
           hz6_source_block_projected_capacity,
           hz6_source_block_projected_bytes,
           hz6_frontcache_projected_capacity,
           hz6_frontcache_projected_bytes,
           hz6_transfer_projected_capacity,
           hz6_transfer_projected_bytes,
           hz6_elastic_projected_static_bytes,
           hz6_elastic_projected_static_plus_payload_bytes,
           hz6_elastic_projected_savings_bytes);
    printf("[HZ6_ELASTIC_OVERFLOW_PROJECTION] "
           "descriptor_local_cap=%zu descriptor_overflow=%zu "
           "descriptor_overflow_bytes=%zu route_local_cap=%zu "
           "route_overflow=%zu route_overflow_bytes=%zu "
           "source_block_local_cap=%zu source_block_overflow=%zu "
           "source_block_overflow_bytes=%zu frontcache_local_cap=%zu "
           "frontcache_overflow=%zu frontcache_overflow_bytes=%zu "
           "transfer_local_cap=%zu transfer_overflow=%zu "
           "transfer_overflow_bytes=%zu overflow_total_bytes=%zu\n",
           hz6_descriptor_local_cap_2x,
           hz6_overflow_descriptor_count,
           hz6_overflow_descriptor_bytes,
           hz6_route_local_cap_2x,
           hz6_overflow_route_count,
           hz6_overflow_route_bytes,
           hz6_source_block_local_cap_2x,
           hz6_overflow_source_block_count,
           hz6_overflow_source_block_bytes,
           hz6_frontcache_local_cap_2x,
           hz6_overflow_frontcache_count,
           hz6_overflow_frontcache_bytes,
           hz6_transfer_local_cap_2x,
           hz6_overflow_transfer_count,
           hz6_overflow_transfer_bytes,
           hz6_overflow_total_bytes);
#endif

    duration_sec = (double)(end_ns - start_ns) / 1e9;
    printf("Throughput = %.0f operations per second, relative time: %.3fs.\n",
           (duration_sec > 0.0) ? ((double)total_allocs / duration_sec) : 0.0,
           duration_sec);
    printf("[OPS] allocs=%llu frees=%llu\n",
           (unsigned long long)total_allocs,
           (unsigned long long)total_frees);

#include "bench_larson_compare_tail.inc"
