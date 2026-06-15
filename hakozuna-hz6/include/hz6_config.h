#ifndef HZ6_CONFIG_H
#define HZ6_CONFIG_H

#include <stddef.h>

#ifndef HZ6_ROUTE_TABLE_CAPACITY
#define HZ6_ROUTE_TABLE_CAPACITY ((size_t)64)
#endif

#ifndef HZ6_ROUTE_PAGE_GRANULARITY
#define HZ6_ROUTE_PAGE_GRANULARITY ((size_t)4096)
#endif

#ifndef HZ6_TRANSFER_CACHE_CAPACITY
#define HZ6_TRANSFER_CACHE_CAPACITY ((size_t)64)
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

#ifndef HZ6_SHARED_ROUTE_DIRECTORY_FIRST_L1
#define HZ6_SHARED_ROUTE_DIRECTORY_FIRST_L1 0
#endif

#ifndef HZ6_ELASTIC_ROUTE_OVERFLOW_L1
#define HZ6_ELASTIC_ROUTE_OVERFLOW_L1 0
#endif

#ifndef HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
#define HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1 0
#endif

#ifndef HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
#define HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1 0
#endif

#ifndef HZ6_ELASTIC_SOURCE_BLOCK_LOCALIZE_DRYRUN_L1
#define HZ6_ELASTIC_SOURCE_BLOCK_LOCALIZE_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1
#define HZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1
#define HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1 0
#endif

#ifndef HZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1
#define HZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1
#define HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1 0
#endif

#ifndef HZ6_ELASTIC_SLOT_OWNER_CONSUMER_DRYRUN_L1
#define HZ6_ELASTIC_SLOT_OWNER_CONSUMER_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_SLOT_OWNER_LOGICAL_FASTPATH_L1
#define HZ6_ELASTIC_SLOT_OWNER_LOGICAL_FASTPATH_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_DRAIN_DRYRUN_L1
#define HZ6_ELASTIC_DEPOT_DRAIN_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_SLOT_LOCALIZE_L1
#define HZ6_ELASTIC_DEPOT_SLOT_LOCALIZE_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_SLOT_TRANSFER_SCOPED_L1
#define HZ6_ELASTIC_DEPOT_SLOT_TRANSFER_SCOPED_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1
#define HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1
#define HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_L1
#define HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET_L1
#define HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET_L1 0
#endif

#ifndef HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET
#define HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET ((size_t)2048)
#endif

#ifndef HZ6_ELASTIC_DFTLC_REHOME_INTERSECTION_DRYRUN_L0
#define HZ6_ELASTIC_DFTLC_REHOME_INTERSECTION_DRYRUN_L0 0
#endif

#ifndef HZ6_OWNER_EQUAL_CALLSITE_DRYRUN_L1
#define HZ6_OWNER_EQUAL_CALLSITE_DRYRUN_L1 0
#endif

#ifndef HZ6_FREE_LOCAL_CACHE_OWNER_PREDICATE_DRYRUN_L0
#define HZ6_FREE_LOCAL_CACHE_OWNER_PREDICATE_DRYRUN_L0 0
#endif

#ifndef HZ6_DEPOT_DESCRIPTOR_OWNER_EQUAL_FASTPATH_L1
#define HZ6_DEPOT_DESCRIPTOR_OWNER_EQUAL_FASTPATH_L1 0
#endif

#ifndef HZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1
#define HZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1 0
#endif

#ifndef HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY
#define HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY ((size_t)131072)
#endif

#ifndef HZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY
#define HZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY ((size_t)196608)
#endif

#ifndef HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY
#define HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY ((size_t)16384)
#endif

#ifndef HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY
#define HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY ((size_t)262144)
#endif

#ifndef HZ6_OWNER_LOCALITY_INDEX_CAPACITY
#define HZ6_OWNER_LOCALITY_INDEX_CAPACITY HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY
#endif

#ifndef HZ6_OWNER_LOCALITY_INDEX_L1
#define HZ6_OWNER_LOCALITY_INDEX_L1 0
#endif

#ifndef HZ6_LOCAL_EXACT_FIRST_FREE_L1
#define HZ6_LOCAL_EXACT_FIRST_FREE_L1 0
#endif

/* ToyDirectMapTrusted max4 Linux/Ubuntu default:
 * direct local free/alloc/reuse plus trusted local-cache ownership, bounded to
 * a small inclusive class-id window. */
#ifndef HZ6_LOCAL_CACHE_DIRECT_FREE_L1
#define HZ6_LOCAL_CACHE_DIRECT_FREE_L1 1
#endif

