#pragma once
#include "hz3_config.h"

#if HZ3_S62_PURGE

// S62-2: DtorSmallPagePurgeBox - purge retired small pages
// Thread-exit only (cold). Calls madvise(DONTNEED) on free_bits=1 pages.
// Uses consecutive run batching and budget control (HZ3_S62_PURGE_MAX_CALLS).
void hz3_s62_purge(void);

#endif
