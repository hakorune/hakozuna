// Windows-native MT remote random_mixed compare bench.
// Usage: bench_random_mixed_mt_remote_compare [threads] [iters] [working_set] [min_size] [max_size] [remote_pct] [ring_slots]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

typedef struct RemoteRing {
    void** items;
    uint32_t capacity;
    volatile LONG head;
    volatile LONG tail;
} RemoteRing;

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
    RemoteRing* send_ring;
    RemoteRing* recv_ring;
    uint64_t ops;
    uint64_t allocs;
    uint64_t local_frees;
    uint64_t remote_sends;
    uint64_t remote_received_frees;
    uint64_t remote_fallback_frees;
} ThreadState;

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

static int ring_init(RemoteRing* ring, uint32_t capacity) {
    ring->items = (void**)calloc(capacity, sizeof(void*));
    if (!ring->items) {
        return 0;
    }
    ring->capacity = capacity;
    ring->head = 0;
    ring->tail = 0;
    return 1;
}

static void ring_destroy(RemoteRing* ring) {
    free(ring->items);
    ring->items = NULL;
    ring->capacity = 0;
    ring->head = 0;
    ring->tail = 0;
}

static int ring_push(RemoteRing* ring, void* ptr) {
    LONG tail = ring->tail;
    LONG head = ring->head;
    if ((uint32_t)(tail - head) >= ring->capacity) {
        return 0;
    }
    ring->items[(uint32_t)tail % ring->capacity] = ptr;
    MemoryBarrier();
    ring->tail = tail + 1;
    return 1;
}

static void* ring_pop(RemoteRing* ring) {
    LONG head = ring->head;
    LONG tail = ring->tail;
    void* ptr;
    if (head == tail) {
        return NULL;
    }
    ptr = ring->items[(uint32_t)head % ring->capacity];
    MemoryBarrier();
    ring->head = head + 1;
    return ptr;
}

static void drain_remote(ThreadState* ts) {
    for (;;) {
        void* ptr = ring_pop(ts->recv_ring);
        if (!ptr) {
            break;
        }
        bench_free(ptr);
        ts->remote_received_frees++;
        ts->ops++;
    }
}

static unsigned __stdcall bench_thread(void* arg) {
    ThreadState* ts = (ThreadState*)arg;
    uint32_t seed = ts->seed;
    uint32_t i;

    for (i = 0; i < ts->iters; ++i) {
        uint32_t r;
        uint32_t idx;
        size_t size;
        void* ptr;

        drain_remote(ts);

        r = rng_next(&seed);
        idx = r % ts->ws;
        size = pick_size(rng_next(&seed), ts->min_size, ts->max_size);
        ptr = ts->slots[idx];

        if (ptr) {
            if (ts->threads > 1 && (rng_next(&seed) % 100u) < ts->remote_pct) {
                if (ring_push(ts->send_ring, ptr)) {
                    ts->remote_sends++;
                } else {
                    bench_free(ptr);
                    ts->remote_fallback_frees++;
                }
            } else {
                bench_free(ptr);
                ts->local_frees++;
            }
            ts->slots[idx] = NULL;
            ts->ops++;
            continue;
        }

        ptr = bench_alloc(size);
        if (!ptr) {
            continue;
        }
        ts->slots[idx] = ptr;
        ts->allocs++;
        ts->ops++;
    }

    drain_remote(ts);
    return 0;
}

