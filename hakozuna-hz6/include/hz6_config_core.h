#ifndef HZ6_ROUTE_TABLE_CAPACITY
#define HZ6_ROUTE_TABLE_CAPACITY ((size_t)64)
#endif

#ifndef HZ6_ROUTE_PAGE_GRANULARITY
#define HZ6_ROUTE_PAGE_GRANULARITY ((size_t)4096)
#endif

#ifndef HZ6_TRANSFER_CACHE_CAPACITY
#define HZ6_TRANSFER_CACHE_CAPACITY ((size_t)64)
#endif

#ifndef HZ6_PROFILE_SPEED_TRANSFER_CAPACITY
#define HZ6_PROFILE_SPEED_TRANSFER_CAPACITY 64u
#endif

#ifndef HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY
#define HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY 128u
#endif

#ifndef HZ6_LARGE_SPAN_CENTRAL_CLASS_BYTES_CAP
#define HZ6_LARGE_SPAN_CENTRAL_CLASS_BYTES_CAP \
  ((size_t)8u * 1024u * 1024u)
#endif

#ifndef HZ6_LARGE_SPAN_CENTRAL_GLOBAL_BYTES_CAP
#define HZ6_LARGE_SPAN_CENTRAL_GLOBAL_BYTES_CAP \
  HZ6_LARGE_SPAN_CENTRAL_CLASS_BYTES_CAP
#endif

#ifndef HZ6_LARGE_DIRECT_RETAIN_L1
/* Experimental >1MiB direct-large retention.  Default keeps the low-RSS
 * direct-release contract; enabled lanes retain exact-size direct objects in
 * the large central pool up to the existing byte caps. */
#define HZ6_LARGE_DIRECT_RETAIN_L1 0
#endif

#ifndef HZ6_LARGE_DIRECT_RETAIN_BYTES_CAP
#define HZ6_LARGE_DIRECT_RETAIN_BYTES_CAP \
  HZ6_LARGE_SPAN_CENTRAL_CLASS_BYTES_CAP
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_L1
/* Linux SourceLayer retained-mmap cache.  Intended for LD_PRELOAD speed lanes:
 * release keeps exact-size mappings in a bounded process-local cache and
 * reserve reuses them before calling mmap. */
#define HZ6_LINUX_MMAP_RETAIN_L1 0
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT
#define HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT ((size_t)4096)
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_BYTES_CAP
#define HZ6_LINUX_MMAP_RETAIN_BYTES_CAP ((size_t)256u * 1024u * 1024u)
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_PURGE_ON_RELEASE_L1
#define HZ6_LINUX_MMAP_RETAIN_PURGE_ON_RELEASE_L1 0
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
#define HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1 0
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_TLS_L1
#define HZ6_LINUX_MMAP_RETAIN_TLS_L1 0
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT
#define HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT ((size_t)256)
#endif

#ifndef HZ6_LINUX_MMAP_RETAIN_TLS_BYTES_CAP
#define HZ6_LINUX_MMAP_RETAIN_TLS_BYTES_CAP ((size_t)32u * 1024u * 1024u)
#endif

#ifndef HZ6_LARGE_SPAN_TRUSTED_LOCAL_FREE_L1
/* Candidate-only LargeSpan local free shortcut.  hz6_free() has already
 * routed local frees through the local-owner branch before calling the front. */
#define HZ6_LARGE_SPAN_TRUSTED_LOCAL_FREE_L1 0
#endif

#ifndef HZ6_TRANSFER_SHARD_COUNT
#define HZ6_TRANSFER_SHARD_COUNT ((size_t)4)
#endif

#ifndef HZ6_OBJECT_DESCRIPTOR_CAPACITY
#define HZ6_OBJECT_DESCRIPTOR_CAPACITY ((size_t)64)
#endif

#ifndef HZ6_ALLOCATOR_VISIBILITY_CAPACITY
#define HZ6_ALLOCATOR_VISIBILITY_CAPACITY ((size_t)256)
#endif

#ifndef HZ6_SOURCE_BLOCK_CAPACITY
#define HZ6_SOURCE_BLOCK_CAPACITY ((size_t)16)
#endif

