// Windows-native Larson-style allocator comparison bench.
// Usage: bench_larson_compare [runtime_sec] [min_size] [max_size] [chunks_per_thread] [rounds] [seed] [threads]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench_modern_allocator_adapter.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

typedef struct LarsonThreadData {
    void** blocks;
    size_t* sizes;
    size_t live_count;
    size_t iterations_per_check;
    size_t min_size;
    size_t max_size;
    uint32_t seed;
    uint64_t alloc_count;
    uint64_t free_count;
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats_after;
#endif
} LarsonThreadData;

static volatile LONG g_stop_flag = 0;

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

static unsigned __stdcall larson_thread(void* arg) {
    LarsonThreadData* td = (LarsonThreadData*)arg;
    uint32_t seed = td->seed;

    hz_bench_allocator_thread_setup();

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
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats;
    memset(&hz6_stats, 0, sizeof(hz6_stats));
#endif

    if (runtime_sec == 0 || chunks_per_thread == 0 || rounds == 0 || threads == 0 || min_size == 0 || max_size < min_size) {
        fprintf(stderr, "bench_larson_compare: invalid args\n");
        fprintf(stderr, "usage: %s <runtime_sec> <min> <max> <chunks_per_thread> <rounds> <seed> <threads>\n", argv[0]);
        return 2;
    }

    total_slots = (size_t)threads * (size_t)chunks_per_thread;

    printf("[BENCH_ARGS] runtime=%u min=%u max=%u chunks=%u rounds=%u seed=%u threads=%u\n",
           runtime_sec, min_size, max_size, chunks_per_thread, rounds, seed, threads);

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
        for (i = 0; i < chunks_per_thread; ++i) {
            size_t slot = base + i;
            size_t size = pick_size(rng_next(&local_seed), min_size, max_size);
            void* p = bench_alloc(size);
            if (!p) {
                fprintf(stderr, "bench_larson_compare: warmup alloc failed at thread=%zu slot=%zu size=%zu\n", t, i, size);
                hz_bench_dump_stats(stderr, "larson_warmup_alloc_fail");
                goto cleanup;
            }
            touch_allocation(p, size, local_seed);
            all_blocks[slot] = p;
            all_sizes[slot] = size;
        }

        tds[t].blocks = all_blocks + base;
        tds[t].sizes = all_sizes + base;
        tds[t].live_count = (size_t)chunks_per_thread;
        tds[t].iterations_per_check = (size_t)chunks_per_thread * (size_t)rounds;
        tds[t].min_size = min_size;
        tds[t].max_size = max_size;
        tds[t].seed = seed + (uint32_t)(t * 131u) + 1u;
        tds[t].alloc_count = 0;
        tds[t].free_count = 0;
    }

    InterlockedExchange(&g_stop_flag, 0);
    start_ns = now_ns();

    for (t = 0; t < threads; ++t) {
        uintptr_t h = _beginthreadex(NULL, 0, larson_thread, &tds[t], 0, NULL);
        if (h == 0) {
            fprintf(stderr, "bench_larson_compare: thread start failed at thread=%zu\n", t);
            InterlockedExchange(&g_stop_flag, 1);
            for (size_t j = 0; j < t; ++j) {
                WaitForSingleObject(handles[j], INFINITE);
                CloseHandle(handles[j]);
            }
            goto cleanup;
        }
        handles[t] = (HANDLE)h;
    }

    Sleep(runtime_sec * 1000u);
    InterlockedExchange(&g_stop_flag, 1);

    WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    end_ns = now_ns();

    for (t = 0; t < threads; ++t) {
        total_allocs += tds[t].alloc_count;
        total_frees += tds[t].free_count;
#if defined(HZ_BENCH_USE_HZ6)
        hz6_stats.route_valid += tds[t].hz6_stats_after.route_valid;
        hz6_stats.route_invalid += tds[t].hz6_stats_after.route_invalid;
        hz6_stats.route_miss += tds[t].hz6_stats_after.route_miss;
        hz6_stats.transfer_push += tds[t].hz6_stats_after.transfer_push;
        hz6_stats.transfer_pop += tds[t].hz6_stats_after.transfer_pop;
        hz6_stats.source_alloc += tds[t].hz6_stats_after.source_alloc;
        hz6_stats.frontcache_reuse_hit +=
            tds[t].hz6_stats_after.frontcache_reuse_hit;
        hz6_stats.frontcache_reuse_invalid +=
            tds[t].hz6_stats_after.frontcache_reuse_invalid;
        hz6_stats.transfer_reuse_hit +=
            tds[t].hz6_stats_after.transfer_reuse_hit;
        hz6_stats.transfer_reuse_invalid +=
            tds[t].hz6_stats_after.transfer_reuse_invalid;
        hz6_stats.source_refill_starvation +=
            tds[t].hz6_stats_after.source_refill_starvation;
        hz6_stats.source_refill_saturation +=
            tds[t].hz6_stats_after.source_refill_saturation;
        hz6_stats.source_refill_boost +=
            tds[t].hz6_stats_after.source_refill_boost;
        hz6_stats.source_refill_clamp +=
            tds[t].hz6_stats_after.source_refill_clamp;
        hz6_stats.alloc_fail += tds[t].hz6_stats_after.alloc_fail;
        hz6_stats.descriptor_exhausted +=
            tds[t].hz6_stats_after.descriptor_exhausted;
        hz6_stats.route_register_fail +=
            tds[t].hz6_stats_after.route_register_fail;
        hz6_stats.source_block_exhausted +=
            tds[t].hz6_stats_after.source_block_exhausted;
        hz6_stats.descriptor_probe_total +=
            tds[t].hz6_stats_after.descriptor_probe_total;
        if (tds[t].hz6_stats_after.descriptor_probe_max >
            hz6_stats.descriptor_probe_max) {
            hz6_stats.descriptor_probe_max =
                tds[t].hz6_stats_after.descriptor_probe_max;
        }
        hz6_stats.route_register_probe_total +=
            tds[t].hz6_stats_after.route_register_probe_total;
        if (tds[t].hz6_stats_after.route_register_probe_max >
            hz6_stats.route_register_probe_max) {
            hz6_stats.route_register_probe_max =
                tds[t].hz6_stats_after.route_register_probe_max;
        }
        hz6_stats.route_unregister_probe_total +=
            tds[t].hz6_stats_after.route_unregister_probe_total;
        if (tds[t].hz6_stats_after.route_unregister_probe_max >
            hz6_stats.route_unregister_probe_max) {
            hz6_stats.route_unregister_probe_max =
                tds[t].hz6_stats_after.route_unregister_probe_max;
        }
        hz6_stats.source_block_probe_total +=
            tds[t].hz6_stats_after.source_block_probe_total;
        if (tds[t].hz6_stats_after.source_block_probe_max >
            hz6_stats.source_block_probe_max) {
            hz6_stats.source_block_probe_max =
                tds[t].hz6_stats_after.source_block_probe_max;
        }
        hz6_stats.large_span_central_push +=
            tds[t].hz6_stats_after.large_span_central_push;
        hz6_stats.large_span_central_pop +=
            tds[t].hz6_stats_after.large_span_central_pop;
        hz6_stats.large_span_source_alloc +=
            tds[t].hz6_stats_after.large_span_source_alloc;
#endif
        CloseHandle(handles[t]);
    }

    duration_sec = (double)(end_ns - start_ns) / 1e9;
    printf("Throughput = %.0f operations per second, relative time: %.3fs.\n",
           (duration_sec > 0.0) ? ((double)total_allocs / duration_sec) : 0.0,
           duration_sec);
    printf("[OPS] allocs=%llu frees=%llu\n",
           (unsigned long long)total_allocs,
           (unsigned long long)total_frees);
