#ifndef BENCH_ALLOCATOR_COMPARE_HZ6_SUMMARY_H
#define BENCH_ALLOCATOR_COMPARE_HZ6_SUMMARY_H

#include "hz6.h"
#include <stddef.h>

#if defined(HZ_BENCH_USE_HZ6)
void bench_print_hz6_summary(const Hz6StatsSnapshot* hz6_stats,
                             size_t hz6_pre_free_owns_false,
                             size_t hz6_duplicate_alloc_ptr,
                             size_t peak_kb);
#endif

#endif