int main(int argc, char** argv) {
    uint32_t threads = (argc > 1) ? parse_u32(argv[1], 16u) : 16u;
    uint32_t iters = (argc > 2) ? parse_u32(argv[2], 2000000u) : 2000000u;
    uint32_t ws = (argc > 3) ? parse_u32(argv[3], 400u) : 400u;
    uint32_t min_size = (argc > 4) ? parse_u32(argv[4], 16u) : 16u;
    uint32_t max_size = (argc > 5) ? parse_u32(argv[5], 2048u) : 2048u;
    uint32_t remote_pct = (argc > 6) ? parse_u32(argv[6], 90u) : 90u;
    uint32_t ring_slots = (argc > 7) ? parse_u32(argv[7], 65536u) : 65536u;

    ThreadState* states = NULL;
    HANDLE* handles = NULL;
    RemoteRing* rings = NULL;
    uint64_t start_ns;
    uint64_t end_ns;
    double dt;
    uint64_t total_ops = 0;
    uint64_t total_remote_sends = 0;
    uint64_t total_remote_fallbacks = 0;
    uint64_t total_remote_received = 0;
    uint64_t total_local_frees = 0;
    uint32_t i;
    int rc = 1;

    if (threads == 0 || iters == 0 || ws == 0 || min_size == 0 || max_size < min_size || remote_pct > 100u || ring_slots == 0) {
        fprintf(stderr, "bench_random_mixed_mt_remote_compare: invalid args\n");
        fprintf(stderr, "usage: %s <threads> <iters> <ws> <min> <max> <remote_pct> [ring_slots]\n", argv[0]);
        return 2;
    }

    printf("[BENCH_ARGS] threads=%u iters=%u ws=%u min=%u max=%u remote_pct=%u ring_slots=%u\n",
           threads, iters, ws, min_size, max_size, remote_pct, ring_slots);

    states = (ThreadState*)calloc(threads, sizeof(ThreadState));
    handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    rings = (RemoteRing*)calloc(threads, sizeof(RemoteRing));
    if (!states || !handles || !rings) {
        fprintf(stderr, "bench_random_mixed_mt_remote_compare: state allocation failed\n");
        goto cleanup;
    }

    for (i = 0; i < threads; ++i) {
        if (!ring_init(&rings[i], ring_slots)) {
            fprintf(stderr, "bench_random_mixed_mt_remote_compare: ring allocation failed at thread=%u\n", i);
            goto cleanup;
        }
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
        states[i].send_ring = &rings[i];
        states[i].recv_ring = &rings[(i + threads - 1u) % threads];
        if (!states[i].slots) {
            fprintf(stderr, "bench_random_mixed_mt_remote_compare: slots allocation failed at thread=%u\n", i);
            goto cleanup;
        }
    }

    start_ns = now_ns();
    for (i = 0; i < threads; ++i) {
        uintptr_t h = _beginthreadex(NULL, 0, bench_thread, &states[i], 0, NULL);
        if (h == 0) {
            fprintf(stderr, "bench_random_mixed_mt_remote_compare: thread start failed at thread=%u\n", i);
            goto cleanup;
        }
        handles[i] = (HANDLE)h;
    }

    WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    end_ns = now_ns();

    for (i = 0; i < threads; ++i) {
        total_ops += states[i].ops;
        total_remote_sends += states[i].remote_sends;
        total_remote_fallbacks += states[i].remote_fallback_frees;
        total_remote_received += states[i].remote_received_frees;
        total_local_frees += states[i].local_frees;
    }

    for (i = 0; i < threads; ++i) {
        drain_remote(&states[i]);
    }

    for (i = 0; i < threads; ++i) {
        uint32_t j;
        for (j = 0; j < ws; ++j) {
            if (states[i].slots[j]) {
                bench_free(states[i].slots[j]);
                states[i].slots[j] = NULL;
            }
        }
    }

    dt = (double)(end_ns - start_ns) / 1e9;
    printf("bench_random_mixed_mt_remote: threads=%u ops=%llu time=%.6f ops/s=%.2f\n",
           threads, (unsigned long long)total_ops, dt, (dt > 0.0) ? ((double)total_ops / dt) : 0.0);
    printf("[EFFECTIVE_REMOTE] target=%u actual=%.2f fallback_rate=%.2f recv_frees=%llu\n",
           remote_pct,
           (total_remote_sends + total_local_frees + total_remote_fallbacks > 0) ? (100.0 * (double)total_remote_sends / (double)(total_remote_sends + total_local_frees + total_remote_fallbacks)) : 0.0,
           (total_remote_sends + total_remote_fallbacks > 0) ? (100.0 * (double)total_remote_fallbacks / (double)(total_remote_sends + total_remote_fallbacks)) : 0.0,
           (unsigned long long)total_remote_received);
    rc = 0;

cleanup:
    if (handles) {
        for (i = 0; i < threads; ++i) {
            if (handles[i]) {
                CloseHandle(handles[i]);
            }
        }
    }
    if (states) {
        for (i = 0; i < threads; ++i) {
            free(states[i].slots);
        }
    }
    if (rings) {
        for (i = 0; i < threads; ++i) {
            ring_destroy(&rings[i]);
        }
    }
    free(rings);
    free(handles);
    free(states);
    return rc;
}