#ifndef HZ6_SOURCE_RUN_MAX_SLOTS
#define HZ6_SOURCE_RUN_MAX_SLOTS ((size_t)4096)
#endif

#ifndef HZ6_SOURCE_RUN_INLINE_META_L1
#define HZ6_SOURCE_RUN_INLINE_META_L1 1
#endif

#ifndef HZ6_TOY_SOURCE_BLOCK_BYTES
#define HZ6_TOY_SOURCE_BLOCK_BYTES ((size_t)65536)
#endif

#ifndef HZ6_MIDPAGE_RUN_BYTES
/* MidPage default keeps the R1 direct API seed conservative.  LD_PRELOAD
 * performance bundles may raise this to reduce 8K/32K source-run churn. */
#define HZ6_MIDPAGE_RUN_BYTES ((size_t)65536)
#endif

#ifndef HZ6_MIDPAGE_32K_RUN_BYTES
/* Candidate class-specific MidPage run size.  Default follows
 * HZ6_MIDPAGE_RUN_BYTES; preload A/B can raise only the 32K class without
 * changing the 8K guard. */
#define HZ6_MIDPAGE_32K_RUN_BYTES HZ6_MIDPAGE_RUN_BYTES
#endif

#ifndef HZ6_MIDPAGE_LOW_WATER_REFILL_L1
#define HZ6_MIDPAGE_LOW_WATER_REFILL_L1 0
#endif

#ifndef HZ6_MIDPAGE_8K_LOW_WATER_REFILL
#define HZ6_MIDPAGE_8K_LOW_WATER_REFILL ((size_t)128)
#endif

#ifndef HZ6_MIDPAGE_32K_LOW_WATER_REFILL
#define HZ6_MIDPAGE_32K_LOW_WATER_REFILL ((size_t)64)
#endif

#ifndef HZ6_SOURCE_RUN_BITMAP_WORDS
#define HZ6_SOURCE_RUN_BITMAP_WORDS \
  ((size_t)((HZ6_SOURCE_RUN_MAX_SLOTS + 63u) / 64u))
#endif

#ifndef HZ6_SOURCE_RUN_REUSE_L1
#define HZ6_SOURCE_RUN_REUSE_L1 0
#endif

#ifndef HZ6_SOURCE_RUN_SLOT_LOOKUP_L1
#define HZ6_SOURCE_RUN_SLOT_LOOKUP_L1 0
#endif

#ifndef HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1
#define HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1 0
#endif

#ifndef HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1
#define HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1
#define HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
#define HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
#define HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY
#define HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY ((size_t)4096)
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY
#define HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY \
  ((size_t)(HZ6_SOURCE_BLOCK_CAPACITY * 16u))
#endif

#ifndef HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
#define HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
#define HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1 0
#endif

#ifndef HZ6_DESCRIPTORLESS_FRONTCACHE_L1
#define HZ6_DESCRIPTORLESS_FRONTCACHE_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
#define HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1 0
#endif

#ifndef HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1
#define HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_COLD_GOV_L1
#define HZ6_DESCRIPTOR_COLD_GOV_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_AVAIL_COUNT_L1
#define HZ6_DESCRIPTOR_AVAIL_COUNT_L1 0
#endif

#ifndef HZ6_THIN_DESCRIPTOR_L1
#define HZ6_THIN_DESCRIPTOR_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_NO_BACKPTR_L1
#define HZ6_DESCRIPTOR_NO_BACKPTR_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_SIDE_OWNER16_L1
#define HZ6_DESCRIPTOR_SIDE_OWNER16_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
#define HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 0
#endif

#ifndef HZ6_OWNER_SOURCE_SIDE_META_DRYRUN
#define HZ6_OWNER_SOURCE_SIDE_META_DRYRUN 0
#endif

#ifndef HZ6_OWNER_SOURCE_SIDE_META_L2
#define HZ6_OWNER_SOURCE_SIDE_META_L2 0
#endif

