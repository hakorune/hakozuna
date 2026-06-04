// Windows-native Larson-style allocator comparison bench.
// Usage: bench_larson_compare [runtime_sec] [min_size] [max_size] [chunks_per_thread] [rounds] [seed] [threads] [warmup_mode]

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

static int warmup_thread_blocks(LarsonThreadData* td) {
    uint32_t local_seed = td->seed ^ 0x9E3779B9u;
    size_t i;

    for (i = 0; i < td->live_count; ++i) {
        size_t size = pick_size(rng_next(&local_seed), td->min_size, td->max_size);
        void* p = bench_alloc(size);
        if (!p) {
            hz_bench_dump_stats(stderr, "larson_worker_warmup_alloc_fail");
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

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
static const char* hz6_front_attr_name(size_t index) {
    switch (index) {
        case HZ6_FRONT_ATTR_LOCAL2P:
            return "local2p";
        case HZ6_FRONT_ATTR_MIDPAGE:
            return "midpage";
        case HZ6_FRONT_ATTR_LARGE:
            return "large";
        case HZ6_FRONT_ATTR_TOY:
            return "toy";
        default:
            return "unknown";
    }
}

static const char* hz6_alloc_path_name(Hz6AllocPath path) {
    switch (path) {
        case HZ6_ALLOC_PATH_LOCAL_REUSE:
            return "local_reuse";
        case HZ6_ALLOC_PATH_TRANSFER_REUSE:
            return "transfer_reuse";
        case HZ6_ALLOC_PATH_PREFILL_REUSE:
            return "prefill_reuse";
        case HZ6_ALLOC_PATH_SOURCE_PREFILL:
            return "source_prefill";
        case HZ6_ALLOC_PATH_DIRECT_SOURCE:
            return "direct_source";
        case HZ6_ALLOC_PATH_RELEASED_REUSE:
            return "released_reuse";
        case HZ6_ALLOC_PATH_OOM:
            return "oom";
        case HZ6_ALLOC_PATH_UNSUPPORTED:
            return "unsupported";
        default:
            return "unknown";
    }
}

static void print_hz6_front_alloc_paths(const Hz6StatsSnapshot* stats) {
    size_t front;
    size_t path;
    for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
        printf("[HZ6_PATH] front=%s", hz6_front_attr_name(front));
        for (path = 0; path < HZ6_ALLOC_PATH_COUNT; ++path) {
            printf(" %s=%zu", hz6_alloc_path_name((Hz6AllocPath)path),
                   stats->front_alloc_path[front][path]);
        }
        printf("\n");
    }
}

static void print_hz6_front_prefill_paths(const Hz6StatsSnapshot* stats) {
    size_t front;
    const char* front_name;
    for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
        switch (front) {
            case HZ6_FRONT_ATTR_LOCAL2P:
                front_name = "local2p";
                break;
            case HZ6_FRONT_ATTR_MIDPAGE:
                front_name = "midpage";
                break;
            case HZ6_FRONT_ATTR_LARGE:
                front_name = "large";
                break;
            case HZ6_FRONT_ATTR_TOY:
                front_name = "toy";
                break;
            default:
                front_name = "unknown";
                break;
        }
        printf("[HZ6_PREFILL] front=%s attempt=%zu filled=%zu fallback=%zu\n",
               front_name,
               stats->front_source_prefill_attempt[front],
               stats->front_source_prefill_filled[front],
               stats->front_source_prefill_fallback[front]);
    }
}

static void print_hz6_frontcache_class_diag(const Hz6StatsSnapshot* stats) {
    size_t class_id;
    for (class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
        size_t push = stats->frontcache_push_by_class[class_id];
        size_t pop_empty = stats->frontcache_pop_empty_by_class[class_id];
        if (push == 0 && pop_empty == 0) {
            continue;
        }
        printf("[HZ6_FRONTCACHE_CLASS] class=%zu push=%zu pop_empty=%zu\n",
               class_id,
               push,
               pop_empty);
    }
}
#endif

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
        tds[t].live_count = (size_t)chunks_per_thread;
        tds[t].iterations_per_check = (size_t)chunks_per_thread * (size_t)rounds;
        tds[t].min_size = min_size;
        tds[t].max_size = max_size;
        tds[t].seed = seed + (uint32_t)(t * 131u) + 1u;
        tds[t].warmup_in_worker = worker_warmup;
        tds[t].alloc_count = 0;
        tds[t].free_count = 0;
    }

    InterlockedExchange(&g_stop_flag, 0);
    InterlockedExchange(&g_ready_count, 0);
    InterlockedExchange(&g_start_flag, worker_warmup ? 0 : 1);

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
            fprintf(stderr, "bench_larson_compare: worker warmup failed\n");
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
#if defined(HZ_BENCH_USE_HZ6)
        hz6_stats.route_valid += tds[t].hz6_stats_after.route_valid;
        hz6_stats.route_invalid += tds[t].hz6_stats_after.route_invalid;
        hz6_stats.route_miss += tds[t].hz6_stats_after.route_miss;
        hz6_stats.route_visibility_lookup +=
            tds[t].hz6_stats_after.route_visibility_lookup;
        hz6_stats.route_visibility_hit +=
            tds[t].hz6_stats_after.route_visibility_hit;
        hz6_stats.route_visibility_hit_local_owner +=
            tds[t].hz6_stats_after.route_visibility_hit_local_owner;
        hz6_stats.route_visibility_hit_foreign_owner +=
            tds[t].hz6_stats_after.route_visibility_hit_foreign_owner;
        hz6_stats.route_visibility_miss +=
            tds[t].hz6_stats_after.route_visibility_miss;
        hz6_stats.route_visibility_probe_total +=
            tds[t].hz6_stats_after.route_visibility_probe_total;
        if (tds[t].hz6_stats_after.route_visibility_probe_max >
            hz6_stats.route_visibility_probe_max) {
            hz6_stats.route_visibility_probe_max =
                tds[t].hz6_stats_after.route_visibility_probe_max;
        }
        hz6_stats.transfer_push += tds[t].hz6_stats_after.transfer_push;
        hz6_stats.transfer_pop += tds[t].hz6_stats_after.transfer_pop;
        hz6_stats.transfer_current +=
            tds[t].hz6_stats_after.transfer_current;
        if (tds[t].hz6_stats_after.transfer_current_max >
            hz6_stats.transfer_current_max) {
            hz6_stats.transfer_current_max =
                tds[t].hz6_stats_after.transfer_current_max;
        }
        hz6_stats.remote_free_attempt +=
            tds[t].hz6_stats_after.remote_free_attempt;
        hz6_stats.remote_free_strict_owner_block +=
            tds[t].hz6_stats_after.remote_free_strict_owner_block;
        hz6_stats.remote_free_transfer_fail +=
            tds[t].hz6_stats_after.remote_free_transfer_fail;
        hz6_stats.route_rehome_attempt +=
            tds[t].hz6_stats_after.route_rehome_attempt;
        hz6_stats.route_rehome_success +=
            tds[t].hz6_stats_after.route_rehome_success;
        hz6_stats.route_rehome_fail +=
            tds[t].hz6_stats_after.route_rehome_fail;
        hz6_stats.descriptor_source_route_allocator_match +=
            tds[t].hz6_stats_after.descriptor_source_route_allocator_match;
        hz6_stats.descriptor_source_route_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_source_route_allocator_mismatch;
        hz6_stats.descriptor_source_current_allocator_match +=
            tds[t].hz6_stats_after.descriptor_source_current_allocator_match;
        hz6_stats.descriptor_source_current_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_source_current_allocator_mismatch;
        hz6_stats.descriptor_storage_lookup +=
            tds[t].hz6_stats_after.descriptor_storage_lookup;
        hz6_stats.descriptor_storage_hit +=
            tds[t].hz6_stats_after.descriptor_storage_hit;
        hz6_stats.descriptor_storage_miss +=
            tds[t].hz6_stats_after.descriptor_storage_miss;
        hz6_stats.descriptor_storage_probe_total +=
            tds[t].hz6_stats_after.descriptor_storage_probe_total;
        if (tds[t].hz6_stats_after.descriptor_storage_probe_max >
            hz6_stats.descriptor_storage_probe_max) {
            hz6_stats.descriptor_storage_probe_max =
                tds[t].hz6_stats_after.descriptor_storage_probe_max;
        }
        hz6_stats.descriptor_storage_route_allocator_match +=
            tds[t].hz6_stats_after.descriptor_storage_route_allocator_match;
        hz6_stats.descriptor_storage_route_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_storage_route_allocator_mismatch;
        hz6_stats.descriptor_storage_current_allocator_match +=
            tds[t].hz6_stats_after.descriptor_storage_current_allocator_match;
        hz6_stats.descriptor_storage_current_allocator_mismatch +=
            tds[t].hz6_stats_after.descriptor_storage_current_allocator_mismatch;
        hz6_stats.lifecycle_owner_mismatch +=
            tds[t].hz6_stats_after.lifecycle_owner_mismatch;
        hz6_stats.lifecycle_foreign_free_attempt +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_attempt;
        hz6_stats.lifecycle_foreign_free_handled +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_handled;
        hz6_stats.lifecycle_foreign_free_invalid +=
            tds[t].hz6_stats_after.lifecycle_foreign_free_invalid;
        hz6_stats.visible_first_attempt +=
            tds[t].hz6_stats_after.visible_first_attempt;
        hz6_stats.visible_first_hit += tds[t].hz6_stats_after.visible_first_hit;
        hz6_stats.visible_first_miss +=
            tds[t].hz6_stats_after.visible_first_miss;
        hz6_stats.visible_first_visible_invalid +=
            tds[t].hz6_stats_after.visible_first_visible_invalid;
        hz6_stats.visible_first_local_fallback +=
            tds[t].hz6_stats_after.visible_first_local_fallback;
        hz6_stats.visible_first_local_fallback_invalid +=
            tds[t].hz6_stats_after.visible_first_local_fallback_invalid;
        hz6_stats.visible_first_local_lookup_skipped +=
            tds[t].hz6_stats_after.visible_first_local_lookup_skipped;
        hz6_stats.negative_filter_attempt +=
            tds[t].hz6_stats_after.negative_filter_attempt;
        hz6_stats.negative_filter_not_armed +=
            tds[t].hz6_stats_after.negative_filter_not_armed;
        hz6_stats.negative_filter_rehome_blocked +=
            tds[t].hz6_stats_after.negative_filter_rehome_blocked;
        hz6_stats.negative_filter_skip_local +=
            tds[t].hz6_stats_after.negative_filter_skip_local;
        hz6_stats.negative_filter_maybe_local +=
            tds[t].hz6_stats_after.negative_filter_maybe_local;
        hz6_stats.negative_filter_shadow_false_skip +=
            tds[t].hz6_stats_after.negative_filter_shadow_false_skip;
        hz6_stats.negative_filter_shadow_local_valid +=
            tds[t].hz6_stats_after.negative_filter_shadow_local_valid;
        hz6_stats.negative_filter_shadow_local_invalid +=
            tds[t].hz6_stats_after.negative_filter_shadow_local_invalid;
        hz6_stats.negative_filter_range_probe_total +=
            tds[t].hz6_stats_after.negative_filter_range_probe_total;
        if (tds[t].hz6_stats_after.negative_filter_range_probe_max >
            hz6_stats.negative_filter_range_probe_max) {
            hz6_stats.negative_filter_range_probe_max =
                tds[t].hz6_stats_after.negative_filter_range_probe_max;
        }
        hz6_stats.shared_dir_lookup +=
            tds[t].hz6_stats_after.shared_dir_lookup;
        hz6_stats.shared_dir_hit +=
            tds[t].hz6_stats_after.shared_dir_hit;
        hz6_stats.shared_dir_miss +=
            tds[t].hz6_stats_after.shared_dir_miss;
        hz6_stats.shared_dir_stale +=
            tds[t].hz6_stats_after.shared_dir_stale;
        hz6_stats.shared_dir_hit_local_allocator +=
            tds[t].hz6_stats_after.shared_dir_hit_local_allocator;
        hz6_stats.shared_dir_hit_foreign_allocator +=
            tds[t].hz6_stats_after.shared_dir_hit_foreign_allocator;
        hz6_stats.shared_dir_would_skip_local +=
            tds[t].hz6_stats_after.shared_dir_would_skip_local;
        hz6_stats.shared_dir_register +=
            tds[t].hz6_stats_after.shared_dir_register;
        hz6_stats.shared_dir_unregister +=
            tds[t].hz6_stats_after.shared_dir_unregister;
        hz6_stats.shared_dir_probe_total +=
            tds[t].hz6_stats_after.shared_dir_probe_total;
        if (tds[t].hz6_stats_after.shared_dir_probe_max >
            hz6_stats.shared_dir_probe_max) {
            hz6_stats.shared_dir_probe_max =
                tds[t].hz6_stats_after.shared_dir_probe_max;
        }
        hz6_stats.shared_dir_first_attempt +=
            tds[t].hz6_stats_after.shared_dir_first_attempt;
        hz6_stats.shared_dir_first_hit +=
            tds[t].hz6_stats_after.shared_dir_first_hit;
        hz6_stats.shared_dir_first_fallback +=
            tds[t].hz6_stats_after.shared_dir_first_fallback;
        hz6_stats.shared_dir_first_invalid +=
            tds[t].hz6_stats_after.shared_dir_first_invalid;
        hz6_stats.source_owned_prepare +=
            tds[t].hz6_stats_after.source_owned_prepare;
        hz6_stats.source_owned_route_hit_local_owner +=
            tds[t].hz6_stats_after.source_owned_route_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_local_owner +=
            tds[t].hz6_stats_after.source_owned_visibility_hit_local_owner;
        hz6_stats.source_owned_visibility_hit_foreign_owner +=
            tds[t].hz6_stats_after.source_owned_visibility_hit_foreign_owner;
        hz6_stats.source_owned_remote_free_attempt +=
            tds[t].hz6_stats_after.source_owned_remote_free_attempt;
        hz6_stats.source_owned_release +=
            tds[t].hz6_stats_after.source_owned_release;
        hz6_stats.source_alloc += tds[t].hz6_stats_after.source_alloc;
        hz6_stats.local2p_source_alloc +=
            tds[t].hz6_stats_after.local2p_source_alloc;
        hz6_stats.midpage_source_alloc +=
            tds[t].hz6_stats_after.midpage_source_alloc;
        hz6_stats.large_source_alloc +=
            tds[t].hz6_stats_after.large_source_alloc;
        hz6_stats.toy_source_alloc += tds[t].hz6_stats_after.toy_source_alloc;
        for (size_t front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
            for (size_t path = 0; path < HZ6_ALLOC_PATH_COUNT; ++path) {
                hz6_stats.front_alloc_path[front][path] +=
                    tds[t].hz6_stats_after.front_alloc_path[front][path];
            }
        }
        hz6_stats.frontcache_reuse_hit +=
            tds[t].hz6_stats_after.frontcache_reuse_hit;
        hz6_stats.frontcache_reuse_invalid +=
            tds[t].hz6_stats_after.frontcache_reuse_invalid;
        for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT;
             ++class_id) {
            hz6_stats.frontcache_push_by_class[class_id] +=
                tds[t].hz6_stats_after.frontcache_push_by_class[class_id];
            hz6_stats.frontcache_pop_empty_by_class[class_id] +=
                tds[t].hz6_stats_after.frontcache_pop_empty_by_class[class_id];
        }
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
        hz6_stats.source_admission_open +=
            tds[t].hz6_stats_after.source_admission_open;
        hz6_stats.source_admission_boosted +=
            tds[t].hz6_stats_after.source_admission_boosted;
        hz6_stats.source_admission_clamped +=
            tds[t].hz6_stats_after.source_admission_clamped;
        hz6_stats.control_plane_normal +=
            tds[t].hz6_stats_after.control_plane_normal;
        hz6_stats.control_plane_burst_supply_would_open +=
            tds[t].hz6_stats_after.control_plane_burst_supply_would_open;
        hz6_stats.control_plane_close_would_start +=
            tds[t].hz6_stats_after.control_plane_close_would_start;
        hz6_stats.source_prefill_attempt +=
            tds[t].hz6_stats_after.source_prefill_attempt;
        hz6_stats.source_prefill_filled +=
            tds[t].hz6_stats_after.source_prefill_filled;
        hz6_stats.source_prefill_fallback +=
            tds[t].hz6_stats_after.source_prefill_fallback;
        for (size_t front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
            hz6_stats.front_source_prefill_attempt[front] +=
                tds[t].hz6_stats_after.front_source_prefill_attempt[front];
            hz6_stats.front_source_prefill_filled[front] +=
                tds[t].hz6_stats_after.front_source_prefill_filled[front];
            hz6_stats.front_source_prefill_fallback[front] +=
                tds[t].hz6_stats_after.front_source_prefill_fallback[front];
        }
        hz6_stats.front_source_ops_alloc +=
            tds[t].hz6_stats_after.front_source_ops_alloc;
        hz6_stats.front_source_slot_alloc +=
            tds[t].hz6_stats_after.front_source_slot_alloc;
        hz6_stats.front_source_prefill_alloc +=
            tds[t].hz6_stats_after.front_source_prefill_alloc;
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
        hz6_stats.route_exact_lookup_probe_total +=
            tds[t].hz6_stats_after.route_exact_lookup_probe_total;
        if (tds[t].hz6_stats_after.route_exact_lookup_probe_max >
            hz6_stats.route_exact_lookup_probe_max) {
            hz6_stats.route_exact_lookup_probe_max =
                tds[t].hz6_stats_after.route_exact_lookup_probe_max;
        }
        hz6_stats.owner_locality_lookup +=
            tds[t].hz6_stats_after.owner_locality_lookup;
        hz6_stats.owner_locality_hit_local_allocator +=
            tds[t].hz6_stats_after.owner_locality_hit_local_allocator;
        hz6_stats.owner_locality_hit_foreign_allocator +=
            tds[t].hz6_stats_after.owner_locality_hit_foreign_allocator;
        hz6_stats.owner_locality_miss +=
            tds[t].hz6_stats_after.owner_locality_miss;
        hz6_stats.owner_locality_register +=
            tds[t].hz6_stats_after.owner_locality_register;
        hz6_stats.owner_locality_unregister +=
            tds[t].hz6_stats_after.owner_locality_unregister;
        hz6_stats.owner_locality_probe_total +=
            tds[t].hz6_stats_after.owner_locality_probe_total;
        if (tds[t].hz6_stats_after.owner_locality_probe_max >
            hz6_stats.owner_locality_probe_max) {
            hz6_stats.owner_locality_probe_max =
                tds[t].hz6_stats_after.owner_locality_probe_max;
        }
        hz6_stats.route_lookup_probe_total +=
            tds[t].hz6_stats_after.route_lookup_probe_total;
        if (tds[t].hz6_stats_after.route_lookup_probe_max >
            hz6_stats.route_lookup_probe_max) {
            hz6_stats.route_lookup_probe_max =
                tds[t].hz6_stats_after.route_lookup_probe_max;
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
        hz6_stats.memory_descriptor_table_bytes +=
            tds[t].hz6_stats_after.memory_descriptor_table_bytes;
        hz6_stats.memory_route_table_bytes +=
            tds[t].hz6_stats_after.memory_route_table_bytes;
        hz6_stats.memory_source_block_table_bytes +=
            tds[t].hz6_stats_after.memory_source_block_table_bytes;
        hz6_stats.memory_frontcache_table_bytes +=
            tds[t].hz6_stats_after.memory_frontcache_table_bytes;
        hz6_stats.memory_transfer_table_bytes +=
            tds[t].hz6_stats_after.memory_transfer_table_bytes;
        if (tds[t].hz6_stats_after.memory_ownerlocality_index_bytes >
            hz6_stats.memory_ownerlocality_index_bytes) {
            hz6_stats.memory_ownerlocality_index_bytes =
                tds[t].hz6_stats_after.memory_ownerlocality_index_bytes;
        }
        hz6_stats.memory_active_descriptors +=
            tds[t].hz6_stats_after.memory_active_descriptors;
        hz6_stats.memory_local_free_descriptors +=
            tds[t].hz6_stats_after.memory_local_free_descriptors;
        hz6_stats.memory_transfer_free_descriptors +=
            tds[t].hz6_stats_after.memory_transfer_free_descriptors;
        hz6_stats.memory_remote_pending_descriptors +=
            tds[t].hz6_stats_after.memory_remote_pending_descriptors;
        hz6_stats.memory_dead_with_ptr_descriptors +=
            tds[t].hz6_stats_after.memory_dead_with_ptr_descriptors;
        hz6_stats.memory_active_source_blocks +=
            tds[t].hz6_stats_after.memory_active_source_blocks;
        hz6_stats.memory_registered_source_blocks +=
            tds[t].hz6_stats_after.memory_registered_source_blocks;
        hz6_stats.memory_ref_nonzero_source_blocks +=
            tds[t].hz6_stats_after.memory_ref_nonzero_source_blocks;
        hz6_stats.memory_source_block_payload_bytes +=
            tds[t].hz6_stats_after.memory_source_block_payload_bytes;
        hz6_stats.memory_source_block_committed_estimate +=
            tds[t].hz6_stats_after.memory_source_block_committed_estimate;
        hz6_stats.route_active_current +=
            tds[t].hz6_stats_after.route_active_current;
        hz6_stats.route_active_max +=
            tds[t].hz6_stats_after.route_active_max;
        hz6_stats.route_tombstone_current +=
            tds[t].hz6_stats_after.route_tombstone_current;
        hz6_stats.memory_frontcache_total +=
            tds[t].hz6_stats_after.memory_frontcache_total;
        if (tds[t].hz6_stats_after.memory_frontcache_largest_bin >
            hz6_stats.memory_frontcache_largest_bin) {
            hz6_stats.memory_frontcache_largest_bin =
                tds[t].hz6_stats_after.memory_frontcache_largest_bin;
        }
        if (tds[t].hz6_stats_after.metadata_descriptor_entry_bytes >
            hz6_stats.metadata_descriptor_entry_bytes) {
            hz6_stats.metadata_descriptor_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_descriptor_thin_hot_entry_bytes >
            hz6_stats.metadata_descriptor_thin_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_thin_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_thin_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_thin_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_thin_hot_table_bytes;
        hz6_stats.metadata_descriptor_thin_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_thin_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_entry_bytes >
            hz6_stats.metadata_descriptor_ownerless_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_ownerless_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_ownerless_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_table_bytes;
        hz6_stats.metadata_descriptor_ownerless_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_entry_bytes >
            hz6_stats.metadata_descriptor_owner16_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_owner16_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_owner16_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_table_bytes;
        hz6_stats.metadata_descriptor_owner16_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_route_entry_bytes >
            hz6_stats.metadata_route_entry_bytes) {
            hz6_stats.metadata_route_entry_bytes =
                tds[t].hz6_stats_after.metadata_route_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_route_slim_entry_bytes >
            hz6_stats.metadata_route_slim_entry_bytes) {
            hz6_stats.metadata_route_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_route_slim_entry_bytes;
        }
        hz6_stats.metadata_route_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_route_slim_table_bytes;
        hz6_stats.metadata_route_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_route_slim_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_source_block_entry_bytes >
            hz6_stats.metadata_source_block_entry_bytes) {
            hz6_stats.metadata_source_block_entry_bytes =
                tds[t].hz6_stats_after.metadata_source_block_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_source_block_slim_entry_bytes >
            hz6_stats.metadata_source_block_slim_entry_bytes) {
            hz6_stats.metadata_source_block_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_source_block_slim_entry_bytes;
        }
        hz6_stats.metadata_source_block_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_source_block_slim_table_bytes;
        hz6_stats.metadata_source_block_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_source_block_slim_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_frontcache_entry_bytes >
            hz6_stats.metadata_frontcache_entry_bytes) {
            hz6_stats.metadata_frontcache_entry_bytes =
                tds[t].hz6_stats_after.metadata_frontcache_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_frontcache_slim_entry_bytes >
            hz6_stats.metadata_frontcache_slim_entry_bytes) {
            hz6_stats.metadata_frontcache_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_frontcache_slim_entry_bytes;
        }
        hz6_stats.metadata_frontcache_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_frontcache_slim_table_bytes;
        hz6_stats.metadata_frontcache_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_frontcache_slim_savings_bytes;
        hz6_stats.large_span_central_push +=
            tds[t].hz6_stats_after.large_span_central_push;
        hz6_stats.large_span_central_pop +=
            tds[t].hz6_stats_after.large_span_central_pop;
        hz6_stats.large_span_source_alloc +=
            tds[t].hz6_stats_after.large_span_source_alloc;
