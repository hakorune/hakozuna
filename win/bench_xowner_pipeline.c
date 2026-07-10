// Diagnostic-only producer/consumer pipeline for cross-owner free attribution.
// Each producer allocates and a different consumer frees every queued object.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "bench_modern_allocator_adapter.h"

typedef struct PipelineRing {
    void** slots;
    uint32_t capacity;
    volatile LONG head;
    volatile LONG tail;
} PipelineRing;

typedef struct PipelineThread {
    uint32_t id;
    uint32_t min_size;
    uint32_t max_size;
    uint32_t seed;
    PipelineRing* ring;
    uint64_t allocs;
    uint64_t frees;
    uint64_t push_waits;
    uint64_t pop_waits;
    uint64_t cleanup_local;
} PipelineThread;

static volatile LONG g_ready;
static volatile LONG g_start;
static volatile LONG g_stop;
static volatile LONG g_producers_done;
static uint32_t g_producer_count;
static uint32_t g_consumer_count;

static uint32_t parse_u32(const char* text, uint32_t fallback) {
    char* end = NULL;
    unsigned long value;
    if (!text) return fallback;
    value = strtoul(text, &end, 10);
    if (end == text || (end && *end != '\0') || value > 0xFFFFFFFFul) {
        return fallback;
    }
    return (uint32_t)value;
}

static uint32_t rng_next(uint32_t* state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static uint64_t now_ns(void) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return ((uint64_t)counter.QuadPart * 1000000000ull) /
           (uint64_t)frequency.QuadPart;
}

static int ring_push(PipelineRing* ring, void* ptr) {
    LONG tail = ring->tail;
    LONG head = ring->head;
    if ((uint32_t)(tail - head) >= ring->capacity) return 0;
    ring->slots[(uint32_t)tail % ring->capacity] = ptr;
    MemoryBarrier();
    ring->tail = tail + 1;
    return 1;
}

static void* ring_pop(PipelineRing* ring) {
    LONG head = ring->head;
    LONG tail = ring->tail;
    void* ptr;
    if (head == tail) return NULL;
    ptr = ring->slots[(uint32_t)head % ring->capacity];
    MemoryBarrier();
    ring->head = head + 1;
    return ptr;
}

static unsigned __stdcall producer_main(void* arg) {
    PipelineThread* state = (PipelineThread*)arg;
    InterlockedIncrement(&g_ready);
    while (InterlockedCompareExchange(&g_start, 0, 0) == 0) {
        SwitchToThread();
    }
    while (InterlockedCompareExchange(&g_stop, 0, 0) == 0) {
        uint32_t span = state->max_size - state->min_size + 1u;
        size_t size = state->min_size + (rng_next(&state->seed) % span);
        void* ptr = hz_bench_alloc(size);
        if (!ptr) continue;
        state->allocs += 1u;
        while (!ring_push(state->ring, ptr)) {
            state->push_waits += 1u;
            if (InterlockedCompareExchange(&g_stop, 0, 0) != 0) {
                hz_bench_free(ptr);
                state->cleanup_local += 1u;
                ptr = NULL;
                break;
            }
            SwitchToThread();
        }
    }
    InterlockedIncrement(&g_producers_done);
    return 0;
}

static unsigned __stdcall consumer_main(void* arg) {
    PipelineThread* state = (PipelineThread*)arg;
    InterlockedIncrement(&g_ready);
    while (InterlockedCompareExchange(&g_start, 0, 0) == 0) {
        SwitchToThread();
    }
    for (;;) {
        void* ptr = ring_pop(state->ring);
        if (ptr) {
            hz_bench_free(ptr);
            state->frees += 1u;
            continue;
        }
        if (InterlockedCompareExchange(&g_stop, 0, 0) != 0 &&
            InterlockedCompareExchange(&g_producers_done, 0, 0) ==
                (LONG)g_producer_count) {
            break;
        }
        state->pop_waits += 1u;
        SwitchToThread();
    }
    return 0;
}