#ifndef HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
#define HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 1
#endif

#ifndef HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
#define HZ6_LOCAL_CACHE_DIRECT_REUSE_L1 1
#endif

#ifndef HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1
/* Selected production direct-reuse shortcut.  Diagnostic builds still use the
 * wrapper path so frontcache attribution counters remain available. */
#define HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1 0
#endif

#ifndef HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1
/* Candidate MidPage preload-boundary success-path shape.  The caller has
 * already selected a valid MidPage class and passes a non-null descriptor out
 * pointer, so the local-reuse helper can avoid generic entry checks while
 * preserving descriptor activation validation. */
#define HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 0
#endif

#ifndef HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS
/* Inclusive class gate for trusted-class reuse experiments.  The default covers
 * both MidPage 8K/32K classes; class5-only controls can isolate the 32K path. */
#define HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS \
  HZ6_MIDPAGE_8K_CLASS_ID
#endif

#ifndef HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1
/* Candidate production trusted-free shortcut.  Diagnostic and cold-retire
 * builds keep the wrapper path so frontcache counters and retire hooks remain
 * available. */
#define HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 0
#endif

#ifndef HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS
#define HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS 0
#endif

#ifndef HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS
#define HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS 15u
#endif

#ifndef HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS
/* Inclusive class-id gate.  The default max4 covers class IDs 0..4. */
#define HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS 4
#endif

#ifndef HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
/* Default same-allocator local-cache shortcut.  Callers must already be
 * on an allocator-owned frontcache path or have checked local ownership. */
#define HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1 1
#endif

#ifndef HZ6_SAME_OWNER_FAST_L1
#define HZ6_SAME_OWNER_FAST_L1 0
#endif

#ifndef HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1
/* Candidate-only shortcut for same-owner free paths.  hz6_free() must have
 * already proven descriptor ownership before this path skips the second owner
 * equality check in the same-owner cache transition. */
#define HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1 0
#endif

#ifndef HZ6_SAME_OWNER_FAST_MIN_CLASS
#define HZ6_SAME_OWNER_FAST_MIN_CLASS 0
#endif

#ifndef HZ6_SAME_OWNER_FAST_MAX_CLASS
#define HZ6_SAME_OWNER_FAST_MAX_CLASS HZ6_FRONT_CACHE_CLASS_COUNT
#endif

#ifndef HZ6_FRONTCACHE_SPILL_ON_DESCRIPTOR_EXHAUSTION
#define HZ6_FRONTCACHE_SPILL_ON_DESCRIPTOR_EXHAUSTION 0
#endif

#ifndef HZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS
#define HZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS 0
#endif

#ifndef HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1
/* Candidate MidPage-only borrow path.  When the 8K MidPage bin misses, try a
 * 32K local-free entry before allocating new source.  This keeps the broad
 * larger-bin borrow off for Toy/small rows. */
#define HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1 0
#endif

#ifndef HZ6_FRONTCACHE_CAP_ON_FREE
#define HZ6_FRONTCACHE_CAP_ON_FREE 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ACTIVATION_ROUTE_REPAIR_L1
/* Diagnostic-only repair probe.  Do not promote as-is: the current repair path
 * is intentionally narrow and registers repaired source-run slots as TOY. */
#define HZ6_SOURCE_BLOCK_ACTIVATION_ROUTE_REPAIR_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_RELEASE_LIVE_GUARD_L1
/* Diagnostic-only lifetime guard for source-block release investigations. */
#define HZ6_SOURCE_BLOCK_RELEASE_LIVE_GUARD_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1
/* Experimental source-run free route shortcut.  Requires range-index and
 * slot-descriptor map; exact route remains the fallback/control path. */
#define HZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS
/* Class gate for SourceBlockRoute behavior experiments.  The default preserves
 * the original all-class behavior; selected-small variants can narrow this to
 * keep larger fixed-size classes on the exact-route fallback. */
#define HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS HZ6_FRONT_CACHE_CLASS_COUNT
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1
/* Keep Toy/small on exact-route fallback when testing SourceBlockRoute as a
 * MidPage/small-mid shortcut.  Toy rows can be too small for the source-block
 * route arithmetic to pay for itself. */
#define HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 1
#endif

#ifndef HZ6_PRELOAD_FAST_FREE_L1
/* Candidate-only LD_PRELOAD free shortcut.  Reuses the preload ownership route
 * in HZ6's internal free dispatch instead of routing again in hz6_free(). */
