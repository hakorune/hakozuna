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
#elif defined(HZ_BENCH_USE_HZ8)
#include "h8.h"
#if defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
#include "h8_medium_page8k_remote.h"
#endif
#if defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
#include "h8_medium_page_shadow.h"
#endif
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
#include "h8_medium_domain_shadow.h"
#endif
#if defined(H8_ADAPTIVE_TRANSFER_SHADOW_L0)
#include "h8_adaptive_shadow.h"
#endif
#if defined(H8_RECLAIM_ADAPTER_SHADOW_L0)
#include "h8_reclaim_shadow.h"
#endif
#if defined(H8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0)
#include "h8_magazine_tail_shadow.h"
#endif
#if defined(H8_SMALL_REUSE_VISIBILITY_SHADOW_L0)
#include "h8_small_reuse_visibility_shadow.h"
#endif
#if defined(H8_SMALL_AVAILABLE_INDEX_DIAG)
#include "h8_small_available_index.h"
#endif
#elif defined(HZ_BENCH_USE_HZ10)
#include "hz10_public_entry.h"
#elif defined(HZ_BENCH_USE_HZ11)
#include "hz11.h"
#include "hz11_class_diag.h"
#elif defined(HZ_BENCH_USE_HZ12)
#include "hz12.h"
#if defined(HZ_BENCH_HZ12_OWNER_MAP_CONTROL) || \
    defined(HZ_BENCH_HZ12_ALLOC_OWNER_MAP_CONTROL)
#include "hz12_shadow.h"
#endif
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    H11Stats hz11_stats_after;
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
#elif defined(HZ_BENCH_USE_HZ8)
    (void)ta;
    return h8_malloc(size);
#elif defined(HZ_BENCH_USE_HZ10)
    (void)ta;
    return hz10_malloc(size);
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    return hz11_malloc(size);
#elif defined(HZ_BENCH_USE_HZ12)
    void* ptr = hz12_malloc(size);
#if defined(HZ_BENCH_HZ12_OWNER_MAP_CONTROL) || \
    defined(HZ_BENCH_HZ12_ALLOC_OWNER_MAP_CONTROL)
    if (ptr) h12_shadow_on_alloc(ptr, (uint32_t)ta->thread_index);
#else
    (void)ta;
#endif
    return ptr;
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
#elif defined(HZ_BENCH_USE_HZ8)
    (void)ta;
    return h8_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ10)
    (void)ta;
    (void)ptr;
    (void)size;
    return NULL;
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    return hz11_realloc(ptr, size);
#elif defined(HZ_BENCH_USE_HZ12)
    (void)ta;
    return hz12_realloc(ptr, size);
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
#elif defined(HZ_BENCH_USE_HZ8)
    (void)ta;
    h8_free(ptr);
#elif defined(HZ_BENCH_USE_HZ10)
    (void)ta;
    (void)hz10_free(ptr);
#elif defined(HZ_BENCH_USE_HZ11)
    (void)ta;
    hz11_free(ptr);
#elif defined(HZ_BENCH_USE_HZ12)
#if defined(HZ_BENCH_HZ12_OWNER_MAP_CONTROL)
    uint32_t owner_id;
    (void)h12_shadow_owner_for_ptr(ptr, &owner_id);
#else
    (void)ta;
#endif
    hz12_free(ptr);
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    hz11_stats(&ta->hz11_stats_after);
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    hz11_stats(&ta->hz11_stats_after);
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
#if defined(HZ_BENCH_USE_HZ12) && \
    (defined(HZ_BENCH_HZ12_OWNER_MAP_CONTROL) || \
     defined(HZ_BENCH_HZ12_ALLOC_OWNER_MAP_CONTROL))
    if (threads > HZ12_SHADOW_MAX_OWNERS ||
        !h12_shadow_init((uint32_t)threads)) {
        fprintf(stderr, "HZ12 owner-map control init failed\n");
        return 2;
    }
#endif
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    H11Stats hz11_stats_sum;
    memset(&hz11_stats_sum, 0, sizeof(hz11_stats_sum));
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
        H11Stats* s = &args[i].hz11_stats_after;
        hz11_stats_sum.malloc_count += s->malloc_count;
        hz11_stats_sum.malloc_hit += s->malloc_hit;
        hz11_stats_sum.refill_count += s->refill_count;
        hz11_stats_sum.free_count += s->free_count;
        hz11_stats_sum.token_hit += s->token_hit;
        hz11_stats_sum.token_miss += s->token_miss;
        hz11_stats_sum.direct_hit_count += s->direct_hit_count;
        hz11_stats_sum.direct_miss_count += s->direct_miss_count;
        hz11_stats_sum.overflow_count += s->overflow_count;
        hz11_stats_sum.flush_count += s->flush_count;
        hz11_stats_sum.flush_items += s->flush_items;
        hz11_stats_sum.cached_bytes += s->cached_bytes;
        hz11_stats_sum.refill_from_transfer += s->refill_from_transfer;
        hz11_stats_sum.refill_from_central += s->refill_from_central;
        hz11_stats_sum.refill_from_span += s->refill_from_span;
