// Windows-friendly allocator comparison bench.
// Usage: bench_allocator_compare [threads] [iters_per_thread] [working_set] [min_size] [max_size]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HZ_BENCH_USE_HZ3)
#include "hz3.h"
#elif defined(HZ_BENCH_USE_HZ4)
#include "hz4_win_api.h"
#elif defined(HZ_BENCH_USE_HZ6)
#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_profiles.h"
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
#include "hz5_policy.h"
#elif defined(HZ_BENCH_USE_HZ11)
#include "hz11.h"
#elif defined(HZ_BENCH_USE_MIMALLOC)
#include <mimalloc.h>
#elif defined(HZ_BENCH_USE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#include <process.h>
#else
#include <pthread.h>
#include <sys/resource.h>
#include <time.h>
#endif

#if defined(HZ_BENCH_USE_HZ6)
#ifndef HZ_BENCH_HZ6_PROFILE
#define HZ_BENCH_HZ6_PROFILE HZ6_PROFILE_STRICT
#endif
#ifndef HZ_BENCH_TRACE_LAST_OP
#define HZ_BENCH_TRACE_LAST_OP 0
#endif
#endif

#if defined(HZ_BENCH_USE_HZ5_POLICY)
#ifndef HZ_BENCH_HZ5_ALIGN
#define HZ_BENCH_HZ5_ALIGN 16u
#endif
#endif

typedef struct {
    uint32_t seed;
    size_t thread_index;
    size_t iters;
    size_t ws;
    size_t min_size;
    size_t max_size;
    size_t alloc_attempts;
    size_t alloc_successes;
    size_t alloc_failures;
    size_t frees;
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
    size_t hz6_pre_free_owns_false;
    size_t hz6_duplicate_alloc_ptr;
#endif
#if defined(HZ_BENCH_USE_HZ6)
    Hz6Allocator hz6_allocator;
    Hz6StatsSnapshot hz6_stats_after;
#endif
} ThreadArg;

#if defined(_WIN32) && defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
static __declspec(thread) LONG t_last_thread = -1;
static __declspec(thread) LONG t_last_op = 0;
static __declspec(thread) LONG t_last_size = 0;
static __declspec(thread) LONG t_last_owns = -1;
static __declspec(thread) PVOID t_last_ptr = NULL;

static void bench_record_last_op(ThreadArg* ta,
                                 size_t thread,
                                 LONG op,
                                 void* ptr,
                                 size_t size) {
    t_last_thread = (LONG)thread;
    t_last_op = op;
    t_last_ptr = ptr;
    t_last_size = (LONG)size;
#if defined(HZ_BENCH_USE_HZ6)
    t_last_owns = ptr ? (LONG)hz6_owns(&ta->hz6_allocator, ptr) : -1;
#else
    (void)ta;
#endif
}

