// Windows HZ3 aligned-policy probe.
//
// Exercises posix_memalign(size, alignment) and validates the returned
// alignment contract. The companion PowerShell runner toggles
// HZ3_PAGE_MEDIUM_ALIGNED to compare the default large-aligned path with the
// opt-in page-medium research lane.

#if !defined(_WIN32)
#error "bench_hz3_aligned_policy_probe is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#include <process.h>

#include "hz3.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int posix_memalign(void** memptr, size_t alignment, size_t size);

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
    uint32_t working_set;
    uint32_t remote_pct;
    size_t size;
    size_t alignment;
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
    uint64_t checksum;
    volatile LONG failed;
} ThreadState;

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

static size_t parse_size(const char* s, size_t fallback) {
    char* end = NULL;
    unsigned long long v;
    if (!s || !*s) {
        return fallback;
    }
    v = strtoull(s, &end, 10);
    if (end == s || (end && *end != '\0')) {
        return fallback;
    }
    return (size_t)v;
}

static int is_pow2(size_t x) {
    return x != 0 && (x & (x - 1u)) == 0;
}

static void* os_calloc(size_t count, size_t size) {
    if (count != 0 && size > (SIZE_MAX / count)) {
        return NULL;
    }
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * size);
}

static void os_free(void* ptr) {
    if (ptr) {
        HeapFree(GetProcessHeap(), 0, ptr);
    }
}

static uint32_t rng_next(uint32_t* s) {
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

static int probe_alloc_one(size_t size, size_t alignment, size_t iter, void** out, uint64_t* checksum) {
    void* ptr = NULL;
    int rc = posix_memalign(&ptr, alignment, size);
    if (rc != 0 || !ptr) {
        fprintf(stderr, "[ALLOC_FAIL] iter=%zu rc=%d size=%zu align=%zu\n",
                iter, rc, size, alignment);
        return 0;
    }
    if (((uintptr_t)ptr & (uintptr_t)(alignment - 1u)) != 0) {
        fprintf(stderr, "[MISALIGNED] iter=%zu ptr=%p size=%zu align=%zu\n",
                iter, ptr, size, alignment);
        hz3_free(ptr);
        return 0;
    }

    ((volatile unsigned char*)ptr)[0] = (unsigned char)iter;
    ((volatile unsigned char*)ptr)[size - 1u] = (unsigned char)(iter >> 8);
    *checksum += (uintptr_t)ptr & 0xffffu;
    *out = ptr;
    return 1;
}

static int ring_init(RemoteRing* ring, uint32_t capacity) {
    ring->items = (void**)os_calloc(capacity, sizeof(void*));
    if (!ring->items) {
        return 0;
    }
    ring->capacity = capacity;
    ring->head = 0;
    ring->tail = 0;
    return 1;
}

static void ring_destroy(RemoteRing* ring) {
    os_free(ring->items);
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
        hz3_free(ptr);
        ts->remote_received_frees++;
        ts->ops++;
    }
}

static unsigned __stdcall remote_thread(void* arg) {
    ThreadState* ts = (ThreadState*)arg;
    uint32_t seed = ts->seed;
    uint32_t i;

    for (i = 0; i < ts->iters; i++) {
        uint32_t r;
        uint32_t idx;
        void* ptr;

        drain_remote(ts);

        r = rng_next(&seed);
        idx = r % ts->working_set;
        ptr = ts->slots[idx];
        if (ptr) {
            if (ts->threads > 1 && (rng_next(&seed) % 100u) < ts->remote_pct) {
                if (ring_push(ts->send_ring, ptr)) {
                    ts->remote_sends++;
                } else {
                    hz3_free(ptr);
                    ts->remote_fallback_frees++;
                }
            } else {
                hz3_free(ptr);
                ts->local_frees++;
            }
            ts->slots[idx] = NULL;
            ts->ops++;
            continue;
        }

        ptr = NULL;
        if (!probe_alloc_one(ts->size, ts->alignment, i, &ptr, &ts->checksum)) {
            ts->failed = 1;
            return 1;
        }
        ts->slots[idx] = ptr;
        ts->allocs++;
        ts->ops++;
    }

    drain_remote(ts);
    return 0;
}