#define HZ11_MAX_FIELD(name) if (s->name > hz11_stats_sum.name) { hz11_stats_sum.name = s->name; }
        HZ11_MAX_FIELD(span_create_count)
        HZ11_MAX_FIELD(transfer_remove_hit)
        HZ11_MAX_FIELD(transfer_remove_miss)
        HZ11_MAX_FIELD(transfer_insert)
        HZ11_MAX_FIELD(transfer_insert_spill)
        HZ11_MAX_FIELD(central_remove_hit)
        HZ11_MAX_FIELD(central_remove_miss)
        HZ11_MAX_FIELD(central_insert)
        HZ11_MAX_FIELD(span_return_count)
        HZ11_MAX_FIELD(span_reuse_count)
        HZ11_MAX_FIELD(central_full_span_count)
        HZ11_MAX_FIELD(central_partial_span_count)
        HZ11_MAX_FIELD(central_objects)
        HZ11_MAX_FIELD(returned_push)
        HZ11_MAX_FIELD(returned_pop_hit)
        HZ11_MAX_FIELD(returned_pop_miss)
#undef HZ11_MAX_FIELD
#endif
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
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    printf(" hz11_malloc=%llu hz11_hit=%llu hz11_refill=%llu"
           " hz11_free=%llu hz11_direct_hit=%llu hz11_direct_miss=%llu"
           " hz11_overflow=%llu hz11_flush=%llu hz11_flush_items=%llu"
           " hz11_cached_bytes=%zu hz11_span_create=%llu"
           " hz11_returned_push=%llu hz11_returned_pop_hit=%llu"
           " hz11_returned_pop_miss=%llu"
           " hz11_refill_transfer=%llu hz11_refill_central=%llu"
           " hz11_refill_span=%llu hz11_xfer_hit=%llu hz11_xfer_miss=%llu"
           " hz11_xfer_insert=%llu hz11_xfer_spill=%llu"
           " hz11_central_hit=%llu hz11_central_miss=%llu"
           " hz11_central_insert=%llu",
           (unsigned long long)hz11_stats_sum.malloc_count,
           (unsigned long long)hz11_stats_sum.malloc_hit,
           (unsigned long long)hz11_stats_sum.refill_count,
           (unsigned long long)hz11_stats_sum.free_count,
           (unsigned long long)hz11_stats_sum.direct_hit_count,
           (unsigned long long)hz11_stats_sum.direct_miss_count,
           (unsigned long long)hz11_stats_sum.overflow_count,
           (unsigned long long)hz11_stats_sum.flush_count,
           (unsigned long long)hz11_stats_sum.flush_items,
           hz11_stats_sum.cached_bytes,
           (unsigned long long)hz11_stats_sum.span_create_count,
           (unsigned long long)hz11_stats_sum.returned_push,
           (unsigned long long)hz11_stats_sum.returned_pop_hit,
           (unsigned long long)hz11_stats_sum.returned_pop_miss,
           (unsigned long long)hz11_stats_sum.refill_from_transfer,
           (unsigned long long)hz11_stats_sum.refill_from_central,
           (unsigned long long)hz11_stats_sum.refill_from_span,
           (unsigned long long)hz11_stats_sum.transfer_remove_hit,
           (unsigned long long)hz11_stats_sum.transfer_remove_miss,
           (unsigned long long)hz11_stats_sum.transfer_insert,
           (unsigned long long)hz11_stats_sum.transfer_insert_spill,
           (unsigned long long)hz11_stats_sum.central_remove_hit,
           (unsigned long long)hz11_stats_sum.central_remove_miss,
           (unsigned long long)hz11_stats_sum.central_insert);
    printf("\n");
    hz11_class_diag_dump_stats();
    hz11_matrix_diag_dump_stats();
#endif
#if defined(HZ_BENCH_USE_HZ8) && \
    defined(H8_ADAPTIVE_TRANSFER_SHADOW_L0)
    h8_adaptive_shadow_dump();
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_RECLAIM_ADAPTER_SHADOW_L0)
    h8_reclaim_shadow_dump();
#endif
#if defined(HZ_BENCH_USE_HZ8) && \
    defined(H8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0)
    h8_magazine_tail_shadow_dump();
