// hz3_tcache.c - Thread-local cache core (init, destructor, slow path)
// Split into modules: hz3_tcache_stats.c, hz3_tcache_remote.c,
//                     hz3_tcache_pressure.c, hz3_tcache_alloc.c
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_eco_mode.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_inbox.h"
#include "hz3_knobs.h"
#include "hz3_epoch.h"
#include "hz3_small.h"
#include "hz3_small_v2.h"
#include "hz3_seg_hdr.h"
#include "hz3_tag.h"
#include "hz3_medium_debug.h"
#include "hz3_watch_ptr.h"
#include "hz3_owner_lease.h"
#include "hz3_owner_stash.h"
#include "hz3_large.h"
#if HZ3_LANE_SPLIT
#include "hz3_lane.h"
#endif

#include <sys/mman.h>
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S58_TCACHE_DECAY
#include "hz3_tcache_decay.h"
#endif
#if HZ3_S61_DTOR_HARD_PURGE
#include "hz3_dtor_hard_purge.h"
#endif
#if HZ3_S62_RETIRE
#include "hz3_s62_retire.h"
#endif
#if HZ3_S62_SUB4K_RETIRE
#include "hz3_s62_sub4k.h"
#endif
#if HZ3_S62_PURGE
#include "hz3_s62_purge.h"
#endif
#if HZ3_S62_OBSERVE
#include "hz3_s62_observe.h"
#endif
#if HZ3_S65_RELEASE_LEDGER
#include "hz3_release_ledger.h"
#endif
#if HZ3_S65_MEDIUM_RECLAIM
#include "hz3_s65_medium_reclaim.h"
#endif

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "hz3_dtor_stats.h"

#ifndef HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL
#define HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL 0
#endif
#ifndef HZ3_S65_DTOR_RECLAIM_AFTER_INBOX_DRAIN
#define HZ3_S65_DTOR_RECLAIM_AFTER_INBOX_DRAIN 0
#endif

#include "hz3_tcache_s65_dtor_inbox.inc"
#include "hz3_tcache_s203_alloc_stats.inc"
#include "hz3_tcache_s204_larson_diag.inc"
#include "hz3_tcache_state.inc"
#include "hz3_tcache_s62_atexit.inc"
#include "hz3_tcache_destructor.inc"
#include "hz3_tcache_init.inc"
#include "hz3_tcache_shard_accessors.inc"
#include "hz3_tcache_s209_miss_seq.inc"

#include "hz3_tcache_slowpath.inc"