#ifndef HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY
#if HZ6_THIN_DESCRIPTOR_L1
#define HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY ((size_t)4096)
#else
#define HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY ((size_t)0)
#endif
#endif

#ifndef HZ6_DESCRIPTOR_COLD_GOV_DETACH_BUDGET
#define HZ6_DESCRIPTOR_COLD_GOV_DETACH_BUDGET ((size_t)256)
#endif

#ifndef HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST
#define HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST 0
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_COMPACT_L1
#define HZ6_ROUTE_TOMBSTONE_COMPACT_L1 0
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_COMPACT_MIN
#define HZ6_ROUTE_TOMBSTONE_COMPACT_MIN ((size_t)1024)
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1
#define HZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1 0
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1
#define HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1 0
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
#define HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1 0
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN
#define HZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN ((size_t)1024)
#endif

#ifndef HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN
#define HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN ((size_t)1024)
#endif

#ifndef HZ6_ROUTE_RETAINED_OVERFLOW_L1
#define HZ6_ROUTE_RETAINED_OVERFLOW_L1 0
#endif

#ifndef HZ6_ROUTE_DOUBLE_HASH_L1
#define HZ6_ROUTE_DOUBLE_HASH_L1 0
#endif

#ifndef HZ6_ROUTE_HASH_XOR_FOLD_L1
#define HZ6_ROUTE_HASH_XOR_FOLD_L1 0
#endif

#ifndef HZ6_ROUTE_LINEAR_WRAP_L1
#define HZ6_ROUTE_LINEAR_WRAP_L1 0
#endif

#ifndef HZ6_ROUTE_LOOP_CARRY_L1
#define HZ6_ROUTE_LOOP_CARRY_L1 0
#endif

#ifndef HZ6_ROUTE_PACKED_META_L1
#define HZ6_ROUTE_PACKED_META_L1 0
#endif

#ifndef HZ6_ROUTE_LAST_HIT_CACHE_L1
/* Default exact-route shortcut.  The cache is invalidated on route mutation
 * and validates descriptor generation/state before returning VALID. */
#define HZ6_ROUTE_LAST_HIT_CACHE_L1 1
#endif

#ifndef HZ6_ROUTE_BYTES16_MINUS1_L2
#define HZ6_ROUTE_BYTES16_MINUS1_L2 0
#endif

#ifndef HZ6_FRONT_CACHE_CLASS_COUNT
#define HZ6_FRONT_CACHE_CLASS_COUNT 16u
#endif

#ifndef HZ6_FRONT_CACHE_BIN_CAPACITY
#define HZ6_FRONT_CACHE_BIN_CAPACITY ((size_t)8)
#endif

#ifndef HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY
#define HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY HZ6_FRONT_CACHE_BIN_CAPACITY
#endif

#ifndef HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY
#define HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY HZ6_FRONT_CACHE_BIN_CAPACITY
#endif

#ifndef HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
/* Default-off class-specific frontcache backing storage.  The selected
 * baseline still uses the flat [class][capacity] array; Ubuntu preload A/B can
 * enable this to reduce allocator-local fixed RSS. */
#define HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 0
#endif

#ifndef HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY ((size_t)128)
#endif

#ifndef HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY ((size_t)1024)
#endif

#ifndef HZ6_FRONT_CACHE_CLASS2_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS2_STORAGE_CAPACITY HZ6_FRONT_CACHE_BIN_CAPACITY
#endif

#ifndef HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY ((size_t)1024)
#endif

#ifndef HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY HZ6_FRONT_CACHE_BIN_CAPACITY
#endif

#ifndef HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY ((size_t)3072)
#endif

#ifndef HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY
#define HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY ((size_t)64)
#endif

#ifndef HZ6_DIAGNOSTIC_PROBES
#define HZ6_DIAGNOSTIC_PROBES 0
#endif

#ifndef HZ6_TOY_SMALL_HOTPATH_DIAG_L1
#define HZ6_TOY_SMALL_HOTPATH_DIAG_L1 0
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
#define HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1 1
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY
#define HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY ((size_t)8192)
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1
/* Control/profile RSS shape.  Keep the default inline table for the selected
 * preload lane; externalize only in explicit profiles to remove the Toy map
 * from the allocator's fixed static footprint. */
