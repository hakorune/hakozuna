// macOS Redis-style workload benchmark.
// Usage: bench_redis_workload_compare [threads] [cycles] [ops_per_cycle] [min_size] [max_size]
//
// The benchmark itself uses plain CRT malloc/free/realloc.
// Allocator comparisons are done externally via DYLD_INSERT_LIBRARIES.

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static uint32_t g_pool_capacity = 0;

static uint32_t parse_u32(const char* s, uint32_t def) {
    if (!s) {
        return def;
    }
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

static uint32_t rng_next(uint32_t* s) {
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline size_t pick_size(uint32_t r, size_t min_size, size_t max_size) {
    size_t span = (max_size > min_size) ? (max_size - min_size + 1u) : 1u;
    return min_size + (r % span);
}

static inline void* bench_alloc(size_t size) {
    return malloc(size);
}

static inline void bench_free(void* ptr) {
    free(ptr);
}

static char* alloc_string(uint32_t* seed, size_t min_size, size_t max_size, uint32_t ordinal) {
    size_t size = pick_size(rng_next(seed), min_size, max_size);
    char* ptr = (char*)bench_alloc(size);
    if (!ptr) {
        return NULL;
    }
    (void)snprintf(ptr, size, "key-%u", ordinal);
    if (size > 0) {
        ptr[size - 1] = '\0';
    }
    return ptr;
}

static void* redis_worker(void* raw_arg) {
    ThreadArg* arg = (ThreadArg*)raw_arg;
    char** pool = NULL;
    uint32_t count = 0;
    uint32_t seed = arg->seed;

    pool = (char**)calloc(g_pool_capacity, sizeof(char*));
    if (!pool) {
        return NULL;
    }

    for (uint32_t i = 0; i < g_pool_capacity / 2u; ++i) {
        pool[count] = alloc_string(&seed, arg->min_size, arg->max_size, i);
        if (pool[count]) {
            count++;
        }
    }

    for (uint32_t cycle = 0; cycle < arg->cycles; ++cycle) {
        for (uint32_t i = 0; i < arg->ops_per_cycle; ++i) {
            RedisOp effective = arg->op;
            if (effective == REDIS_RANDOM) {
                uint32_t r = rng_next(&seed) % 100u;
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

    for (uint32_t i = 0; i < count; ++i) {
        if (pool[i]) {
            bench_free(pool[i]);
        }
    }
    free(pool);
    return NULL;
}

static void run_pattern(RedisOp op, uint32_t threads, uint32_t cycles, uint32_t ops_per_cycle, uint32_t min_size, uint32_t max_size) {
    ThreadArg* args = NULL;
    pthread_t* tids = NULL;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t total_ops = 0;

    args = (ThreadArg*)calloc(threads, sizeof(ThreadArg));
    tids = (pthread_t*)calloc(threads, sizeof(pthread_t));
    if (!args || !tids) {
        fprintf(stderr, "run_pattern: allocation failed\n");
        free(tids);
        free(args);
        return;
    }

    start_ns = now_ns();
    for (uint32_t i = 0; i < threads; ++i) {
        args[i].op = op;
        args[i].cycles = cycles;
        args[i].ops_per_cycle = ops_per_cycle;
        args[i].min_size = min_size;
        args[i].max_size = max_size;
        args[i].seed = 0x12345678u + (i * 977u) + (uint32_t)op;
        args[i].ops_done = 0;
        if (pthread_create(&tids[i], NULL, redis_worker, &args[i]) != 0) {
            fprintf(stderr, "run_pattern: thread start failed at thread=%u\n", i);
            threads = i;
            break;
        }
    }

    for (uint32_t i = 0; i < threads; ++i) {
        pthread_join(tids[i], NULL);
    }
    end_ns = now_ns();

    for (uint32_t i = 0; i < threads; ++i) {
        total_ops += args[i].ops_done;
    }

    double dt = (double)(end_ns - start_ns) / 1e9;
    printf("Pattern: %s\n", kOpNames[op]);
    printf("Throughput: %.2f M ops/sec\n", (dt > 0.0) ? ((double)total_ops / dt) / 1000000.0 : 0.0);
    printf("Ops: %llu\n", (unsigned long long)total_ops);
    printf("---\n");

    free(tids);
    free(args);
}

int main(int argc, char** argv) {
    uint32_t threads = (argc > 1) ? parse_u32(argv[1], 4u) : 4u;
    uint32_t cycles = (argc > 2) ? parse_u32(argv[2], 500u) : 500u;
    uint32_t ops_per_cycle = (argc > 3) ? parse_u32(argv[3], 2000u) : 2000u;
    uint32_t min_size = (argc > 4) ? parse_u32(argv[4], 16u) : 16u;
    uint32_t max_size = (argc > 5) ? parse_u32(argv[5], 256u) : 256u;

    if (threads == 0 || cycles == 0 || ops_per_cycle == 0 || min_size == 0 || max_size < min_size) {
        fprintf(stderr, "bench_redis_workload_compare: invalid args\n");
        fprintf(stderr, "usage: %s <threads> <cycles> <ops_per_cycle> <min_size> <max_size>\n", argv[0]);
        return 2;
    }

    g_pool_capacity = ops_per_cycle * 2u;
    if (g_pool_capacity < 1024u) {
        g_pool_capacity = 1024u;
    }

    printf("[BENCH_ARGS] threads=%u cycles=%u ops=%u min=%u max=%u\n",
           threads, cycles, ops_per_cycle, min_size, max_size);

    for (uint32_t op = 0; op < REDIS_OP_COUNT; ++op) {
        run_pattern((RedisOp)op, threads, cycles, ops_per_cycle, min_size, max_size);
    }

    return 0;
}
