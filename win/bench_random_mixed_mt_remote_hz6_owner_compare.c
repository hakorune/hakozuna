// Windows-native HZ6 owner-aware mt-remote benchmark.
// Usage: bench_random_mixed_mt_remote_hz6_owner_compare [profile] [threads] [iters] [working_set] [min_size] [max_size] [remote_pct] [ring_slots]

#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_api_owner.h"
#include "hz6_profiles.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

typedef struct ThreadState {
    uint32_t id;
    uint32_t threads;
    uint32_t iters;
    uint32_t ws;
    uint32_t min_size;
    uint32_t max_size;
    uint32_t remote_pct;
    uint32_t seed;
    void** slots;
    Hz6Allocator allocator;
    Hz6ProfileId profile;
    uint64_t ops;
    uint64_t allocs;
    uint64_t local_frees;
    uint64_t remote_frees;
    uint64_t remote_fallback_frees;
    uint64_t alloc_failures;
    uint64_t owner_switch_failures;
    int fatal_error;
} ThreadState;

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

static uint32_t rng_next(uint32_t* s) {
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

static uint64_t now_ns(void) {
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    {
        uint64_t whole = (uint64_t)(counter.QuadPart / freq.QuadPart);
        uint64_t rem = (uint64_t)(counter.QuadPart % freq.QuadPart);
        return (whole * 1000000000ULL) +
               ((rem * 1000000000ULL) / (uint64_t)freq.QuadPart);
    }
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1u) : 1u;
    return min_size + (r % span);
}

static const char* profile_name(Hz6ProfileId profile) {
    switch (profile) {
        case HZ6_PROFILE_STRICT:
            return "strict";
        case HZ6_PROFILE_SPEED:
            return "speed";
        case HZ6_PROFILE_RSS:
            return "rss";
        case HZ6_PROFILE_REMOTE:
            return "remote";
        default:
            return "unknown";
    }
}

static int parse_profile(const char* value, Hz6ProfileId* profile) {
    if (!value || !profile) {
        return 0;
    }
    if (strcmp(value, "strict") == 0) {
        *profile = HZ6_PROFILE_STRICT;
        return 1;
    }
    if (strcmp(value, "speed") == 0) {
        *profile = HZ6_PROFILE_SPEED;
        return 1;
    }
    if (strcmp(value, "rss") == 0) {
        *profile = HZ6_PROFILE_RSS;
        return 1;
    }
    if (strcmp(value, "remote") == 0) {
        *profile = HZ6_PROFILE_REMOTE;
        return 1;
    }
    return 0;
}

static void touch_payload(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }
    volatile unsigned char* bytes = (volatile unsigned char*)ptr;
    bytes[0] ^= 0x5au;
    bytes[size / 2u] ^= 0xa5u;
    bytes[size - 1u] ^= 0x3cu;
}

static int set_owner_slot(ThreadState* ts, uint32_t slot) {
    if (hz6_allocator_debug_set_owner_slot(&ts->allocator, slot)) {
        return 1;
    }
    ts->owner_switch_failures++;
    ts->fatal_error = 1;
    InterlockedExchange(&g_stop_flag, 1);
    fprintf(stderr,
            "mt_remote_hz6_owner: owner slot switch failed thread=%u slot=%u\n",
            ts->id, slot);
    return 0;
}

static void print_stats_total(const ThreadState* states, uint32_t threads) {
    Hz6StatsSnapshot total = {0};
    uint64_t alloc_failures = 0;
    uint64_t owner_switch_failures = 0;
    uint64_t remote_frees = 0;
    uint64_t remote_fallback_frees = 0;
    uint64_t local_frees = 0;
    uint64_t ops = 0;

    for (uint32_t i = 0; i < threads; ++i) {
        const ThreadState* ts = &states[i];
        Hz6StatsSnapshot s = hz6_stats_snapshot(&ts->allocator);
        total.route_valid += s.route_valid;
        total.route_invalid += s.route_invalid;
        total.route_miss += s.route_miss;
        total.transfer_push += s.transfer_push;
        total.transfer_pop += s.transfer_pop;
        total.source_alloc += s.source_alloc;
        total.alloc_fail += s.alloc_fail;
        total.descriptor_exhausted += s.descriptor_exhausted;
        total.route_register_fail += s.route_register_fail;
        total.source_block_exhausted += s.source_block_exhausted;
        total.large_span_central_push += s.large_span_central_push;
        total.large_span_central_pop += s.large_span_central_pop;
        total.large_span_source_alloc += s.large_span_source_alloc;
        alloc_failures += ts->alloc_failures;
        owner_switch_failures += ts->owner_switch_failures;
        remote_frees += ts->remote_frees;
        remote_fallback_frees += ts->remote_fallback_frees;
        local_frees += ts->local_frees;
        ops += ts->ops;
    }

    printf("[OWNER_AWARE] threads=%u ops=%llu local_frees=%llu remote_frees=%llu remote_fallback_frees=%llu alloc_failures=%llu owner_switch_failures=%llu\n",
           threads,
           (unsigned long long)ops,
           (unsigned long long)local_frees,
           (unsigned long long)remote_frees,
           (unsigned long long)remote_fallback_frees,
           (unsigned long long)alloc_failures,
           (unsigned long long)owner_switch_failures);
    printf("[HZ6_STATS] label=mt_remote_hz6_owner_final route_valid=%zu route_invalid=%zu route_miss=%zu transfer_push=%zu transfer_pop=%zu source_alloc=%zu alloc_fail=%zu descriptor_exhausted=%zu route_register_fail=%zu source_block_exhausted=%zu large_span_central_push=%zu large_span_central_pop=%zu large_span_source_alloc=%zu\n",
           total.route_valid,
           total.route_invalid,
           total.route_miss,
           total.transfer_push,
           total.transfer_pop,
           total.source_alloc,
           total.alloc_fail,
           total.descriptor_exhausted,
           total.route_register_fail,
           total.source_block_exhausted,
           total.large_span_central_push,
           total.large_span_central_pop,
           total.large_span_source_alloc);
}

