#!/usr/bin/env bash
# Sourced by hz5_build_arg_parser.sh after build profile helper functions exist.
# Human-facing profile aliases live here; low-level flags stay in the parser.

hz5_try_apply_profile_alias() {
  case "$1" in
    --linux-hz5-profile-pagerun64-main)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base 2
      ;;
    --linux-hz5-profile-pagerun64-cross128)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base 2
      LINUX_LARGEFRONT_SOURCE_BATCH_COUNT=16
      ;;
    --linux-hz5-profile-pagerun64-large128|--linux-hz5-profile-large128-rss)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 4
      ;;
    --linux-hz5-profile-pagerun64-large128-batch8)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 8
      ;;
    --linux-hz5-profile-pagerun64-large128-batch16|--linux-hz5-profile-large128-source16)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      ;;
    --linux-hz5-profile-large128-ownerfast)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-drain1|--linux-hz5-profile-large128-r50-drain)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      ;;
    --linux-hz5-profile-large128-drainbulk)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_DRAIN_BULK_LOCAL=1
      ;;
    --linux-hz5-profile-large128-draintrust)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_DRAIN_TRUST_REMOTE_STATE=1
      ;;
    --linux-hz5-profile-large128-transfer128)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_TRANSFER128=1
      LINUX_LARGEFRONT_TRANSFER128_CAP=128
      LINUX_LARGEFRONT_TRANSFER128_CHUNK_CAP=16
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-takeonly)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_DRAIN_TAKE_ONLY=1
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-popbudget1)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_DRAIN_POP_BUDGET=1
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-remotehold4)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_DRAIN_TAKE_ONLY=1
      LINUX_LARGEFRONT_REMOTE_HOLD=1
      LINUX_LARGEFRONT_REMOTE_HOLD_CAP=4
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-drain1-hold4|--linux-hz5-profile-large128-r50-hold)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_REMOTE_HOLD=1
      LINUX_LARGEFRONT_REMOTE_HOLD_CAP=4
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-drain1-hold8|--linux-hz5-profile-large128-r50-hold8)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_REMOTE_HOLD=1
      LINUX_LARGEFRONT_REMOTE_HOLD_CAP=8
      ;;
    --linux-hz5-profile-large128-direct-header)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_DIRECT_HEADER_LOOKUP=1
      ;;
    --linux-hz5-profile-large128-base-directmap)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_BASE_DIRECTMAP=1
      ;;
    --linux-hz5-profile-large128-r50-drain-directmap)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_BASE_DIRECTMAP=1
      ;;
    --linux-hz5-profile-large128-global-remote)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_REMOTE_GLOBAL_128K=1
      ;;
    --linux-hz5-profile-large128-remote-first)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_REMOTE_FIRST_128K=1
      ;;
    --linux-hz5-profile-large128-remote-first-gated)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_REMOTE_FIRST_GATED_128K=1
      ;;
    --linux-hz5-profile-large128-chunk16)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_REMOTE_BATCH=1
      LINUX_LARGEFRONT_REMOTE_BATCH_CAP=16
      LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX=1
      LINUX_LARGEFRONT_REMOTE_CHUNK_CAP=16
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-policy-l7|--linux-hz5-profile-large128-policy-l7)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_REMOTE_HOLD=1
      LINUX_LARGEFRONT_REMOTE_HOLD_CAP=4
      LINUX_LARGEFRONT_POLICY_L7=1
      LINUX_LARGEFRONT_POLICY_L7_REMAINDER_LOCAL_THRESHOLD=32
      ;;
    --linux-hz5-profile-large128-policy-l8-shadow)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch_base 16
      LINUX_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET=1
      LINUX_LARGEFRONT_REMOTE_HOLD=1
      LINUX_LARGEFRONT_REMOTE_HOLD_CAP=4
      LINUX_LARGEFRONT_POLICY_L0=1
      LINUX_LARGEFRONT_POLICY_L8_SHADOW=1
      LINUX_LARGEFRONT_POLICY_L8_HEAVY_SPANS=32
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-rb32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch16_rbatch_base 32
      ;;
    --linux-hz5-profile-pagerun64-large128-b16-rb64)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_large128_batch16_rbatch_base 64
      ;;
    --linux-hz5-profile-pagerun64-large128-policy-l0)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l0_base 2
      ;;
    --linux-hz5-profile-pagerun64-large128-policy-l1a)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l1a_base 2
      ;;
    --linux-hz5-profile-pagerun64-large128-policy-l1b)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_policy_l1b_base
      ;;
    *)
      return 1
      ;;
  esac

  return 0
}
