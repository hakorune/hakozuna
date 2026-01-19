#ifndef HZ3_ECO_MODE_H
#define HZ3_ECO_MODE_H

#include <stdint.h>

// Compile-time switch (default OFF, enable with -DHZ3_ECO_MODE=1)
#ifndef HZ3_ECO_MODE
#define HZ3_ECO_MODE 0
#endif

#define HZ3_ECO_STAGE_SMALL 0
#define HZ3_ECO_STAGE_LARGE 1

#define HZ3_ECO_SAMPLE_INTERVAL_NS  1000000000ULL  // 1 second
#define HZ3_ECO_SAMPLE_INTERVAL_OPS 1000000        // 1M ops

#define HZ3_ECO_RATE_THRESH_DEFAULT 10000000       // 10M allocs/sec

// Batch sizes per stage
#define HZ3_ECO_REFILL_BURST_SMALL  8
#define HZ3_ECO_REFILL_BURST_LARGE  16
#define HZ3_ECO_REFILL_BATCH_SMALL  32
#define HZ3_ECO_REFILL_BATCH_LARGE  64

// Array sizes for stack allocation (use max when eco mode enabled)
// These are compile-time constants for VLA-free code
#define HZ3_ECO_REFILL_BURST_ARRAY_SIZE  HZ3_ECO_REFILL_BURST_LARGE
#define HZ3_ECO_REFILL_BATCH_ARRAY_SIZE  HZ3_ECO_REFILL_BATCH_LARGE

// Init (call once at startup)
void hz3_eco_init(void);

// Update stage at refill boundary (pass ops since last call)
// Returns current stage (0=SMALL, 1=LARGE)
int hz3_eco_update_stage(uint64_t ops_since_last);

// Get current stage (fast, no side effects)
int hz3_eco_get_stage(void);

// Batch size getters (use at refill boundary)
static inline int hz3_eco_refill_burst(void) {
    return hz3_eco_get_stage() ? HZ3_ECO_REFILL_BURST_LARGE : HZ3_ECO_REFILL_BURST_SMALL;
}

static inline int hz3_eco_refill_batch(void) {
    return hz3_eco_get_stage() ? HZ3_ECO_REFILL_BATCH_LARGE : HZ3_ECO_REFILL_BATCH_SMALL;
}

#endif // HZ3_ECO_MODE_H
