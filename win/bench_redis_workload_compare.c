// Windows-native Redis-style workload benchmark.
// Usage: bench_redis_workload_compare [threads] [cycles] [ops_per_cycle] [min_size] [max_size]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HZ_BENCH_USE_HZ3)
#include "hz3.h"
#elif defined(HZ_BENCH_USE_HZ4)
#include "hz4_win_api.h"
#elif defined(HZ_BENCH_USE_MIMALLOC)
#include <mimalloc.h>
#elif defined(HZ_BENCH_USE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

typedef enum RedisOp {
    REDIS_SET = 0,
    REDIS_GET = 1,
    REDIS_LPUSH = 2,
    REDIS_LPOP = 3,
    REDIS_RANDOM = 4,
    REDIS_OP_COUNT = 5
} RedisOp;

typedef struct ThreadArg {
    RedisOp op;
    uint32_t cycles;
    uint32_t ops_per_cycle;
    uint32_t min_size;
    uint32_t max_size;
    uint32_t seed;
    uint64_t ops_done;
} ThreadArg;

static const char* kOpNames[REDIS_OP_COUNT] = {
    "SET", "GET", "LPUSH", "LPOP", "RANDOM"
};

static uint32_t g_threads = 4;
static uint32_t g_pool_capacity = 0;

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
        return (whole * 1000000000ULL) + ((rem * 1000000000ULL) / (uint64_t)freq.QuadPart);
    }
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1u) : 1u;
    return min_size + (r % span);
}