#define HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 0
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT
#define HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT ((size_t)4)
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_MAP_TRUSTED_OWNER_L1
/* Default active-map shortcut.  The map is allocator-local and validates
 * descriptor pointer/generation/state before bypassing the owner comparison. */
#define HZ6_TOY_SMALL_ACTIVE_MAP_TRUSTED_OWNER_L1 1
#endif

#ifndef HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1
/* Conservative negative filter for preload/core free paths.  The envelope only
 * widens on Toy active-map registration; stale-wide ranges fall back to the
 * normal bounded probe, so this cannot skip an owned Toy pointer. */
#define HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1 0
#endif

#ifndef HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
/* Candidate Toy active-map register shortcut.  The common insert/update case
 * lands on an empty or same-pointer base slot; collision cases keep the normal
 * bounded probe path. */
#define HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1 1
#endif

#ifndef HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1
/* Selected Toy active-map free shortcut.  After the raw frontcache pop
 * selected shape, base-slot-first lookup improves tiny/mixed and reduces
 * target route fallbacks without changing route safety counters. */
#define HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1 1
#endif

#ifndef HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1
/* Candidate Toy active-map index code-shape control.  The selected preload
 * capacity is power-of-two, so this replaces the modulo with a mask without
 * changing the hash domain.  Enable only with a power-of-two active-map
 * capacity. */
#define HZ6_TOY_ACTIVE_MAP_MASK_INDEX_L1 0
#endif

#ifndef HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1
/* Candidate Toy active-map index experiment.  Use page-granularity address
 * bits before mixing to test whether low slot bits are hurting the 4K-heavy
 * rows.  This is control-only because small Toy objects can share pages. */
#define HZ6_TOY_ACTIVE_MAP_SHIFT12_INDEX_L1 0
#endif

#ifndef HZ6_TOY_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1
/* Candidate trusted activation shortcut.  A descriptor popped from the local
 * frontcache already carries state/ptr/generation validation; for Toy-sized
 * descriptors, skip the repeated source-block bounds check on the hot path. */
#define HZ6_TOY_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1 1
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
/* Candidate-only MidPage free shortcut.  Keep separate from the Toy active map
 * so MidPage can use an alignment gate and class-specific capacity. */
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2
/* Store the MidPage active map outside Hz6Allocator when enabled.  This keeps
 * the allocator object smaller for preload thread-local allocators. */
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2
/* Allow MidPage active-map entries for slots whose mmap run base is not
 * naturally 8K aligned.  Exact pointer and descriptor generation/state
 * validation still guard the hit path. */
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY ((size_t)8192)
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT ((size_t)2)
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1
/* Candidate index for unaligned MidPage active-map slots.  The selected
 * unaligned lane can use 4K-distinct pointers, so keep this as an A/B knob
 * before changing the stable 8K-shift default. */
#define HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1
/* Candidate collision policy.  If the bounded probe range is full, keep the
 * existing active-map entries instead of evicting the base slot for the new
 * pointer. */
#define HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
/* Candidate register code shape.  The common MidPage insert/update case can
 * land on an empty or same-pointer base slot without the bounded probe loop. */
#define HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1
/* Candidate free code shape.  Test the base hash slot before entering the
 * bounded probe loop; collision cases keep the normal probe path. */
#define HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1
/* Candidate free code shape.  Use the base-slot-first free lookup only while
 * the allocator-local MidPage active map is larger than the Toy active map. */
#define HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1
/* Candidate collision control. Salt the MidPage active-map hash by 8K/32K
 * class and probe the dominant 32K class first on free. */
#define HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1
/* Candidate code shape.  The selected MidPage active map capacity is a power
 * of two, so hash and probe wrapping can use a mask instead of modulo. */
#define HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1 0
#endif

#ifndef HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1
/* Candidate malloc path. After a MidPage direct-local miss, prefill the run
 * and pop with descriptor ownership so active-map registration avoids an exact
 * route lookup. */