int main(int argc, char** argv) {
    uint32_t runtime_sec = (argc > 1) ? parse_u32(argv[1], 5u) : 5u;
    uint32_t producers = (argc > 2) ? parse_u32(argv[2], 4u) : 4u;
    uint32_t consumers = (argc > 3) ? parse_u32(argv[3], 4u) : 4u;
    uint32_t ring_capacity = (argc > 4) ? parse_u32(argv[4], 4096u) : 4096u;
    uint32_t min_size = (argc > 5) ? parse_u32(argv[5], 8u) : 8u;
    uint32_t max_size = (argc > 6) ? parse_u32(argv[6], 1024u) : 1024u;
    HANDLE* handles = NULL;
    PipelineRing* rings = NULL;
    PipelineThread* states = NULL;
    uint32_t total_threads;
    uint32_t i;
    uint64_t start_ns;
    uint64_t elapsed_ns;
    uint64_t total_allocs = 0u;
    uint64_t total_frees = 0u;
    uint64_t total_waits = 0u;
    uint64_t total_cleanup = 0u;
    if (producers == 0u || consumers == 0u || producers != consumers ||
        ring_capacity < 2u || min_size == 0u || max_size < min_size) {
        fprintf(stderr, "usage: %s <seconds> <producers> <consumers> <ring> <min> <max>\n", argv[0]);
        return 2;
    }
    g_producer_count = producers;
    g_consumer_count = consumers;
    total_threads = producers + consumers;
    handles = (HANDLE*)calloc(total_threads, sizeof(HANDLE));
    rings = (PipelineRing*)calloc(producers, sizeof(PipelineRing));
    states = (PipelineThread*)calloc(total_threads, sizeof(PipelineThread));
    if (!handles || !rings || !states) return 3;
    for (i = 0u; i < producers; ++i) {
        rings[i].slots = (void**)calloc(ring_capacity, sizeof(void*));
        rings[i].capacity = ring_capacity;
        if (!rings[i].slots) return 3;
        states[i].id = i;
        states[i].min_size = min_size;
        states[i].max_size = max_size;
        states[i].seed = 0x9e3779b9u ^ (i * 2654435761u);
        states[i].ring = &rings[i];
        handles[i] = (HANDLE)_beginthreadex(NULL, 0, producer_main, &states[i], 0, NULL);
        states[producers + i].id = i;
        states[producers + i].min_size = min_size;
        states[producers + i].max_size = max_size;
        states[producers + i].seed = 0x243f6a88u ^ (i * 2246822519u);
        states[producers + i].ring = &rings[i];
        handles[producers + i] = (HANDLE)_beginthreadex(NULL, 0, consumer_main,
                                                          &states[producers + i], 0, NULL);
        if (!handles[i] || !handles[producers + i]) return 4;
    }
    while (InterlockedCompareExchange(&g_ready, 0, 0) < (LONG)total_threads) {
        SwitchToThread();
    }
    start_ns = now_ns();
    InterlockedExchange(&g_start, 1);
    Sleep(runtime_sec * 1000u);
    InterlockedExchange(&g_stop, 1);
    WaitForMultipleObjects(total_threads, handles, TRUE, INFINITE);
    elapsed_ns = now_ns() - start_ns;
    for (i = 0u; i < total_threads; ++i) {
        total_allocs += states[i].allocs;
        total_frees += states[i].frees;
        total_waits += states[i].push_waits + states[i].pop_waits;
        total_cleanup += states[i].cleanup_local;
        CloseHandle(handles[i]);
    }
    printf("[XOWNER_PIPELINE] producers=%u consumers=%u ring=%u allocs=%llu "
           "cross_owner_frees=%llu local_cleanup=%llu waits=%llu "
           "sharing_factor=2.00 cross_owner_rate=100.0 ops/s=%.2f elapsed=%.3f\n",
           producers, consumers, ring_capacity,
           (unsigned long long)total_allocs,
           (unsigned long long)total_frees,
           (unsigned long long)total_cleanup,
           (unsigned long long)total_waits,
           (double)total_frees * 1000000000.0 / (double)elapsed_ns,
           (double)elapsed_ns / 1000000000.0);
    for (i = 0u; i < producers; ++i) free(rings[i].slots);
    free(states);
    free(rings);
    free(handles);
    return 0;
}
