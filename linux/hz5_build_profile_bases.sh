#!/usr/bin/env bash
# Sourced by build_linux_hz5_standalone.sh before argument parsing.
# Compound profile helper functions live here; human aliases live in hz5_build_profile_aliases.sh.

enable_local2p_fast_cookie_base() {
  LINUX_LOCAL2P=1
  LINUX_LOCAL2P_TLS_PACKED=1
  LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
  LINUX_LOCAL2P_DIRECT_ROUTE=1
  LINUX_LOCAL2P_DIRECT_INIT=1
  LINUX_LOCAL2P_OBJECT_NODE=1
  LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
  LINUX_LOCAL2P_ROUTE_COOKIE=1
  LINUX_LOCAL2P_REUSE_STATE_ONLY=1
  LINUX_LOCAL2P_SLIM_CHECK=1
  LINUX_LOCAL2P_FAST_COOKIE=1
}

enable_local2p_tls_fast_return_base() {
  enable_local2p_fast_cookie_base
  LINUX_LOCAL2P_TLS_FAST_RETURN=1
}

enable_local2p_exact_api_base() {
  enable_local2p_tls_fast_return_base
  LINUX_LOCAL2P_EXACT_API=1
}

enable_local2p_speed_linkflags_base() {
  enable_local2p_exact_api_base
  LINUX_LOCAL2P_SPEED_LINKFLAGS=1
}

enable_general_region_outbox_base() {
  BUILD_PRELOAD_FULL=1
  LINUX_SMALLFRONT_S1=1
  LINUX_SMALLFRONT_REMOTE_OUTBOX=1
  LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
  LINUX_MIDFRONT_M1=1
  LINUX_MIDFRONT_OWNER_FAST_STATE=1
  LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
  LINUX_LARGEFRONT_L1=1
  LINUX_LARGEFRONT_OWNER_INBOX=1
  LINUX_LARGEFRONT_OWNER_FAST_STATE=1
  LINUX_LARGEFRONT_REGION_MAP=1
  HZ5_STANDALONE_EXACT_ONLY=0
}

enable_midpage_region_base() {
  enable_general_region_outbox_base
  LINUX_MIDPAGEFRONT_M2=1
  LINUX_MIDPAGEFRONT_REGION_ARRAY=1
  LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP=16
}

enable_midpage_region_shadow_base() {
  enable_midpage_region_base
  LINUX_MIDPAGEFRONT_REMOTE_SHADOW=1
  LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE=1
}

enable_midpage_shadow_allocfirst_base() {
  enable_midpage_region_shadow_base
  PRELOAD_MIDPAGE_ALLOC_FIRST=1
}

enable_midpage_nodeless_base() {
  enable_midpage_shadow_allocfirst_base
  LINUX_MIDPAGEFRONT_NODELESS_RUN=1
}

enable_midpage_m4mag_base() {
  enable_midpage_shadow_allocfirst_base
  LINUX_MIDPAGEFRONT_M4_MAGAZINE=1
}

enable_midpage_m4packet_base() {
  enable_midpage_m4mag_base
  LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET=1
}

enable_midpage_m4packet_freefirst_base() {
  enable_midpage_m4packet_base
  PRELOAD_FREE_MIDPAGE_FIRST=1
}

enable_midpage_m4packet_freefirst_tlslink_base() {
  enable_midpage_m4packet_freefirst_base
  PRELOAD_TLS_INITIAL_EXEC=1
  PRELOAD_SPEED_LINKFLAGS=1
}

enable_midpage_m4packet_freefirst_tlslink_allocelide_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE=1
}

enable_midpage_m4packet_freefirst_tlslink_ptrmag_base() {
  enable_midpage_m4packet_freefirst_tlslink_allocelide_base
  LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG=1
}

enable_midpage_m4packet_freefirst_tlslink_absalloc_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=1
}

enable_midpage_m4packet_freefirst_tlslink_regcache_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_TLS_REGION_CACHE=1
}

enable_midpage_m4packet_freefirst_tlslink_slotswitch_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_SLOT_SWITCH=1
}

