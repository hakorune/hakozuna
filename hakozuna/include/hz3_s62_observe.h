#pragma once

#include "hz3_config.h"

#if HZ3_S62_OBSERVE

// S62-0: OBSERVE - count potential purge candidates from free_bits
// Thread-exit only (cold). Does NOT call madvise - stats only.
// Purpose: measure current free_bits purge potential before implementing S62-1 PageRetireBox.
void hz3_s62_observe(void);

// Register atexit one-shot dump for S62-0 OBSERVE.
// Best-effort: ensures `[HZ3_S62_OBSERVE]` can appear even if thread destructors do not run.
void hz3_s62_observe_register_atexit(void);

#endif