static LONG WINAPI bench_allocator_unhandled_exception_filter(
    struct _EXCEPTION_POINTERS* exception_info) {
    DWORD code = 0;
    void* address = NULL;
    ULONG_PTR info0 = 0;
    ULONG_PTR info1 = 0;
    if (exception_info && exception_info->ExceptionRecord) {
        code = exception_info->ExceptionRecord->ExceptionCode;
        address = exception_info->ExceptionRecord->ExceptionAddress;
        if (exception_info->ExceptionRecord->NumberParameters > 0) {
            info0 = exception_info->ExceptionRecord->ExceptionInformation[0];
        }
        if (exception_info->ExceptionRecord->NumberParameters > 1) {
            info1 = exception_info->ExceptionRecord->ExceptionInformation[1];
        }
    }
    PVOID last_ptr = t_last_ptr;
    MEMORY_BASIC_INFORMATION last_mbi;
    MEMORY_BASIC_INFORMATION fault_mbi;
    SIZE_T last_vq = last_ptr ? VirtualQuery(last_ptr, &last_mbi, sizeof(last_mbi)) : 0;
    SIZE_T fault_vq = info1 ? VirtualQuery((void*)info1, &fault_mbi, sizeof(fault_mbi)) : 0;
    fprintf(stderr,
            "bench_allocator_compare: unhandled exception code=0x%08lx address=%p info0=%p info1=%p last_thread=%ld last_op=%ld last_ptr=%p last_size=%ld last_owns=%ld\n",
            (unsigned long)code,
            address,
            (void*)info0,
            (void*)info1,
            t_last_thread,
            t_last_op,
            last_ptr,
            t_last_size,
            t_last_owns);
    if (last_vq) {
        fprintf(stderr,
                "bench_allocator_compare: last_ptr_vq base=%p alloc_base=%p state=0x%lx protect=0x%lx region=%zu\n",
                last_mbi.BaseAddress,
                last_mbi.AllocationBase,
                (unsigned long)last_mbi.State,
                (unsigned long)last_mbi.Protect,
                (size_t)last_mbi.RegionSize);
    }
    if (fault_vq) {
        fprintf(stderr,
                "bench_allocator_compare: fault_vq base=%p alloc_base=%p state=0x%lx protect=0x%lx region=%zu\n",
                fault_mbi.BaseAddress,
                fault_mbi.AllocationBase,
                (unsigned long)fault_mbi.State,
                (unsigned long)fault_mbi.Protect,
                (size_t)fault_mbi.RegionSize);
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static inline void bench_thread_setup(ThreadArg* ta) {
#if defined(HZ_BENCH_USE_HZ6)
    hz6_allocator_init_with_profile(&ta->hz6_allocator, HZ_BENCH_HZ6_PROFILE);
#else
    (void)ta;
#endif
}

static inline void bench_thread_teardown(ThreadArg* ta) {
#if defined(HZ_BENCH_USE_HZ6)
    hz6_allocator_destroy(&ta->hz6_allocator);
#else
    (void)ta;
#endif
}
#if defined(HZ_BENCH_USE_HZ6)
#include "bench_allocator_compare_hz6_summary.h"
#endif


static inline void* bench_alloc(ThreadArg* ta, size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    return hz3_malloc(size);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    return hz4_win_malloc(size);
#elif defined(HZ_BENCH_USE_HZ6)
    return hz6_malloc(&ta->hz6_allocator, size);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    static const Hz5PolicyHooks hooks = {0};
    return hz5_policy_alloc_aligned(size, (size_t)HZ_BENCH_HZ5_ALIGN, &hooks);
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    return hz11_malloc(size);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    return mi_malloc(size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    return tc_malloc(size);
#else
    (void)ta;
    return malloc(size);
#endif
}

static inline void* bench_realloc(ThreadArg* ta, void* ptr, size_t size) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    return hz3_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    return hz4_win_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ6)
    (void)ta;
    (void)ptr;
    (void)size;
    return NULL;
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    (void)ptr;
    (void)size;
    return NULL;
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    return hz11_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    return mi_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    return tc_realloc(ptr, size);
#else
    (void)ta;
    return realloc(ptr, size);
#endif
}

static inline void bench_free(ThreadArg* ta, void* ptr) {
#if defined(HZ_BENCH_USE_HZ3)
    (void)ta;
    hz3_free(ptr);
#elif defined(HZ_BENCH_USE_HZ4)
    (void)ta;
    hz4_win_free(ptr);
#elif defined(HZ_BENCH_USE_HZ6)
    hz6_free(&ta->hz6_allocator, ptr);
#elif defined(HZ_BENCH_USE_HZ5_POLICY)
    (void)ta;
    static const Hz5PolicyHooks hooks = {0};
    (void)hz5_policy_free(ptr, &hooks);
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    hz11_free(ptr);
#elif defined(HZ_BENCH_USE_MIMALLOC)
    (void)ta;
    mi_free(ptr);
#elif defined(HZ_BENCH_USE_TCMALLOC)
    (void)ta;
    tc_free(ptr);
#else
    (void)ta;
    free(ptr);
#endif
}

#ifndef HZ_BENCH_DISABLE_REALLOC
#define HZ_BENCH_DISABLE_REALLOC 0
#endif

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
    {
        uint64_t whole = (uint64_t)(counter.QuadPart / freq.QuadPart);
        uint64_t rem = (uint64_t)(counter.QuadPart % freq.QuadPart);
        return (whole * 1000000000ULL) + ((rem * 1000000000ULL) / (uint64_t)freq.QuadPart);
    }
}

