#define _GNU_SOURCE
#include "hz3_eco_mode.h"
#include "hz3_platform.h"
#include <stdlib.h>
#include <time.h>

#if HZ3_ECO_MODE

// Auto-init via constructor (LD_PRELOAD scenario)
__attribute__((constructor))
static void hz3_eco_mode_constructor(void) {
    hz3_eco_init();
}

// Global config (read once at init, not in hot path)
static int g_eco_enabled = 0;
static uint64_t g_eco_rate_thresh = HZ3_ECO_RATE_THRESH_DEFAULT;

// Per-thread state
static HZ3_TLS uint64_t tls_alloc_count = 0;
static HZ3_TLS uint64_t tls_last_sample_ns = 0;
static HZ3_TLS int tls_eco_stage = HZ3_ECO_STAGE_SMALL;

// Get current time in nanoseconds (portable)
static inline uint64_t hz3_eco_get_ns(void) {
    struct timespec ts;
#if defined(_WIN32)
    hz3_clock_gettime_monotonic(&ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void hz3_eco_init(void) {
    // Read ENV once at startup (not in hot path)
    const char* env = getenv("HZ3_ECO_ENABLED");
    g_eco_enabled = env && atoi(env);

    const char* thresh = getenv("HZ3_ECO_RATE_THRESH");
    if (thresh) {
        g_eco_rate_thresh = strtoull(thresh, NULL, 10);
    }
}

// Called at refill boundary (not per-alloc) to minimize hot path overhead
int hz3_eco_update_stage(uint64_t ops_since_last) {
    if (!g_eco_enabled) {
        return HZ3_ECO_STAGE_SMALL;
    }

    tls_alloc_count += ops_since_last;

    uint64_t now = hz3_eco_get_ns();

    // First sample: initialize timestamp and return (avoid division by zero)
    if (tls_last_sample_ns == 0) {
        tls_last_sample_ns = now;
        return tls_eco_stage;
    }

    uint64_t elapsed = now - tls_last_sample_ns;

    // Trigger on time OR count (whichever first)
    // Low-rate workloads won't reach 1M ops, so time trigger is essential
    if (elapsed >= HZ3_ECO_SAMPLE_INTERVAL_NS ||
        tls_alloc_count >= HZ3_ECO_SAMPLE_INTERVAL_OPS) {

        if (elapsed > 0) {
            uint64_t rate = (tls_alloc_count * 1000000000ULL) / elapsed;
            tls_eco_stage = (rate >= g_eco_rate_thresh) ? HZ3_ECO_STAGE_LARGE : HZ3_ECO_STAGE_SMALL;
        }
        tls_alloc_count = 0;
        tls_last_sample_ns = now;
    }

    return tls_eco_stage;
}

int hz3_eco_get_stage(void) {
    return g_eco_enabled ? tls_eco_stage : HZ3_ECO_STAGE_SMALL;
}

#else  // !HZ3_ECO_MODE

void hz3_eco_init(void) {
    // No-op when eco mode is disabled at compile time
}

int hz3_eco_update_stage(uint64_t ops_since_last) {
    (void)ops_since_last;
    return HZ3_ECO_STAGE_SMALL;
}

int hz3_eco_get_stage(void) {
    return HZ3_ECO_STAGE_SMALL;
}

#endif // HZ3_ECO_MODE
