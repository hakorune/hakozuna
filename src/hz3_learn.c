#define _GNU_SOURCE

#include "hz3_learn.h"
#include "hz3_knobs.h"

#include <string.h>

// ============================================================================
// Day 6: Learning v0 - simple heuristic-based knob tuning
// Rules (safety-first):
//   - Change only 1 sc per epoch
//   - Change only 1 knob per epoch
//   - Clamp: refill_batch in {2..16}, bin_target in {0..HZ3_BIN_HARD_CAP}
// ============================================================================

void hz3_learn_update(Hz3Stats* stats) {
#if HZ3_LEARN_ENABLE
    if (!stats) return;

    // Find the most active size class (highest refill_calls)
    int max_sc = 0;
    for (int sc = 1; sc < HZ3_NUM_SC; sc++) {
        if (stats->refill_calls[sc] > stats->refill_calls[max_sc]) {
            max_sc = sc;
        }
    }

    int changed = 0;

    // Rule 1: If refill_calls is high -> increase bin_target
    // This reduces slow path frequency
    if (!changed && stats->refill_calls[max_sc] > 100) {
        if (g_hz3_knobs.bin_target[max_sc] < HZ3_BIN_HARD_CAP) {
            g_hz3_knobs.bin_target[max_sc]++;
            atomic_fetch_add_explicit(&g_hz3_knobs_ver, 1, memory_order_release);
            changed = 1;
        }
    }

    // Rule 2: If bin_trim_excess is high -> decrease bin_target
    // This reduces memory waste from over-caching
    if (!changed && stats->bin_trim_excess[max_sc] > 50) {
        if (g_hz3_knobs.bin_target[max_sc] > 0) {
            g_hz3_knobs.bin_target[max_sc]--;
            atomic_fetch_add_explicit(&g_hz3_knobs_ver, 1, memory_order_release);
            changed = 1;
        }
    }

    // Rule 3: If central_pop_miss is high -> increase refill_batch
    // This improves batching efficiency
    if (!changed && stats->central_pop_miss[max_sc] > 20) {
        if (g_hz3_knobs.refill_batch[max_sc] < 16) {
            g_hz3_knobs.refill_batch[max_sc]++;
            atomic_fetch_add_explicit(&g_hz3_knobs_ver, 1, memory_order_release);
            changed = 1;
        }
    }
    // Reset stats for next epoch
    memset(stats, 0, sizeof(*stats));
#else
    (void)stats;  // Suppress unused warning
#endif
}
