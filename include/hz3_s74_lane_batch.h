#pragma once

#include "hz3_config.h"
#include <stdint.h>

#if HZ3_S74_LANE_BATCH && HZ3_S74_STATS
void hz3_s74_stats_refill(uint32_t burst);
void hz3_s74_stats_flush(uint32_t batch);
void hz3_s74_stats_lease_call(void);
#else
static inline void hz3_s74_stats_refill(uint32_t burst) {
    (void)burst;
}
static inline void hz3_s74_stats_flush(uint32_t batch) {
    (void)batch;
}
static inline void hz3_s74_stats_lease_call(void) {
}
#endif
