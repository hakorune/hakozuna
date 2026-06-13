#define _GNU_SOURCE

#include "hz3_inbox.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_stash_guard.h"
#include "hz3_arena.h"
#include "hz3_segmap.h"
#include "hz3_tag.h"
#include "hz3_sc.h"
#include "hz3_s65_medium_reclaim.h"

#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "hz3_inbox_globals.inc"
#include "hz3_inbox_s65_stats.inc"
#include "hz3_inbox_s203_stats.inc"
#include "hz3_inbox_s197_s195_init.inc"
#include "hz3_inbox_aligned_ops.inc"
#include "hz3_inbox_push_ops.inc"
#include "hz3_inbox_drain_ops.inc"
#include "hz3_inbox_mixed_ops.inc"