#define HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1 0
#endif

#ifndef HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1
/* Candidate MidPage front path. After a prefill_run succeeds, retry the local
 * frontcache without another transfer-first probe. */
#define HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1 0
#endif

#ifndef HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1
/* Candidate malloc path. MidPage alloc returns the activated descriptor to
 * active-map registration, avoiding a post-alloc exact route lookup without
 * changing prefill policy. */
#define HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1
/* Candidate negative filter. Track a conservative min/max pointer envelope for
 * registered MidPage active-map entries and skip hash probes outside it. */
#define HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1
/* Candidate overwrite policy. If the bounded probe window is full, prefer
 * evicting an existing entry from the same MidPage class before the base slot. */
#define HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1 0
#endif

#ifndef HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1
/* Candidate direct-reuse shortcut. Local-free MidPage descriptors already keep
 * their source block alive, so trusted-owner activation can skip the repeated
 * source-block bounds check on the hot reuse path. */
#define HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1 0
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1
/* Candidate free shortcut. After the MidPage active map has validated exact
 * ptr/generation/class/state, cache the descriptor directly into the matching
 * frontcache bin instead of routing through the generic cache helper. */
#define HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1 0
#endif

#ifndef HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1
/* Candidate malloc shortcut. For MidPage direct-local reuse, skip the
 * transfer-first probe before the local frontcache pop. The preload target rows
 * are local/free-cache dominated and have no transfer successes, so this tests
 * whether the empty transfer probe is pure overhead. */
#define HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1 0
#endif

#ifndef HZ6_TOY_CLASS_ID_FAST_ALLOC_L1
/* Default Toy alloc shortcut.  hz6_malloc() already selected class_id, so
 * Toy alloc can validate size against class bytes instead of classifying again. */
#define HZ6_TOY_CLASS_ID_FAST_ALLOC_L1 1
#endif

#ifndef HZ6_TOY_PRECLASSIFIED_MALLOC_L1
/* Candidate Toy malloc shortcut.  The allocation registry maps every
 * size <= 4096 and align <= 16 to Toy classes 0..4, but the direct branch
 * regressed the focused 1024..4096 preload row, so keep it off by default. */
#define HZ6_TOY_PRECLASSIFIED_MALLOC_L1 0
#endif

#ifndef HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1
/* Candidate preload-boundary Toy malloc shortcut.  Keep the broad hz6_malloc()
 * code shape unchanged and classify only LD_PRELOAD size <= 4096 requests
 * directly into Toy classes. */
#define HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 0
#endif

#ifndef HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES
#define HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES ((size_t)4096)
#endif

#ifndef HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES
#define HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES ((size_t)1)
#endif

#ifndef HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1
/* Candidate profile code shape.  Toy direct-class malloc can skip the generic
 * direct-local alloc front/transfer gates and go straight to local reuse. */
#define HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 0
#endif

#ifndef HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1
/* Candidate preload-boundary code shape.  The LD_PRELOAD TLS allocator is
 * initialized once and not destroyed during normal process lifetime, so profile
 * boundary helpers can skip their duplicate owner liveness check and rely on
 * the generic hz6_malloc() fallback for non-boundary paths. */
#define HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1 0
#endif

#ifndef HZ6_TOY_FULL_BLOCK_PREFILL_L1
/* Candidate-only Toy source miss policy.  When enabled, ToyFront asks the
 * source-block prefill path to consume more slots from the newly-created 64K
 * block instead of only the profile source_batch. */
#define HZ6_TOY_FULL_BLOCK_PREFILL_L1 0
#endif

#ifndef HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS
#define HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS ((size_t)256)
#endif

#ifndef HZ6_VISIBLE_FIRST_FREE_L1
#define HZ6_VISIBLE_FIRST_FREE_L1 0
#endif

#ifndef HZ6_NEGATIVE_FILTER_L1
#define HZ6_NEGATIVE_FILTER_L1 0
#endif

#ifndef HZ6_SHARED_ROUTE_DIRECTORY_L1
#define HZ6_SHARED_ROUTE_DIRECTORY_L1 0
#endif
