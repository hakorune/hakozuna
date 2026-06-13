#define _GNU_SOURCE

#include "hz3_central.h"
#include "hz3_arena.h"
#include "hz3_tcache.h"
#include "hz3_central_shadow.h"
#include "hz3_platform.h"
#include "hz3_sc.h"

#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "hz3_central_globals.inc"
#include "hz3_central_s65_stats.inc"
#include "hz3_central_cold_external.inc"
#include "hz3_central_atomic_fast.inc"
#include "hz3_central_xfer_init.inc"
#include "hz3_central_hot_ops.inc"
#include "hz3_central_cold_ops.inc"
#include "hz3_central_xfer_ops.inc"
