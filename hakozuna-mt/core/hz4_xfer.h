#ifndef HZ4_XFER_H
#define HZ4_XFER_H

#include "hz4_config.h"

#if HZ4_XFER_CACHE && !HZ4_ALLOW_ARCHIVED_BOXES
#error "HZ4_XFER_CACHE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Stub: types required for struct compilation even if disabled/mocked
typedef struct hz4_xfer_batch hz4_xfer_batch_t;

#endif
