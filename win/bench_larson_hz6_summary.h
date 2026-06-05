#ifndef BENCH_LARSON_HZ6_SUMMARY_H
#define BENCH_LARSON_HZ6_SUMMARY_H

#include "bench_modern_allocator_adapter.h"

#if defined(HZ_BENCH_USE_HZ6)
void bench_print_larson_hz6_summary(const Hz6StatsSnapshot* hz6_stats);
#endif

#endif
