#pragma once

#include "hz3_config.h"

#if HZ3_S62_SUB4K_RETIRE

// S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k runs to free_bits
// Thread-exit only (cold). Does NOT call madvise - just updates free_bits.
void hz3_s62_sub4k_retire(void);

#endif