static inline void* bench_alloc(size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    return hz3_malloc(size);
#elif defined(HZ_BENCH_USE_HZ4)
    return hz4_win_malloc(size);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    return mi_malloc(size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    return tc_malloc(size);
#else
    return malloc(size);
#endif
}

static inline void bench_free(void* ptr) {
#if defined(HZ_BENCH_USE_HZ3)
    hz3_free(ptr);
#elif defined(HZ_BENCH_USE_HZ4)
    hz4_win_free(ptr);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    mi_free(ptr);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    tc_free(ptr);
#else
    free(ptr);
#endif
}

static char* alloc_string(uint32_t* seed, size_t min_size, size_t max_size, uint32_t ordinal) {
    size_t size = pick_size(rng_next(seed), min_size, max_size);
    char* ptr = (char*)bench_alloc(size);
    if (!ptr) {
        return NULL;
    }
    _snprintf(ptr, size, "key-%u", ordinal);
    if (size > 1) {
        ptr[size - 1] = '\0';
    }
    return ptr;
}

static unsigned __stdcall redis_worker(void* raw_arg) {
    ThreadArg* arg = (ThreadArg*)raw_arg;
    char** pool = NULL;
    uint32_t count = 0;
    uint32_t seed = arg->seed;
    uint32_t i;
    uint32_t cycle;

    pool = (char**)calloc(g_pool_capacity, sizeof(char*));
    if (!pool) {
        return 0;
    }

    for (i = 0; i < g_pool_capacity / 2u; ++i) {
        pool[count] = alloc_string(&seed, arg->min_size, arg->max_size, i);
        if (pool[count]) {
            count++;
        }
    }

    for (cycle = 0; cycle < arg->cycles; ++cycle) {
        for (i = 0; i < arg->ops_per_cycle; ++i) {
            RedisOp effective = arg->op;
            uint32_t r;
            if (effective == REDIS_RANDOM) {
                r = rng_next(&seed) % 100u;
                if (r < 70u) {
                    effective = REDIS_GET;
                } else if (r < 90u) {
                    effective = REDIS_SET;
                } else if (r < 95u) {
                    effective = REDIS_LPUSH;
                } else {
                    effective = REDIS_LPOP;
                }
            }

            switch (effective) {
                case REDIS_SET: {
                    char* ptr = alloc_string(&seed, arg->min_size, arg->max_size, i + cycle);
                    if (ptr) {
                        bench_free(ptr);
                    }
                    arg->ops_done++;
                    break;
                }
                case REDIS_GET: {
                    if (count > 0) {
                        char* ptr = pool[rng_next(&seed) % count];
                        if (ptr) {
                            volatile size_t len = strlen(ptr);
                            (void)len;
                        }
                    }
                    arg->ops_done++;
                    break;
                }
                case REDIS_LPUSH: {
                    if (count < g_pool_capacity) {
                        pool[count] = alloc_string(&seed, arg->min_size, arg->max_size, i + cycle);
                        if (pool[count]) {
                            count++;
                        }
                    } else if (count > 0) {
                        uint32_t victim = rng_next(&seed) % count;
                        bench_free(pool[victim]);
                        pool[victim] = alloc_string(&seed, arg->min_size, arg->max_size, i + cycle);
                    }
                    arg->ops_done++;
                    break;
                }
                case REDIS_LPOP: {
                    if (count > 0) {
                        uint32_t victim = rng_next(&seed) % count;
                        bench_free(pool[victim]);
                        pool[victim] = pool[count - 1u];
                        pool[count - 1u] = NULL;
                        count--;
                    }
                    arg->ops_done++;
                    break;
                }
                case REDIS_RANDOM:
                default:
                    break;
            }
        }
    }

    for (i = 0; i < count; ++i) {
        if (pool[i]) {
            bench_free(pool[i]);
        }
    }
    free(pool);
    return 0;
}

static void run_pattern(RedisOp op, uint32_t threads, uint32_t cycles, uint32_t ops_per_cycle, uint32_t min_size, uint32_t max_size) {
    ThreadArg* args = NULL;
    HANDLE* handles = NULL;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t total_ops = 0;
    double dt;
    uint32_t i;

    args = (ThreadArg*)calloc(threads, sizeof(ThreadArg));
    handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    if (!args || !handles) {
        fprintf(stderr, "run_pattern: allocation failed\n");
        free(handles);
        free(args);
        return;
    }

    start_ns = now_ns();
    for (i = 0; i < threads; ++i) {
        args[i].op = op;
        args[i].cycles = cycles;
        args[i].ops_per_cycle = ops_per_cycle;
        args[i].min_size = min_size;
        args[i].max_size = max_size;
        args[i].seed = 0x12345678u + (i * 977u) + (uint32_t)op;
        args[i].ops_done = 0;
        {
            uintptr_t h = _beginthreadex(NULL, 0, redis_worker, &args[i], 0, NULL);
            if (h == 0) {
                fprintf(stderr, "run_pattern: thread start failed at thread=%u\n", i);
                threads = i;
                break;
            }
            handles[i] = (HANDLE)h;
        }
    }

    if (threads > 0) {
        WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    }
    end_ns = now_ns();

    for (i = 0; i < threads; ++i) {
        total_ops += args[i].ops_done;
        if (handles[i]) {
            CloseHandle(handles[i]);
        }
    }

    dt = (double)(end_ns - start_ns) / 1e9;
    printf("Pattern: %s\n", kOpNames[op]);
    printf("Throughput: %.2f M ops/sec\n", (dt > 0.0) ? ((double)total_ops / dt) / 1000000.0 : 0.0);
    printf("Ops: %llu\n", (unsigned long long)total_ops);
    printf("---\n");

    free(handles);
    free(args);
}

int main(int argc, char** argv) {
    uint32_t threads = (argc > 1) ? parse_u32(argv[1], 4u) : 4u;
    uint32_t cycles = (argc > 2) ? parse_u32(argv[2], 500u) : 500u;
    uint32_t ops_per_cycle = (argc > 3) ? parse_u32(argv[3], 2000u) : 2000u;
    uint32_t min_size = (argc > 4) ? parse_u32(argv[4], 16u) : 16u;
    uint32_t max_size = (argc > 5) ? parse_u32(argv[5], 256u) : 256u;
    uint32_t op;

    if (threads == 0 || cycles == 0 || ops_per_cycle == 0 || min_size == 0 || max_size < min_size) {
        fprintf(stderr, "bench_redis_workload_compare: invalid args\n");
        fprintf(stderr, "usage: %s <threads> <cycles> <ops_per_cycle> <min_size> <max_size>\n", argv[0]);
        return 2;
    }

    g_threads = threads;
    g_pool_capacity = ops_per_cycle * 2u;
    if (g_pool_capacity < 1024u) {
        g_pool_capacity = 1024u;
    }

    printf("[BENCH_ARGS] threads=%u cycles=%u ops=%u min=%u max=%u\n",
           threads, cycles, ops_per_cycle, min_size, max_size);

    for (op = 0; op < REDIS_OP_COUNT; ++op) {
        run_pattern((RedisOp)op, threads, cycles, ops_per_cycle, min_size, max_size);
    }

    return 0;
}