static unsigned __stdcall bench_thread(void* arg) {
    ThreadState* ts = (ThreadState*)arg;
    uint32_t seed = ts->seed;

    hz6_allocator_init_with_profile(&ts->allocator, ts->profile);
    if (!set_owner_slot(ts, 0u)) {
        return 1;
    }

    for (uint32_t i = 0; i < ts->iters; ++i) {
        uint32_t r;
        uint32_t idx;
        size_t size;
        void* ptr;

        if (InterlockedCompareExchange(&g_stop_flag, 0, 0) != 0 || ts->fatal_error) {
            break;
        }

        r = rng_next(&seed);
        idx = r % ts->ws;
        size = pick_size(rng_next(&seed), ts->min_size, ts->max_size);
        ptr = ts->slots[idx];

        if (ptr) {
            if (ts->threads > 1u && (rng_next(&seed) % 100u) < ts->remote_pct) {
                if (set_owner_slot(ts, 1u)) {
                    if (!hz6_free_remote(&ts->allocator, ptr)) {
                        ts->remote_fallback_frees++;
                        if (!set_owner_slot(ts, 0u)) {
                            break;
                        }
                        hz6_free(&ts->allocator, ptr);
                        ts->local_frees++;
                    } else {
                        ts->remote_frees++;
                    }
                    ts->slots[idx] = NULL;
                    ts->ops++;
                    (void)set_owner_slot(ts, 0u);
                }
            } else {
                hz6_free(&ts->allocator, ptr);
                ts->local_frees++;
                ts->slots[idx] = NULL;
                ts->ops++;
            }
            continue;
        }

        ptr = hz6_malloc(&ts->allocator, size);
        if (!ptr) {
            ts->alloc_failures++;
            if (ts->alloc_failures <= 3u) {
                fprintf(stderr,
                        "mt_remote_hz6_owner_alloc_fail: thread=%u size=%zu profile=%s\n",
                        ts->id, size, profile_name(ts->profile));
            }
            continue;
        }

        touch_payload(ptr, size);
        ts->slots[idx] = ptr;
        ts->allocs++;
        ts->ops++;
    }

    (void)set_owner_slot(ts, 0u);
    for (uint32_t i = 0; i < ts->ws; ++i) {
        if (ts->slots[i]) {
            hz6_free(&ts->allocator, ts->slots[i]);
            ts->slots[i] = NULL;
        }
    }

    hz6_allocator_destroy(&ts->allocator);
    return 0;
}

static void usage(const char* argv0) {
    fprintf(stderr,
            "usage: %s [profile] [threads] [iters] [working_set] [min_size] [max_size] [remote_pct] [ring_slots]\n"
            "  profile: strict | speed | rss | remote\n",
            argv0);
}

