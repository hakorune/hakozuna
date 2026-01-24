#pragma once

#include "hz3_config.h"

#if HZ3_S85_SMALL_V2_SLOW_STATS && HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

#include <stdint.h>

void hz3_s85_small_v2_slow_record(int sc,
                                 int got_xfer,
                                 int got_stash,
                                 int got_central,
                                 int did_page_alloc,
                                 int page_alloc_ok);

#else

static inline void hz3_s85_small_v2_slow_record(int sc,
                                               int got_xfer,
                                               int got_stash,
                                               int got_central,
                                               int did_page_alloc,
                                               int page_alloc_ok) {
    (void)sc;
    (void)got_xfer;
    (void)got_stash;
    (void)got_central;
    (void)did_page_alloc;
    (void)page_alloc_ok;
}

#endif