#define HZ6_PRELOAD_FAST_FREE_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1
/* Candidate preload-boundary shortcut.  When preload already found a local
 * MidPage exact route, re-arm the MidPage active map so hz6_free() can consume
 * the descriptor without repeating the full route path. */
#define HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1 0
#endif

#ifndef HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
/* Diagnostic-only LD_PRELOAD probe.  After Toy/MidPage active maps miss, try
 * descriptor-validated source-run metadata and then keep the normal RouteLayer
 * lookup. */
#define HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1 0
#endif

#ifndef HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1
/* Candidate LD_PRELOAD free shortcut.  After active maps miss, use existing
 * source-run metadata only for validated MidPage routes, with RouteLayer
 * fallback preserved on any miss/mismatch. */
#define HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
/* Candidate preload-boundary shortcut.  Unlike HZ6_PRELOAD_FAST_FREE_L1, this
 * only reuses preload's exact route for local MidPage frees. */
#define HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS
#define HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS
#define HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS 15u
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1
/* Candidate preload free ordering.  Try the MidPage active map before the Toy
 * active map to test whether MidPage-heavy rows can avoid the Toy miss wall. */
#define HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1
/* Candidate selective preload free ordering.  Try MidPage before Toy only for
 * pointers that pass the MidPage active-map alignment gate. */
#define HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1
/* Candidate selective preload free ordering.  Try MidPage before Toy only when
 * the allocator currently has more MidPage active-map entries than Toy
 * active-map entries. */
#define HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR
#define HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR ((size_t)1)
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DENOMINATOR
#define HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DENOMINATOR ((size_t)1)
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DELTA
#define HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DELTA ((size_t)0)
#endif

#ifndef HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
/* Production DSO code-shape control.  When enabled, preload hook phase
 * counters are compiled out instead of runtime-gated by HZ6_PRELOAD_STATS. */
#define HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1
/* Code-shape control for the selected 1:1 current-bias predicate.  Semantics
 * match the default numerator=1, denominator=1, delta=0 configuration. */
#define HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1
/* Diagnostic-only selective MidPage-first free hint. */
#define HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1
/* Diagnostic-only page-table shape for the selective MidPage-first free hint.
 * Reuses the preload hook mh_* counters and does not change free ordering. */
#define HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1
/* Behavior control for selective MidPage-first preload free ordering.  Only
 * hinted pages try the MidPage active map before the Toy active map. */
#define HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1 0
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY
#define HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY 32768
#endif

#ifndef HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT
#define HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT 4
#endif

#ifndef HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1
/* Diagnostic-only allocator-local page-kind selector.  Active-map
 * registration records whether a page most recently carried Toy, MidPage, or
 * mixed live entries; preload free only compares the advisory prediction with
 * the actual active-map hit and never changes ownership or routing behavior. */
#define HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1 0
#endif

#ifndef HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1
/* Behavior control for the advisory page-kind selector.  Toy/MidPage page
 * predictions only choose the first active-map probe; unknown/mixed pages keep
 * the selected current-bias order and all misses fall through to RouteLayer. */
#define HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1 0
#endif

#if HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1 || \
    HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1
#define HZ6_PAGE_KIND_FREE_SELECTOR_ACTIVE_L1 1
#else
#define HZ6_PAGE_KIND_FREE_SELECTOR_ACTIVE_L1 0
#endif

#ifndef HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY
#define HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY 32768
#endif

#ifndef HZ6_PAGE_KIND_FREE_SELECTOR_PROBE_LIMIT
#define HZ6_PAGE_KIND_FREE_SELECTOR_PROBE_LIMIT 4
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1
/* Candidate preload-boundary MidPage malloc shortcut.  This keeps the selected
 * hz6_malloc() code shape clean while allowing a target DSO to skip the empty
 * transfer-first probe for MidPage direct local reuse. */
#define HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
/* Candidate code-shape isolation for the MidPage preload malloc boundary. */
#define HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES
/* Control for the preload-boundary MidPage shortcut lower bound.  The selected
 * default keeps both 8K and 32K MidPage classes on the shortcut path. */
#define HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES ((size_t)4096)
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1
/* Candidate preload-boundary code-shape control.  The LD_PRELOAD boundary
 * already limits this helper to 4097..32768-byte requests, so classify directly
 * into MidPage 8K/32K instead of building a policy struct. */
#define HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1 0
#endif

#ifndef HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1
/* Candidate selected-boundary code-shape control.  The preload hook has already
 * checked the MidPage size envelope, so pass the resolved class into a narrow
 * MidPage helper instead of rechecking the generic size policy. */
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 0
#endif

