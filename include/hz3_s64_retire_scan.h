#pragma once

#include "hz3_config.h"

#if HZ3_S64_RETIRE_SCAN

// ============================================================================
// S64-A: RetireScanBox - epoch-based scan for empty pages
// ============================================================================
//
// Unlike S62 (thread-exit only), S64 runs on every epoch tick.
// Scans central bin (my_shard only) for fully-empty pages and retires them.
// Retired pages are enqueued to S64_PurgeDelayBox for delayed madvise.
//
// Hot path: 0 (epoch tick only)
// Thread boundary: my_shard fixed (TLS-based)

// Called from hz3_epoch_force() after S60 purge tick.
void hz3_s64_retire_scan_tick(void);

#else  // !HZ3_S64_RETIRE_SCAN

static inline void hz3_s64_retire_scan_tick(void) {}

#endif  // HZ3_S64_RETIRE_SCAN
