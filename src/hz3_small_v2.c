#include "hz3_small_v2.h"

#include "hz3_arena.h"
#include "hz3_eco_mode.h"
#include "hz3_central_bin.h"
#include "hz3_config.h"
#include "hz3_dtor_stats.h"
#include "hz3_list_invariants.h"
#include "hz3_list_util.h"
#include "hz3_owner_excl.h"
#include "hz3_owner_lease.h"
#include "hz3_owner_stash.h"
#include "hz3_s62_stale_check.h"
#include "hz3_s85_small_v2_slow_stats.h"
#include "hz3_sc.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_small.h"
#include "hz3_small_xfer.h"
#include "hz3_tcache.h"
#include "hz3_epoch.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#if HZ3_S62_RETIRE_MPROTECT
#include <errno.h>
#include <sys/mman.h>
#endif

// ============================================================================
// Include split .inc files (single TU maintained)
// ============================================================================

#include "small_v2/hz3_small_v2_types.inc"
#include "small_v2/hz3_small_v2_stats.inc"
#include "small_v2/hz3_small_v2_central.inc"
#include "small_v2/hz3_small_v2_push_remote.inc"
#include "small_v2/hz3_small_v2_collision.inc"
#include "small_v2/hz3_small_v2_page_alloc.inc"
#include "small_v2/hz3_small_v2_fill.inc"
#include "small_v2/hz3_small_v2_refill.inc"
#include "small_v2/hz3_small_v2_free.inc"
#include "small_v2/hz3_small_v2_s62_api.inc"
#include "small_v2/hz3_small_v2_s69_livecount.inc"
#include "small_v2/hz3_small_v2_s72_boundary.inc"
