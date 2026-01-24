// hz3_config.h - Umbrella header for hz3 configuration
//
// hz3 is developed as a separate track. The LD_PRELOAD shim may exist even when
// hz3 is not fully implemented yet.
//
// Configuration layers (SSOT):
//   1) Header defaults (sub-headers): conservative, safe in any build context.
//   2) Build profile (hakozuna/hz3/Makefile): hz3 lane "known-good" defaults.
//   3) A/B overrides: pass diffs via HZ3_LDPRELOAD_DEFS_EXTRA (never replace defs).
//
// Sub-headers:
//   - hz3_config_core.h: Core enables, shim, debug, guardrails
//   - hz3_config_small_v2.h: Small v2 / PTAG / Sub4k / Large cache / Stats
//   - hz3_config_tcache_soa.h: TCache SoA / Bin count / Sparse remote
//   - hz3_config_scale.h: Scale lane (S42-S44 Transfer/OwnerStash)
//   - hz3_config_rss_memory.h: RSS / Memory pressure (S45-S61)
//   - hz3_config_sub4k.h: Sub4k retire/purge/triage (S62-S63)
//   - hz3_config_archive.h: Archived / NO-GO flags
#pragma once

#include "hz3_platform.h"

// ============================================================================
// Include sub-headers
// ============================================================================

#include "hz3_config_core.h"
#include "hz3_config_small_v2.h"
#include "hz3_config_tcache_soa.h"
#include "hz3_config_scale.h"
#include "hz3_config_rss_memory.h"
#include "hz3_config_sub4k.h"
#include "hz3_config_archive.h"