#endif
        CloseHandle(handles[t]);
        handles[t] = NULL;
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
           "route_visibility_lookup=%zu route_visibility_hit=%zu "
           "route_visibility_hit_local_owner=%zu "
           "route_visibility_hit_foreign_owner=%zu "
           "route_visibility_miss=%zu route_visibility_probe_total=%zu "
           "route_visibility_probe_max=%zu "
           "route_exact_lookup_probe_total=%zu "
           "route_exact_lookup_probe_max=%zu "
           "owner_locality_lookup=%zu "
           "owner_locality_hit_local_allocator=%zu "
           "owner_locality_hit_foreign_allocator=%zu "
           "owner_locality_miss=%zu "
           "owner_locality_register=%zu "
           "owner_locality_unregister=%zu "
           "owner_locality_probe_total=%zu "
           "owner_locality_probe_max=%zu "
           "transfer_push=%zu transfer_pop=%zu transfer_current=%zu "
           "transfer_current_max=%zu remote_free_attempt=%zu "
            "remote_free_strict_owner_block=%zu remote_free_transfer_fail=%zu "
            "route_rehome_attempt=%zu route_rehome_success=%zu "
            "route_rehome_fail=%zu "
            "descriptor_source_route_allocator_match=%zu "
            "descriptor_source_route_allocator_mismatch=%zu "
            "descriptor_source_current_allocator_match=%zu "
            "descriptor_source_current_allocator_mismatch=%zu "
            "descriptor_storage_lookup=%zu "
            "descriptor_storage_hit=%zu "
            "descriptor_storage_miss=%zu "
            "descriptor_storage_probe_total=%zu "
            "descriptor_storage_probe_max=%zu "
            "descriptor_storage_route_allocator_match=%zu "
            "descriptor_storage_route_allocator_mismatch=%zu "
            "descriptor_storage_current_allocator_match=%zu "
            "descriptor_storage_current_allocator_mismatch=%zu "
            "lifecycle_owner_mismatch=%zu "
            "lifecycle_foreign_free_attempt=%zu "
           "lifecycle_foreign_free_handled=%zu "
           "lifecycle_foreign_free_invalid=%zu "
           "visible_first_attempt=%zu visible_first_hit=%zu "
           "visible_first_miss=%zu visible_first_visible_invalid=%zu "
           "visible_first_local_fallback=%zu "
           "visible_first_local_fallback_invalid=%zu "
           "visible_first_local_lookup_skipped=%zu "
           "negative_filter_attempt=%zu negative_filter_not_armed=%zu "
           "negative_filter_rehome_blocked=%zu "
           "negative_filter_skip_local=%zu "
           "negative_filter_maybe_local=%zu "
           "negative_filter_shadow_false_skip=%zu "
           "negative_filter_shadow_local_valid=%zu "
           "negative_filter_shadow_local_invalid=%zu "
           "negative_filter_range_probe_total=%zu "
           "negative_filter_range_probe_max=%zu "
           "shared_dir_lookup=%zu shared_dir_hit=%zu shared_dir_miss=%zu "
           "shared_dir_stale=%zu shared_dir_hit_local_allocator=%zu "
           "shared_dir_hit_foreign_allocator=%zu "
           "shared_dir_would_skip_local=%zu shared_dir_register=%zu "
           "shared_dir_unregister=%zu shared_dir_probe_total=%zu "
           "shared_dir_probe_max=%zu "
           "shared_dir_first_attempt=%zu shared_dir_first_hit=%zu "
           "shared_dir_first_fallback=%zu shared_dir_first_invalid=%zu "
           "source_owned_prepare=%zu "
           "source_owned_route_hit_local_owner=%zu "
           "source_owned_visibility_hit_local_owner=%zu "
           "source_owned_visibility_hit_foreign_owner=%zu "
           "source_owned_remote_free_attempt=%zu "
           "source_owned_release=%zu source_alloc=%zu "
           "local2p_source_alloc=%zu midpage_source_alloc=%zu "
           "large_source_alloc=%zu toy_source_alloc=%zu "
           "front_source_ops_alloc=%zu front_source_slot_alloc=%zu "
           "front_source_prefill_alloc=%zu toy_source_prefill_call=%zu "
           "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
           "transfer_reuse_hit=%zu transfer_reuse_invalid=%zu "
           "source_refill_starvation=%zu source_refill_saturation=%zu "
           "source_refill_boost=%zu source_refill_clamp=%zu "
           "source_admission_open=%zu source_admission_boosted=%zu "
           "source_admission_clamped=%zu "
           "control_plane_normal=%zu "
           "control_plane_burst_supply_would_open=%zu "
           "control_plane_close_would_start=%zu "
           "source_prefill_attempt=%zu source_prefill_filled=%zu "
           "source_prefill_fallback=%zu alloc_fail=%zu "
           "descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu "
           "descriptor_probe_total=%zu descriptor_probe_max=%zu "
           "route_lookup_probe_total=%zu route_lookup_probe_max=%zu "
           "route_register_probe_total=%zu route_register_probe_max=%zu "
           "route_unregister_probe_total=%zu route_unregister_probe_max=%zu "
           "source_block_probe_total=%zu source_block_probe_max=%zu "
           "large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
           "larson_main_final",
           hz6_stats.route_valid,
           hz6_stats.route_invalid,
           hz6_stats.route_miss,
           hz6_stats.route_visibility_lookup,
           hz6_stats.route_visibility_hit,
           hz6_stats.route_visibility_hit_local_owner,
           hz6_stats.route_visibility_hit_foreign_owner,
           hz6_stats.route_visibility_miss,
           hz6_stats.route_visibility_probe_total,
           hz6_stats.route_visibility_probe_max,
           hz6_stats.route_exact_lookup_probe_total,
           hz6_stats.route_exact_lookup_probe_max,
           hz6_stats.owner_locality_lookup,
           hz6_stats.owner_locality_hit_local_allocator,
           hz6_stats.owner_locality_hit_foreign_allocator,
           hz6_stats.owner_locality_miss,
           hz6_stats.owner_locality_register,
           hz6_stats.owner_locality_unregister,
           hz6_stats.owner_locality_probe_total,
           hz6_stats.owner_locality_probe_max,
           hz6_stats.transfer_push,
           hz6_stats.transfer_pop,
           hz6_stats.transfer_current,
           hz6_stats.transfer_current_max,
           hz6_stats.remote_free_attempt,
           hz6_stats.remote_free_strict_owner_block,
           hz6_stats.remote_free_transfer_fail,
            hz6_stats.route_rehome_attempt,
            hz6_stats.route_rehome_success,
            hz6_stats.route_rehome_fail,
            hz6_stats.descriptor_source_route_allocator_match,
            hz6_stats.descriptor_source_route_allocator_mismatch,
            hz6_stats.descriptor_source_current_allocator_match,
            hz6_stats.descriptor_source_current_allocator_mismatch,
            hz6_stats.descriptor_storage_lookup,
            hz6_stats.descriptor_storage_hit,
            hz6_stats.descriptor_storage_miss,
            hz6_stats.descriptor_storage_probe_total,
            hz6_stats.descriptor_storage_probe_max,
            hz6_stats.descriptor_storage_route_allocator_match,
            hz6_stats.descriptor_storage_route_allocator_mismatch,
            hz6_stats.descriptor_storage_current_allocator_match,
            hz6_stats.descriptor_storage_current_allocator_mismatch,
            hz6_stats.lifecycle_owner_mismatch,
           hz6_stats.lifecycle_foreign_free_attempt,
           hz6_stats.lifecycle_foreign_free_handled,
           hz6_stats.lifecycle_foreign_free_invalid,
           hz6_stats.visible_first_attempt,
           hz6_stats.visible_first_hit,
           hz6_stats.visible_first_miss,
           hz6_stats.visible_first_visible_invalid,
           hz6_stats.visible_first_local_fallback,
           hz6_stats.visible_first_local_fallback_invalid,
           hz6_stats.visible_first_local_lookup_skipped,
           hz6_stats.negative_filter_attempt,
           hz6_stats.negative_filter_not_armed,
           hz6_stats.negative_filter_rehome_blocked,
           hz6_stats.negative_filter_skip_local,
           hz6_stats.negative_filter_maybe_local,
           hz6_stats.negative_filter_shadow_false_skip,
           hz6_stats.negative_filter_shadow_local_valid,
           hz6_stats.negative_filter_shadow_local_invalid,
           hz6_stats.negative_filter_range_probe_total,
           hz6_stats.negative_filter_range_probe_max,
           hz6_stats.shared_dir_lookup,
           hz6_stats.shared_dir_hit,
           hz6_stats.shared_dir_miss,
           hz6_stats.shared_dir_stale,
           hz6_stats.shared_dir_hit_local_allocator,
           hz6_stats.shared_dir_hit_foreign_allocator,
           hz6_stats.shared_dir_would_skip_local,
           hz6_stats.shared_dir_register,
           hz6_stats.shared_dir_unregister,
           hz6_stats.shared_dir_probe_total,
           hz6_stats.shared_dir_probe_max,
           hz6_stats.shared_dir_first_attempt,
           hz6_stats.shared_dir_first_hit,
           hz6_stats.shared_dir_first_fallback,
           hz6_stats.shared_dir_first_invalid,
           hz6_stats.source_owned_prepare,
           hz6_stats.source_owned_route_hit_local_owner,
           hz6_stats.source_owned_visibility_hit_local_owner,
           hz6_stats.source_owned_visibility_hit_foreign_owner,
           hz6_stats.source_owned_remote_free_attempt,
           hz6_stats.source_owned_release,
           hz6_stats.source_alloc,
           hz6_stats.local2p_source_alloc,
           hz6_stats.midpage_source_alloc,
           hz6_stats.large_source_alloc,
           hz6_stats.toy_source_alloc,
           hz6_stats.front_source_ops_alloc,
           hz6_stats.front_source_slot_alloc,
           hz6_stats.front_source_prefill_alloc,
           hz6_stats.toy_source_prefill_call,
           hz6_stats.frontcache_reuse_hit,
           hz6_stats.frontcache_reuse_invalid,
           hz6_stats.transfer_reuse_hit,
           hz6_stats.transfer_reuse_invalid,
           hz6_stats.source_refill_starvation,
           hz6_stats.source_refill_saturation,
           hz6_stats.source_refill_boost,
           hz6_stats.source_refill_clamp,
           hz6_stats.source_admission_open,
           hz6_stats.source_admission_boosted,
           hz6_stats.source_admission_clamped,
           hz6_stats.control_plane_normal,
           hz6_stats.control_plane_burst_supply_would_open,
           hz6_stats.control_plane_close_would_start,
           hz6_stats.source_prefill_attempt,
           hz6_stats.source_prefill_filled,
           hz6_stats.source_prefill_fallback,
           hz6_stats.alloc_fail,
           hz6_stats.descriptor_exhausted,
           hz6_stats.route_register_fail,
           hz6_stats.source_block_exhausted,
           hz6_stats.descriptor_probe_total,
           hz6_stats.descriptor_probe_max,
           hz6_stats.route_lookup_probe_total,
           hz6_stats.route_lookup_probe_max,
           hz6_stats.route_register_probe_total,
           hz6_stats.route_register_probe_max,
           hz6_stats.route_unregister_probe_total,
           hz6_stats.route_unregister_probe_max,
           hz6_stats.source_block_probe_total,
           hz6_stats.source_block_probe_max,
           hz6_stats.large_span_central_push,
           hz6_stats.large_span_central_pop,
           hz6_stats.large_span_source_alloc);
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    printf("[HZ6_MEMORY_ATTR] "
           "descriptor_table_bytes=%zu "
           "route_table_bytes=%zu "
           "source_block_table_bytes=%zu "
           "frontcache_table_bytes=%zu "
           "transfer_table_bytes=%zu "
           "ownerlocality_index_bytes=%zu "
           "active_descriptors=%zu "
           "local_free_descriptors=%zu "
           "transfer_free_descriptors=%zu "
           "remote_pending_descriptors=%zu "
           "dead_with_ptr_descriptors=%zu "
           "active_source_blocks=%zu "
           "registered_source_blocks=%zu "
           "ref_nonzero_source_blocks=%zu "
           "source_block_payload_bytes=%zu "
           "source_block_committed_estimate=%zu "
           "route_active_current=%zu "
           "route_active_max=%zu "
           "route_tombstone_current=%zu "
           "frontcache_total=%zu "
           "frontcache_largest_bin=%zu\n",
           hz6_stats.memory_descriptor_table_bytes,
           hz6_stats.memory_route_table_bytes,
           hz6_stats.memory_source_block_table_bytes,
           hz6_stats.memory_frontcache_table_bytes,
           hz6_stats.memory_transfer_table_bytes,
           hz6_stats.memory_ownerlocality_index_bytes,
           hz6_stats.memory_active_descriptors,
           hz6_stats.memory_local_free_descriptors,
           hz6_stats.memory_transfer_free_descriptors,
           hz6_stats.memory_remote_pending_descriptors,
           hz6_stats.memory_dead_with_ptr_descriptors,
           hz6_stats.memory_active_source_blocks,
           hz6_stats.memory_registered_source_blocks,
           hz6_stats.memory_ref_nonzero_source_blocks,
           hz6_stats.memory_source_block_payload_bytes,
           hz6_stats.memory_source_block_committed_estimate,
           hz6_stats.route_active_current,
           hz6_stats.route_active_max,
           hz6_stats.route_tombstone_current,
           hz6_stats.memory_frontcache_total,
           hz6_stats.memory_frontcache_largest_bin);
    printf("[HZ6_METADATA_SLIM] "
           "descriptor_entry_bytes=%zu "
           "descriptor_thin_hot_entry_bytes=%zu "
           "descriptor_thin_hot_table_bytes=%zu "
           "descriptor_thin_hot_savings_bytes=%zu "
           "descriptor_ownerless_hot_entry_bytes=%zu "
           "descriptor_ownerless_hot_table_bytes=%zu "
           "descriptor_ownerless_hot_savings_bytes=%zu "
           "descriptor_owner16_hot_entry_bytes=%zu "
           "descriptor_owner16_hot_table_bytes=%zu "
           "descriptor_owner16_hot_savings_bytes=%zu "
           "route_entry_bytes=%zu "
           "route_slim_entry_bytes=%zu "
           "route_slim_table_bytes=%zu "
           "route_slim_savings_bytes=%zu "
           "source_block_entry_bytes=%zu "
           "source_block_slim_entry_bytes=%zu "
           "source_block_slim_table_bytes=%zu "
           "source_block_slim_savings_bytes=%zu "
           "frontcache_entry_bytes=%zu "
           "frontcache_slim_entry_bytes=%zu "
           "frontcache_slim_table_bytes=%zu "
           "frontcache_slim_savings_bytes=%zu\n",
           hz6_stats.metadata_descriptor_entry_bytes,
           hz6_stats.metadata_descriptor_thin_hot_entry_bytes,
           hz6_stats.metadata_descriptor_thin_hot_table_bytes,
           hz6_stats.metadata_descriptor_thin_hot_savings_bytes,
           hz6_stats.metadata_descriptor_ownerless_hot_entry_bytes,
           hz6_stats.metadata_descriptor_ownerless_hot_table_bytes,
           hz6_stats.metadata_descriptor_ownerless_hot_savings_bytes,
           hz6_stats.metadata_descriptor_owner16_hot_entry_bytes,
           hz6_stats.metadata_descriptor_owner16_hot_table_bytes,
           hz6_stats.metadata_descriptor_owner16_hot_savings_bytes,
           hz6_stats.metadata_route_entry_bytes,
           hz6_stats.metadata_route_slim_entry_bytes,
           hz6_stats.metadata_route_slim_table_bytes,
           hz6_stats.metadata_route_slim_savings_bytes,
           hz6_stats.metadata_source_block_entry_bytes,
           hz6_stats.metadata_source_block_slim_entry_bytes,
           hz6_stats.metadata_source_block_slim_table_bytes,
           hz6_stats.metadata_source_block_slim_savings_bytes,
           hz6_stats.metadata_frontcache_entry_bytes,
           hz6_stats.metadata_frontcache_slim_entry_bytes,
           hz6_stats.metadata_frontcache_slim_table_bytes,
           hz6_stats.metadata_frontcache_slim_savings_bytes);
    print_hz6_front_alloc_paths(&hz6_stats);
    print_hz6_front_prefill_paths(&hz6_stats);
    print_hz6_frontcache_class_diag(&hz6_stats);
#endif
#else
    hz_bench_dump_stats(stdout, "larson_main_final");
#endif
    printf("Done sleeping...\n");

cleanup:
    if (handles) {
        for (size_t i = 0; i < threads; ++i) {
            if (handles[i]) {
                CloseHandle(handles[i]);
                handles[i] = NULL;
            }
        }
    }
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
    return exit_code;
}
