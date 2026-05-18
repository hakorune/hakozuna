#pragma once

#include "hz3_config.h"

#if defined(_WIN32) && HZ3_S257_PROCESS_MEMORY_OBS
void hz3_win_process_stats_register(void);
#else
static inline void hz3_win_process_stats_register(void) {}
#endif
