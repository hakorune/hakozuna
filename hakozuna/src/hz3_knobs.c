#define _GNU_SOURCE

#include "hz3_knobs.h"
#include "hz3_types.h"

#include <pthread.h>

// ============================================================================
// Day 6: Global knobs with Day 5 default values
// Initialized statically to match Day 5 behavior exactly when LEARN_ENABLE=0
// ============================================================================

// Day 5 equivalent defaults (learning OFF = same behavior)
// S15-1: sc=0 uses HZ3_BIN_CAP_SC0 (32) for thicker bin
// S-OOM: Extended to 16 size classes (4KB-64KB)
Hz3Knobs g_hz3_knobs = {
    .refill_batch = {12, 8, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2},  // sc=0..15
    .bin_target = {HZ3_BIN_CAP_SC0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    .outbox_flush_n = 0,  // unused v0
    // S64: Epoch-based retire/purge defaults
    .s64_retire_budget_objs = HZ3_S64_RETIRE_BUDGET_OBJS,
    .s64_purge_delay_epochs = HZ3_S64_PURGE_DELAY_EPOCHS,
    .s64_purge_budget_pages = (HZ3_S64_PURGE_BUDGET_PAGES > 255) ? 255 : HZ3_S64_PURGE_BUDGET_PAGES,
    // S64-1: TCacheDumpBox default
    .s64_tdump_budget_objs = HZ3_S64_TDUMP_BUDGET_OBJS,
};

_Atomic(uint32_t) g_hz3_knobs_ver = 0;

// pthread_once for thread-safe initialization
static pthread_once_t g_hz3_knobs_once = PTHREAD_ONCE_INIT;

static void hz3_knobs_do_init(void) {
    // Static initialization is sufficient for now
    // Future: load from config file or environment
}

void hz3_knobs_init(void) {
    pthread_once(&g_hz3_knobs_once, hz3_knobs_do_init);
}
