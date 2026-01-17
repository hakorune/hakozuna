// Mixed working-set bench (Windows-friendly)
// Usage: bench_mixed_ws [threads] [iters_per_thread] [working_set] [min_size] [max_size]

#include "hz3.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HZ3_BENCH_USE_MIMALLOC)
#include <mimalloc.h>
#elif defined(HZ3_BENCH_USE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#endif

#ifndef HZ3_BENCH_USE_CRT
#define HZ3_BENCH_USE_CRT 0
#endif

static inline void* bench_alloc(size_t size) {
#if HZ3_BENCH_USE_CRT
    return malloc(size);
#elif defined(HZ3_BENCH_USE_MIMALLOC)
    return mi_malloc(size);
#elif defined(HZ3_BENCH_USE_TCMALLOC)
    return tc_malloc(size);
#else
    return hz3_malloc(size);
#endif
}

static inline void* bench_realloc(void* ptr, size_t size) {
#if HZ3_BENCH_USE_CRT
    return realloc(ptr, size);
#elif defined(HZ3_BENCH_USE_MIMALLOC)
    return mi_realloc(ptr, size);
#elif defined(HZ3_BENCH_USE_TCMALLOC)
    return tc_realloc(ptr, size);
#else
    return hz3_realloc(ptr, size);
#endif
}

static inline void bench_free(void* ptr) {
#if HZ3_BENCH_USE_CRT
    free(ptr);
#elif defined(HZ3_BENCH_USE_MIMALLOC)
    mi_free(ptr);
#elif defined(HZ3_BENCH_USE_TCMALLOC)
    tc_free(ptr);
#else
    hz3_free(ptr);
#endif
}

typedef struct {
    uint32_t seed;
    size_t iters;
    size_t ws;
    size_t min_size;
    size_t max_size;
} ThreadArg;

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
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
}

static unsigned __stdcall bench_thread(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;
    uint32_t seed = ta->seed;
    size_t ws = ta->ws ? ta->ws : 1;
    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        return 0;
    }

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
            bench_free(slots[idx]);
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        void* p = bench_alloc(size);
        if (!p) {
            continue;
        }
        if ((i & 0x3fu) == 0) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(p, new_size);
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
            bench_free(slots[i]);
        }
    }
    free(slots);
    return 0;
}
#else
#include <pthread.h>
#include <time.h>

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

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
            bench_free(slots[idx]);
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        void* p = bench_alloc(size);
        if (!p) {
            continue;
        }
        if ((i & 0x3fu) == 0) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(p, new_size);
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
            bench_free(slots[i]);
        }
    }
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

    printf("threads=%zu iters=%zu ws=%zu size=%zu..%zu time=%.3f ops/s=%.3f\n",
           threads, iters, ws, min_size, max_size, sec, ops_sec);

    free(args);
    return 0;
}
