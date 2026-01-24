#pragma once

#include "hz3_types.h"

// ============================================================================
// Day 6: Learning v0 - heuristic-based knob tuning
// Gate: HZ3_LEARN_ENABLE (default 0 = OFF)
// ============================================================================

#ifndef HZ3_LEARN_ENABLE
#define HZ3_LEARN_ENABLE 0
#endif

// Called from epoch to update global knobs based on TLS stats
// Only active when HZ3_LEARN_ENABLE=1
void hz3_learn_update(Hz3Stats* stats);
