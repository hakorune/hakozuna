// Windows-native SSOT random_mixed compare bench.
// Usage: bench_random_mixed_compare [iters] [working_set] [min_size] [max_size] [seed]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench_modern_allocator_adapter.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>

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

static inline void* bench_alloc(size_t size) {
    return hz_bench_alloc(size);
}

static inline void bench_free(void* ptr) {
    hz_bench_free(ptr);
}

int main(int argc, char** argv) {
    uint32_t iters = (argc > 1) ? parse_u32(argv[1], 20000000u) : 20000000u;
    uint32_t ws = (argc > 2) ? parse_u32(argv[2], 400u) : 400u;
    uint32_t min_size = (argc > 3) ? parse_u32(argv[3], 16u) : 16u;
    uint32_t max_size = (argc > 4) ? parse_u32(argv[4], 2048u) : 2048u;
    uint32_t seed = (argc > 5) ? parse_u32(argv[5], 0x12345678u) : 0x12345678u;

    if (iters == 0 || ws == 0 || min_size == 0 || max_size == 0 || min_size > max_size) {
        fprintf(stderr, "bench_random_mixed_compare: invalid args\n");
        fprintf(stderr, "usage: %s <iters> <ws> <min> <max> [seed]\n", argv[0]);
        return 2;
    }

    printf("[BENCH_ARGS] iters=%u ws=%u min=%u max=%u seed=%u\n",
           iters, ws, min_size, max_size, seed);

    hz_bench_allocator_thread_setup();

    uint64_t ops = 0;
    uint64_t t0 = now_ns();
    int rc = 0;

    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        fprintf(stderr, "bench_random_mixed_compare: slots alloc failed\n");
        hz_bench_allocator_thread_teardown();
        return 1;
    }

    for (uint32_t i = 0; i < iters; i++) {
        uint32_t r = rng_next(&seed);
        uint32_t idx = r % ws;
        size_t sz = min_size + (r % (max_size - min_size + 1u));

        if (slots[idx]) {
            bench_free(slots[idx]);
            slots[idx] = NULL;
            ops++;
        } else {
            void* p = bench_alloc(sz);
            if (!p) {
                fprintf(stderr, "bench_random_mixed_compare: alloc failed at iter=%u size=%zu\n", i, sz);
                hz_bench_dump_stats(stderr, "random_mixed_alloc_fail");
                rc = 1;
                break;
            }
            slots[idx] = p;
            ops++;
        }
    }

    for (uint32_t i = 0; i < ws; i++) {
        if (slots[i]) {
            bench_free(slots[i]);
            slots[i] = NULL;
            ops++;
        }
    }

    uint64_t t1 = now_ns();
    double dt = (double)(t1 - t0) / 1e9;
    double ops_s = (dt > 0.0) ? ((double)ops / dt) : 0.0;
    size_t peak_kb = peak_working_set_kb();

    printf("bench_random_mixed_compare: ops=%llu time=%.6f ops/s=%.2f\n",
           (unsigned long long)ops, dt, ops_s);
    printf("[RSS] peak_kb=%zu\n", peak_kb);
    hz_bench_dump_stats(stdout, "random_mixed_final");

    free(slots);
    hz_bench_allocator_thread_teardown();
    return rc;
}
