# HZ6 Current Task

HZ6 is now in R1 implementation. Keep the implementation modular from the
start: API, route, frontcache, transfer, source, owner, policy, and future
fronts should stay separated.

## Current Claim

```text
HZ6 should be a route-safe, transfer-first, RSS-aware allocator family.

It should promote:
  HZ3 thin local cache
  HZ4 remote grouping
  HZ5 fail-closed descriptor ownership and low-RSS profile discipline

It should not directly port HZ3/HZ4/HZ5 internals.
```

## Active Documents

```text
hakozuna-hz6/README.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_STATUS.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
```

## Proposed First Engineering Steps

```text
1. Review and tighten HZ6 folder layout. DONE.
2. Review HZ6_BLUEPRINT.md and select the first prototype. DONE.
3. Add headers/contracts first. DONE:
   include/hz6_contract.h
   include/hz6.h
   api/hz6_allocator.h
   route/hz6_route.h
   transfer/hz6_transfer.h
   source/hz6_source.h
4. Add route smoke before allocator behavior. DONE.
5. Add transfer smoke before full malloc/free integration. DONE.
6. Add first toy allocation path. DONE:
   api/hz6_allocator.c
   source/hz6_source.c
   frontcache/hz6_frontcache.c
   route/hz6_route.c
7. Add first transfer-first toy path. DONE:
   hz6_free_remote()
   ACTIVE -> TRANSFER_FREE
   transfer pop before source allocation
8. Choose first real target. DONE:
   Linux LargeFront 128K transfer seed was selected first.
9. Keep modular cleanup moving before real target:
   size-class mapping moved to frontcache/hz6_size_class.*
   toy allocation/free transitions moved to fronts/toy/
   allocator API calls toy front through Hz6FrontOps
   front selection moved to fronts/hz6_front.*
10. First real-front seed. DONE:
   fronts/large/hz6_large128_front.*
   handles >4KiB..128KiB through the front registry
   uses transfer-first reuse in HZ6_PROFILE_REMOTE
11. Front utility common path. DONE:
   fronts/hz6_front_util.*
   removes duplicated descriptor/cache/transfer transitions from toy/large
12. Front range correctness. DONE:
   toy front limited to <=4KiB
   Large128 front handles >4KiB..128KiB
   unsupported >128KiB allocation returns NULL in R1 smoke
13. Build script hygiene. DONE:
   linux/build_hz6_r1_smokes.sh uses source/include arrays
   new modules should be added to HZ6_SOURCES/HZ6_INCLUDES explicitly
14. Smoke split. DONE:
   tests/hz6_r1_core_contract_smoke.c now covers owner/profile/frontcache
   after later route/transfer/source smoke splits
   tests/hz6_r1_allocator_smoke.c covers allocator/front integration
15. Linux SourceLayer seed. DONE:
   source/linux_source_mmap.*
   reserve/commit/decommit/release covered by contract smoke
16. Large128 SourceLayer backing. DONE:
   descriptors carry source kind / release metadata
   Large128 uses Linux mmap source ops in the Linux R1 smoke
   destroy and cache overflow release through descriptor SourceLayer metadata
17. Allocator-level SourceRegistry. DONE:
   source/hz6_source_registry.*
   allocator owns system/Linux source selection
   fronts name source kind instead of including OS-specific source backends
18. Local2P exact seed front. DONE:
   fronts/local2p/hz6_local2p_front.*
   exact 64KiB requests route to HZ6_FRONT_LOCAL2P before Large128
   remote transfer reuse is covered by allocator smoke
19. Route backend seam. DONE:
   route/hz6_route_backend.*
   allocator and front util use backend wrapper instead of direct table calls
   exact table remains the only backend, covered by contract smoke
20. Transfer backend seam. DONE:
   transfer/hz6_transfer_backend.*
   allocator and front util use backend wrapper instead of direct cache calls
   single-cache remains the only backend, covered by contract smoke
21. Sharded transfer backend seed. DONE:
   transfer backend can split one object buffer into fixed shards
   speed/remote profiles select sharded transfer
   contract smoke covers bounded push, class pop, and aggregate count
22. Descriptor owner token lifecycle seed. DONE:
   allocator has an owner record
   descriptors carry owner tokens while ACTIVE / LOCAL_FREE
   remote transfer clears owner and transfer pop restores it
   allocator smoke covers owner assign / clear / restore
23. Owner-dead orphan seed. DONE:
   hz6_allocator_mark_owner_dead()
   owned ACTIVE / LOCAL_FREE descriptors become ORPHAN
   dead owner malloc is rejected
   orphan free is fail-closed in allocator smoke
24. Orphan release path. DONE:
   hz6_allocator_release_orphan()
   unregisters route and releases descriptor SourceLayer backing
   allocator smoke verifies route miss and descriptor DEAD after release
25. OS-paged SourceLayer abstraction. DONE:
   HZ6_SOURCE_OS_PAGED
   Linux maps OS-paged to mmap
   Windows maps OS-paged to VirtualAlloc behind _WIN32
   Local2P/Large128 fronts no longer name Linux-specific source kind
26. MidPage seed front. DONE:
   fronts/midpage/hz6_midpage_front.*
   >4KiB..32KiB requests route to HZ6_FRONT_MIDPAGE
   remote transfer reuse is covered by allocator smoke
27. Dedicated safety smoke. DONE:
   tests/hz6_r1_safety_smoke.c
   covers interior invalid, foreign miss, local/remote double-free, owner-dead
   orphan rejection, and orphan release
28. R1 smoke runner naming. DONE:
   linux/build_hz6_r1_smokes.sh is the canonical R1 runner
   linux/build_hz6_r1_contract_smoke.sh remains as a compatibility wrapper
29. ScavengeLayer seed. DONE:
   scavenge/hz6_scavenge.*
   bounded release accounting is covered by contract smoke
   hz6_allocator_scavenge_orphans() releases ORPHAN descriptors only
   safety smoke covers over-budget retention and route/source release
30. Frontcache invalidation for scavenging. DONE:
   hz6_frontcache_remove()
   hz6_allocator_scavenge_local_free() removes stale cache entries before
   releasing LOCAL_FREE descriptors
   safety smoke covers over-budget local retention and route/source release
31. Slow-path scavenge policy hook. DONE:
   Hz6ProfileConfig carries scavenge_local_free_bytes / scavenge_orphan_bytes
   hz6_allocator_scavenge_profile() applies profile budgets explicitly
   RSS profile scavenges cached objects, strict profile keeps auto scavenge off
32. Cross-owner orphan adoption. DONE:
   hz6_allocator_adopt_orphan()
   dead-owner ORPHAN descriptors can move to a live allocator as LOCAL_FREE
   ACTIVE descriptors cannot be adopted
   allocator smoke covers adoption reuse and source-route removal
33. Transfer observability seed. DONE:
   hz6_transfer_count_class()
   hz6_transfer_backend_count_class()
   hz6_transfer_backend_shard_count_at()
   contract smoke verifies class visibility and shard distribution
34. Profile transfer capacity wiring. DONE:
   Hz6ProfileConfig.transfer_capacity now selects transfer backend capacity
   capacity is capped by HZ6_TRANSFER_CACHE_CAPACITY
   allocator smoke covers RSS profile capacity and capped remote profile capacity
35. Explicit source prefill seed. DONE:
   hz6_front_prefill_source_kind()
   uses profile.source_batch from slow-path tests without changing malloc hit path
   allocator smoke covers RSS profile source_batch prefill and no hidden refill
36. Route backend variant seed. DONE:
   HZ6_ROUTE_BACKEND_PAGE_TABLE
   shares MISS / VALID / INVALID result contract with exact-table backend
   contract smoke covers register, valid lookup, invalid interior, unregister
37. Strict remote pending drain. DONE:
   strict_owner_remote uses ACTIVE -> REMOTE_PENDING
   hz6_allocator_drain_remote_pending() moves REMOTE_PENDING -> LOCAL_FREE
   allocator and safety smoke cover strict remote reuse after explicit drain
38. Remote-pending owner-death safety. DONE:
   hz6_allocator_mark_owner_dead() now converts REMOTE_PENDING to ORPHAN
   hz6_allocator_drain_remote_pending() rejects dead owners
   safety smoke covers pending remote orphan release after owner death
39. Page route granularity contract. DONE:
   HZ6_ROUTE_BACKEND_PAGE_TABLE now carries an explicit page granularity
   lookup filters entries by page envelope before preserving VALID / INVALID / MISS
   contract smoke covers custom page granularity and object-end MISS
40. MidPage page-run policy seed. DONE:
   fronts/midpage/hz6_midpage_front.* now owns an 8K / 32K page-run geometry
   hz6_midpage_policy_for_size() exposes class, slot bytes, run bytes, slots/run
   allocator smoke covers 8K class routing, descriptor bytes, and local reuse
41. Descriptor source pointer split. DONE:
   Hz6ObjectDescriptor now stores user ptr and source_ptr separately
   release uses source_ptr/source_bytes while route/cache still use user ptr
   allocator smoke covers user ptr != source_ptr release behavior
42. Source-block slot helper. DONE:
   hz6_front_source_slot_kind()/ops() allocate a source block and expose one user slot
   descriptor records user bytes separately from source bytes
   allocator smoke covers MidPage 8K slot inside a 64KiB source block
43. Shared SourceBlock ownership seed. DONE:
   Hz6SourceBlock tracks source block release metadata and descriptor references
   hz6_front_source_block_slot() registers multiple user slots against one block
   allocator smoke verifies first slot release does not release the shared source block
44. MidPage run prefill seed. DONE:
   hz6_midpage_prefill_run() creates one 64KiB SourceBlock for 8K / 32K classes
   prefilled slots enter the MidPage local cache as LOCAL_FREE descriptors
   allocator smoke verifies 8 slots share one SourceBlock and avoid source refill
45. MidPage alloc miss run refill. DONE:
   MidPage malloc now tries transfer cache and local cache before run prefill
   only if both are empty, alloc miss pre-fills a 64KiB run into local cache
   allocator smoke verifies normal 8K malloc now comes from a SourceBlock run
46. Shared SourceBlock scavenge budget. DONE:
   scavenge charges shared SourceBlock descriptors by user slot bytes
   non-shared descriptors still charge source bytes
   safety smoke verifies one 8K slot can be scavenged without releasing the 64KiB run
47. SourceBlock route envelope. DONE:
   route table supports invalid-range entries alongside exact-valid slot entries
   SourceBlock slot registration installs an invalid envelope for the whole run
   allocator smoke verifies unregistered run slots are INVALID and become MISS after final release
48. Route invalid-range contract smoke. DONE:
   contract smoke verifies invalid-range registration and unregister
   exact-valid entries take priority over invalid-range envelopes
   backend wrapper exposes the same invalid-range contract
49. SourceBlock route teardown order. DONE:
   shared SourceBlock release now unregisters the invalid route envelope before
   SourceLayer release
   allocator destroy already follows the same route-before-source teardown order
   allocator smoke covers final SourceBlock release turning the envelope back to MISS
50. Page-table invalid-range smoke. DONE:
   PAGE_TABLE backend now has direct invalid-range smoke coverage
   exact-valid entries inside a page-range envelope still take priority
   unregistering the exact entry falls back to INVALID until the envelope is removed
51. MidPage 32K run prefill smoke. DONE:
   allocator smoke now verifies 32K class prefill creates two slots from one
   64KiB SourceBlock
   both 32K slots route as HZ6_FRONT_MIDPAGE / HZ6_MIDPAGE_32K_CLASS_ID
   the shared SourceBlock avoids a second source refill while both slots are consumed
52. SourceBlock duplicate-slot failure smoke. DONE:
   allocator smoke verifies duplicate exact slot registration is rejected
   SourceBlock refcount is restored after the failed registration path
   descriptor reuse is verified by registering the next SourceBlock slot after
   duplicate rejection without releasing the backing SourceBlock
53. Profile-selected route backend seed. DONE:
   Hz6ProfileConfig now carries route_page_granularity
   allocator init selects PAGE_TABLE when profile route granularity is nonzero
   REMOTE profile smoke verifies PAGE_TABLE route backend selection
54. Profile route contract smoke. DONE:
   contract smoke verifies SPEED / REMOTE use page route granularity
   contract smoke verifies STRICT / RSS keep exact-table route selection
   status docs now describe profile-selected route backend rather than exact-only routing
55. Allocator profile route init smoke. DONE:
   allocator smoke verifies SPEED initializes PAGE_TABLE
   allocator smoke verifies REMOTE initializes PAGE_TABLE
   allocator smoke verifies STRICT / RSS initialize EXACT_TABLE
56. Transfer shard capacity observability. DONE:
   transfer backend exposes per-shard capacity for diagnostics
   contract smoke verifies uneven capacity is preserved across shards
   inactive shard capacity reports zero
57. Transfer uneven shard fill smoke. DONE:
   contract smoke fills an uneven sharded backend to full capacity
   shard counts match the 3/2 capacity split
   one extra push is rejected after all shard capacity is consumed
58. Transfer shard steal smoke. DONE:
   contract smoke verifies pop checks the class home shard first
   if the home shard is empty, pop steals from a later non-empty shard
   this keeps consumer-visible transfer reuse from depending on producer shard locality
59. Transfer sharded class-filter smoke. DONE:
   contract smoke verifies sharded class pop retains other classes
   retained class entries can be popped afterward
   this preserves class-specific transfer cache semantics in mixed-class shards
60. Large128 profile prefill seed. DONE:
   Large128 front exposes hz6_large128_prefill()
   allocator smoke verifies RSS profile source_batch pre-fills Large128 cache
   consuming the prefilled Large128 objects avoids additional source refill
61. Local2P profile prefill seed. DONE:
   Local2P front exposes hz6_local2p_prefill()
   allocator smoke verifies RSS profile source_batch pre-fills Local2P cache
   consuming the prefilled Local2P objects avoids additional source refill
62. FrontOps prefill hook seed. DONE:
   Hz6FrontOps now carries an optional prefill hook
   Local2P and Large128 register front-specific prefill hooks
   allocator smoke calls Local2P / Large128 prefill through hz6_front_for_id()
63. Front registry prefill helper. DONE:
   hz6_front_prefill_by_id() invokes optional front prefill hooks by front id
   allocator smoke uses the registry helper for Local2P / Large128 prefill
   allocator smoke verifies fronts without a prefill hook return zero
64. Size-based prefill helper. DONE:
   hz6_front_prefill_for_allocation() selects a front through the allocation
   registry and invokes its optional prefill hook
   hz6_allocator_prefill_size() exposes the helper at the allocator layer
   allocator smoke verifies size-based prefill for Large128 / Local2P and
   originally kept MidPage out until prefill became class-aware
65. Large128 profile batch refill. DONE:
   Large128 alloc miss now uses profile.source_batch through the shared
   prefill helper before falling back to one-off source allocation
   transfer-first profiles still consume TRANSFER_FREE spans before cached
   LOCAL_FREE spans after a batch refill
   allocator smoke verifies Large128 source allocation is batched up to the
   frontcache bin capacity
66. Local2P profile batch refill. DONE:
   Local2P exact 64K alloc miss now uses the same profile.source_batch prefill
   helper as Large128
   transfer-first profiles keep TRANSFER_FREE reuse ahead of cached LOCAL_FREE
   slots after the batch refill
   allocator smoke verifies Local2P source allocation is batched up to the
   frontcache bin capacity
67. Class-aware FrontOps prefill. DONE:
   Hz6FrontOps prefill hooks now receive the selected class id
   hz6_front_prefill_by_id_class() exposes class-specific front prefill through
   the registry while hz6_front_prefill_by_id() remains compatible for
   single-class fronts
   MidPage registers a prefill hook that delegates to run prefill, so
   hz6_allocator_prefill_size() can now prefill MidPage by allocation size
68. Allocator-level front prefill wrappers. DONE:
   hz6_allocator_prefill_front() exposes front-id prefill without requiring
   callers to include the FrontLayer registry header
   hz6_allocator_prefill_front_class() exposes class-specific front prefill at
   the allocator layer
   allocator smoke verifies Large128 front prefill and MidPage 8K class prefill
   through these wrappers
69. Transfer explicit home shard pop. DONE:
   hz6_transfer_backend_pop_from_shard() lets a caller provide the consumer
   home shard instead of deriving it only from class id
   hz6_transfer_backend_pop() remains a compatibility wrapper using class id as
   the home seed
   contract smoke verifies explicit home shard pop prefers the requested shard
   and then steals from another non-empty shard for the same class
70. Front reuse consumer shard connection. DONE:
   FrontLayer transfer reuse now passes allocator owner slot as the consumer
   home shard to hz6_transfer_backend_pop_from_shard()
   cached-first and transfer-first reuse share one transfer activation helper
   allocator smoke verifies a consumer with owner slot 1 pops the same-class
   object from shard 1 before shard 0
71. Transfer explicit producer shard push. DONE:
   hz6_transfer_backend_push_to_shard() lets a caller provide the producer
   shard instead of relying only on backend round-robin state
   hz6_transfer_backend_push() remains a compatibility wrapper using
   next_push_shard
   contract smoke verifies explicit push prefers the requested shard and falls
   back to another shard when the requested shard is full
72. Remote free producer shard connection. DONE:
   FrontLayer remote free now passes allocator owner slot as the producer shard
   to hz6_transfer_backend_push_to_shard()
   allocator smoke verifies owner slot 1 remote frees land in shard 1 and are
   reused first by the same consumer home shard
73. Transfer shard policy helpers. DONE:
   hz6_profile_transfer_producer_shard() and
   hz6_profile_transfer_consumer_shard() centralize producer/consumer shard
   seed selection in PolicyLayer
   FrontLayer transfer push/pop now asks PolicyLayer for shard seeds instead of
   reading owner slot directly
   contract smoke verifies remote profile shard seeds and single-shard profiles
74. Explicit transfer shard policy id. DONE:
   Hz6ProfileConfig now carries transfer_shard_policy
   HZ6_TRANSFER_SHARD_OWNER_SLOT is the default profile policy
   HZ6_TRANSFER_SHARD_CLASS_ID is covered as a contract variant for future
   class-oriented transfer experiments
75. Explicit route backend policy id. DONE:
   Hz6ProfileConfig now carries route_backend_policy
   HZ6_ROUTE_POLICY_EXACT_TABLE and HZ6_ROUTE_POLICY_PAGE_TABLE make route
   backend selection explicit instead of inferring it from page granularity
   allocator init selects the route backend from route_backend_policy
   contract smoke verifies strict/rss exact policy and speed/remote page policy
76. Profile source kind policy. DONE:
   Hz6ProfileConfig now carries source_kind
   Large128, Local2P, and MidPage source/refill paths use profile.source_kind
   instead of hard-coding HZ6_SOURCE_OS_PAGED in each front
   contract smoke verifies speed/remote source_kind is HZ6_SOURCE_OS_PAGED
77. Source refill batch policy helper. DONE:
   hz6_profile_source_refill_batch() centralizes source refill batch selection
   in PolicyLayer
   Large128 and Local2P alloc miss paths call the helper instead of reading
   profile.source_batch directly
   contract smoke verifies speed/rss refill batch policy values
78. Scavenge budget policy helpers. DONE:
   hz6_profile_scavenge_orphan_budget() and
   hz6_profile_scavenge_local_free_budget() centralize explicit profile
   scavenge budgets in PolicyLayer
   hz6_allocator_scavenge_profile() now asks PolicyLayer for budgets instead
   of reading profile scavenge fields directly
   contract smoke verifies rss budgets and strict zero-budget policy
79. Allocator transfer observability wrappers. DONE:
   hz6_allocator_transfer_backend_kind(), transfer_capacity(), transfer_count(),
   transfer_count_class(), transfer_shard_count_at(), and
   transfer_shard_capacity_at() expose transfer diagnostics through the
   allocator API boundary
   allocator smoke now checks profile transfer shape and producer shard routing
   without reaching into allocator.transfer_backend directly
80. Allocator route observability wrappers. DONE:
   hz6_allocator_route_lookup(), route_backend_kind(), and
   route_page_granularity() expose route diagnostics through the allocator API
   boundary
   allocator and safety smoke now use allocator route lookup for descriptor
   validation instead of directly calling hz6_route_backend_lookup()
81. Allocator profile observability wrappers. DONE:
   hz6_allocator_profile_id(), profile_transfer_first(),
   profile_strict_owner_remote(), profile_source_kind(),
   profile_source_refill_batch(), and profile_transfer_capacity() expose
   profile decisions through the allocator API boundary
   allocator smoke now checks transfer-first and source refill batch behavior
   without reading allocator.profile fields directly
82. Front profile access boundary cleanup. DONE:
   Large128, Local2P, MidPage, and shared FrontUtil now obtain source kind,
   source refill batch, transfer-first, and strict-remote policy through
   allocator/profile helper APIs instead of reading allocator.profile fields
   directly
   rg confirms tests/fronts/api no longer contain allocator.profile field
   accesses outside allocator internals
83. Allocator stats snapshot boundary cleanup. DONE:
   allocator smoke now reads source_alloc through hz6_stats_snapshot() instead
   of direct allocator.stats field access
   rg confirms tests/fronts/api no longer contain allocator.stats field access
   outside allocator/front internals
84. Allocator source registry boundary helper. DONE:
   hz6_allocator_source_ops() exposes SourceLayer ops lookup through the
   allocator API boundary
   shared FrontUtil and MidPage now obtain source ops through the allocator
   helper instead of directly reading allocator.source_registry
85. Allocator route unregister boundary helper. DONE:
   hz6_allocator_route_unregister_exact() exposes exact route removal through
   the allocator API boundary
   FrontUtil, MidPage, and allocator smoke no longer call exact route
   unregister through allocator.route_backend directly
86. Allocator frontcache boundary helpers. DONE:
   hz6_allocator_frontcache_push(), pop(), remove(), count(), and capacity()
   expose front-cache operations through the allocator boundary
   allocator internals, shared FrontUtil, and MidPage now avoid direct
   frontcache_bins access outside allocator helper implementations and init
87. Allocator transfer operation boundary helpers. DONE:
   hz6_allocator_transfer_push() and hz6_allocator_transfer_pop() centralize
   profile shard selection plus TransferBackend push/pop
   shared FrontUtil no longer calls TransferBackend push/pop or transfer shard
   policy helpers directly
88. Allocator route registration boundary helpers. DONE:
   hz6_allocator_route_register_exact() and
   hz6_allocator_source_block_register_invalid_range() expose route
   registration through the allocator boundary
   shared FrontUtil and MidPage no longer call RouteBackend register/lookup
   helpers through allocator.route_backend directly
89. Allocator stats note helpers. DONE:
   hz6_allocator_note_source_alloc(), note_transfer_push(), and
   note_transfer_pop() centralize front-originated stats updates in the
   allocator API
   shared FrontUtil and MidPage no longer mutate allocator.stats directly
90. Allocator owner token boundary helper. DONE:
   hz6_allocator_owner_token() exposes owner token reads through the allocator
   API boundary
   hz6_allocator_debug_set_owner_slot() keeps smoke-only shard setup out of
   allocator internals
   shared FrontUtil and allocator smoke no longer read allocator.owner.token
   directly
91. Allocator descriptor preparation boundary helper. DONE:
   hz6_allocator_prepare_descriptor() centralizes descriptor user/source
   pointer, SourceBlock, owner token, generation, class, and initial state
   setup
   shared FrontUtil no longer hand-initializes descriptor source fields for
   one-off, slot-offset, SourceBlock, or prefill allocations
92. Allocator active-descriptor transition helpers. DONE:
   hz6_allocator_cache_active_descriptor() centralizes ACTIVE -> LOCAL_FREE
   plus frontcache push / overflow release
   hz6_allocator_remote_free_active_descriptor() centralizes ACTIVE ->
   TRANSFER_FREE / REMOTE_PENDING transitions and transfer push rollback
   FrontUtil and MidPage no longer hand-write free-path descriptor state
   transitions
93. Allocator descriptor implementation split. DONE:
   api/hz6_allocator_descriptor.c now owns descriptor pool lookup,
   descriptor preparation, activation, source release, and active-descriptor
   cache/remote transitions
   api/hz6_allocator.c is smaller and keeps allocator lifecycle, route,
   transfer, source block, scavenge, and public API orchestration
94. Allocator SourceBlock implementation split. DONE:
   api/hz6_allocator_source_block.c now owns SourceBlock allocation, retain,
   release, and invalid route envelope registration
   api/hz6_allocator.c no longer carries the MidPage run SourceBlock helper
   implementation
95. Allocator reclaim implementation split. DONE:
   api/hz6_allocator_reclaim.c now owns owner-dead orphan conversion, orphan
   release/adoption, scavenge budget execution, and strict remote-pending drain
   api/hz6_allocator.c now keeps lifecycle plus front/policy/route/transfer API
   orchestration separate from reclaim logic
96. Allocator facade implementation split. DONE:
   api/hz6_allocator_facade.c now owns frontcache, front prefill, profile,
   source ops, route diagnostics/registration, transfer diagnostics/ops, and
   stats note wrappers
   api/hz6_allocator.c is now focused on allocator lifecycle and public
   malloc/free/owns/stats entrypoints
97. Reclaim smoke split. DONE:
   tests/hz6_r1_reclaim_smoke.c now owns owner-dead orphan release, profile
   scavenge, and cross-owner orphan adoption coverage
   allocator smoke keeps allocator/front integration coverage and no longer
   carries reclaim-specific scenarios
98. Prefill smoke split. DONE:
   tests/hz6_r1_prefill_smoke.c now owns source-kind, front-registry,
   size-based, and allocator-front prefill coverage
   allocator smoke keeps hot allocation/front integration coverage and no
   longer carries slow-path prefill scenarios
99. SourceBlock smoke split. DONE:
   tests/hz6_r1_sourceblock_smoke.c now owns descriptor source release,
   single-slot source backing, shared SourceBlock refcount/route envelope,
   and MidPage 8K/32K run prefill coverage
   allocator smoke keeps front integration and no longer carries SourceBlock
   lifecycle-specific scenarios
100. Transfer smoke split. DONE:
   tests/hz6_r1_transfer_smoke.c now owns Local2P transfer reuse, profile
   transfer capacity/backend checks, generic remote transfer reuse,
   producer/consumer shard behavior, and strict REMOTE_PENDING drain coverage
   allocator smoke keeps basic allocator/front integration and no longer
   carries transfer-backend-specific scenarios
101. Route smoke split. DONE:
   tests/hz6_r1_route_smoke.c now owns exact route, invalid range envelope,
   exact/table backend, and page-table backend contract coverage
   contract smoke keeps transfer, owner, profile, frontcache, source, and
   scavenge low-level module coverage without RouteLayer bulk
102. Transfer contract smoke split. DONE:
   tests/hz6_r1_transfer_contract_smoke.c now owns bounded transfer cache,
   transfer backend, sharded backend class filtering, shard preference,
   explicit push/pop shard, and uneven shard capacity coverage
   contract smoke keeps owner, profile, frontcache, source, and scavenge
   low-level module coverage without TransferLayer bulk
103. Source contract smoke split. DONE:
   tests/hz6_r1_source_contract_smoke.c now owns source ops validation,
   Linux mmap source ops, source registry lookup, and ScavengeLayer budget
   accounting coverage
   core contract smoke keeps owner, profile, and frontcache low-level module
   coverage without SourceLayer/ScavengeLayer bulk
104. Core contract smoke rename. DONE:
   tests/hz6_r1_core_contract_smoke.c replaces the generic
   hz6_r1_contract_smoke.c name now that route, transfer, source, prefill,
   reclaim, safety, and SourceBlock coverage each have dedicated smoke files
   the remaining core contract smoke covers owner token, profile policy,
   size-class, and FrontCache primitives
105. Smoke runner registry cleanup. DONE:
   linux/build_hz6_r1_smokes.sh now has explicit HZ6_SMOKES entries in
   addition to HZ6_INCLUDES and HZ6_LIB_SOURCES
   adding a new R1 smoke now means adding one registry entry instead of
   appending another build_smoke call at the bottom of the script
106. OwnerLayer implementation split. DONE:
   owner/hz6_owner.c now owns hz6_owner_is_alive()
   owner/hz6_owner.h exposes the OwnerLayer contract without keeping liveness
   logic header-only
   linux/build_hz6_r1_smokes.sh registers owner/hz6_owner.c as an explicit
   HZ6_LIB_SOURCE
107. Owner equality implementation split. DONE:
   owner/hz6_owner.c now owns hz6_owner_equal()
   owner/hz6_owner.h exposes equality and liveness as the OwnerLayer API
   include/hz6_contract.h keeps shared token/result types and route result
   constructors only
108. Front source utility split. DONE:
   fronts/hz6_front_source.c now owns source-backed front allocation,
   source-block slot creation, and prefill helpers
   fronts/hz6_front_util.c now keeps reuse and free-route helpers only
   linux/build_hz6_r1_smokes.sh registers hz6_front_source.c as an explicit
   HZ6_LIB_SOURCE
109. Front source header split. DONE:
   fronts/hz6_front_source.h now exposes SourceLayer-backed front helpers
   fronts/hz6_front_util.h now exposes reusable object and free-route helpers
   front implementations and source/prefill smokes include the narrower
   header that matches the helper family they use
110. Front SourceBlock slot split. DONE:
   fronts/hz6_front_source_block.c now owns shared SourceBlock-backed slot
   creation and descriptor preparation
   fronts/hz6_front_source.c now keeps direct SourceLayer reserve and prefill
   helpers only
   linux/build_hz6_r1_smokes.sh registers hz6_front_source_block.c as an
   explicit HZ6_LIB_SOURCE
```