#endif
#if defined(HZ_BENCH_USE_HZ8) && \
    defined(H8_SMALL_REUSE_VISIBILITY_SHADOW_L0)
    h8_small_reuse_visibility_dump();
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_SMALL_AVAILABLE_INDEX_DIAG)
    h8_small_available_index_dump();
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
    {
        H8Page8KRemoteStats s = h8_page8k_remote_stats();
        printf("[H8_PAGE8K_DISPATCH] alloc_attempt=%llu alloc_served=%llu "
               "free_attempt=%llu free_owner_present=%llu free_owned=%llu "
               "free_success=%llu free_miss=%llu owner_create=%llu\n",
               (unsigned long long)s.dispatch_alloc_attempt,
               (unsigned long long)s.dispatch_alloc_served,
               (unsigned long long)s.dispatch_free_attempt,
               (unsigned long long)s.dispatch_free_owner_present,
               (unsigned long long)s.dispatch_free_owned,
               (unsigned long long)s.dispatch_free_success,
               (unsigned long long)s.dispatch_free_miss,
               (unsigned long long)s.owner_create);
    }
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
    {
        H8MediumPageShadowStats s = h8_medium_page_shadow_stats();
        printf("\n[H8_MEDIUM_PAGE_SHADOW] lookup=%llu hit=%llu miss=%llu "
               "run_mismatch=%llu exact_valid=%llu exact_invalid=%llu "
               "state_match=%llu state_mismatch=%llu\n",
               (unsigned long long)s.lookup, (unsigned long long)s.hit,
               (unsigned long long)s.miss, (unsigned long long)s.run_mismatch,
               (unsigned long long)s.exact_valid,
               (unsigned long long)s.exact_invalid,
               (unsigned long long)s.state_match,
               (unsigned long long)s.state_mismatch);
    }
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
    {
        H8MediumDomainShadowStats s = h8_medium_domain_shadow_stats();
        printf("[H8_MEDIUM_DOMAIN] lookup=%llu medium_hit=%llu "
               "page8k_hit=%llu miss=%llu kind_match=%llu "
               "kind_mismatch=%llu register_conflict=%llu "
               "medium_register=%llu medium_unregister=%llu "
               "page8k_register=%llu\n",
               (unsigned long long)s.lookup,
               (unsigned long long)s.medium_hit,
               (unsigned long long)s.page8k_hit,
               (unsigned long long)s.miss,
               (unsigned long long)s.kind_match,
               (unsigned long long)s.kind_mismatch,
               (unsigned long long)s.register_conflict,
               (unsigned long long)s.medium_register,
               (unsigned long long)s.medium_unregister,
               (unsigned long long)s.page8k_register);
    }
#endif
#if defined(HZ_BENCH_USE_HZ8) && defined(H8_SPEED_ATTRIBUTION_L0)
    {
        H8DebugStats s = h8_debug_stats();
        printf("[H8_SPEED_ATTR] active_hit=%zu active_miss=%zu "
               "freelist=%zu bump=%zu slow_collect=%zu span_commit=%zu "
               "mag_pop=%zu mag_hit=%zu mag_reject=%zu mag_push=%zu "
               "mag_full_preserve=%zu "
               "find_scan=%zu find_steps=%zu find_usable=%zu find_full=%zu "
               "pending_collect=%zu medium_route=%zu medium_route_steps=%zu "
               "medium_collect_calls=%zu medium_collect_runs=%zu\n",
               s.local_active_hit, s.local_active_miss,
               s.local_freelist_pop, s.local_bump_alloc,
               s.local_slow_collect, s.local_span_commit,
               s.reusable_mag_pop_attempt, s.reusable_mag_pop_hit,
               s.reusable_mag_pop_reject, s.reusable_mag_push,
               s.reusable_mag_full_preserve,
               s.local_find_scan, s.local_find_scan_span,
               s.local_find_scan_span_usable, s.local_find_scan_span_full,
               s.pending_collect_call_count, s.medium_route_lookup_count,
               s.medium_route_lookup_step_count,
               s.medium_remote_collect_call_count,
               s.medium_remote_collect_run_count);
        printf("[H8_MAG_CLASS]");
        for (size_t i = 0; i < H8_DEBUG_SMALL_CLASS_CAP; ++i) {
            printf(" c%zu_empty=%zu c%zu_low=%zu", i,
                   s.reusable_mag_empty_pop_by_class[i], i,
                   s.reusable_mag_depth_low_water_by_class[i]);
        }
        printf("\n");
    }
#endif
    fflush(stdout);
    free(args);
    return 0;
}