enable_midpage_m4packet_freefirst_tlslink_m5hit_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1
}

enable_midpage_m4packet_freefirst_tlslink_superfast_base() {
  enable_midpage_m4packet_freefirst_tlslink_ptrmag_base
  PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=1
  PRELOAD_MIDPAGE_SUPERFAST=1
  LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1
}

enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base() {
  enable_midpage_m4packet_freefirst_tlslink_superfast_base
  LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE=1
}

enable_midpage_m6_deferred_free_direct_base() {
  enable_midpage_m4packet_freefirst_tlslink_m5hit_base
  LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE=1
}

enable_midpage_m4packet_freefirst_tlslink_flatcap_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP=1
}

enable_midpage_m4packet_freefirst_tlslink_overarray_base() {
  enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base
  LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY=1
}

enable_midpage_m4packet_freefirst_tlslink_wide32k_base() {
  enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base
  LINUX_MIDPAGEFRONT_WIDE_32K_CLASS=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base() {
  enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base
  LINUX_MIDPAGEFRONT_COARSE_BANDS="$1"
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rssgov_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base "$1"
  LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rssgov_base "$1"
  LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL=0
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_drainhit_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base "$1"
  LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base "$1"
  LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m7ticket_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base "$1"
  LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m8xfer_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base "$1"
  LINUX_MIDPAGEFRONT_M8_OWNER_XFER=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m9budget_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base "$1"
  LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m10slot_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base "$1"
  LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE=1
  LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m11direct_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base "$1"
  LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base "$1"
  LINUX_MIDPAGEFRONT_PAGERUN=1
}

# Saved MidPage/LargeFront profile family:
#   pagerun64           main/mid/cross64 candidate
#   pagerun64-takefirst cross-size fixed profile base
#   pagerun64-adaptive  diagnostic/no-go in mapped-bytes-only form
enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun_base "$1"
  LINUX_MIDPAGEFRONT_PAGERUN_64K=1
  LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP=4096
  LINUX_LARGEFRONT_REGION_BASE_FASTMAP=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base "$1"
  LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_largefirst_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base "$1"
  PRELOAD_FREE_MIDPAGE_FIRST=0
  PRELOAD_FREE_LARGE_FIRST=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_largefirst_base 2
  LINUX_LARGEFRONT_SOURCE_BATCH_COUNT="$1"
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch16_rbatch_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
  LINUX_LARGEFRONT_REMOTE_BATCH=1
  LINUX_LARGEFRONT_REMOTE_BATCH_CAP="$1"
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_adaptive128_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base "$1"
  LINUX_LARGEFRONT_ADAPTIVE128=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_scavenge_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base "$1"
  LINUX_LARGEFRONT_PAYLOAD_SCAVENGE=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_observe_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base "$1"
  LINUX_LARGEFRONT_OBSERVE=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l0_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 4
  LINUX_LARGEFRONT_POLICY_L0=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l1a_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_largefirst_base 2
  LINUX_LARGEFRONT_SOURCE_BATCH_COUNT=8
  LINUX_LARGEFRONT_POLICY_L1A=1
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l1b_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l1a_base
  LINUX_LARGEFRONT_REMOTE_BATCH=1
  LINUX_LARGEFRONT_REMOTE_BATCH_CAP=16
  LINUX_LARGEFRONT_POLICY_L1B=1
}

enable_midpage_m4packet_freefirst_tlslink_tagfree_base() {
  enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base
  PRELOAD_MIDPAGE_TAGGED_FREE=1
}

enable_midpage_m4packet_freefirst_tlslink_m4stats_base() {
  enable_midpage_m4packet_freefirst_tlslink_base
  LINUX_MIDPAGEFRONT_M4_STATS=1
}

enable_midpage_m4packet_routefree_base() {
  enable_midpage_m4packet_base
  PRELOAD_FREE_MIDPAGE_LARGE_FIRST=1
}

enable_midpage_m4packet_crossdrain_base() {
  enable_midpage_m4packet_base
  LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN=1
}