#if defined(HZ_BENCH_USE_HZ6)
static size_t peak_working_set_kb(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return 0;
    }
    return (size_t)(pmc.PeakWorkingSetSize / 1024u);
}
#endif

static unsigned __stdcall bench_thread(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
    size_t thread_index = ta->thread_index;
#endif
    uint32_t seed = ta->seed;
    size_t ws = ta->ws ? ta->ws : 1;
    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        return 0;
    }
    bench_thread_setup(ta);

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
            bench_record_last_op(ta, thread_index, 1, slots[idx], 0);
            if (!hz6_owns(&ta->hz6_allocator, slots[idx])) {
                ++ta->hz6_pre_free_owns_false;
            }
#endif
            bench_free(ta, slots[idx]);
            ++ta->frees;
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        ++ta->alloc_attempts;
        void* p = bench_alloc(ta, size);
        if (!p) {
            ++ta->alloc_failures;
            continue;
        }
        ++ta->alloc_successes;
        if (!HZ_BENCH_DISABLE_REALLOC && ((i & 0x3fu) == 0)) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(ta, p, new_size);
            if (p2) {
                p = p2;
                size = new_size;
            }
        }
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
        if (ws <= 1024) {
            for (size_t scan = 0; scan < ws; ++scan) {
                if (scan != idx && slots[scan] == p) {
                    ++ta->hz6_duplicate_alloc_ptr;
                    break;
                }
            }
        }
        bench_record_last_op(ta, thread_index, 2, p, size);
#endif
        memset(p, 0xA5, size < 64 ? size : 64);
        slots[idx] = p;
    }

    for (size_t i = 0; i < ws; i++) {
        if (slots[i]) {
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
            bench_record_last_op(ta, thread_index, 3, slots[i], 0);
            if (!hz6_owns(&ta->hz6_allocator, slots[i])) {
                ++ta->hz6_pre_free_owns_false;
            }
#endif
            bench_free(ta, slots[i]);
            ++ta->frees;
        }
    }
#if defined(HZ_BENCH_USE_HZ6)
    ta->hz6_stats_after = hz6_stats_snapshot(&ta->hz6_allocator);
#endif
    bench_thread_teardown(ta);
    free(slots);
    return 0;
}
#else
static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#if defined(HZ_BENCH_USE_HZ6)
static size_t peak_working_set_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
    return (size_t)usage.ru_maxrss;
}
#endif