int main(int argc, char** argv) {
    Hz6ProfileId profile = HZ6_PROFILE_STRICT;
    uint32_t threads = 16u;
    uint32_t iters = 2000000u;
    uint32_t ws = 400u;
    uint32_t min_size = 16u;
    uint32_t max_size = 2048u;
    uint32_t remote_pct = 90u;
    uint32_t ring_slots = 65536u;
    ThreadState* states = NULL;
    HANDLE* handles = NULL;
    uint64_t start_ns;
    uint64_t end_ns;
    double dt;
    uint64_t total_ops = 0;
    uint64_t total_allocs = 0;
    uint64_t total_remote_frees = 0;
    uint64_t total_remote_fallback_frees = 0;
    uint64_t total_local_frees = 0;
    uint64_t total_alloc_failures = 0;
    uint64_t total_owner_switch_failures = 0;
    uint32_t i;
    int rc = 1;

    if (argc > 1 && !parse_profile(argv[1], &profile)) {
        usage(argv[0]);
        return 2;
    }
    if (argc > 2) {
        threads = parse_u32(argv[2], threads);
    }
    if (argc > 3) {
        iters = parse_u32(argv[3], iters);
    }
    if (argc > 4) {
        ws = parse_u32(argv[4], ws);
    }
    if (argc > 5) {
        min_size = parse_u32(argv[5], min_size);
    }
    if (argc > 6) {
        max_size = parse_u32(argv[6], max_size);
    }
    if (argc > 7) {
        remote_pct = parse_u32(argv[7], remote_pct);
    }
    if (argc > 8) {
        ring_slots = parse_u32(argv[8], ring_slots);
    }

    if (threads == 0 || threads > 64u || iters == 0 || ws == 0 ||
        min_size == 0 || max_size < min_size || remote_pct > 100u ||
        ring_slots == 0) {
        fprintf(stderr, "bench_random_mixed_mt_remote_hz6_owner_compare: invalid args\n");
        usage(argv[0]);
        return 2;
    }

    printf("[BENCH_ARGS] profile=%s threads=%u iters=%u ws=%u min=%u max=%u remote_pct=%u ring_slots=%u\n",
           profile_name(profile), threads, iters, ws, min_size, max_size,
           remote_pct, ring_slots);

    states = (ThreadState*)calloc(threads, sizeof(ThreadState));
    handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    if (!states || !handles) {
        fprintf(stderr, "bench_random_mixed_mt_remote_hz6_owner_compare: setup allocation failed\n");
        goto cleanup;
    }

    for (i = 0; i < threads; ++i) {
        states[i].id = i;
        states[i].threads = threads;
        states[i].iters = iters;
        states[i].ws = ws;
        states[i].min_size = min_size;
        states[i].max_size = max_size;
        states[i].remote_pct = remote_pct;
        states[i].seed = 0x12345678u + (i * 977u);
        states[i].slots = (void**)calloc(ws, sizeof(void*));
        states[i].profile = profile;
        if (!states[i].slots) {
            fprintf(stderr, "bench_random_mixed_mt_remote_hz6_owner_compare: slots allocation failed at thread=%u\n", i);
            goto cleanup;
        }
    }

    start_ns = now_ns();
    for (i = 0; i < threads; ++i) {
        uintptr_t h = _beginthreadex(NULL, 0, bench_thread, &states[i], 0, NULL);
        if (h == 0) {
            fprintf(stderr, "bench_random_mixed_mt_remote_hz6_owner_compare: thread start failed at thread=%u\n", i);
            InterlockedExchange(&g_stop_flag, 1);
            goto cleanup_threads;
        }
        handles[i] = (HANDLE)h;
    }

    if (WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE) == WAIT_FAILED) {
        fprintf(stderr, "bench_random_mixed_mt_remote_hz6_owner_compare: wait failed\n");
        InterlockedExchange(&g_stop_flag, 1);
        goto cleanup_threads;
    }
    end_ns = now_ns();

    for (i = 0; i < threads; ++i) {
        total_ops += states[i].ops;
        total_allocs += states[i].allocs;
        total_remote_frees += states[i].remote_frees;
        total_remote_fallback_frees += states[i].remote_fallback_frees;
        total_local_frees += states[i].local_frees;
        total_alloc_failures += states[i].alloc_failures;
        total_owner_switch_failures += states[i].owner_switch_failures;
    }

    dt = (double)(end_ns - start_ns) / 1e9;
    printf("bench_random_mixed_mt_remote_hz6_owner: profile=%s threads=%u ops=%llu time=%.6f ops/s=%.2f allocs=%llu remote_frees=%llu remote_fallback_frees=%llu local_frees=%llu\n",
           profile_name(profile),
           threads,
           (unsigned long long)total_ops,
           dt,
           (dt > 0.0) ? ((double)total_ops / dt) : 0.0,
           (unsigned long long)total_allocs,
           (unsigned long long)total_remote_frees,
           (unsigned long long)total_remote_fallback_frees,
           (unsigned long long)total_local_frees);
    printf("[OWNER_AWARE_TOTALS] alloc_failures=%llu owner_switch_failures=%llu\n",
           (unsigned long long)total_alloc_failures,
           (unsigned long long)total_owner_switch_failures);
    print_stats_total(states, threads);
    rc = 0;

cleanup_threads:
    if (handles) {
        for (i = 0; i < threads; ++i) {
            if (handles[i]) {
                CloseHandle(handles[i]);
            }
        }
    }

cleanup:
    if (states) {
        for (i = 0; i < threads; ++i) {
            free(states[i].slots);
            states[i].slots = NULL;
        }
    }
    free(handles);
    free(states);
    return rc;
}