#ifndef HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1
/* Candidate front prefill code shape.  Source-block prefill already prepared
 * the descriptor before registering the exact route; return that descriptor to
 * the cache step instead of doing a self route lookup on the freshly-created
 * slot. */
#define HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1 0
#endif

#ifndef HZ6_MIDPAGE_32K_COLD_RETIRE_L1
/* Default-off RSS control for cold MidPage 32K source blocks.  When the 32K
 * frontcache reaches a high-water mark, an out-of-line helper can drain one or
 * more all-local-free source blocks and release their backing. */
#define HZ6_MIDPAGE_32K_COLD_RETIRE_L1 0
#endif

#ifndef HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER
#define HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER ((size_t)2048)
#endif

#ifndef HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL
#define HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL ((size_t)1)
#endif

#ifndef HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL
#define HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL ((size_t)32)
#endif

#ifndef HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER
/* Retire only near quiescence by default.  Eager release reduced retained
 * payload but caused immediate source churn on active MidPage workloads. */
#define HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER ((size_t)1)
#endif

#ifndef HZ6_PRELOAD_REALLOC_IN_PLACE_L1
/* LD_PRELOAD realloc shortcut: if the requested size fits the current HZ6
 * usable descriptor bytes, return the same pointer instead of malloc/copy/free. */
#define HZ6_PRELOAD_REALLOC_IN_PLACE_L1 1
#endif

#ifndef HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
/* Default-off preload control for pointers returned by real aligned allocation
 * fallbacks.  When enabled, posix_memalign/aligned_alloc real-fallback results
 * are tracked so free() can skip the HZ6 route lookup and call real free
 * directly. */
#define HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1 0
#endif

#ifndef HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY
#define HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY ((size_t)65536)
#endif

#ifndef HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1
/* Default-off LD_PRELOAD profile/control.  Boundary-sized requests that are
 * likely to be reallocated by small growth patterns can be placed into the
 * next MidPage slot class so realloc(size + small_delta) stays in-place. */
#define HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 0
#endif

#ifndef HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1
#define HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1 \
  HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1
#endif

#ifndef HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1
#define HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 \
  HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1
#endif

#ifndef HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
/* Default-off LD_PRELOAD control.  After a Toy 4K -> MidPage realloc copy is
 * observed on this thread, future exact 4K mallocs use the 8K MidPage slot so
 * similar growth can stay in-place without taxing workloads that never realloc
 * across the boundary. */
#define HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1 0
#endif

#ifndef HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
/* Default-off sibling for MidPage 8K -> 32K realloc growth. */
#define HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1 0
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_DRYRUN_L1
/* Diagnostic-only probe for a future SmallRunFront/TinyRunRoute design. */
#define HZ6_SMALL_RUN_ROUTE_DRYRUN_L1 0
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_BEHAVIOR_L1
/* Narrow Toy/small source-run route shortcut.  This is intentionally separate
 * from generic SourceBlockRoute: exact route remains the fallback, and only a
 * fully proven active source-run slot returns VALID. */
#define HZ6_SMALL_RUN_ROUTE_BEHAVIOR_L1 0
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES
/* HZ6's current small class geometry rounds the 2K row into a 4K slot class. */
#define HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES ((size_t)4096)
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES
/* Optional lower gate for size-split SmallRunRoute experiments. */
#define HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES ((size_t)0)
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_TOY_RANGE_ONLY_L1
/* Register range-index entries only for Toy source-runs when the range index
 * exists solely to feed SmallRunRoute. This avoids taxing MidPage/Large rows
 * with a SmallRunRoute hit-then-fallback. */
#define HZ6_SMALL_RUN_ROUTE_TOY_RANGE_ONLY_L1 0
#endif

#ifndef HZ6_SMALL_RUN_ROUTE_ARMED_L1
/* Skip the SmallRunRoute range-index lookup until at least one eligible
 * source-run range has been registered. This removes the empty-table probe on
 * pure non-Toy rows without adding a second prefilter table. */
#define HZ6_SMALL_RUN_ROUTE_ARMED_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1
/* Register SourceBlockRoute range entries after source-run class selection.
 * This lets class-gated SourceBlockRoute lanes avoid range-index hits on
 * classes that should fall back to the normal exact route. */
#define HZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1 0
#endif

#ifndef HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
/* Store source-run slot descriptor maps only for active runs instead of
 * embedding HZ6_SOURCE_RUN_MAX_SLOTS entries in every source block. */
#define HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1 0
#endif

#ifndef HZ6_FRONTCACHE_PACKED_META_L1
#define HZ6_FRONTCACHE_PACKED_META_L1 0
#endif

#endif
