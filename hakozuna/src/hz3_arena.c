#define _GNU_SOURCE

#include "hz3_arena.h"
#include "hz3_types.h"
#include "hz3_oom.h"
#include "hz3_mem_budget.h"
#include "hz3_tcache.h"  // for hz3_dstbin_flush_remote_all, t_hz3_cache
#include "hz3_platform.h"
#include "hz3_win_process_stats.h"
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif

#include <stdatomic.h>
#include <stdio.h>   // for fprintf (gate metrics)
#include <stdlib.h>  // for atexit()
#include <sys/mman.h>
#include <string.h>
#include <sched.h>  // for sched_yield()
#include <time.h>   // for nanosleep()
#include <errno.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE MAP_FIXED
#endif

#include "hz3_arena_globals.inc"
#include "hz3_arena_s256_obs.inc"
#include "hz3_arena_init.inc"
#include "hz3_arena_accessors.inc"
#include "hz3_arena_alloc_slot.inc"
#include "hz3_arena_alloc.inc"
#include "hz3_arena_free.inc"
