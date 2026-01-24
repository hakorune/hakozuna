#pragma once

#include "hz3_config.h"

#if HZ3_S62_RETIRE

// S62-1: PageRetireBox - retire fully-empty small pages to free_bits
// Thread-exit only (cold). Does NOT call madvise - just updates free_bits.
// After this, S62-2 (or S61) can purge the retired pages.
void hz3_s62_retire(void);

#endif
