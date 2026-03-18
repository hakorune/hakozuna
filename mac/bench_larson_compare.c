// macOS / POSIX Larson-style allocator comparison bench.
// Usage: bench_larson_compare [runtime_sec] [min_size] [max_size] [chunks_per_thread] [rounds] [seed] [threads]

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct LarsonThreadData {
    void **blocks;
    size_t *sizes;
    size_t live_count;
    size_t iterations_per_check;
    size_t min_size;
    size_t max_size;
    uint32_t seed;
    uint64_t alloc_count;
    uint64_t free_count;
} LarsonThreadData;

static _Atomic int g_stop_flag = 0;

static uint32_t parse_u32(const char *s, uint32_t def) {
    if (!s) {
        return def;
    }

    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || (end && *end != '\0')) {
        return def;
    }
    if (v > 0xFFFFFFFFu) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t)v;
}

static inline uint32_t rng_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1u) : 1u;
    return min_size + (r % span);
}

static uint64_t now_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline void *bench_alloc(size_t size) {
    return malloc(size);
}

static inline void bench_free(void *ptr) {
    free(ptr);
}

static inline void touch_allocation(void *ptr, size_t size, uint32_t tag) {
    unsigned char *p = (unsigned char *)ptr;
    if (!p || size == 0) {
        return;
    }

    p[0] = (unsigned char)(tag & 0xFFu);
    if (size > 1) {
        p[size - 1] = (unsigned char)((tag >> 8) & 0xFFu);
    }
}

static void *larson_thread(void *arg) {
    LarsonThreadData *td = (LarsonThreadData *)arg;
    uint32_t seed = td->seed;

    for (;;) {
        if (atomic_load_explicit(&g_stop_flag, memory_order_relaxed) != 0) {
            break;
        }

        for (size_t i = 0; i < td->iterations_per_check; ++i) {
            if (atomic_load_explicit(&g_stop_flag, memory_order_relaxed) != 0) {
                break;
            }

            uint32_t r = rng_next(&seed);
            size_t victim = (size_t)(r % (uint32_t)td->live_count);
            if (td->blocks[victim]) {
                bench_free(td->blocks[victim]);
                td->blocks[victim] = NULL;
                td->sizes[victim] = 0;
                td->free_count++;
            }

            size_t new_size = pick_size(rng_next(&seed), td->min_size, td->max_size);
            void *next = bench_alloc(new_size);
            if (!next) {
                continue;
            }

            touch_allocation(next, new_size, r);
            td->blocks[victim] = next;
            td->sizes[victim] = new_size;
            td->alloc_count++;
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    uint32_t runtime_sec = (argc > 1) ? parse_u32(argv[1], 10u) : 10u;
    uint32_t min_size = (argc > 2) ? parse_u32(argv[2], 8u) : 8u;
    uint32_t max_size = (argc > 3) ? parse_u32(argv[3], 1024u) : 1024u;
    uint32_t chunks_per_thread = (argc > 4) ? parse_u32(argv[4], 10000u) : 10000u;
    uint32_t rounds = (argc > 5) ? parse_u32(argv[5], 1u) : 1u;
    uint32_t seed = (argc > 6) ? parse_u32(argv[6], 12345u) : 12345u;
    uint32_t threads = (argc > 7) ? parse_u32(argv[7], 4u) : 4u;

    if (runtime_sec == 0 || chunks_per_thread == 0 || rounds == 0 || threads == 0 || min_size == 0 || max_size < min_size) {
        fprintf(stderr, "bench_larson_compare: invalid args\n");
        fprintf(stderr, "usage: %s <runtime_sec> <min> <max> <chunks_per_thread> <rounds> <seed> <threads>\n", argv[0]);
        return 2;
    }

    size_t total_slots = (size_t)threads * (size_t)chunks_per_thread;
    LarsonThreadData *tds = (LarsonThreadData *)calloc(threads, sizeof(LarsonThreadData));
    pthread_t *tids = (pthread_t *)calloc(threads, sizeof(pthread_t));
    void **all_blocks = (void **)calloc(total_slots, sizeof(void *));
    size_t *all_sizes = (size_t *)calloc(total_slots, sizeof(size_t));
    if (!tds || !tids || !all_blocks || !all_sizes) {
        fprintf(stderr, "bench_larson_compare: setup allocation failed\n");
        free(all_sizes);
        free(all_blocks);
        free(tids);
        free(tds);
        return 1;
    }

    printf("[BENCH_ARGS] runtime=%u min=%u max=%u chunks=%u rounds=%u seed=%u threads=%u\n",
           runtime_sec, min_size, max_size, chunks_per_thread, rounds, seed, threads);

    for (size_t t = 0; t < threads; ++t) {
        size_t base = t * (size_t)chunks_per_thread;
        uint32_t local_seed = seed + (uint32_t)(t * 977u);
        for (size_t i = 0; i < chunks_per_thread; ++i) {
            size_t slot = base + i;
            size_t size = pick_size(rng_next(&local_seed), min_size, max_size);
            void *p = bench_alloc(size);
            if (!p) {
                fprintf(stderr, "bench_larson_compare: warmup alloc failed at thread=%zu slot=%zu size=%zu\n", t, i, size);
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

    atomic_store_explicit(&g_stop_flag, 0, memory_order_relaxed);
    uint64_t start_ns = now_ns();

    for (size_t t = 0; t < threads; ++t) {
        if (pthread_create(&tids[t], NULL, larson_thread, &tds[t]) != 0) {
            fprintf(stderr, "bench_larson_compare: thread start failed at thread=%zu\n", t);
            atomic_store_explicit(&g_stop_flag, 1, memory_order_relaxed);
            for (size_t j = 0; j < t; ++j) {
                (void)pthread_join(tids[j], NULL);
            }
            goto cleanup;
        }
    }

    struct timespec sleep_ts;
    sleep_ts.tv_sec = runtime_sec;
    sleep_ts.tv_nsec = 0;
    nanosleep(&sleep_ts, NULL);

    atomic_store_explicit(&g_stop_flag, 1, memory_order_relaxed);
    for (size_t t = 0; t < threads; ++t) {
        (void)pthread_join(tids[t], NULL);
    }

    uint64_t end_ns = now_ns();
    double duration_sec = (double)(end_ns - start_ns) / 1000000000.0;
    uint64_t total_allocs = 0;
    uint64_t total_frees = 0;
    for (size_t t = 0; t < threads; ++t) {
        total_allocs += tds[t].alloc_count;
        total_frees += tds[t].free_count;
    }

    printf("Throughput = %.0f operations per second, relative time: %.3fs.\n",
           (duration_sec > 0.0) ? ((double)total_allocs / duration_sec) : 0.0,
           duration_sec);
    printf("[SUMMARY] allocs=%llu frees=%llu live_slots=%zu\n",
           (unsigned long long)total_allocs,
           (unsigned long long)total_frees,
           total_slots);

cleanup:
    for (size_t i = 0; i < total_slots; ++i) {
        if (all_blocks && all_blocks[i]) {
            bench_free(all_blocks[i]);
            all_blocks[i] = NULL;
        }
    }

    free(all_sizes);
    free(all_blocks);
    free(tids);
    free(tds);
    return 0;
}
