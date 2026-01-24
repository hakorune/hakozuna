#define _GNU_SOURCE

#include "hz3_owner_stash.h"

#if HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE

#include "hz3_list_invariants.h"
#include "hz3_small_v2.h"
#include "hz3_s74_lane_batch.h"
#include "hz3_tcache.h"
#include "hz3_list_util.h"
#include "hz3_dtor_stats.h"
#include "hz3_arena.h"  // for arena range check
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
#include <sys/mman.h>
#include <errno.h>
#endif

// ============================================================================
// Include split .inc files (single TU maintained)
// ============================================================================

#include "owner_stash/hz3_owner_stash_debug.inc"
#include "owner_stash/hz3_owner_stash_stats.inc"
#include "owner_stash/hz3_owner_stash_s93_packet.inc"
#include "owner_stash/hz3_owner_stash_globals.inc"
#include "owner_stash/hz3_owner_stash_push.inc"
#include "owner_stash/hz3_owner_stash_pop.inc"
#include "owner_stash/hz3_owner_stash_flush.inc"

// S121-D: Extern wrapper for packet flush (NO-GO, archived)
void hz3_s121d_packet_flush_all_extern(void) {
    // NO-OP: S121-D removed
}

// S121-M: Extern wrapper for pending pageq batch flush
void hz3_s121m_flush_pending_extern(void) {
#if HZ3_S121_M_PAGEQ_BATCH
    hz3_s121m_flush_pending(&t_hz3_cache);
#endif
}

#endif  // HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE
