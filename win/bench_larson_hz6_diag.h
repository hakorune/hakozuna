#ifndef BENCH_LARSON_HZ6_DIAG_H
#define BENCH_LARSON_HZ6_DIAG_H

#include "hz6.h"

#include <stddef.h>

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
const char* hz6_front_attr_name(size_t index);
const char* hz6_alloc_path_name(Hz6AllocPath path);
void print_hz6_front_alloc_paths(const Hz6StatsSnapshot* stats);
void print_hz6_front_prefill_paths(const Hz6StatsSnapshot* stats);
void print_hz6_frontcache_class_diag(const Hz6StatsSnapshot* stats);
#endif

#endif