Current R1 smoke:

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected:

```text
hz6-r1-core-contract-smoke ok
hz6-r1-route-smoke ok
hz6-r1-transfer-contract-smoke ok
hz6-r1-source-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-prefill-smoke ok
hz6-r1-sourceblock-smoke ok
hz6-r1-transfer-smoke ok
hz6-r1-reclaim-smoke ok
hz6-r1-safety-smoke ok
```

## Open Decisions

```text
Route table shape:
  PAGE_TABLE backend is now seeded as a contract variant.
  Real Windows sidecar and Linux region/page maps still need implementation.

Transfer shard policy:
  consumer-visible shard remains the default idea, but HZ5 variants were no-go
  as patches. HZ6 must test it from a clean contract.

Fail-closed timing:
  strict profile uses immediate validation.
  speed remote profile may use batch-boundary validation only if invalid
  pointers cannot be promoted to reusable bins.

Profile names:
  use role names such as hz6-speed, hz6-rss, hz6-remote, hz6-strict.
  keep numeric knobs diagnostic-only.
```

## Not Yet

```text
No HZ5 source file migration.
No HZ5 code copy.
No new performance claim.
No benchmark table until an HZ6 prototype exists.
Toy allocation and transfer paths are for contract validation only.
Large128 is still a seed front, not a performance claim.
Large128 has Linux mmap backing, but no full span/source refill policy yet.
Local2P is an exact 64KiB seed front, not the final Windows Local2P profile.
MidPage is a seed front, not the final page-run allocator.
Sharded transfer has single-thread smoke coverage and observability only;
synchronization and real benchmark validation are still pending.
Cross-owner orphan adoption is a seed contract only; no shared route table or
thread-safe owner registry exists yet.
```