static void* bench_thread(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;
    uint32_t seed = ta->seed;
    size_t ws = ta->ws ? ta->ws : 1;
    void** slots = (void**)calloc(ws, sizeof(void*));
    if (!slots) {
        return NULL;
    }
    bench_thread_setup(ta);

    for (size_t i = 0; i < ta->iters; i++) {
        uint32_t r = lcg_next(&seed);
        size_t idx = (size_t)(r % (uint32_t)ws);
        if (slots[idx]) {
            bench_free(ta, slots[idx]);
            ++ta->frees;
            slots[idx] = NULL;
            continue;
        }

        size_t size = pick_size(r, ta->min_size, ta->max_size);
        ++ta->alloc_attempts;
        void* p = bench_alloc(ta, size);
        if (!p) {
            ++ta->alloc_failures;
            continue;
        }
        ++ta->alloc_successes;
        if (!HZ_BENCH_DISABLE_REALLOC && ((i & 0x3fu) == 0)) {
            size_t new_size = size + 16;
            void* p2 = bench_realloc(ta, p, new_size);
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
            bench_free(ta, slots[i]);
            ++ta->frees;
        }
    }
#if defined(HZ_BENCH_USE_HZ6)
    ta->hz6_stats_after = hz6_stats_snapshot(&ta->hz6_allocator);
#endif
    bench_thread_teardown(ta);
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

#if defined(_WIN32) && defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
    SetUnhandledExceptionFilter(bench_allocator_unhandled_exception_filter);
#endif

    printf("[BENCH_ARGS] threads=%zu iters=%zu ws=%zu min=%zu max=%zu\n",
           threads, iters, ws, min_size, max_size);
    fflush(stdout);

    ThreadArg* args = (ThreadArg*)calloc(threads, sizeof(ThreadArg));
    if (!args) {
        fprintf(stderr, "alloc args failed\n");
        fflush(stderr);
        return 1;
    }

    uint64_t start = now_ns();

#if defined(_WIN32)
    HANDLE* handles = (HANDLE*)calloc(threads, sizeof(HANDLE));
    if (!handles) {
        fprintf(stderr, "alloc handles failed\n");
        fflush(stderr);
        free(args);
        return 1;
    }
    for (size_t i = 0; i < threads; i++) {
        args[i].seed = (uint32_t)(1234 + i);
        args[i].thread_index = i;
        args[i].iters = iters;
        args[i].ws = ws;
        args[i].min_size = min_size;
        args[i].max_size = max_size;
        uintptr_t h = _beginthreadex(NULL, 0, bench_thread, &args[i], 0, NULL);
        if (h == 0) {
            fprintf(stderr, "bench_allocator_compare: thread start failed at thread=%zu\n", i);
            fflush(stderr);
            for (size_t j = 0; j < i; ++j) {
                if (handles[j]) {
                    WaitForSingleObject(handles[j], INFINITE);
                    CloseHandle(handles[j]);
                }
            }
            free(handles);
            free(args);
            return 1;
        }
        handles[i] = (HANDLE)h;
    }
    DWORD wait_rc = WaitForMultipleObjects((DWORD)threads, handles, TRUE, INFINITE);
    if (wait_rc == WAIT_FAILED) {
        fprintf(stderr,
                "bench_allocator_compare: WaitForMultipleObjects failed err=%lu\n",
                (unsigned long)GetLastError());
        fflush(stderr);
    }
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
        args[i].thread_index = i;
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
#if defined(HZ_BENCH_USE_HZ6)
    size_t peak_kb = peak_working_set_kb();
#endif

    size_t alloc_attempts = 0;
    size_t alloc_successes = 0;
    size_t alloc_failures = 0;
    size_t frees = 0;
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
    size_t hz6_pre_free_owns_false = 0;
    size_t hz6_duplicate_alloc_ptr = 0;
#endif
#if defined(HZ_BENCH_USE_HZ6)
    Hz6StatsSnapshot hz6_stats;
    memset(&hz6_stats, 0, sizeof(hz6_stats));
#endif
    for (size_t i = 0; i < threads; i++) {
        alloc_attempts += args[i].alloc_attempts;
        alloc_successes += args[i].alloc_successes;
        alloc_failures += args[i].alloc_failures;
        frees += args[i].frees;
#if defined(HZ_BENCH_USE_HZ6) && HZ_BENCH_TRACE_LAST_OP
        hz6_pre_free_owns_false += args[i].hz6_pre_free_owns_false;
        hz6_duplicate_alloc_ptr += args[i].hz6_duplicate_alloc_ptr;
#endif
#include "bench_allocator_compare_hz6_stats_accumulate.inc"
    }

    printf("threads=%zu iters=%zu ws=%zu size=%zu..%zu time=%.3f ops/s=%.3f "
           "alloc_attempts=%zu alloc_success=%zu alloc_fail=%zu frees=%zu",
           threads, iters, ws, min_size, max_size, sec, ops_sec,
           alloc_attempts, alloc_successes, alloc_failures, frees);
#if defined(HZ_BENCH_USE_HZ6)
#if defined(HZ_BENCH_TRACE_LAST_OP) && HZ_BENCH_TRACE_LAST_OP
    bench_print_hz6_summary(&hz6_stats, hz6_pre_free_owns_false,
                            hz6_duplicate_alloc_ptr, peak_kb);
#else
    bench_print_hz6_summary(&hz6_stats, 0, 0, peak_kb);
#endif
#endif
    fflush(stdout);
    free(args);
    return 0;
}
