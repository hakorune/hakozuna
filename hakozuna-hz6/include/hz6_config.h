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

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY ((size_t)8192)
#endif

#ifndef HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT
#define HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT ((size_t)2)
#endif

#ifndef HZ6_TOY_CLASS_ID_FAST_ALLOC_L1
/* Default Toy alloc shortcut.  hz6_malloc() already selected class_id, so
 * Toy alloc can validate size against class bytes instead of classifying again. */
#define HZ6_TOY_CLASS_ID_FAST_ALLOC_L1 1
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

#ifndef HZ6_PRELOAD_REALLOC_IN_PLACE_L1
/* LD_PRELOAD realloc shortcut: if the requested size fits the current HZ6
 * usable descriptor bytes, return the same pointer instead of malloc/copy/free. */
#define HZ6_PRELOAD_REALLOC_IN_PLACE_L1 1
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
