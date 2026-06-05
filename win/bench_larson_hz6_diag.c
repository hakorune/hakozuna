#include "bench_larson_hz6_diag.h"

#include <stdio.h>

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
const char* hz6_front_attr_name(size_t index) {
    switch (index) {
        case HZ6_FRONT_ATTR_LOCAL2P:
            return "local2p";
        case HZ6_FRONT_ATTR_MIDPAGE:
            return "midpage";
        case HZ6_FRONT_ATTR_LARGE:
            return "large";
        case HZ6_FRONT_ATTR_TOY:
            return "toy";
        default:
            return "unknown";
    }
}

const char* hz6_alloc_path_name(Hz6AllocPath path) {
    switch (path) {
        case HZ6_ALLOC_PATH_LOCAL_REUSE:
            return "local_reuse";
        case HZ6_ALLOC_PATH_TRANSFER_REUSE:
            return "transfer_reuse";
        case HZ6_ALLOC_PATH_PREFILL_REUSE:
            return "prefill_reuse";
        case HZ6_ALLOC_PATH_SOURCE_PREFILL:
            return "source_prefill";
        case HZ6_ALLOC_PATH_DIRECT_SOURCE:
            return "direct_source";
        case HZ6_ALLOC_PATH_RELEASED_REUSE:
            return "released_reuse";
        case HZ6_ALLOC_PATH_OOM:
            return "oom";
        case HZ6_ALLOC_PATH_UNSUPPORTED:
            return "unsupported";
        default:
            return "unknown";
    }
}

void print_hz6_front_alloc_paths(const Hz6StatsSnapshot* stats) {
    size_t front;
    size_t path;
    for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
        printf("[HZ6_PATH] front=%s", hz6_front_attr_name(front));
        for (path = 0; path < HZ6_ALLOC_PATH_COUNT; ++path) {
            printf(" %s=%zu", hz6_alloc_path_name((Hz6AllocPath)path),
                   stats->front_alloc_path[front][path]);
        }
        printf("\n");
    }
}

void print_hz6_front_prefill_paths(const Hz6StatsSnapshot* stats) {
    size_t front;
    const char* front_name;
    for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
        switch (front) {
            case HZ6_FRONT_ATTR_LOCAL2P:
                front_name = "local2p";
                break;
            case HZ6_FRONT_ATTR_MIDPAGE:
                front_name = "midpage";
                break;
            case HZ6_FRONT_ATTR_LARGE:
                front_name = "large";
                break;
            case HZ6_FRONT_ATTR_TOY:
                front_name = "toy";
                break;
            default:
                front_name = "unknown";
                break;
        }
        printf("[HZ6_PREFILL] front=%s attempt=%zu filled=%zu fallback=%zu\n",
               front_name,
               stats->front_source_prefill_attempt[front],
               stats->front_source_prefill_filled[front],
               stats->front_source_prefill_fallback[front]);
    }
}

void print_hz6_frontcache_class_diag(const Hz6StatsSnapshot* stats) {
    size_t class_id;
    for (class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
        size_t push = stats->frontcache_push_by_class[class_id];
        size_t pop_empty = stats->frontcache_pop_empty_by_class[class_id];
        if (push == 0 && pop_empty == 0) {
            continue;
        }
        printf("[HZ6_FRONTCACHE_CLASS] class=%zu push=%zu pop_empty=%zu\n",
               class_id,
               push,
               pop_empty);
    }
}
#endif