static int run_single(size_t size, size_t alignment, size_t iters, uint64_t* out_ops, uint64_t* out_checksum) {
    size_t i;
    for (i = 0; i < iters; i++) {
        void* ptr = NULL;
        if (!probe_alloc_one(size, alignment, i, &ptr, out_checksum)) {
            return 0;
        }
        hz3_free(ptr);
    }
    *out_ops = (uint64_t)iters;
    return 1;
}

static int run_remote(size_t size,
                      size_t alignment,
                      uint32_t threads,
                      uint32_t iters,
                      uint32_t working_set,
                      uint32_t remote_pct,
                      uint32_t ring_slots,
                      uint64_t* out_ops,
                      uint64_t* out_remote_sends,
                      uint64_t* out_remote_fallbacks,
                      uint64_t* out_local_frees,
                      uint64_t* out_remote_received,
                      uint64_t* out_checksum) {
    ThreadState* states = NULL;
    HANDLE* handles = NULL;
    RemoteRing* rings = NULL;
    uint32_t i;
    int ok = 0;

    states = (ThreadState*)os_calloc(threads, sizeof(ThreadState));
    handles = (HANDLE*)os_calloc(threads, sizeof(HANDLE));
    rings = (RemoteRing*)os_calloc(threads, sizeof(RemoteRing));
    if (!states || !handles || !rings) {
        goto cleanup;
    }

    for (i = 0; i < threads; i++) {
        if (!ring_init(&rings[i], ring_slots)) {
            goto cleanup;
        }
    }

    for (i = 0; i < threads; i++) {
        states[i].id = i;
        states[i].threads = threads;
        states[i].iters = iters;
        states[i].working_set = working_set;
        states[i].remote_pct = remote_pct;
        states[i].size = size;
        states[i].alignment = alignment;
        states[i].seed = 0x12345678u + (i * 977u);
        states[i].slots = (void**)os_calloc(working_set, sizeof(void*));
        states[i].send_ring = &rings[i];
        states[i].recv_ring = &rings[(i + threads - 1u) % threads];
        if (!states[i].slots) {
            goto cleanup;
        }
    }

    for (i = 0; i < threads; i++) {
        uintptr_t h = _beginthreadex(NULL, 0, remote_thread, &states[i], 0, NULL);
        if (h == 0) {
            goto cleanup;
        }
        handles[i] = (HANDLE)h;
    }

    WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);

    for (i = 0; i < threads; i++) {
        uint32_t j;
        if (states[i].failed) {
            goto cleanup;
        }
        drain_remote(&states[i]);
        for (j = 0; j < working_set; j++) {
            if (states[i].slots[j]) {
                hz3_free(states[i].slots[j]);
                states[i].slots[j] = NULL;
            }
        }
        *out_ops += states[i].ops;
        *out_remote_sends += states[i].remote_sends;
        *out_remote_fallbacks += states[i].remote_fallback_frees;
        *out_local_frees += states[i].local_frees;
        *out_remote_received += states[i].remote_received_frees;
        *out_checksum += states[i].checksum;
    }

    ok = 1;

cleanup:
    if (handles) {
        for (i = 0; i < threads; i++) {
            if (handles[i]) {
                CloseHandle(handles[i]);
            }
        }
    }
    if (states) {
        for (i = 0; i < threads; i++) {
            uint32_t j;
            if (states[i].slots) {
                for (j = 0; j < working_set; j++) {
                    if (states[i].slots[j]) {
                        hz3_free(states[i].slots[j]);
                    }
                }
            }
            os_free(states[i].slots);
        }
    }
    if (rings) {
        for (i = 0; i < threads; i++) {
            void* ptr;
            while ((ptr = ring_pop(&rings[i])) != NULL) {
                hz3_free(ptr);
            }
            ring_destroy(&rings[i]);
        }
    }
    os_free(rings);
    os_free(handles);
    os_free(states);
    return ok;
}

