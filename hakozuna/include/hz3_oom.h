#pragma once

#include <stdint.h>

// OOM observability hook (init/slow path only).
// - where: short tag (ASCII)
// - info0/info1: optional numeric context (size, slots, etc.)
void hz3_oom_note(const char* where, uint64_t info0, uint64_t info1);
