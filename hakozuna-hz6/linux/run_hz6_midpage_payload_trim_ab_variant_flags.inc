apply_midpage_payload_trim_variant_flags() {
  local variant="$1"
  local flags_name="$2"
  case "$variant" in
    selected)
      ;;
    selected_scavenge_before_rss)
      ;;
    selected_malloc_trim_before_rss)
      ;;
    small_boundary_trusted_toy_map8192_scavenge_before_rss)
      apply_small_boundary_trusted_toy_map8192_flags flags
      ;;
    small_boundary_trusted_toy_map8192_malloc_trim_before_rss)
      apply_small_boundary_trusted_toy_map8192_flags flags
      ;;
    run512k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 524288
      ;;
    run256k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 262144
      ;;
    run224k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 229376
      ;;
    run192k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 196608
      ;;
    run128k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 131072
      ;;
    run320k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 327680
      ;;
    run384k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 393216
      ;;
    run448k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 458752
      ;;
    run768k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 786432
      ;;
    run1024k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1048576
      ;;
    run1536k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1572864
      ;;
    run1792k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1835008
      ;;
    run1920k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 1966080
      ;;
    run1984k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2031616
      ;;
    run2048k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2097152
      ;;
    run2112k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2162688
      ;;
    run2176k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2228224
      ;;
    run2240k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2293760
      ;;
    cold_retire)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      ;;
    cold_retire_lw1)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 1
      ;;
    cold_retire_lw256)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 256
      ;;
    cold_retire_eager)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER 65536
      ;;
    cold_retire_hw1024)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER 1024
      ;;
    cold_retire_hw3072)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER 3072
      ;;
    cold_retire_scan64)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL 64
      ;;
    cold_retire_max2)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL 2
      ;;
    cold_retire_max16)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL 16
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL 256
      ;;
    current_bias_fast)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1 1
      ;;
    current_bias_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 0
      ;;
    current_bias_2x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 2
      ;;
    current_bias_4x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 4
      ;;
    page_kind_selector_dryrun)
      hz6_preload_replace_define flags HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1 1
      ;;
    page_kind_selector_first)
      hz6_preload_replace_define flags HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1 1
      ;;
    phase_count_on)
      hz6_preload_replace_define flags HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 0
      ;;
    aligned_free_skip)
      hz6_preload_replace_define flags HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1 1
      ;;
    realloc_in_place_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_IN_PLACE_L1 0
      ;;
    realloc_boundary_slack)
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      ;;
    realloc_boundary_slack_4k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1 1
      ;;
    realloc_boundary_slack_8k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 1
      ;;
    realloc_boundary_adaptive_4k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1 1
      ;;
    realloc_boundary_adaptive_8k)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1 1
      ;;
    realloc_boundary_adaptive)
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1 1
      hz6_preload_replace_define flags \
        HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1 1
      ;;
    phase_count_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 1
      ;;
    preload_fast_free)
      hz6_preload_replace_define flags HZ6_PRELOAD_FAST_FREE_L1 1
      ;;
    frontcache8192)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_BIN_CAPACITY 8192
      ;;
    storage_trim_c4_8192)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 8192
      ;;
    storage_trim_c4_8192_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 8192
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    storage_trim_c4_12288_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 12288
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    storage_trim_c4_16384_c5_4096)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY 16384
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY 4096
      ;;
    boundary_inline)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1 0
      ;;
    boundary_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1 0
      ;;
    opt_o3)
      flags+=("-O3")
      ;;
    no_semantic_interposition)
      flags+=("-fno-semantic-interposition")
      ;;
    opt_o3_no_semantic_interposition)
      flags+=("-O3" "-fno-semantic-interposition")
      ;;
    boundary_min8k)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES 8192
      ;;
    boundary_min16k)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES 16384
      ;;
    midpage_direct_class)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1 1
      ;;
    midpage_boundary_fused)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 1
      ;;
    raw_frontcache_pop)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1 1
      ;;
    midpage_reuse_trusted_class)
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 1
      ;;
    midpage_reuse_trusted_class5)
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS 5
      ;;
    midpage_skip_transfer_first)
      hz6_preload_replace_define flags HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1 1
      ;;
    raw_frontcache_push)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      ;;
    raw_frontcache_push_class4)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS 4
      ;;
    raw_frontcache_push_class5)
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS 5
      ;;
    direct_max5)
      hz6_preload_replace_define flags HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS 5
      ;;
    prefill_descriptor_out)
      hz6_preload_replace_define flags HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1 1
      ;;
    same_owner_fast)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      ;;
    same_owner_trusted)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      ;;
    same_owner_trusted_class4)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MAX_CLASS 4
      ;;
    same_owner_trusted_class5)
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 1
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_SAME_OWNER_FAST_MAX_CLASS 5
      ;;
    preload_midpage_fast_free)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      ;;
    preload_midpage_fast_free_class4)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS 4
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS 4
      ;;
    preload_midpage_fast_free_class5)
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS 5
      ;;
    toy_free_fast)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1 1
      ;;
    toy_active_map_external)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      ;;
    frontcache_packed)
      hz6_preload_replace_define flags HZ6_FRONTCACHE_PACKED_META_L1 1
      ;;
    sourceblock_packed)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 1
      ;;
    frontcache_sourceblock_packed)
      hz6_preload_replace_define flags HZ6_FRONTCACHE_PACKED_META_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 1
      ;;
    toy_addr_envelope)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1 1
      ;;
    toy_preclassified_malloc)
      hz6_preload_replace_define flags HZ6_TOY_PRECLASSIFIED_MALLOC_L1 1
      ;;
    toy_direct_fast_reuse_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      ;;
    toy_trusted_default_off)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 0
      ;;
    preload_toy_direct_class)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
      ;;
    preload_toy_direct_class_fast_reuse)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      ;;
    preload_boundary_trusted_owner)
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    preload_toy_fast_reuse_trusted_owner)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    small_boundary_target)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      ;;
    small_boundary_raw_push)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      ;;
    small_boundary_trusted_owner|small_boundary_trusted)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    small_boundary_trusted_fused)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 1
      ;;
    small_boundary_trusted_toy_map8192)
      apply_small_boundary_trusted_toy_map8192_flags flags
      ;;
    small_boundary_trusted_toy_map8192_external)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      ;;
    small_boundary_trusted_toy_map8192_frontcache_packed)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_FRONTCACHE_PACKED_META_L1 1
      ;;
    small_boundary_trusted_toy_map8192_sourceblock_packed)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 1
      ;;
    small_boundary_trusted_toy_map8192_external_frontcache_packed)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      hz6_preload_replace_define flags HZ6_FRONTCACHE_PACKED_META_L1 1
      ;;
    small_boundary_trusted_toy_map8192_external_sourceblock_packed)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 1
      ;;
    small_boundary_trusted_toy_map8192_external_all_packed)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      hz6_preload_replace_define flags HZ6_FRONTCACHE_PACKED_META_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 1
      ;;
    small_boundary_trusted_toy_map8192_run8_256k)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 262144
      ;;
    small_boundary_trusted_toy_map8192_run8_384k)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 393216
      ;;
    small_boundary_trusted_toy_map8192_run8_512k)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 524288
      ;;
    small_boundary_trusted_toy_map8192_probe8)
      apply_small_boundary_trusted_toy_map8192_flags flags
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT 8
      ;;
    small_boundary_raw_push_trusted_owner|small_boundary_fast)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
      hz6_preload_replace_define flags HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1
      hz6_preload_replace_define flags HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 1
      ;;
    preload_toy_direct_class_max1024)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 1024
      ;;
    preload_toy_direct_class_max256)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 256
      ;;
    preload_toy_direct_class_max512)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 512
      ;;
    preload_toy_direct_class_fast_reuse_max256)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 256
      ;;
    preload_toy_direct_class_fast_reuse_max512)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 512
      ;;
    preload_toy_direct_class_max2048)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 2048
      ;;
    preload_toy_direct_class_max3072)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 3072
      ;;
    preload_toy_direct_class_min1025)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 1025
      ;;
    preload_toy_direct_class_fast_reuse_min1025)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 1025
      ;;
    preload_toy_direct_class_min2049)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 2049
      ;;
    preload_toy_direct_class_min3073)
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES 3073
      ;;
    runroute_dryrun_after_maps)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 0
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1 1
      ;;
    runroute_after_maps)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 0
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS 5
      hz6_preload_replace_define flags HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 1
      ;;
    toy_map64k)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 65536
      ;;
    toy_probe8)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT 8
      ;;
    toy_mask_index)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 1
      ;;
    toy_shift12_index)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 1
      ;;
    toy_shift12_mask)
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 1
      hz6_preload_replace_define flags HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 1
      ;;
    toy_prefill64)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 64
      ;;
    toy_prefill96)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 96
      ;;
    toy_prefill192)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 192
      ;;
    toy_prefill256)
      hz6_preload_replace_define flags HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS 256
      ;;
    sourcerun)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      ;;
    sourcerun_sameclass)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1 1
      ;;
    sourcerun_reclaim)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1 1
      ;;
    run2304k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2359296
      ;;
    run2560k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 2621440
      ;;
    run3072k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 3145728
      ;;
    run4096k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_RUN_BYTES 4194304
      ;;
    *)
      echo "unknown variant: ${variant}" >&2
      exit 2
      ;;
  esac
}
