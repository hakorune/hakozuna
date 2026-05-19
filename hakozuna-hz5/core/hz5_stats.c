#define HZ5_STATS_IMPLEMENTATION 1
#include "hz5_internal.h"

#include <stdio.h>

static _Atomic(uint64_t) g_hz5_stats[HZ5_STAT_COUNT];
static _Atomic(uint64_t) g_hz5_stats_1p[HZ5_STAT_COUNT];
static _Atomic(uint64_t) g_hz5_stats_2p[HZ5_STAT_COUNT];
static _Atomic(uint64_t) g_hz5_stats_16p[HZ5_STAT_COUNT];
static atomic_flag g_hz5_stats_printed = ATOMIC_FLAG_INIT;

static _Atomic(uint64_t)* hz5_stats_bucket(uint32_t pages) {
    if (pages <= 1u) {
        return g_hz5_stats_1p;
    }
    if (pages <= 2u) {
        return g_hz5_stats_2p;
    }
    if (pages <= 16u) {
        return g_hz5_stats_16p;
    }
    return NULL;
}

#if HZ5_DIAGNOSTIC_STATS
static const char* hz5_stats_name(Hz5StatId id) {
    switch (id) {
    case HZ5_STAT_ALLOC_CALL:
        return "alloc_call";
    case HZ5_STAT_ALLOC_TCACHE_HIT:
        return "alloc_tcache_hit";
    case HZ5_STAT_ALLOC_DRAIN_CALL:
        return "alloc_drain_call";
    case HZ5_STAT_ALLOC_DRAIN_HIT:
        return "alloc_drain_hit";
    case HZ5_STAT_ALLOC_SEGMENT:
        return "alloc_segment";
    case HZ5_STAT_FREE_LOCAL_CACHE:
        return "free_local_cache";
    case HZ5_STAT_FREE_LOCAL_RELEASE:
        return "free_local_release";
    case HZ5_STAT_FREE_REMOTE_BUFFERED:
        return "free_remote_buffered";
    case HZ5_STAT_REMOTE_BUFFER_FLUSH:
        return "remote_buffer_flush";
    case HZ5_STAT_REMOTE_BUFFER_FLUSH_ENTRY:
        return "remote_buffer_flush_entry";
    case HZ5_STAT_REMOTE_DRAIN_NODE:
        return "remote_drain_node";
    case HZ5_STAT_REMOTE_DRAIN_CACHE:
        return "remote_drain_cache";
    case HZ5_STAT_REMOTE_DRAIN_RELEASE:
        return "remote_drain_release";
    case HZ5_STAT_SEGMENT_ALLOC_SCAN_STEP:
        return "segment_alloc_scan_step";
    case HZ5_STAT_TLS_DESTRUCTOR_FLUSH:
        return "tls_destructor_flush";
    case HZ5_STAT_TLS_DESTRUCTOR_FLUSH_ENTRY:
        return "tls_destructor_flush_entry";
    case HZ5_STAT_OWNER_DESTRUCTOR_DRAIN:
        return "owner_destructor_drain";
    case HZ5_STAT_OWNER_DESTRUCTOR_RELEASE:
        return "owner_destructor_release";
    case HZ5_STAT_TCACHE_DESTRUCTOR_RELEASE:
        return "tcache_destructor_release";
    case HZ5_STAT_FINAL_PENDING_RELEASE:
        return "final_pending_release";
    default:
        return "unknown";
    }
}
#endif

void hz5_stats_inc(Hz5StatId id) {
    hz5_stats_add(id, 1u);
}

void hz5_stats_add(Hz5StatId id, uint64_t value) {
    if (id >= HZ5_STAT_COUNT) {
        return;
    }
    atomic_fetch_add_explicit(&g_hz5_stats[id], value, memory_order_relaxed);
}

void hz5_stats_inc_pages(Hz5StatId id, uint32_t pages) {
    hz5_stats_add_pages(id, pages, 1u);
}

void hz5_stats_add_pages(Hz5StatId id, uint32_t pages, uint64_t value) {
    if (id >= HZ5_STAT_COUNT) {
        return;
    }
    hz5_stats_add(id, value);
    _Atomic(uint64_t)* bucket = hz5_stats_bucket(pages);
    if (bucket) {
        atomic_fetch_add_explicit(&bucket[id], value, memory_order_relaxed);
    }
}

static void hz5_stats_print_bucket(const char* label, _Atomic(uint64_t)* bucket) {
#if HZ5_DIAGNOSTIC_STATS
    fprintf(stderr, "[HZ5_P5_STATS.%s]", label);
    for (uint32_t i = 0; i < (uint32_t)HZ5_STAT_COUNT; ++i) {
        uint64_t value = atomic_load_explicit(&bucket[i], memory_order_relaxed);
        if (value != 0) {
            fprintf(stderr, " %s=%llu", hz5_stats_name((Hz5StatId)i),
                    (unsigned long long)value);
        }
    }
    fputc('\n', stderr);
#else
    (void)label;
    (void)bucket;
#endif
}

void hz5_stats_print_once(void) {
    if (atomic_flag_test_and_set_explicit(&g_hz5_stats_printed,
                                          memory_order_acq_rel)) {
        return;
    }

    Hz5OwnerToken owner = hz5_owner_current_if_initialized();
    Hz5RemoteBuffer* buffer = hz5_remote_tls_buffer();
    if (buffer && buffer->count != 0) {
        hz5_remote_flush_buffer(buffer);
    }
    if (owner.slot != 0) {
        hz5_remote_release_owner(owner);
    }
    hz5_tcache_release_all();
    hz5_remote_release_all_pending();

    hz5_stats_print_bucket("total", g_hz5_stats);
    hz5_stats_print_bucket("pages1", g_hz5_stats_1p);
    hz5_stats_print_bucket("pages2", g_hz5_stats_2p);
    hz5_stats_print_bucket("pages16", g_hz5_stats_16p);

#if HZ5_DIAGNOSTIC_STATS
    uint64_t remote_pending = 0;
    uint64_t run_starts = 0;
    uint32_t segments = hz5_p1_segment_count();
    for (uint32_t seg_idx = 0; seg_idx < segments; ++seg_idx) {
        Hz5Seg* seg = hz5_p1_segment_at(seg_idx);
        if (!seg) {
            continue;
        }
        for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
            Hz5PageMeta* meta = &seg->page[page];
            if (meta->kind != HZ5_PAGE_RUN_START) {
                continue;
            }
            ++run_starts;
            remote_pending += atomic_load_explicit(&meta->remote_count_hint,
                                                   memory_order_relaxed);
        }
    }
    fprintf(stderr,
            "[HZ5_P7_SAFETY] segments=%u run_starts=%llu remote_pending=%llu\n",
            segments,
            (unsigned long long)run_starts,
            (unsigned long long)remote_pending);
#endif
}
