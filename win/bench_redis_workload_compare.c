// Windows-native Redis-style workload benchmark.
// Usage: bench_redis_workload_compare [threads] [cycles] [ops_per_cycle] [min_size] [max_size]

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
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    Hz6StatsSnapshot hz6_stats_after;
#endif
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
    return hz_bench_alloc(size);
}

static inline void bench_free(void* ptr) {
    hz_bench_free(ptr);
}

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
static void print_hz6_redis_stats(RedisOp op,
                                  const ThreadArg* args,
                                  uint32_t threads) {
    Hz6StatsSnapshot total;
    memset(&total, 0, sizeof(total));

    for (uint32_t i = 0; i < threads; ++i) {
        const Hz6StatsSnapshot* s = &args[i].hz6_stats_after;
        total.source_alloc += s->source_alloc;
        total.alloc_fail += s->alloc_fail;
        total.control_plane_normal += s->control_plane_normal;
        total.control_plane_burst_supply_would_open +=
            s->control_plane_burst_supply_would_open;
        total.control_plane_close_would_start +=
            s->control_plane_close_would_start;
        total.descriptor_exhausted += s->descriptor_exhausted;
        total.route_register_fail += s->route_register_fail;
        total.source_block_exhausted += s->source_block_exhausted;
        total.source_prefill_attempt += s->source_prefill_attempt;
        total.source_prefill_filled += s->source_prefill_filled;
        total.source_prefill_fallback += s->source_prefill_fallback;
        total.source_refill_starvation += s->source_refill_starvation;
        total.source_refill_saturation += s->source_refill_saturation;
        total.source_refill_boost += s->source_refill_boost;
        total.source_refill_clamp += s->source_refill_clamp;
        total.source_admission_open += s->source_admission_open;
        total.source_admission_boosted += s->source_admission_boosted;
        total.source_admission_clamped += s->source_admission_clamped;
        total.route_lookup_probe_total += s->route_lookup_probe_total;
        if (s->route_lookup_probe_max > total.route_lookup_probe_max) {
            total.route_lookup_probe_max = s->route_lookup_probe_max;
        }
        total.route_register_probe_total += s->route_register_probe_total;
        if (s->route_register_probe_max > total.route_register_probe_max) {
            total.route_register_probe_max = s->route_register_probe_max;
        }
        total.route_register_reason_source_run_slot +=
            s->route_register_reason_source_run_slot;
        total.route_register_reason_direct_source +=
            s->route_register_reason_direct_source;
        total.route_register_reason_materialize +=
            s->route_register_reason_materialize;
        total.route_register_reason_rehome += s->route_register_reason_rehome;
        total.route_register_reason_unknown +=
            s->route_register_reason_unknown;
        total.route_unregister_reason_frontcache_overflow +=
            s->route_unregister_reason_frontcache_overflow;
        total.route_unregister_reason_cap_release +=
            s->route_unregister_reason_cap_release;
        total.route_unregister_reason_descriptorless_detach +=
            s->route_unregister_reason_descriptorless_detach;
        total.route_unregister_reason_source_slot_release +=
            s->route_unregister_reason_source_slot_release;
        total.route_unregister_reason_rehome +=
            s->route_unregister_reason_rehome;
        total.route_unregister_reason_unknown +=
            s->route_unregister_reason_unknown;
        total.route_register_used_tombstone += s->route_register_used_tombstone;
        total.route_register_full_probe_with_tombstone +=
            s->route_register_full_probe_with_tombstone;
        total.route_tombstone_current += s->route_tombstone_current;
        if (s->route_tombstone_max > total.route_tombstone_max) {
            total.route_tombstone_max = s->route_tombstone_max;
        }
        total.route_tombstone_compact_attempt +=
            s->route_tombstone_compact_attempt;
        total.route_tombstone_compact_success +=
            s->route_tombstone_compact_success;
        total.route_tombstone_compact_fail_alloc +=
            s->route_tombstone_compact_fail_alloc;
        total.route_tombstone_compact_moved +=
            s->route_tombstone_compact_moved;
        total.route_retained_overflow_attempt +=
            s->route_retained_overflow_attempt;
        total.route_retained_overflow_success +=
            s->route_retained_overflow_success;
        total.route_retained_overflow_fail += s->route_retained_overflow_fail;
        total.source_block_probe_total += s->source_block_probe_total;
        if (s->source_block_probe_max > total.source_block_probe_max) {
            total.source_block_probe_max = s->source_block_probe_max;
        }
        total.descriptor_probe_total += s->descriptor_probe_total;
        if (s->descriptor_probe_max > total.descriptor_probe_max) {
            total.descriptor_probe_max = s->descriptor_probe_max;
        }
    }

    printf("[HZ6_REDIS_STATS] pattern=%s source_alloc=%zu alloc_fail=%zu "
           "control_plane_normal=%zu control_plane_burst_supply_would_open=%zu "
           "control_plane_close_would_start=%zu descriptor_exhausted=%zu "
           "route_register_fail=%zu source_block_exhausted=%zu "
           "source_prefill_attempt=%zu source_prefill_filled=%zu "
           "source_prefill_fallback=%zu source_refill_starvation=%zu "
           "source_refill_saturation=%zu source_refill_boost=%zu "
           "source_refill_clamp=%zu source_admission_open=%zu "
           "source_admission_boosted=%zu source_admission_clamped=%zu "
           "route_lookup_probe_total=%zu route_lookup_probe_max=%zu "
           "route_register_probe_total=%zu route_register_probe_max=%zu "
           "route_register_reason_source_run_slot=%zu "
           "route_register_reason_direct_source=%zu "
           "route_register_reason_materialize=%zu "
           "route_register_reason_rehome=%zu "
           "route_register_reason_unknown=%zu "
           "route_unregister_reason_frontcache_overflow=%zu "
           "route_unregister_reason_cap_release=%zu "
           "route_unregister_reason_descriptorless_detach=%zu "
           "route_unregister_reason_source_slot_release=%zu "
           "route_unregister_reason_rehome=%zu "
           "route_unregister_reason_unknown=%zu "
           "route_register_used_tombstone=%zu "
           "route_register_full_probe_with_tombstone=%zu "
           "route_tombstone_current=%zu route_tombstone_max=%zu "
           "route_tombstone_compact_attempt=%zu "
           "route_tombstone_compact_success=%zu "
           "route_tombstone_compact_fail_alloc=%zu "
           "route_tombstone_compact_moved=%zu "
           "route_retained_overflow_attempt=%zu "
           "route_retained_overflow_success=%zu "
           "route_retained_overflow_fail=%zu "
           "source_block_probe_total=%zu source_block_probe_max=%zu "
           "descriptor_probe_total=%zu descriptor_probe_max=%zu\n",
           kOpNames[op],
           total.source_alloc,
           total.alloc_fail,
           total.control_plane_normal,
           total.control_plane_burst_supply_would_open,
           total.control_plane_close_would_start,
           total.descriptor_exhausted,
           total.route_register_fail,
           total.source_block_exhausted,
           total.source_prefill_attempt,
           total.source_prefill_filled,
           total.source_prefill_fallback,
           total.source_refill_starvation,
           total.source_refill_saturation,
           total.source_refill_boost,
           total.source_refill_clamp,
           total.source_admission_open,
           total.source_admission_boosted,
           total.source_admission_clamped,
           total.route_lookup_probe_total,
           total.route_lookup_probe_max,
           total.route_register_probe_total,
           total.route_register_probe_max,
           total.route_register_reason_source_run_slot,
           total.route_register_reason_direct_source,
           total.route_register_reason_materialize,
           total.route_register_reason_rehome,
           total.route_register_reason_unknown,
           total.route_unregister_reason_frontcache_overflow,
           total.route_unregister_reason_cap_release,
           total.route_unregister_reason_descriptorless_detach,
           total.route_unregister_reason_source_slot_release,
           total.route_unregister_reason_rehome,
           total.route_unregister_reason_unknown,
           total.route_register_used_tombstone,
           total.route_register_full_probe_with_tombstone,
           total.route_tombstone_current,
           total.route_tombstone_max,
           total.route_tombstone_compact_attempt,
           total.route_tombstone_compact_success,
           total.route_tombstone_compact_fail_alloc,
           total.route_tombstone_compact_moved,
           total.route_retained_overflow_attempt,
           total.route_retained_overflow_success,
           total.route_retained_overflow_fail,
           total.source_block_probe_total,
           total.source_block_probe_max,
           total.descriptor_probe_total,
           total.descriptor_probe_max);
}
#endif

static char* alloc_string(uint32_t* seed, size_t min_size, size_t max_size, uint32_t ordinal) {
    size_t size = pick_size(rng_next(seed), min_size, max_size);
    char* ptr = (char*)bench_alloc(size);
    if (!ptr) {
        hz_bench_dump_stats(stderr, "redis_alloc_string_fail");
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

    hz_bench_allocator_thread_setup();

    pool = (char**)calloc(g_pool_capacity, sizeof(char*));
    if (!pool) {
        hz_bench_allocator_thread_teardown();
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
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    if (hz_bench_tls_hz6_initialized) {
        arg->hz6_stats_after = hz6_stats_snapshot(&hz_bench_tls_hz6_allocator);
    }
#endif
    free(pool);
    hz_bench_allocator_thread_teardown();
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
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    print_hz6_redis_stats(op, args, threads);
#endif
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
