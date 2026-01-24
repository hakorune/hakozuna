#pragma once

#include "hz3_types.h"
#include <stdatomic.h>

// ============================================================================
// Day 6: Knobs - runtime-tunable parameters
// Updated only by epoch/learning layer, never by hot path
// Hz3Knobs is defined in hz3_types.h
// ============================================================================

// Global knobs (updated by epoch/learning only)
extern Hz3Knobs g_hz3_knobs;
extern _Atomic(uint32_t) g_hz3_knobs_ver;  // epoch version

// Initialize knobs (called via pthread_once)
void hz3_knobs_init(void);
