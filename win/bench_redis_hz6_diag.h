#ifndef BENCH_REDIS_HZ6_DIAG_H
#define BENCH_REDIS_HZ6_DIAG_H

#include "bench_modern_allocator_adapter.h"

#include <stddef.h>
#include <stdint.h>

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
void print_hz6_redis_stats(const char* pattern,
                           const void* rows,
                           size_t row_stride,
                           size_t hz6_stats_offset,
                           uint32_t threads);
#endif

#endif
