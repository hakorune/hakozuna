#!/usr/bin/env bash
# Sourced by build_linux_hz5_standalone.sh.

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_hz5_standalone.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --out-dir DIR      output directory (default: hakozuna-hz5/out/linux/<arch>)
  --linux-hz5-profile-pagerun64-main
                     saved profile alias: PageRun64 main/mid/cross64 candidate
  --linux-hz5-profile-pagerun64-cross128
                     saved profile alias: PageRun64 + LargeFront takefirst
                     + source batch16 for cross128-style rows
  --linux-hz5-profile-pagerun64-large128
                     saved profile alias: PageRun64 + LargeFront takefirst
                     + source batch4 + Large-first free route for
                     large128-style rows
  --linux-hz5-profile-large128-rss
                     human alias for pagerun64-large128: large128 low-RSS
                     fixed profile
  --linux-hz5-profile-pagerun64-large128-batch8
                     diagnostic alias: same as pagerun64-large128 with
                     LargeFront source batch8
  --linux-hz5-profile-pagerun64-large128-batch16
                     diagnostic alias: same as pagerun64-large128 with
                     LargeFront source batch16
  --linux-hz5-profile-large128-source16
                     human alias for large128-batch16: source-batch16
                     throughput diagnostic
  --linux-hz5-profile-pagerun64-large128-b16-drain1
                     diagnostic alias: same as large128-batch16 with
                     alloc-miss remote drain local budget 1
  --linux-hz5-profile-large128-r50-drain
                     human alias for b16-drain1: r50 drain-budget diagnostic
  --linux-hz5-profile-large128-drainbulk
                     diagnostic alias: source16 plus bulk local-list commit
                     during owner remote drain
  --linux-hz5-profile-pagerun64-large128-b16-takeonly
                     diagnostic alias: same as large128-batch16 with
                     take-first-only remote drain
  --linux-hz5-profile-pagerun64-large128-b16-popbudget1
                     diagnostic alias: same as large128-batch16 with
                     CAS-pop budgeted remote drain local budget 1
  --linux-hz5-profile-pagerun64-large128-b16-remotehold4
                     diagnostic alias: same as large128-batch16 with
                     take-first remote hold cap 4
  --linux-hz5-profile-pagerun64-large128-b16-drain1-hold4
                     diagnostic alias: same as b16-drain1 with
                     remote hold cap 4
  --linux-hz5-profile-large128-r50-hold
                     human alias for b16-drain1-hold4: r50 RemoteHold
                     diagnostic
  --linux-hz5-profile-pagerun64-large128-b16-drain1-hold8
                     diagnostic alias: same as b16-drain1 with
                     remote hold cap 8
  --linux-hz5-profile-large128-r50-hold8
                     human alias for b16-drain1-hold8: wider r50
                     RemoteHold diagnostic
  --linux-hz5-profile-large128-direct-header
                     unsafe diagnostic alias: source16 plus direct
                     ptr-4096 LargeFront header lookup
  --linux-hz5-profile-large128-base-directmap
                     safe diagnostic alias: source16 plus one-slot
                     exact-base LargeFront route cache
  --linux-hz5-profile-large128-global-remote
                     human alias: 128K remote frees go to global recycle
                     instead of owner inbox
  --linux-hz5-profile-large128-remote-first
                     human alias: 128K alloc miss drains owner inbox before
                     local free list
  --linux-hz5-profile-large128-remote-first-gated
                     human alias: 128K alloc checks owner inbox before local
                     free list only when inbox is nonempty
  --linux-hz5-profile-large128-chunk16
                     human alias: source batch16 + remote batch16 using
                     128K owner chunk inbox
  --linux-hz5-profile-pagerun64-large128-b16-policy-l7
                     diagnostic alias: source batch16 with drain1-hold4 and
                     remainder-size local conversion policy
  --linux-hz5-profile-large128-policy-l7
                     human alias for b16-policy-l7: first rule policy
                     diagnostic
  --linux-hz5-profile-large128-policy-l8-shadow
                     human alias: source batch16 + drain1-hold4 with
                     Policy-L8 shadow classification counters only
  --linux-hz5-profile-pagerun64-large128-b16-rb32
                     diagnostic alias: same as large128-batch16 with
                     LargeFront remote batch cap 32
  --linux-hz5-profile-pagerun64-large128-b16-rb64
                     diagnostic alias: same as large128-batch16 with
                     LargeFront remote batch cap 64
  --linux-hz5-profile-pagerun64-large128-policy-l0
                     diagnostic alias: large128 saved profile plus
                     slow-path LargeFront Policy-L0 shadow observation
  --linux-hz5-profile-pagerun64-large128-policy-l1a
                     diagnostic alias: PageRun64 + LargeFront takefirst
                     + conservative LargeFront 128K source-batch selector
  --linux-hz5-profile-pagerun64-large128-policy-l1b
                     diagnostic alias: policy-l1a plus LargeFront remote
                     batch cap selector 16/32/64 at flush boundaries
  --linux-local2p    enable Linux Local2P exact 64K/a8192 TLS span candidate
  --linux-local2p-tls-packed
                     pack Local2P TLS state into one TLS object
  --linux-local2p-initial-exec
                     build with -ftls-model=initial-exec for Local2P speed A/B
  --linux-local2p-direct-route
                     test Local2P exact 64K/a8192 before generic route helper
  --linux-local2p-direct-init
                     initialize Local2P wrapper prefix directly
  --linux-local2p-fast
                     enable packed TLS, initial-exec, direct route, direct init
  --linux-local2p-no-cookie
                     diagnostic only: skip Local2P attribution cookie recompute
  --linux-local2p-no-cas
                     diagnostic only: replace Local2P ACTIVE->FREED CAS with load/store
  --linux-local2p-owner-inbox
                     candidate only: route remote frees to owner MPSC inbox
  --linux-local2p-remote-batch
                     candidate only: batch remote frees before owner inbox push
  --linux-local2p-remote-batch-cap N
                     candidate only: remote batch flush threshold (default: 16)
  --linux-local2p-tls-cap N
                     candidate only: Local2P owner-local TLS cache cap (default: 1)
  --linux-local2p-global-cap N
                     candidate only: Local2P bounded global retained-cache cap (default: 1024)
  --linux-local2p-object-node
                     candidate only: use aligned user pointers as Local2P free-list nodes
  --linux-local2p-same-owner-fast-state
                     candidate only: object-node with owner-local load/store state transition
  --linux-local2p-route-cookie
                     candidate only: fast-state lane using Local2P cookie as direct route guard
  --linux-local2p-reuse-state-only
                     candidate only: route-cookie lane updating only state on TLS reuse
  --linux-local2p-slim-check
                     candidate only: reuse-state-only lane with reduced Local2P free checks
  --linux-local2p-fast-cookie
                     candidate only: slim-check lane with lightweight Local2P cookie
  --linux-local2p-free-first
                     candidate only: fast-cookie lane with Local2P decode first in free
  --linux-local2p-freefirst-fastcookie
                     alias for --linux-local2p-free-first; explicit compound lane name
  --linux-local2p-tls-fast-return
                     candidate only: fast-cookie lane with immediate owner-TLS hit return
  --linux-local2p-exact-api
                     candidate only: tls-fast-return lane with exact 64K/a8192 alloc/free API in benchmark
  --linux-local2p-single-slot-tls
                     candidate only: exact-api lane with TLS_CAP=1 head-only cache
  --linux-local2p-speed-linkflags
                     candidate only: exact-api lane with speed-oriented compile/link flags
  --linux-local2p-rss-retain
                     candidate only: speed-linkflags lane retaining local TLS overflow in bounded global cache
  --linux-local2p-retain-array
                     diagnostic only: store retained owner-local TLS entries in a pointer array
  --linux-smallfront-remote-batch-cap N
                     SmallFront-S1 remote-free sender batch flush threshold (default: 16)
  --linux-smallfront-drain-empty-gated
                     candidate only: skip SmallFront owner-inbox exchange when
                     an acquire load observes an empty inbox
  --linux-smallfront-remote-outbox
                     candidate only: keep multiple sender-side SmallFront
                     remote-free outbox slots keyed by owner/class
  --linux-hz5-general-region-outbox
                     candidate preset: SmallFront remote outbox cap8,
                     MidFront rb16/owner-fast, and LargeFront region-map
  --linux-hz5-general-directfree
                     candidate preset: general-region-outbox plus MidFront
                     remote ACTIVE->LOCAL_FREE state transition
  --linux-hz5-general-midfirst
                     diagnostic preset: general-region-outbox plus MidFront
                     first in the preload free() ownership dispatch
  --linux-hz5-general-midcache
                     diagnostic preset: general-region-outbox plus MidFront
                     TLS page lookup cache
  --linux-hz5-general-midrbuf
                     candidate preset: general-region-outbox plus HZ4-style
                     MidFront sender rbuf grouping
  --linux-hz5-general-routesplit
                     candidate preset: general-region-outbox but MidFront only
                     handles <=4096 and LargeFront handles >4096
  --linux-hz5-general-midpage
                     candidate preset: general-region-outbox plus
                     MidPageFront-M2 hash-map control for 2049..32768
  --linux-hz5-general-midpage-region
                     candidate preset: general-region-outbox plus
                     MidPageFront-M2.2 region-array lookup
  --linux-hz5-general-midpage-region-shadow
                     diagnostic preset: midpage-region plus remote-shadow
                     pending bitmap experiment
  --linux-hz5-general-midpage-region-shadow-frontfirst
                     diagnostic preset: midpage-region-shadow plus
                     MidPageFront first in preload free() dispatch
  --linux-hz5-general-midpage-region-shadow-tlscache
                     diagnostic preset: midpage-region-shadow plus TLS
                     region lookup cache
  --linux-hz5-general-midpage-region-shadow-hotslot
                     diagnostic preset: midpage-region-shadow plus one-entry
                     TLS hot object cache per MidPageFront class
  --linux-hz5-general-midpage-region-shadow-activetrust
                     diagnostic preset: midpage-region-shadow without local
                     alloc-side remote bitmap check
  --linux-hz5-general-midpage-region-shadow-allocfirst
                     diagnostic preset: midpage-region-shadow with preload
                     MidPageFront alloc-before-can-handle dispatch
  --linux-hz5-general-midpage-region-shadow-localunsafe
                     unsafe diagnostic: allocfirst plus skip MidPageFront
                     owner-local bitmap state checks; r0 upper bound only
  --linux-hz5-general-midpage-region-shadow-m4mag
                     diagnostic preset: allocfirst plus MidPageFront-M4
                     descriptor-owned owner-local slot magazine
  --linux-hz5-general-midpage-region-shadow-m4packet
                     diagnostic preset: m4mag plus descriptor page-packet
                     remote handoff
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst
                     diagnostic preset: m4packet plus MidPageFront-first
                     preload free dispatch
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink
                     diagnostic preset: m4packet-freefirst plus preload
                     initial-exec TLS and speed link flags
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-allocelide
                     unsafe diagnostic: freefirst-tlslink plus M4 owner-local
                     alloc-side slot-state transition elision upper bound
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-ptrmag
                     unsafe diagnostic: allocelide plus pointer-only M4 local
                     magazine pop upper bound
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-absalloc
                     diagnostic preset: freefirst-tlslink plus MidPageFront
                     absolute-first malloc routing
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-regcache
                     diagnostic preset: freefirst-tlslink plus MidPageFront
                     TLS region lookup cache
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-slotswitch
                     diagnostic preset: freefirst-tlslink plus fixed-class
                     MidPageFront slot-index dispatch
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m5hit
                     candidate diagnostic: FrontCache-M5a MidPage hit-only
                     cache entry with remote drain moved to miss/refill path
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast
                     unsafe upper-bound: M5 hit-only + pointer magazine,
                     alloc-state elision, and MidPage preload fast bypass
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide
                     unsafe upper-bound: superfast plus owner-local MidPage
                     free-side slot-state elision
  --linux-hz5-general-midpage-m6-deferred-free-direct
                     diagnostic proof lane: M4 packet + M5 hit-only plus
                     M6 classless raw-free quarantine for direct MidPage API
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-flatcap
                     diagnostic preset: freefirst-tlslink plus flat M4
                     magazine cap 64 for every MidPage class
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-overarray
                     diagnostic preset: superfast-freeelide plus TLS secondary
                     overflow array for M4 magazine-full slots
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-wide32k
                     unsafe speed upper-bound: superfast-freeelide plus one
                     32K MidPage class for every 2049..32768 allocation
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-16-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 4K/16K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-16-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 4K/8K/16K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 4K/8K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 8K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rssgov
                     RSS diagnostic preset: band8/32 plus empty MidPage
                     slab release
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint
                     RSS diagnostic preset: band8/32 plus explicit checkpoint
                     release only; no release on allocator refill/miss
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote
                     remote-heavy candidate: band8/32 checkpoint lane plus
                     remote-only M6 deferred free
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m7ticket
                     remote-heavy candidate: band8/32 checkpoint lane plus
                     M7 page+bitmask remote ticket transfer
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m8xfer
                     remote-heavy diagnostic: M6 remote producer path plus
                     owner-side page+bitset transfer cache
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m9budget
                     remote-heavy diagnostic: M6 remote producer path plus
                     budgeted owner packet drain
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m10slot
                     remote-heavy diagnostic: flat page+slot remote ring
                     without M7 page aggregation
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m11direct
                     remote-heavy diagnostic: M6 remote with direct
                     LIVE->CACHE remote flush and owner drain state-check only
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun
                     remote-heavy diagnostic: M6 remote with PageRun owner
                     local bitset path bypassing M4 magazine hot path
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64
                     remote-heavy diagnostic: PageRun plus 64K MidPage class
                     for the 32769..65536 route gap
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-takefirst
                     remote-heavy diagnostic: PageRun64 plus LargeFront
                     drain-take-first for cross-size remote rows
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-adaptive128
                     remote-heavy diagnostic: PageRun64 plus LargeFront
                     takefirst and adaptive 128K source refill
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-scavenge
                     remote-heavy diagnostic: PageRun64 plus LargeFront
                     takefirst and retained 128K payload scavenging
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-observe
                     observation preset: PageRun64 plus LargeFront
                     takefirst and LargeFront phase counters; not for medians
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-drainhit
                     remote-heavy diagnostic: band8/32 checkpoint lane with
                     periodic M4 remote packet drain restored on M5 alloc hits
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 8K/16K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rssgov
                     RSS diagnostic preset: band8/16/32 plus empty MidPage
                     slab release
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rsscheckpoint
                     RSS diagnostic preset: band8/16/32 plus explicit
                     checkpoint release only; no release on refill/miss
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rsscheckpoint-drainhit
                     remote-heavy diagnostic: band8/16/32 checkpoint lane with
                     periodic M4 remote packet drain restored on M5 alloc hits
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band16-32
                     diagnostic preset: superfast-freeelide plus coarse
                     MidPage bands 16K/32K
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-tagfree
                     diagnostic preset: superfast-freeelide plus MidPage
                     tagged free wrapper
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m4stats
                     observation preset: freefirst-tlslink plus M4 class
                     counters; not for performance medians
  --linux-midpagefront-m6-raw-cap N
                     M6 raw quarantine cap for deferred-free diagnostics
                     (default: 64)
  --linux-midpagefront-m6-deferred-free
                     enable M6 classless raw-free quarantine on the selected
                     MidPageFront preset
  --linux-midpagefront-m6-remote-deferred-free
                     defer only remote MidPage frees through the M6 raw
                     quarantine while preserving owner-local immediate free
  --linux-midpagefront-m6-remote-no-refill-flush
                     diagnostic: do not flush remote-only M6 raw quarantine
                     from owner-local alloc refill
  --linux-midpagefront-m7-remote-ticket
                     enable M7 page+bitmask remote ticket transfer on the
                     selected MidPageFront preset
  --linux-midpagefront-m7-remote-page-cap N
                     M7 sender TLS page batch cap (default: 64)
  --linux-midpagefront-m8-owner-xfer
                     enable M8 owner-side page+bitset transfer cache on the
                     selected MidPageFront preset
  --linux-midpagefront-m8-xfer-page-cap N
                     M8 owner transfer page cap per class (default: 64)
  --linux-midpagefront-m9-budget-drain
                     enable M9 budgeted owner remote-packet drain on the
                     selected MidPageFront preset
  --linux-midpagefront-m10-remote-slot-ring
                     enable M10 flat page+slot remote ring on the selected
                     MidPageFront preset
  --linux-midpagefront-m11-remote-direct-cache
                     enable M11 remote direct-cache state transition on the
                     selected MidPageFront preset
  --linux-midpagefront-m4-remote-drain-hit-interval N
                     hit-count interval for drainhit diagnostics
                     (default: 64)
  --linux-midpagefront-empty-slab-release
                     MidPageFront RSS diagnostic: return fully cached
                     64KiB slabs to the region source and madvise them
  --linux-midpagefront-empty-release-checkpoint
                     with empty-slab release enabled, do not release retired
                     slabs on refill; require hz5_midpagefront_release_retired
  --linux-midpagefront-empty-retain-cap N
                     number of fully cached MidPage slabs retained per
                     thread/class before empty-slab release (default: 64)
  --linux-hz5-general-midpage-region-shadow-m4packet-routefree
                     diagnostic preset: m4packet plus MidPageFront then
                     LargeFront preload free dispatch
  --linux-hz5-general-midpage-region-shadow-m4packet-crossdrain
                     diagnostic preset: m4packet plus low-cost MidPageFront
                     owner pending mask drained from other front-end misses
  --linux-hz5-general-midpage-region-shadow-slotswitch
                     diagnostic preset: midpage-region-shadow-allocfirst plus
                     fixed-class MidPageFront slot index dispatch
  --linux-hz5-general-midpage-region-shadow-tlslink
                     diagnostic preset: midpage-region-shadow-allocfirst plus
                     preload-wide initial-exec TLS and speed link flags
  --linux-hz5-general-midpage-region-shadow-tlsie
                     diagnostic preset: midpage-region-shadow-allocfirst plus
                     preload-wide initial-exec TLS only
  --linux-hz5-general-midpage-region-shadow-linkonly
                     diagnostic preset: midpage-region-shadow-allocfirst plus
                     preload-wide speed link flags only
  --linux-hz5-general-midpage-region-shadow-nodeless
                     diagnostic preset: midpage-region-shadow-allocfirst plus
                     MidPageFront nodeless local page-run cache
  --linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
                     diagnostic preset: nodeless plus per-class TLS pointer
                     cache to reduce partial/refill churn
  --linux-hz5-general-midpage-region-shadow-nodeless-stats
                     diagnostic preset: nodeless plus MidPageFront nodeless
                     observation counters; not for performance medians
  --linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats
                     diagnostic preset: nodeless-ptrcache plus observation
                     counters; not for performance medians
  --linux-hz5-general-midpage-region-frontfirst
                     diagnostic preset: midpage-region plus MidPageFront first
                     in preload free() ownership dispatch
  --linux-hz5-general-midoutbox
                     candidate preset: general-region-outbox plus MidFront
                     owner/class sender outbox
  --linux-ownerhub-r1
                     diagnostic only: enable shared owner pending-mask
                     observation; use only with HZ5_OWNERHUB_STATS=1
  --linux-ownerhub-r2
                     candidate only: OwnerHub coordinated cross-front drain
                     on allocation miss; does not enable diagnostic counters
  --linux-ownerhub-r3
                     candidate only: OwnerHub front-dirty coordinated drain;
                     uses coarse per-front pending bits instead of class bits
  --linux-midfront-m1
                     enable Linux MidFront-M1 ordinary malloc 4K..64K front-end
  --linux-midpagefront-m2
                     enable Linux MidPageFront-M2 ordinary malloc 2049..32768
  --linux-midpagefront-region-array
                     candidate only: use 64MiB region + descriptor-array
                     lookup for MidPageFront-M2 instead of slab hash lookup
  --linux-midpagefront-remote-shadow
                     candidate only: remote free marks a separate pending
                     bitmap so owner-local active bits avoid locked CAS
  --linux-midpagefront-local-fast-state
                     diagnostic only: owner-local MidPageFront active-bit
                     transitions use load/store instead of CAS
  --linux-midpagefront-tls-region-cache
                     diagnostic only: cache MidPageFront region lookup in TLS
  --linux-midpagefront-tls-hot-slot
                     diagnostic only: one-entry TLS hot object cache per class
  --linux-midpagefront-local-active-trust
                     diagnostic only: skip local alloc-side remote bitmap check
  --linux-midpagefront-remote-batch-cap N
                     MidPageFront remote-free sender batch threshold (default: 16)
  --linux-midfront-owner-fast-state
                     candidate only: MidFront owner-local load/store state transition
  --linux-midfront-max-bytes N
                     cap MidFront claimed ordinary malloc size (default: 65536)
  --linux-midfront-remote-batch-cap N
                     MidFront remote-free sender batch flush threshold (default: 16)
  --linux-midfront-remote-outbox
                     candidate only: keep multiple sender-side MidFront
                     remote-free outbox slots keyed by owner/class
  --linux-midfront-remote-outbox-slots N
                     MidFront sender outbox slot count (default: 4)
  --linux-midfront-remote-rbuf
                     candidate only: collect MidFront remote frees in a
                     sender TLS ring, then group by owner/class on flush
  --linux-midfront-remote-rbuf-cap N
                     MidFront rbuf entry count (default: 128)
  --linux-midfront-remote-rbuf-threshold N
                     MidFront rbuf flush threshold (default: 96)
  --linux-midfront-outbox-flush-on-miss
                     candidate only: publish MidFront sender outbox slots on
                     local allocation miss before owner-inbox drain
  --linux-midfront-remote-direct-free-state
                     diagnostic only: remote free transitions ACTIVE->LOCAL_FREE
                     so owner drain can skip REMOTE_PENDING->LOCAL_FREE CAS
  --linux-midfront-remote-trust-drain-state
                     diagnostic only: with direct-free-state, owner drain trusts
                     inbox provenance and skips LOCAL_FREE state load before
                     pushing to owner cache
  --linux-midfront-lookup-cache
                     diagnostic only: cache two MidFront page-map lookups per
                     thread before the hash table probe
  --linux-midfront-drain-all-on-miss
                     candidate only: drain all MidFront owner inbox classes on local miss
  --linux-midfront-drain-mask-on-miss
                     candidate only: drain pending MidFront owner inbox classes on local miss
  --linux-midfront-drain-mask-hit-stop
                     candidate only: drain requested MidFront inbox class first,
                     then stop if it fills the local class cache
  --linux-midfront-drain-take-first
                     candidate only: activate and return the first requested-class
                     remote span directly during owner-inbox drain
  --linux-midfront-drain-empty-gated
                     candidate only: skip MidFront owner-inbox exchange when
                     an acquire load observes an empty inbox
  --linux-midfront-remote-global-recycle
                     candidate only: recycle MidFront remote frees through global class stacks
  --linux-largefront-l1
                     enable Linux LargeFront-L1 ordinary malloc >64K..1M front-end;
                     implies SmallFront, MidFront, preload full, and exact-only off
  --linux-largefront-owner-fast-state
                     candidate only: LargeFront owner-local load/store state transition
  --linux-largefront-owner-inbox
                     candidate only: route LargeFront remote frees through owner inbox
                     instead of global recycle
  --linux-largefront-remote-batch
                     candidate only: batch LargeFront remote frees before owner inbox publish
  --linux-largefront-remote-batch-cap N
                     LargeFront remote batch flush threshold (default: 16)
  --linux-largefront-source-batch-count N
                     LargeFront source refill span count (default: 16)
  --linux-largefront-alloc-drain-local-budget N
                     candidate only: limit extra remote spans drained into
                     owner-local LargeFront cache on alloc miss; 0 drains all
  --linux-largefront-drain-take-first
                     candidate only: activate and return the first requested-class
                     LargeFront remote span during owner-inbox drain
  --linux-largefront-drain-empty-gated
                     candidate only: skip LargeFront owner-inbox exchange when
                     an acquire load observes an empty inbox
  --linux-largefront-map-base-only
                     diagnostic only: map only the returned base page for
                     LargeFront spans; reduces map insert pressure but weakens
                     interior-pointer invalid-free attribution
  --linux-largefront-region-map
                     candidate only: register LargeFront source regions instead
                     of every covered 4K page; keeps interior invalid-free
                     attribution without per-page insertion
  --linux-largefront-adaptive128
                     candidate only: choose LargeFront 128K source refill
                     batch by slow-path mapped-span pressure
  --linux-largefront-payload-scavenge
                     diagnostic only: madvise retained 128K payloads after
                     the owner-local free-list cap is exceeded
  --linux-largefront-observe
                     observation only: compile LargeFront phase counters;
                     print with HZ5_LARGEFRONT_OBSERVE=1
  --linux-largefront-policy-l0
                     observation only: compile LargeFront slow-path policy
                     counters; print with HZ5_LARGEFRONT_POLICY_L0=1
  --linux-largefront-drain-bulk-local
                     diagnostic only: bulk-commit owner-drained remote spans
                     into the local free list
  --linux-largefront-policy-l1a
                     candidate only: choose LargeFront 128K source refill
                     batch 4/8/16 from slow-path mapped-span pressure
  --linux-largefront-policy-l1b
                     candidate only: adapt LargeFront 128K remote batch cap
                     16/32/64 at remote-flush boundaries
  --linux-largefront-scavenge-local-cap N
                     retained 128K owner-local spans kept before payload
                     scavenging (default: 4)
  --linux-largefront-lower-classes
                     candidate only: add 8K/16K/32K/64K classes to LargeFront
  --linux-p11-speed-core
                     diagnostic only: compile the legacy P2 run/tcache path with HZ5_P11_SPEED_CORE=1
  --linux-p25-bridge-attr
                     preserve P25 bridge topology with wrapper attr CAS guard
  --linux-p25-bridge-attr-no-cas
                     diagnostic only: validate bridge attr with load/store, no CAS
  --linux-p25-bridge-attr-no-cookie
                     diagnostic only: bridge attr CAS without cookie recompute
  --linux-p25-bridge-attr-readonly-state
                     diagnostic only: validate state but do not mark it freed
  --linux-p43        enable Linux P43 segment-slot source candidate lane
  --linux-p43-no-prepared-bridge
                     disable P43 PreparedBridge for source-only A/B
  --linux-p43-unsafe-no-lookup
                     bypass P43 free lookup for lookup-cost A/B
  --linux-p43-trust-fast-lookup
                     trust P43 fast lookup hits as active for candidate A/B
  --linux-p43-trust-wrapper-source
                     trust decoded P25 wrapper source and skip P43 lookup
  --linux-p43-decoded-raw-lookup
                     decode wrapper first, then validate raw through P43 lookup
  --linux-p43-decoded-raw-fastlookup
                     decoded raw lookup uses only P43 fast table and masks
  --linux-p43-decoded-raw-allocated
                     decoded raw lookup uses fast table and allocated bit only
  --linux-p43-token  store P43 segment/slot token in the HZ5 wrapper
  --linux-p43-token-bridge
                     validate wrapper token, then release through P25 bridge
  --linux-preload-full
                     build an experimental full LD_PRELOAD front-end; disables
                     standalone exact-only gating for this output directory
  --linux-smallfront-s1
                     build HZ5-SmallFront-S1 for ordinary malloc <= 2048;
                     implies --linux-preload-full and disables exact-only gate
  --trace-lane       enable route/reuse counters; disables SPEED_LANE
  --help             show this message
EOF
}