int main(int argc, char** argv) {
    size_t size = (argc > 1) ? parse_size(argv[1], 4096u) : 4096u;
    size_t alignment = (argc > 2) ? parse_size(argv[2], 4096u) : 4096u;
    size_t iters = (argc > 3) ? parse_size(argv[3], 100000u) : 100000u;
    uint32_t threads = (uint32_t)((argc > 4) ? parse_size(argv[4], 1u) : 1u);
    uint32_t remote_pct = (uint32_t)((argc > 5) ? parse_size(argv[5], 0u) : 0u);
    uint32_t working_set = (uint32_t)((argc > 6) ? parse_size(argv[6], 256u) : 256u);
    uint32_t ring_slots = (uint32_t)((argc > 7) ? parse_size(argv[7], 65536u) : 65536u);
    uint64_t start_ns;
    uint64_t end_ns;
    double seconds;
    double ops_per_sec;
    PROCESS_MEMORY_COUNTERS_EX pmc;
    size_t peak_rss_kb = 0;
    size_t rss_kb = 0;
    size_t private_kb = 0;
    uint64_t checksum = 0;
    uint64_t ops = 0;
    uint64_t remote_sends = 0;
    uint64_t remote_fallbacks = 0;
    uint64_t local_frees = 0;
    uint64_t remote_received = 0;
    double actual_remote_pct = 0.0;
    double fallback_pct = 0.0;

    if (size == 0 || iters == 0 || threads == 0 || threads > 64 ||
        working_set == 0 || ring_slots == 0 || remote_pct > 100 ||
        !is_pow2(alignment) || alignment < sizeof(void*)) {
        fprintf(stderr,
                "usage: %s <size> <alignment> <iters> [threads] [remote_pct] [working_set] [ring_slots]\n"
                "size>0, iters>0, threads=1..64, remote_pct=0..100, alignment power-of-two and >= sizeof(void*)\n",
                argv[0]);
        return 2;
    }

    start_ns = now_ns();
    if (threads == 1 && remote_pct == 0) {
        if (!run_single(size, alignment, iters, &ops, &checksum)) {
            return 3;
        }
    } else {
        if (!run_remote(size,
                        alignment,
                        threads,
                        (uint32_t)iters,
                        working_set,
                        remote_pct,
                        ring_slots,
                        &ops,
                        &remote_sends,
                        &remote_fallbacks,
                        &local_frees,
                        &remote_received,
                        &checksum)) {
            return 3;
        }
    }
    end_ns = now_ns();

    seconds = (double)(end_ns - start_ns) / 1000000000.0;
    ops_per_sec = seconds > 0.0 ? ((double)ops / seconds) : 0.0;
    if (remote_sends + local_frees + remote_fallbacks > 0) {
        actual_remote_pct = 100.0 * (double)remote_sends /
                            (double)(remote_sends + local_frees + remote_fallbacks);
    }
    if (remote_sends + remote_fallbacks > 0) {
        fallback_pct = 100.0 * (double)remote_fallbacks /
                       (double)(remote_sends + remote_fallbacks);
    }
    memset(&pmc, 0, sizeof(pmc));
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        peak_rss_kb = (size_t)(pmc.PeakWorkingSetSize / 1024u);
        rss_kb = (size_t)(pmc.WorkingSetSize / 1024u);
        private_kb = (size_t)(pmc.PrivateUsage / 1024u);
    }

    printf("hz3_aligned_policy_probe: size=%zu align=%zu threads=%u iters=%zu ws=%u remote_pct=%u ops=%llu time=%.6f ops/s=%.2f actual_remote=%.2f fallback_rate=%.2f recv_frees=%llu peak_rss_kb=%zu rss_kb=%zu private_kb=%zu checksum=%llu\n",
           size,
           alignment,
           threads,
           iters,
           working_set,
           remote_pct,
           (unsigned long long)ops,
           seconds,
           ops_per_sec,
           actual_remote_pct,
           fallback_pct,
           (unsigned long long)remote_received,
           peak_rss_kb,
           rss_kb,
           private_kb,
           (unsigned long long)checksum);
    return 0;
}
