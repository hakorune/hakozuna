#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(HZ_PHASE_USE_HZ8)
#include "h8.h"
#elif defined(HZ_PHASE_USE_HZ10)
#include "hz10_public_entry.h"
#endif

typedef void* (*HzPhaseMallocFn)(size_t size);
typedef void (*HzPhaseFreeFn)(void* ptr);

#if defined(HZ_PHASE_USE_TCMALLOC_DYNAMIC)
static HMODULE g_tcmalloc_module;
static HzPhaseMallocFn g_tcmalloc_malloc;
static HzPhaseFreeFn g_tcmalloc_free;
#endif

static int hz_phase_init(void) {
#if defined(HZ_PHASE_USE_TCMALLOC_DYNAMIC)
    g_tcmalloc_module = LoadLibraryA("tcmalloc_minimal.dll");
    if (!g_tcmalloc_module) return 0;
    g_tcmalloc_malloc =
        (HzPhaseMallocFn)(uintptr_t)GetProcAddress(g_tcmalloc_module, "tc_malloc");
    g_tcmalloc_free =
        (HzPhaseFreeFn)(uintptr_t)GetProcAddress(g_tcmalloc_module, "tc_free");
    return g_tcmalloc_malloc != NULL && g_tcmalloc_free != NULL;
#else
    return 1;
#endif
}

static void* hz_phase_malloc(size_t size) {
#if defined(HZ_PHASE_USE_HZ8)
    return h8_malloc(size);
#elif defined(HZ_PHASE_USE_HZ10)
    return hz10_malloc(size);
#elif defined(HZ_PHASE_USE_TCMALLOC_DYNAMIC)
    return g_tcmalloc_malloc(size);
#else
    return malloc(size);
#endif
}

static void hz_phase_free(void* ptr) {
#if defined(HZ_PHASE_USE_HZ8)
    h8_free(ptr);
#elif defined(HZ_PHASE_USE_HZ10)
    (void)hz10_free(ptr);
#elif defined(HZ_PHASE_USE_TCMALLOC_DYNAMIC)
    g_tcmalloc_free(ptr);
#else
    free(ptr);
#endif
}

static uint64_t hz_phase_ticks(void) {
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return (uint64_t)value.QuadPart;
}

int main(int argc, char** argv) {
    size_t size = 16384u;
    size_t batch = 256u;
    size_t rounds = 1024u;
    size_t warmup = 64u;
    LARGE_INTEGER frequency;
    void** ptrs;
    uint64_t alloc_ticks = 0;
    uint64_t free_ticks = 0;
    size_t failures = 0;

    if (argc > 1) size = (size_t)strtoull(argv[1], NULL, 10);
    if (argc > 2) batch = (size_t)strtoull(argv[2], NULL, 10);
    if (argc > 3) rounds = (size_t)strtoull(argv[3], NULL, 10);
    if (argc > 4) warmup = (size_t)strtoull(argv[4], NULL, 10);
    if (size == 0 || batch == 0 || rounds == 0) return 2;

    if (!hz_phase_init() || !QueryPerformanceFrequency(&frequency)) return 3;
    (void)SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1u);

    ptrs = (void**)calloc(batch, sizeof(*ptrs));
    if (!ptrs) return 4;

    for (size_t round = 0; round < warmup + rounds; ++round) {
        uint64_t begin = hz_phase_ticks();
        for (size_t i = 0; i < batch; ++i) {
            void* ptr = hz_phase_malloc(size);
            ptrs[i] = ptr;
            if (!ptr) {
                ++failures;
                continue;
            }
            *(volatile unsigned char*)ptr = (unsigned char)i;
        }
        uint64_t middle = hz_phase_ticks();
        for (size_t i = 0; i < batch; ++i) {
            if (ptrs[i]) hz_phase_free(ptrs[i]);
            ptrs[i] = NULL;
        }
        uint64_t end = hz_phase_ticks();
        if (round >= warmup) {
            alloc_ticks += middle - begin;
            free_ticks += end - middle;
        }
    }

    {
        double operations = (double)batch * (double)rounds;
        double tick_ns = 1.0e9 / (double)frequency.QuadPart;
        printf("size=%zu batch=%zu rounds=%zu alloc_ns_op=%.3f "
               "free_ns_op=%.3f failures=%zu\n",
               size, batch, rounds, (double)alloc_ticks * tick_ns / operations,
               (double)free_ticks * tick_ns / operations, failures);
    }

#if defined(HZ_PHASE_REPORT_HZ8_STATS)
    {
        H8DebugStats stats = h8_debug_stats();
        printf("diag run_create=%zu empty_transition=%zu empty_retain=%zu "
               "empty_budget_reject=%zu empty_reactivate=%zu "
               "mark_live_resident=%zu mark_live_decommitted=%zu "
               "reuse_active=%zu reuse_owner=%zu reuse_global=%zu "
               "owner_scan=%zu owner_steps=%zu global_scan=%zu "
               "global_steps=%zu\n",
               stats.medium_run_create_count,
               stats.medium_empty_transition_count,
               stats.medium_empty_retain_count,
               stats.medium_empty_budget_reject_count,
               stats.medium_empty_reactivate_count,
               stats.medium_alloc_mark_live_resident,
               stats.medium_alloc_mark_live_decommitted,
               stats.medium_run_reuse_active_count,
               stats.medium_run_reuse_owner_list_count,
               stats.medium_run_reuse_global_count,
               stats.medium_owner_scan_count,
               stats.medium_owner_scan_step_count,
               stats.medium_global_scan_count,
               stats.medium_global_scan_step_count);
    }
#endif

    free(ptrs);
    /* Keep tcmalloc loaded through process teardown; it owns TLS state. */
    return failures == 0 ? 0 : 5;
}