#if defined(HZ_BENCH_USE_HZ6)
    printf("[HZ6_STATS] label=%s route_valid=%zu route_invalid=%zu route_miss=%zu "
           "transfer_push=%zu transfer_pop=%zu source_alloc=%zu "
           "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
           "transfer_reuse_hit=%zu transfer_reuse_invalid=%zu "
           "source_refill_starvation=%zu source_refill_saturation=%zu "
           "source_refill_boost=%zu source_refill_clamp=%zu alloc_fail=%zu "
           "descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu "
           "descriptor_probe_total=%zu descriptor_probe_max=%zu "
           "route_register_probe_total=%zu route_register_probe_max=%zu "
           "route_unregister_probe_total=%zu route_unregister_probe_max=%zu "
           "source_block_probe_total=%zu source_block_probe_max=%zu "
           "large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
           "larson_main_final",
           hz6_stats.route_valid,
           hz6_stats.route_invalid,
           hz6_stats.route_miss,
           hz6_stats.transfer_push,
           hz6_stats.transfer_pop,
           hz6_stats.source_alloc,
           hz6_stats.frontcache_reuse_hit,
           hz6_stats.frontcache_reuse_invalid,
           hz6_stats.transfer_reuse_hit,
           hz6_stats.transfer_reuse_invalid,
           hz6_stats.source_refill_starvation,
           hz6_stats.source_refill_saturation,
           hz6_stats.source_refill_boost,
           hz6_stats.source_refill_clamp,
           hz6_stats.alloc_fail,
           hz6_stats.descriptor_exhausted,
           hz6_stats.route_register_fail,
           hz6_stats.source_block_exhausted,
           hz6_stats.descriptor_probe_total,
           hz6_stats.descriptor_probe_max,
           hz6_stats.route_register_probe_total,
           hz6_stats.route_register_probe_max,
           hz6_stats.route_unregister_probe_total,
           hz6_stats.route_unregister_probe_max,
           hz6_stats.source_block_probe_total,
           hz6_stats.source_block_probe_max,
           hz6_stats.large_span_central_push,
           hz6_stats.large_span_central_pop,
           hz6_stats.large_span_source_alloc);
#else
    hz_bench_dump_stats(stdout, "larson_main_final");
#endif
    printf("Done sleeping...\n");

cleanup:
    if (all_blocks) {
        for (size_t i = 0; i < total_slots; ++i) {
            if (all_blocks[i]) {
                bench_free(all_blocks[i]);
                all_blocks[i] = NULL;
            }
        }
    }
    free(all_sizes);
    free(all_blocks);
    free(handles);
    free(tds);
    hz_bench_allocator_thread_teardown();
    return 0;
}
