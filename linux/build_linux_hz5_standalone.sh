#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
OUT_DIR=""
ENABLE_LINUX_P43=0
LINUX_LOCAL2P=0
LINUX_LOCAL2P_TLS_PACKED=0
LINUX_LOCAL2P_TLS_INITIAL_EXEC=0
LINUX_LOCAL2P_DIRECT_ROUTE=0
LINUX_LOCAL2P_DIRECT_INIT=0
LINUX_LOCAL2P_NO_COOKIE=0
LINUX_LOCAL2P_NO_CAS=0
LINUX_LOCAL2P_OWNER_INBOX=0
LINUX_LOCAL2P_REMOTE_BATCH=0
LINUX_LOCAL2P_REMOTE_BATCH_CAP=16
LINUX_LOCAL2P_TLS_CAP=1
LINUX_LOCAL2P_GLOBAL_CAP=1024
LINUX_LOCAL2P_OBJECT_NODE=0
LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=0
LINUX_LOCAL2P_ROUTE_COOKIE=0
LINUX_LOCAL2P_REUSE_STATE_ONLY=0
LINUX_LOCAL2P_SLIM_CHECK=0
LINUX_LOCAL2P_FAST_COOKIE=0
LINUX_LOCAL2P_FREE_FIRST=0
LINUX_LOCAL2P_TLS_FAST_RETURN=0
LINUX_LOCAL2P_EXACT_API=0
LINUX_LOCAL2P_SINGLE_SLOT_TLS=0
LINUX_LOCAL2P_SPEED_LINKFLAGS=0
LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL=0
LINUX_LOCAL2P_RETAIN_ARRAY=0
LINUX_P11_SPEED_CORE=0
LINUX_P25_BRIDGE_ATTR=0
LINUX_P25_BRIDGE_ATTR_NO_CAS=0
LINUX_P25_BRIDGE_ATTR_NO_COOKIE=0
LINUX_P25_BRIDGE_ATTR_READONLY_STATE=0
LINUX_P43_PREPARED_BRIDGE=1
LINUX_P43_UNSAFE_NO_LOOKUP=0
LINUX_P43_TRUST_FAST_LOOKUP=0
LINUX_P43_TRUST_WRAPPER_SOURCE=0
LINUX_P43_DECODED_RAW_LOOKUP=0
LINUX_P43_RAW_FAST_LOOKUP_ONLY=0
LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY=0
LINUX_P43_WRAPPER_TOKEN=0
LINUX_P43_WRAPPER_TOKEN_BRIDGE=0
TRACE_LANE=0
BUILD_PRELOAD_FULL=0
PRELOAD_FREE_MID_FIRST=0
PRELOAD_FREE_MIDPAGE_FIRST=0
PRELOAD_FREE_MIDPAGE_LARGE_FIRST=0
PRELOAD_MIDPAGE_ALLOC_FIRST=0
PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=0
PRELOAD_MIDPAGE_SUPERFAST=0
PRELOAD_MIDPAGE_TAGGED_FREE=0
PRELOAD_TLS_INITIAL_EXEC=0
PRELOAD_SPEED_LINKFLAGS=0
LINUX_OWNERHUB_R1=0
LINUX_OWNERHUB_R2=0
LINUX_OWNERHUB_R3=0
LINUX_SMALLFRONT_S1=0
LINUX_SMALLFRONT_REMOTE_BATCH_CAP=16
LINUX_SMALLFRONT_DRAIN_EMPTY_GATED=0
LINUX_SMALLFRONT_REMOTE_OUTBOX=0
LINUX_MIDPAGEFRONT_M2=0
LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP=16
LINUX_MIDPAGEFRONT_REGION_ARRAY=0
LINUX_MIDPAGEFRONT_REMOTE_SHADOW=0
LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE=0
LINUX_MIDPAGEFRONT_TLS_REGION_CACHE=0
LINUX_MIDPAGEFRONT_TLS_HOT_SLOT=0
LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST=0
LINUX_MIDPAGEFRONT_SLOT_SWITCH=0
LINUX_MIDPAGEFRONT_NODELESS_RUN=0
LINUX_MIDPAGEFRONT_NODELESS_STATS=0
LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE=0
LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK=0
LINUX_MIDPAGEFRONT_M4_MAGAZINE=0
LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET=0
LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT=0
LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_HIT_INTERVAL=64
LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN=0
LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE=0
LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG=0
LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE=0
LINUX_MIDPAGEFRONT_M5_HIT_ONLY=0
LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE=0
LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE=0
LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL=1
LINUX_MIDPAGEFRONT_M6_RAW_CAP=64
LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET=0
LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP=64
LINUX_MIDPAGEFRONT_M8_OWNER_XFER=0
LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP=64
LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN=0
LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING=0
LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE=0
LINUX_MIDPAGEFRONT_PAGERUN=0
LINUX_MIDPAGEFRONT_PAGERUN_64K=0
LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP=0
LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY=0
LINUX_MIDPAGEFRONT_WIDE_32K_CLASS=0
LINUX_MIDPAGEFRONT_COARSE_BANDS=0
LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE=0
LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL=1
LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP=64
LINUX_MIDPAGEFRONT_M4_STATS=0
LINUX_MIDFRONT_M1=0
LINUX_MIDFRONT_OWNER_FAST_STATE=0
LINUX_MIDFRONT_MAX_BYTES=65536
LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
LINUX_MIDFRONT_REMOTE_OUTBOX=0
LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS=4
LINUX_MIDFRONT_REMOTE_RBUF=0
LINUX_MIDFRONT_REMOTE_RBUF_CAP=128
LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD=96
LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS=0
LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE=0
LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE=0
LINUX_MIDFRONT_LOOKUP_CACHE=0
LINUX_MIDFRONT_DRAIN_ALL_ON_MISS=0
LINUX_MIDFRONT_DRAIN_MASK_ON_MISS=0
LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP=0
LINUX_MIDFRONT_DRAIN_TAKE_FIRST=0
LINUX_MIDFRONT_DRAIN_EMPTY_GATED=0
LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE=0
LINUX_LARGEFRONT_L1=0
LINUX_LARGEFRONT_OWNER_FAST_STATE=0
LINUX_LARGEFRONT_OWNER_INBOX=0
LINUX_LARGEFRONT_REMOTE_BATCH=0
LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=0
LINUX_LARGEFRONT_DRAIN_EMPTY_GATED=0
LINUX_LARGEFRONT_MAP_BASE_ONLY=0
LINUX_LARGEFRONT_REGION_MAP=0
LINUX_LARGEFRONT_LOWER_CLASSES=0
LINUX_LARGEFRONT_ADAPTIVE128=0
LINUX_LARGEFRONT_PAYLOAD_SCAVENGE=0
LINUX_LARGEFRONT_OBSERVE=0
LINUX_LARGEFRONT_REMOTE_BATCH_CAP=16
LINUX_LARGEFRONT_SOURCE_BATCH_COUNT=16
LINUX_LARGEFRONT_SCAVENGE_LOCAL_CAP=4
HZ5_STANDALONE_EXACT_ONLY=1

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
                     + source batch4 for large128-style rows
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

require_value() {
  local opt="$1"
  if [[ $# -lt 2 || -z "${2:-}" || "${2:-}" == --* ]]; then
    echo "missing value for ${opt}" >&2
    exit 1
  fi
}

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
}

enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base() {
  enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base "$1"
  LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1
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

while [[ $# -gt 0 ]]; do
  case "$1" in
    --linux-hz5-profile-pagerun64-main)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base 2
      shift
      ;;
    --linux-hz5-profile-pagerun64-cross128)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base 2
      LINUX_LARGEFRONT_SOURCE_BATCH_COUNT=16
      shift
      ;;
    --linux-hz5-profile-pagerun64-large128)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base 2
      LINUX_LARGEFRONT_SOURCE_BATCH_COUNT=4
      shift
      ;;
    --arch)
      require_value "$@"
      ARCH="$2"
      shift 2
      ;;
    --out-dir)
      require_value "$@"
      OUT_DIR="$2"
      shift 2
      ;;
    --linux-local2p)
      LINUX_LOCAL2P=1
      shift
      ;;
    --linux-local2p-tls-packed)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      shift
      ;;
    --linux-local2p-initial-exec)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      shift
      ;;
    --linux-local2p-direct-route)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      shift
      ;;
    --linux-local2p-direct-init)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      shift
      ;;
    --linux-local2p-fast)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      shift
      ;;
    --linux-local2p-no-cookie)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_NO_COOKIE=1
      shift
      ;;
    --linux-local2p-no-cas)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_NO_CAS=1
      shift
      ;;
    --linux-local2p-owner-inbox)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_OWNER_INBOX=1
      shift
      ;;
    --linux-local2p-remote-batch)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_OWNER_INBOX=1
      LINUX_LOCAL2P_REMOTE_BATCH=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      LINUX_LOCAL2P_ROUTE_COOKIE=1
      LINUX_LOCAL2P_REUSE_STATE_ONLY=1
      LINUX_LOCAL2P_SLIM_CHECK=1
      LINUX_LOCAL2P_FAST_COOKIE=1
      shift
      ;;
    --linux-local2p-remote-batch-cap)
      require_value "$@"
      LINUX_LOCAL2P_REMOTE_BATCH_CAP="$2"
      shift 2
      ;;
    --linux-local2p-tls-cap)
      require_value "$@"
      LINUX_LOCAL2P_TLS_CAP="$2"
      shift 2
      ;;
    --linux-local2p-global-cap)
      require_value "$@"
      LINUX_LOCAL2P_GLOBAL_CAP="$2"
      shift 2
      ;;
    --linux-local2p-object-node)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      shift
      ;;
    --linux-local2p-same-owner-fast-state)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      shift
      ;;
    --linux-local2p-route-cookie)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      LINUX_LOCAL2P_ROUTE_COOKIE=1
      shift
      ;;
    --linux-local2p-reuse-state-only)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      LINUX_LOCAL2P_ROUTE_COOKIE=1
      LINUX_LOCAL2P_REUSE_STATE_ONLY=1
      shift
      ;;
    --linux-local2p-slim-check)
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
      shift
      ;;
    --linux-local2p-fast-cookie)
      enable_local2p_fast_cookie_base
      shift
      ;;
    --linux-local2p-free-first|--linux-local2p-freefirst-fastcookie)
      enable_local2p_fast_cookie_base
      LINUX_LOCAL2P_FREE_FIRST=1
      shift
      ;;
    --linux-local2p-tls-fast-return)
      enable_local2p_tls_fast_return_base
      shift
      ;;
    --linux-local2p-exact-api)
      enable_local2p_exact_api_base
      shift
      ;;
    --linux-local2p-single-slot-tls)
      enable_local2p_exact_api_base
      LINUX_LOCAL2P_SINGLE_SLOT_TLS=1
      shift
      ;;
    --linux-local2p-speed-linkflags)
      enable_local2p_speed_linkflags_base
      shift
      ;;
    --linux-local2p-rss-retain)
      enable_local2p_speed_linkflags_base
      LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL=1
      shift
      ;;
    --linux-local2p-retain-array)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_RETAIN_ARRAY=1
      shift
      ;;
    --linux-p11-speed-core)
      LINUX_P11_SPEED_CORE=1
      shift
      ;;
    --linux-p25-bridge-attr)
      LINUX_P25_BRIDGE_ATTR=1
      shift
      ;;
    --linux-p25-bridge-attr-no-cas)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_NO_CAS=1
      shift
      ;;
    --linux-p25-bridge-attr-no-cookie)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_NO_COOKIE=1
      shift
      ;;
    --linux-p25-bridge-attr-readonly-state)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_READONLY_STATE=1
      shift
      ;;
    --linux-p43)
      ENABLE_LINUX_P43=1
      shift
      ;;
    --linux-p43-no-prepared-bridge)
      ENABLE_LINUX_P43=1
      LINUX_P43_PREPARED_BRIDGE=0
      shift
      ;;
    --linux-p43-unsafe-no-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_UNSAFE_NO_LOOKUP=1
      shift
      ;;
    --linux-p43-trust-fast-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_TRUST_FAST_LOOKUP=1
      shift
      ;;
    --linux-p43-trust-wrapper-source)
      ENABLE_LINUX_P43=1
      LINUX_P43_TRUST_WRAPPER_SOURCE=1
      shift
      ;;
    --linux-p43-decoded-raw-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      shift
      ;;
    --linux-p43-decoded-raw-fastlookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      LINUX_P43_RAW_FAST_LOOKUP_ONLY=1
      shift
      ;;
    --linux-p43-decoded-raw-allocated)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      LINUX_P43_RAW_FAST_LOOKUP_ONLY=1
      LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY=1
      shift
      ;;
    --linux-p43-token)
      ENABLE_LINUX_P43=1
      LINUX_P43_WRAPPER_TOKEN=1
      shift
      ;;
    --linux-p43-token-bridge)
      ENABLE_LINUX_P43=1
      LINUX_P43_WRAPPER_TOKEN=1
      LINUX_P43_WRAPPER_TOKEN_BRIDGE=1
      shift
      ;;
    --linux-preload-full)
      BUILD_PRELOAD_FULL=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-smallfront-s1)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-smallfront-remote-batch-cap)
      require_value "$@"
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP="$2"
      shift 2
      ;;
    --linux-smallfront-drain-empty-gated)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_DRAIN_EMPTY_GATED=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-smallfront-remote-outbox)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-region-outbox)
      enable_general_region_outbox_base
      shift
      ;;
    --linux-hz5-general-directfree)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midfirst)
      BUILD_PRELOAD_FULL=1
      PRELOAD_FREE_MID_FIRST=1
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
      shift
      ;;
    --linux-hz5-general-midcache)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_LOOKUP_CACHE=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midrbuf)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_REMOTE_RBUF=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-routesplit)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_MAX_BYTES=4096
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      LINUX_LARGEFRONT_LOWER_CLASSES=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midpage)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midpage-region)
      enable_midpage_region_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow)
      enable_midpage_region_shadow_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-frontfirst)
      enable_midpage_region_shadow_base
      PRELOAD_FREE_MIDPAGE_FIRST=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-tlscache)
      enable_midpage_region_shadow_base
      LINUX_MIDPAGEFRONT_TLS_REGION_CACHE=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-hotslot)
      enable_midpage_region_shadow_base
      LINUX_MIDPAGEFRONT_TLS_HOT_SLOT=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-activetrust)
      enable_midpage_region_shadow_base
      LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-allocfirst)
      enable_midpage_shadow_allocfirst_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-localunsafe)
      enable_midpage_shadow_allocfirst_base
      LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4mag)
      enable_midpage_m4mag_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet)
      enable_midpage_m4packet_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst)
      enable_midpage_m4packet_freefirst_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink)
      enable_midpage_m4packet_freefirst_tlslink_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-allocelide)
      enable_midpage_m4packet_freefirst_tlslink_allocelide_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-ptrmag)
      enable_midpage_m4packet_freefirst_tlslink_ptrmag_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-absalloc)
      enable_midpage_m4packet_freefirst_tlslink_absalloc_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-regcache)
      enable_midpage_m4packet_freefirst_tlslink_regcache_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-slotswitch)
      enable_midpage_m4packet_freefirst_tlslink_slotswitch_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast)
      enable_midpage_m4packet_freefirst_tlslink_superfast_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide)
      enable_midpage_m4packet_freefirst_tlslink_superfast_freeelide_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m5hit)
      enable_midpage_m4packet_freefirst_tlslink_m5hit_base
      shift
      ;;
    --linux-hz5-general-midpage-m6-deferred-free-direct)
      enable_midpage_m6_deferred_free_direct_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-flatcap)
      enable_midpage_m4packet_freefirst_tlslink_flatcap_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-overarray)
      enable_midpage_m4packet_freefirst_tlslink_overarray_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-wide32k)
      enable_midpage_m4packet_freefirst_tlslink_wide32k_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-16-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-16-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 4
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 5
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rssgov)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rssgov_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m7ticket)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m7ticket_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m8xfer)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m8xfer_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m9budget)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m9budget_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m10slot)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m10slot_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m11direct)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_m11direct_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-takefirst)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_takefirst_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-adaptive128)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_adaptive128_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-scavenge)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_scavenge_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-observe)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_m6remote_pagerun64_observe_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-drainhit)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_drainhit_base 2
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 6
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rssgov)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rssgov_base 6
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rsscheckpoint)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_base 6
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rsscheckpoint-drainhit)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_rsscheckpoint_drainhit_base 6
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band16-32)
      enable_midpage_m4packet_freefirst_tlslink_coarse_bands_base 3
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-tagfree)
      enable_midpage_m4packet_freefirst_tlslink_tagfree_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m4stats)
      enable_midpage_m4packet_freefirst_tlslink_m4stats_base
      shift
      ;;
    --linux-midpagefront-m6-raw-cap)
      require_value "$@"
      LINUX_MIDPAGEFRONT_M6_RAW_CAP="$2"
      shift 2
      ;;
    --linux-midpagefront-m6-deferred-free)
      LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE=1
      shift
      ;;
    --linux-midpagefront-m6-remote-deferred-free)
      LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE=1
      shift
      ;;
    --linux-midpagefront-m6-remote-no-refill-flush)
      LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL=0
      shift
      ;;
    --linux-midpagefront-m7-remote-ticket)
      LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET=1
      shift
      ;;
    --linux-midpagefront-m7-remote-page-cap)
      require_value "$@"
      LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP="$2"
      shift 2
      ;;
    --linux-midpagefront-m8-owner-xfer)
      LINUX_MIDPAGEFRONT_M8_OWNER_XFER=1
      shift
      ;;
    --linux-midpagefront-m8-xfer-page-cap)
      require_value "$@"
      LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP="$2"
      shift 2
      ;;
    --linux-midpagefront-m9-budget-drain)
      LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN=1
      shift
      ;;
    --linux-midpagefront-m10-remote-slot-ring)
      LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING=1
      shift
      ;;
    --linux-midpagefront-m11-remote-direct-cache)
      LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE=1
      shift
      ;;
    --linux-midpagefront-pagerun)
      LINUX_MIDPAGEFRONT_PAGERUN=1
      shift
      ;;
    --linux-midpagefront-pagerun-64k)
      LINUX_MIDPAGEFRONT_PAGERUN_64K=1
      shift
      ;;
    --linux-midpagefront-m4-remote-drain-hit-interval)
      require_value "$@"
      LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_HIT_INTERVAL="$2"
      shift 2
      ;;
    --linux-midpagefront-empty-slab-release)
      LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE=1
      shift
      ;;
    --linux-midpagefront-empty-release-checkpoint)
      LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE=1
      LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL=0
      shift
      ;;
    --linux-midpagefront-empty-retain-cap)
      require_value "$@"
      LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP="$2"
      shift 2
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-routefree)
      enable_midpage_m4packet_routefree_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-m4packet-crossdrain)
      enable_midpage_m4packet_crossdrain_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-slotswitch)
      enable_midpage_shadow_allocfirst_base
      LINUX_MIDPAGEFRONT_SLOT_SWITCH=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-tlslink)
      enable_midpage_shadow_allocfirst_base
      PRELOAD_TLS_INITIAL_EXEC=1
      PRELOAD_SPEED_LINKFLAGS=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-tlsie)
      enable_midpage_shadow_allocfirst_base
      PRELOAD_TLS_INITIAL_EXEC=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-linkonly)
      enable_midpage_shadow_allocfirst_base
      PRELOAD_SPEED_LINKFLAGS=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-nodeless)
      enable_midpage_nodeless_base
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-nodeless-ptrcache)
      enable_midpage_nodeless_base
      LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-nodeless-stats)
      enable_midpage_nodeless_base
      LINUX_MIDPAGEFRONT_NODELESS_STATS=1
      shift
      ;;
    --linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats)
      enable_midpage_nodeless_base
      LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE=1
      LINUX_MIDPAGEFRONT_NODELESS_STATS=1
      shift
      ;;
    --linux-hz5-general-midpage-region-frontfirst)
      BUILD_PRELOAD_FULL=1
      PRELOAD_FREE_MIDPAGE_FIRST=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_REGION_ARRAY=1
      LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midoutbox)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_REMOTE_OUTBOX=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-hz5-general-midoutbox-flush)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_SMALLFRONT_REMOTE_OUTBOX=1
      LINUX_SMALLFRONT_REMOTE_BATCH_CAP=8
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      LINUX_MIDFRONT_REMOTE_BATCH_CAP=16
      LINUX_MIDFRONT_REMOTE_OUTBOX=1
      LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-ownerhub-r1)
      BUILD_PRELOAD_FULL=1
      LINUX_OWNERHUB_R1=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-ownerhub-r2)
      BUILD_PRELOAD_FULL=1
      LINUX_OWNERHUB_R2=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-ownerhub-r3)
      BUILD_PRELOAD_FULL=1
      LINUX_OWNERHUB_R3=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-m1)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-m2)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-region-array)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_REGION_ARRAY=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-remote-shadow)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_REMOTE_SHADOW=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-local-fast-state)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-tls-region-cache)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_REGION_ARRAY=1
      LINUX_MIDPAGEFRONT_TLS_REGION_CACHE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-tls-hot-slot)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_TLS_HOT_SLOT=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-local-active-trust)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDPAGEFRONT_M2=1
      LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midpagefront-remote-batch-cap)
      require_value "$@"
      LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP="$2"
      shift 2
      ;;
    --linux-midfront-owner-fast-state)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_OWNER_FAST_STATE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-max-bytes)
      require_value "$@"
      LINUX_MIDFRONT_MAX_BYTES="$2"
      shift 2
      ;;
    --linux-midfront-remote-batch-cap)
      require_value "$@"
      LINUX_MIDFRONT_REMOTE_BATCH_CAP="$2"
      shift 2
      ;;
    --linux-midfront-remote-outbox)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_OUTBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-remote-outbox-slots)
      require_value "$@"
      LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS="$2"
      shift 2
      ;;
    --linux-midfront-remote-rbuf)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_RBUF=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-remote-rbuf-cap)
      require_value "$@"
      LINUX_MIDFRONT_REMOTE_RBUF_CAP="$2"
      shift 2
      ;;
    --linux-midfront-remote-rbuf-threshold)
      require_value "$@"
      LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD="$2"
      shift 2
      ;;
    --linux-midfront-outbox-flush-on-miss)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_OUTBOX=1
      LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-remote-direct-free-state)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-remote-trust-drain-state)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE=1
      LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-lookup-cache)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_LOOKUP_CACHE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-drain-all-on-miss)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_DRAIN_ALL_ON_MISS=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-drain-mask-on-miss)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_DRAIN_MASK_ON_MISS=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-drain-mask-hit-stop)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_DRAIN_MASK_ON_MISS=1
      LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-drain-take-first)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_DRAIN_TAKE_FIRST=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-drain-empty-gated)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_DRAIN_EMPTY_GATED=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-midfront-remote-global-recycle)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-l1)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-owner-fast-state)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_FAST_STATE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-owner-inbox)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-remote-batch)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_REMOTE_BATCH=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-remote-batch-cap)
      require_value "$@"
      LINUX_LARGEFRONT_REMOTE_BATCH_CAP="$2"
      shift 2
      ;;
    --linux-largefront-source-batch-count)
      require_value "$@"
      LINUX_LARGEFRONT_SOURCE_BATCH_COUNT="$2"
      BUILD_PRELOAD_FULL=1
      shift 2
      ;;
    --linux-largefront-drain-take-first)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-drain-empty-gated)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_DRAIN_EMPTY_GATED=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-map-base-only)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_MAP_BASE_ONLY=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-region-map)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_REGION_MAP=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-adaptive128)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_REGION_MAP=1
      LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1
      LINUX_LARGEFRONT_ADAPTIVE128=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-payload-scavenge)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_REGION_MAP=1
      LINUX_LARGEFRONT_PAYLOAD_SCAVENGE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-observe)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_OWNER_INBOX=1
      LINUX_LARGEFRONT_REGION_MAP=1
      LINUX_LARGEFRONT_OBSERVE=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --linux-largefront-scavenge-local-cap)
      require_value "$@"
      LINUX_LARGEFRONT_SCAVENGE_LOCAL_CAP="$2"
      shift 2
      ;;
    --linux-largefront-lower-classes)
      BUILD_PRELOAD_FULL=1
      LINUX_SMALLFRONT_S1=1
      LINUX_MIDFRONT_M1=1
      LINUX_LARGEFRONT_L1=1
      LINUX_LARGEFRONT_LOWER_CLASSES=1
      HZ5_STANDALONE_EXACT_ONLY=0
      shift
      ;;
    --trace-lane)
      TRACE_LANE=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ "$LINUX_LOCAL2P" -eq 1 && \
      ( "$LINUX_P25_BRIDGE_ATTR" -eq 1 || "$ENABLE_LINUX_P43" -eq 1 ) ]]; then
  echo "local2p, p25attr, and p43 lanes are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_LOCAL2P_NO_COOKIE" -eq 1 && \
      "$LINUX_LOCAL2P_NO_CAS" -eq 1 ]]; then
  echo "local2p diagnostic variants are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_LOCAL2P_REMOTE_BATCH_CAP" -lt 1 ]]; then
  echo "remote batch cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_SMALLFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
  echo "smallfront remote batch cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
  echo "midfront remote batch cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
  echo "midpagefront remote batch cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDPAGEFRONT_M6_RAW_CAP" -lt 1 ]]; then
  echo "midpagefront M6 raw cap must be >= 1" >&2
  exit 1
fi
if [[ "$LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP" -lt 1 ]]; then
  echo "midpagefront M7 remote page cap must be >= 1" >&2
  exit 1
fi
if [[ "$LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP" -lt 1 ]]; then
  echo "midpagefront M8 transfer page cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP" -lt 1 ]]; then
  echo "midpagefront empty retain cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_MAX_BYTES" -lt 2049 || \
      "$LINUX_MIDFRONT_MAX_BYTES" -gt 65536 ]]; then
  echo "midfront max bytes must be in 2049..65536" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS" -lt 1 ]]; then
  echo "midfront remote outbox slots must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_OUTBOX" -eq 1 && \
      "$LINUX_MIDFRONT_REMOTE_RBUF" -eq 1 ]]; then
  echo "midfront remote outbox and rbuf are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_RBUF_CAP" -lt 1 ]]; then
  echo "midfront remote rbuf cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD" -lt 1 || \
      "$LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD" -gt \
      "$LINUX_MIDFRONT_REMOTE_RBUF_CAP" ]]; then
  echo "midfront remote rbuf threshold must be in 1..cap" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE" -eq 1 && \
      "$LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE" -ne 1 ]]; then
  echo "midfront trust-drain-state requires direct-free-state" >&2
  exit 1
fi

if [[ "$LINUX_LARGEFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
  echo "largefront remote batch cap must be >= 1" >&2
  exit 1
fi

if [[ "$LINUX_LARGEFRONT_MAP_BASE_ONLY" -eq 1 && \
      "$LINUX_LARGEFRONT_REGION_MAP" -eq 1 ]]; then
  echo "largefront base-only and region-map diagnostics are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_MIDFRONT_DRAIN_ALL_ON_MISS" -eq 1 && \
      "$LINUX_MIDFRONT_DRAIN_MASK_ON_MISS" -eq 1 ]]; then
  echo "midfront drain-all and drain-mask are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 && "$ENABLE_LINUX_P43" -eq 1 ]]; then
  echo "p25attr and p43 lanes are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 ]]; then
  p25attr_mode_count=0
  [[ "$LINUX_P25_BRIDGE_ATTR_NO_CAS" -eq 1 ]] && ((p25attr_mode_count += 1))
  [[ "$LINUX_P25_BRIDGE_ATTR_NO_COOKIE" -eq 1 ]] && ((p25attr_mode_count += 1))
  [[ "$LINUX_P25_BRIDGE_ATTR_READONLY_STATE" -eq 1 ]] && ((p25attr_mode_count += 1))
  if [[ "$p25attr_mode_count" -gt 1 ]]; then
    echo "p25attr diagnostic variants are mutually exclusive" >&2
    exit 1
  fi
fi

if [[ "$ENABLE_LINUX_P43" -eq 1 ]]; then
  p43_mode_count=0
  [[ "$LINUX_P43_PREPARED_BRIDGE" -eq 0 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_UNSAFE_NO_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_TRUST_FAST_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_TRUST_WRAPPER_SOURCE" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_WRAPPER_TOKEN" -eq 1 ]] && ((p43_mode_count += 1))
  if [[ "$p43_mode_count" -gt 1 ]]; then
    echo "P43 lane selectors are mutually exclusive" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 0 && \
        ( "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 || \
          "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ) ]]; then
    echo "rawlookup submodes require --linux-p43-decoded-raw-lookup" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 && \
        "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ]]; then
    echo "rawlookup fast/allocated submodes are mutually exclusive" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN_BRIDGE" -eq 1 && \
        "$LINUX_P43_WRAPPER_TOKEN" -eq 0 ]]; then
    echo "--linux-p43-token-bridge requires --linux-p43-token" >&2
    exit 1
  fi
fi

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

HZ5_DIR="${ROOT_DIR}/hakozuna-hz5"
SOURCE_COMMIT="$(git -C "$ROOT_DIR" rev-parse HEAD)"
OUT_DIR="${OUT_DIR:-${HZ5_DIR}/out/linux/${ARCH}}"
LIB="${OUT_DIR}/libhakozuna_hz5_standalone.so"
PRELOAD_HYBRID_LIB="${OUT_DIR}/libhakozuna_hz5_preload_hybrid.so"
PRELOAD_FULL_LIB="${OUT_DIR}/libhakozuna_hz5_preload_full.so"
BENCH="${OUT_DIR}/bench_hz5_standalone_aligned64k"
REMOTE_BENCH="${OUT_DIR}/bench_hz5_standalone_remote64k"
RSS_BENCH="${OUT_DIR}/bench_hz5_standalone_rss_plateau"
MIXED_BENCH="${OUT_DIR}/bench_hz5_standalone_mixed_prelude"
SAFETY_BENCH="${OUT_DIR}/bench_hz5_standalone_safety"
MIDPAGE_DIRECT_BENCH="${OUT_DIR}/bench_hz5_midpage_direct"
GENERIC_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"
GENERIC_REMOTE_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_remote64k"
GENERIC_RSS_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_rss_plateau"
GENERIC_MIXED_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_prelude"
BUILD_CONFIG="${OUT_DIR}/hz5_build_config.env"
SPEED_LANE=1
if [[ "$TRACE_LANE" -eq 1 ]]; then
  SPEED_LANE=0
fi

mkdir -p "$OUT_DIR"

COMMON_FLAGS=(
  -O3
  -fPIC
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-unused-variable
  -std=c11
  -D_GNU_SOURCE
  -DNDEBUG
  -DHZ5_DIAGNOSTIC_STATS=0
  -DBENCHLAB_HZ5_SPEED_LANE="${SPEED_LANE}"
  -DBENCHLAB_HZ5_TRACE_LANE="${TRACE_LANE}"
  -DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1
  -DBENCHLAB_HZ5_STANDALONE_EXACT_ONLY="${HZ5_STANDALONE_EXACT_ONLY}"
  -DBENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192=1
  -DBENCHLAB_HZ5_P25_STATS=0
  -DHZ5_P11_SPEED_CORE="${LINUX_P11_SPEED_CORE}u"
  -DHZ5_DESC_SOURCE_COMMIT=\"${SOURCE_COMMIT}\"
  -I"${HZ5_DIR}/include"
  -I"${HZ5_DIR}/contract"
  -I"${HZ5_DIR}/policy"
  -I"${HZ5_DIR}/route"
  -I"${HZ5_DIR}/wrapper"
  -I"${HZ5_DIR}/lowpage"
  -I"${HZ5_DIR}/fallback"
  -I"${HZ5_DIR}/smallfront"
  -I"${HZ5_DIR}/midpagefront"
  -I"${HZ5_DIR}/midfront"
  -I"${HZ5_DIR}/largefront"
  -I"${HZ5_DIR}/ownerhub"
)
SPEED_LINK_COMPILE_FLAGS=()
SHARED_LINK_FLAGS=()
if [[ "$LINUX_LOCAL2P_SPEED_LINKFLAGS" -eq 1 ||
      "$PRELOAD_SPEED_LINKFLAGS" -eq 1 ]]; then
  SPEED_LINK_COMPILE_FLAGS+=(
    -fno-semantic-interposition
    -fno-plt
    -fno-stack-protector
  )
  if [[ "$ARCH" == "x86_64" || "$ARCH" == "amd64" ]]; then
    SPEED_LINK_COMPILE_FLAGS+=(-fcf-protection=none)
  fi
  SHARED_LINK_FLAGS+=(-Wl,-Bsymbolic-functions)
fi
COMMON_FLAGS+=("${SPEED_LINK_COMPILE_FLAGS[@]}")

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR=1)
  if [[ "$LINUX_P25_BRIDGE_ATTR_NO_CAS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS=1)
  fi
  if [[ "$LINUX_P25_BRIDGE_ATTR_NO_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE=1)
  fi
  if [[ "$LINUX_P25_BRIDGE_ATTR_READONLY_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE=1)
  fi
fi

if [[ "$LINUX_LOCAL2P" -eq 1 ]]; then
  COMMON_FLAGS+=(
    -DBENCHLAB_HZ5_LINUX_LOCAL2P=1
    -DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP="${LINUX_LOCAL2P_TLS_CAP}u"
    -DBENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP="${LINUX_LOCAL2P_GLOBAL_CAP}u"
  )
  if [[ "$LINUX_LOCAL2P_TLS_PACKED" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED=1)
  fi
  if [[ "$LINUX_LOCAL2P_DIRECT_ROUTE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE=1)
  fi
  if [[ "$LINUX_LOCAL2P_DIRECT_INIT" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT=1)
  fi
  if [[ "$LINUX_LOCAL2P_TLS_INITIAL_EXEC" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_INITIAL_EXEC=1)
    COMMON_FLAGS+=(-ftls-model=initial-exec)
  fi
  if [[ "$LINUX_LOCAL2P_NO_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE=1)
  fi
  if [[ "$LINUX_LOCAL2P_NO_CAS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_NO_CAS=1)
  fi
  if [[ "$LINUX_LOCAL2P_OWNER_INBOX" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX=1)
  fi
  if [[ "$LINUX_LOCAL2P_REMOTE_BATCH" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH=1)
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH_CAP="${LINUX_LOCAL2P_REMOTE_BATCH_CAP}u"
    )
  fi
  if [[ "$LINUX_LOCAL2P_OBJECT_NODE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE=1)
  fi
  if [[ "$LINUX_LOCAL2P_SAME_OWNER_FAST_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1)
  fi
  if [[ "$LINUX_LOCAL2P_ROUTE_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE=1)
  fi
  if [[ "$LINUX_LOCAL2P_REUSE_STATE_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY=1)
  fi
  if [[ "$LINUX_LOCAL2P_SLIM_CHECK" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_SLIM_CHECK=1)
  fi
  if [[ "$LINUX_LOCAL2P_FAST_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_FAST_COOKIE=1)
  fi
  if [[ "$LINUX_LOCAL2P_FREE_FIRST" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST=1)
  fi
  if [[ "$LINUX_LOCAL2P_TLS_FAST_RETURN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_FAST_RETURN=1)
  fi
  if [[ "$LINUX_LOCAL2P_SINGLE_SLOT_TLS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS=1)
  fi
  if [[ "$LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL=1)
  fi
  if [[ "$LINUX_LOCAL2P_RETAIN_ARRAY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY=1)
  fi
fi

if [[ "$ENABLE_LINUX_P43" -eq 1 ]]; then
  COMMON_FLAGS+=(
    -DBENCHLAB_HZ5_LINUX_P43_PORT=1
    -DBENCHLAB_HZ5_P43_SEGMENT_SLOTS=1
    -DBENCHLAB_HZ5_P43_FAST_LOOKUP=1
    -DBENCHLAB_HZ5_P43_LOCKLESS_LOOKUP=1
    -DBENCHLAB_HZ5_P43_SLOT_DECOMMIT=0
    -DBENCHLAB_HZ5_P43_PAGE_NOACCESS=0
  )
  if [[ "$LINUX_P43_PREPARED_BRIDGE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_P43_PREPARED_BRIDGE=1
      -DBENCHLAB_HZ5_P43_LOCKLESS_CONTRACT=1
    )
  fi
  if [[ "$LINUX_P43_UNSAFE_NO_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_UNSAFE_NO_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_TRUST_FAST_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_TRUST_FAST_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_TRUST_WRAPPER_SOURCE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE=1)
  fi
  if [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_RAW_FAST_LOOKUP_ONLY=1)
  fi
  if [[ "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_RAW_ALLOCATED_LOOKUP_ONLY=1)
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_WRAPPER_TOKEN=1)
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN_BRIDGE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE=1)
  fi
fi

if [[ "$LINUX_SMALLFRONT_S1" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_SMALLFRONT_S1=1)
  COMMON_FLAGS+=(
    -DHZ5_SMALLFRONT_REMOTE_BATCH_CAP="${LINUX_SMALLFRONT_REMOTE_BATCH_CAP}u"
  )
  if [[ "$LINUX_SMALLFRONT_DRAIN_EMPTY_GATED" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_SMALLFRONT_DRAIN_EMPTY_GATED=1)
  fi
  if [[ "$LINUX_SMALLFRONT_REMOTE_OUTBOX" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_SMALLFRONT_REMOTE_OUTBOX=1)
  fi
fi
if [[ "$LINUX_MIDPAGEFRONT_M2" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2=1)
  COMMON_FLAGS+=(
    -DHZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP="${LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP}u"
  )
  if [[ "$LINUX_MIDPAGEFRONT_REGION_ARRAY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_REMOTE_SHADOW" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_TLS_REGION_CACHE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_TLS_HOT_SLOT" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_SLOT_SWITCH" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_SLOT_SWITCH=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_NODELESS_RUN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_NODELESS_STATS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_MAGAZINE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT=1
      -DHZ5_MIDPAGEFRONT_REMOTE_DRAIN_HIT_INTERVAL="${LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_HIT_INTERVAL}u"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M5_HIT_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE=1
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE=1
    )
    if [[ "$LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL" -eq 0 ]]; then
      COMMON_FLAGS+=(
        -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL=0)
    fi
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE" -eq 1 || \
        "$LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DHZ5_MIDPAGEFRONT_M6_RAW_CAP="${LINUX_MIDPAGEFRONT_M6_RAW_CAP}u"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET=1
      -DHZ5_MIDPAGEFRONT_M6_RAW_CAP="${LINUX_MIDPAGEFRONT_M6_RAW_CAP}u"
      -DHZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP="${LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP}u"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M8_OWNER_XFER" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER=1
      -DHZ5_MIDPAGEFRONT_M8_XFER_PAGE_CAP="${LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP}u"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING=1
      -DHZ5_MIDPAGEFRONT_M6_RAW_CAP="${LINUX_MIDPAGEFRONT_M6_RAW_CAP}u"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE=1
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_PAGERUN" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN=1
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_PAGERUN_64K" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K=1
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_WIDE_32K_CLASS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_WIDE_32K_CLASS=1)
  fi
  if [[ "$LINUX_MIDPAGEFRONT_COARSE_BANDS" -ne 0 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS="${LINUX_MIDPAGEFRONT_COARSE_BANDS}"
    )
  fi
  if [[ "$LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE=1
      -DHZ5_MIDPAGEFRONT_EMPTY_RETAIN_CAP="${LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP}u"
    )
    if [[ "$LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL" -eq 0 ]]; then
      COMMON_FLAGS+=(
        -DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL=0)
    fi
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M4_STATS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_STATS=1)
  fi
fi
if [[ "$LINUX_OWNERHUB_R1" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_OWNERHUB_R1=1)
fi
if [[ "$LINUX_OWNERHUB_R2" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_OWNERHUB_R2=1)
fi
if [[ "$LINUX_OWNERHUB_R3" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_OWNERHUB_R3=1)
fi
if [[ "$LINUX_MIDFRONT_M1" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_M1=1)
  COMMON_FLAGS+=(
    -DHZ5_MIDFRONT_MAX_BYTES="${LINUX_MIDFRONT_MAX_BYTES}u"
    -DHZ5_MIDFRONT_REMOTE_BATCH_CAP="${LINUX_MIDFRONT_REMOTE_BATCH_CAP}u"
  )
  if [[ "$LINUX_MIDFRONT_OWNER_FAST_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_OWNER_FAST_STATE=1)
  fi
  if [[ "$LINUX_MIDFRONT_DRAIN_ALL_ON_MISS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_ALL_ON_MISS=1)
  fi
  if [[ "$LINUX_MIDFRONT_DRAIN_MASK_ON_MISS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS=1)
  fi
  if [[ "$LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP=1)
  fi
  if [[ "$LINUX_MIDFRONT_DRAIN_TAKE_FIRST" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_TAKE_FIRST=1)
  fi
  if [[ "$LINUX_MIDFRONT_DRAIN_EMPTY_GATED" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_EMPTY_GATED=1)
  fi
  if [[ "$LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE=1)
  fi
  if [[ "$LINUX_MIDFRONT_REMOTE_OUTBOX" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX=1)
    COMMON_FLAGS+=(
      -DHZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS="${LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS}u"
    )
  fi
  if [[ "$LINUX_MIDFRONT_REMOTE_RBUF" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_RBUF=1)
    COMMON_FLAGS+=(
      -DHZ5_MIDFRONT_REMOTE_RBUF_CAP="${LINUX_MIDFRONT_REMOTE_RBUF_CAP}u"
      -DHZ5_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD="${LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD}u"
    )
  fi
  if [[ "$LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS=1)
  fi
  if [[ "$LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE=1)
  fi
  if [[ "$LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE=1)
  fi
  if [[ "$LINUX_MIDFRONT_LOOKUP_CACHE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_MIDFRONT_LOOKUP_CACHE=1)
  fi
fi
if [[ "$LINUX_LARGEFRONT_L1" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_L1=1)
  COMMON_FLAGS+=(
    -DHZ5_LARGEFRONT_SOURCE_BATCH_COUNT="${LINUX_LARGEFRONT_SOURCE_BATCH_COUNT}u"
  )
  if [[ "$LINUX_LARGEFRONT_OWNER_FAST_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE=1)
  fi
  if [[ "$LINUX_LARGEFRONT_OWNER_INBOX" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
  fi
  if [[ "$LINUX_LARGEFRONT_REMOTE_BATCH" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH=1)
    COMMON_FLAGS+=(
      -DHZ5_LARGEFRONT_REMOTE_BATCH_CAP="${LINUX_LARGEFRONT_REMOTE_BATCH_CAP}u"
    )
  fi
  if [[ "$LINUX_LARGEFRONT_DRAIN_TAKE_FIRST" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1)
  fi
  if [[ "$LINUX_LARGEFRONT_DRAIN_EMPTY_GATED" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED=1)
  fi
  if [[ "$LINUX_LARGEFRONT_MAP_BASE_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_MAP_BASE_ONLY=1)
  fi
  if [[ "$LINUX_LARGEFRONT_REGION_MAP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP=1)
  fi
  if [[ "$LINUX_LARGEFRONT_ADAPTIVE128" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128=1)
  fi
  if [[ "$LINUX_LARGEFRONT_PAYLOAD_SCAVENGE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE=1)
    COMMON_FLAGS+=(
      -DHZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP="${LINUX_LARGEFRONT_SCAVENGE_LOCAL_CAP}u"
    )
  fi
  if [[ "$LINUX_LARGEFRONT_OBSERVE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP=1)
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE=1)
  fi
  if [[ "$LINUX_LARGEFRONT_LOWER_CLASSES" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES=1)
  fi
fi

{
  echo "commit=${SOURCE_COMMIT}"
  if [[ -n "$(git -C "$ROOT_DIR" status --porcelain --untracked-files=all)" ]]; then
    echo "dirty=1"
  else
    echo "dirty=0"
  fi
  echo "arch=${ARCH}"
  echo "trace_lane=${TRACE_LANE}"
  echo "speed_lane=${SPEED_LANE}"
  echo "linux_local2p=${LINUX_LOCAL2P}"
  echo "linux_local2p_tls_packed=${LINUX_LOCAL2P_TLS_PACKED}"
  echo "linux_local2p_tls_initial_exec=${LINUX_LOCAL2P_TLS_INITIAL_EXEC}"
  echo "linux_local2p_direct_route=${LINUX_LOCAL2P_DIRECT_ROUTE}"
  echo "linux_local2p_direct_init=${LINUX_LOCAL2P_DIRECT_INIT}"
  echo "linux_local2p_no_cookie=${LINUX_LOCAL2P_NO_COOKIE}"
  echo "linux_local2p_no_cas=${LINUX_LOCAL2P_NO_CAS}"
  echo "linux_local2p_owner_inbox=${LINUX_LOCAL2P_OWNER_INBOX}"
  echo "linux_local2p_remote_batch=${LINUX_LOCAL2P_REMOTE_BATCH}"
  echo "linux_local2p_remote_batch_cap=${LINUX_LOCAL2P_REMOTE_BATCH_CAP}"
  echo "linux_local2p_tls_cap=${LINUX_LOCAL2P_TLS_CAP}"
  echo "linux_local2p_global_cap=${LINUX_LOCAL2P_GLOBAL_CAP}"
  echo "linux_local2p_object_node=${LINUX_LOCAL2P_OBJECT_NODE}"
  echo "linux_local2p_same_owner_fast_state=${LINUX_LOCAL2P_SAME_OWNER_FAST_STATE}"
  echo "linux_local2p_route_cookie=${LINUX_LOCAL2P_ROUTE_COOKIE}"
  echo "linux_local2p_reuse_state_only=${LINUX_LOCAL2P_REUSE_STATE_ONLY}"
  echo "linux_local2p_slim_check=${LINUX_LOCAL2P_SLIM_CHECK}"
  echo "linux_local2p_fast_cookie=${LINUX_LOCAL2P_FAST_COOKIE}"
  echo "linux_local2p_free_first=${LINUX_LOCAL2P_FREE_FIRST}"
  echo "linux_local2p_tls_fast_return=${LINUX_LOCAL2P_TLS_FAST_RETURN}"
  echo "linux_local2p_exact_api=${LINUX_LOCAL2P_EXACT_API}"
  echo "linux_local2p_single_slot_tls=${LINUX_LOCAL2P_SINGLE_SLOT_TLS}"
  echo "linux_local2p_speed_linkflags=${LINUX_LOCAL2P_SPEED_LINKFLAGS}"
  echo "linux_local2p_local_overflow_global=${LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL}"
  echo "build_preload_full=${BUILD_PRELOAD_FULL}"
  echo "preload_free_mid_first=${PRELOAD_FREE_MID_FIRST}"
  echo "preload_free_midpage_first=${PRELOAD_FREE_MIDPAGE_FIRST}"
  echo "preload_free_midpage_large_first=${PRELOAD_FREE_MIDPAGE_LARGE_FIRST}"
  echo "preload_midpage_alloc_first=${PRELOAD_MIDPAGE_ALLOC_FIRST}"
  echo "preload_midpage_alloc_abs_first=${PRELOAD_MIDPAGE_ALLOC_ABS_FIRST}"
  echo "preload_midpage_superfast=${PRELOAD_MIDPAGE_SUPERFAST}"
  echo "preload_midpage_tagged_free=${PRELOAD_MIDPAGE_TAGGED_FREE}"
  echo "preload_tls_initial_exec=${PRELOAD_TLS_INITIAL_EXEC}"
  echo "preload_speed_linkflags=${PRELOAD_SPEED_LINKFLAGS}"
  echo "linux_ownerhub_r1=${LINUX_OWNERHUB_R1}"
  echo "linux_ownerhub_r2=${LINUX_OWNERHUB_R2}"
  echo "linux_ownerhub_r3=${LINUX_OWNERHUB_R3}"
  echo "linux_smallfront_s1=${LINUX_SMALLFRONT_S1}"
  echo "linux_smallfront_remote_batch_cap=${LINUX_SMALLFRONT_REMOTE_BATCH_CAP}"
  echo "linux_smallfront_drain_empty_gated=${LINUX_SMALLFRONT_DRAIN_EMPTY_GATED}"
  echo "linux_smallfront_remote_outbox=${LINUX_SMALLFRONT_REMOTE_OUTBOX}"
  echo "linux_midpagefront_m2=${LINUX_MIDPAGEFRONT_M2}"
  echo "linux_midpagefront_remote_batch_cap=${LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP}"
  echo "linux_midpagefront_region_array=${LINUX_MIDPAGEFRONT_REGION_ARRAY}"
  echo "linux_midpagefront_remote_shadow=${LINUX_MIDPAGEFRONT_REMOTE_SHADOW}"
  echo "linux_midpagefront_local_fast_state=${LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE}"
  echo "linux_midpagefront_tls_region_cache=${LINUX_MIDPAGEFRONT_TLS_REGION_CACHE}"
  echo "linux_midpagefront_tls_hot_slot=${LINUX_MIDPAGEFRONT_TLS_HOT_SLOT}"
  echo "linux_midpagefront_local_active_trust=${LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST}"
  echo "linux_midpagefront_slot_switch=${LINUX_MIDPAGEFRONT_SLOT_SWITCH}"
  echo "linux_midpagefront_nodeless_run=${LINUX_MIDPAGEFRONT_NODELESS_RUN}"
  echo "linux_midpagefront_nodeless_ptrcache=${LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE}"
  echo "linux_midpagefront_nodeless_stats=${LINUX_MIDPAGEFRONT_NODELESS_STATS}"
  echo "linux_midpagefront_unsafe_local_nocheck=${LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK}"
  echo "linux_midpagefront_m4_magazine=${LINUX_MIDPAGEFRONT_M4_MAGAZINE}"
  echo "linux_midpagefront_m4_remote_packet=${LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET}"
  echo "linux_midpagefront_m4_remote_drain_on_hit=${LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT}"
  echo "linux_midpagefront_m4_remote_drain_hit_interval=${LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_HIT_INTERVAL}"
  echo "linux_midpagefront_m4_cross_drain=${LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN}"
  echo "linux_midpagefront_m4_unsafe_alloc_elide=${LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE}"
  echo "linux_midpagefront_m4_unsafe_ptr_mag=${LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG}"
  echo "linux_midpagefront_m4_unsafe_free_elide=${LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE}"
  echo "linux_midpagefront_m5_hit_only=${LINUX_MIDPAGEFRONT_M5_HIT_ONLY}"
  echo "linux_midpagefront_m6_deferred_free=${LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE}"
  echo "linux_midpagefront_m6_remote_deferred_free=${LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE}"
  echo "linux_midpagefront_m6_remote_flush_on_refill=${LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL}"
  echo "linux_midpagefront_m6_raw_cap=${LINUX_MIDPAGEFRONT_M6_RAW_CAP}"
  echo "linux_midpagefront_m7_remote_ticket=${LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET}"
  echo "linux_midpagefront_m7_remote_page_cap=${LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP}"
  echo "linux_midpagefront_m8_owner_xfer=${LINUX_MIDPAGEFRONT_M8_OWNER_XFER}"
  echo "linux_midpagefront_m8_xfer_page_cap=${LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP}"
  echo "linux_midpagefront_m9_budget_drain=${LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN}"
  echo "linux_midpagefront_m10_remote_slot_ring=${LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING}"
  echo "linux_midpagefront_m11_remote_direct_cache=${LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE}"
  echo "linux_midpagefront_pagerun=${LINUX_MIDPAGEFRONT_PAGERUN}"
  echo "linux_midpagefront_pagerun_64k=${LINUX_MIDPAGEFRONT_PAGERUN_64K}"
  echo "linux_midpagefront_m4_flat_mag_cap=${LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP}"
  echo "linux_midpagefront_m4_overflow_array=${LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY}"
  echo "linux_midpagefront_wide_32k_class=${LINUX_MIDPAGEFRONT_WIDE_32K_CLASS}"
  echo "linux_midpagefront_coarse_bands=${LINUX_MIDPAGEFRONT_COARSE_BANDS}"
  echo "linux_midpagefront_empty_slab_release=${LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE}"
  echo "linux_midpagefront_empty_release_on_refill=${LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL}"
  echo "linux_midpagefront_empty_retain_cap=${LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP}"
  echo "linux_midpagefront_m4_stats=${LINUX_MIDPAGEFRONT_M4_STATS}"
  echo "linux_midfront_m1=${LINUX_MIDFRONT_M1}"
  echo "linux_midfront_owner_fast_state=${LINUX_MIDFRONT_OWNER_FAST_STATE}"
  echo "linux_midfront_max_bytes=${LINUX_MIDFRONT_MAX_BYTES}"
  echo "linux_midfront_remote_batch_cap=${LINUX_MIDFRONT_REMOTE_BATCH_CAP}"
  echo "linux_midfront_remote_outbox=${LINUX_MIDFRONT_REMOTE_OUTBOX}"
  echo "linux_midfront_remote_outbox_slots=${LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS}"
  echo "linux_midfront_remote_rbuf=${LINUX_MIDFRONT_REMOTE_RBUF}"
  echo "linux_midfront_remote_rbuf_cap=${LINUX_MIDFRONT_REMOTE_RBUF_CAP}"
  echo "linux_midfront_remote_rbuf_flush_threshold=${LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD}"
  echo "linux_midfront_outbox_flush_on_miss=${LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS}"
  echo "linux_midfront_remote_direct_free_state=${LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE}"
  echo "linux_midfront_remote_trust_drain_state=${LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE}"
  echo "linux_midfront_lookup_cache=${LINUX_MIDFRONT_LOOKUP_CACHE}"
  echo "linux_midfront_drain_all_on_miss=${LINUX_MIDFRONT_DRAIN_ALL_ON_MISS}"
  echo "linux_midfront_drain_mask_on_miss=${LINUX_MIDFRONT_DRAIN_MASK_ON_MISS}"
  echo "linux_midfront_drain_mask_hit_stop=${LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP}"
  echo "linux_midfront_drain_take_first=${LINUX_MIDFRONT_DRAIN_TAKE_FIRST}"
  echo "linux_midfront_drain_empty_gated=${LINUX_MIDFRONT_DRAIN_EMPTY_GATED}"
  echo "linux_midfront_remote_global_recycle=${LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE}"
  echo "linux_largefront_l1=${LINUX_LARGEFRONT_L1}"
  echo "linux_largefront_owner_fast_state=${LINUX_LARGEFRONT_OWNER_FAST_STATE}"
  echo "linux_largefront_owner_inbox=${LINUX_LARGEFRONT_OWNER_INBOX}"
  echo "linux_largefront_remote_batch=${LINUX_LARGEFRONT_REMOTE_BATCH}"
  echo "linux_largefront_drain_take_first=${LINUX_LARGEFRONT_DRAIN_TAKE_FIRST}"
  echo "linux_largefront_drain_empty_gated=${LINUX_LARGEFRONT_DRAIN_EMPTY_GATED}"
  echo "linux_largefront_map_base_only=${LINUX_LARGEFRONT_MAP_BASE_ONLY}"
  echo "linux_largefront_region_map=${LINUX_LARGEFRONT_REGION_MAP}"
  echo "linux_largefront_lower_classes=${LINUX_LARGEFRONT_LOWER_CLASSES}"
  echo "linux_largefront_adaptive128=${LINUX_LARGEFRONT_ADAPTIVE128}"
  echo "linux_largefront_payload_scavenge=${LINUX_LARGEFRONT_PAYLOAD_SCAVENGE}"
  echo "linux_largefront_observe=${LINUX_LARGEFRONT_OBSERVE}"
  echo "linux_largefront_remote_batch_cap=${LINUX_LARGEFRONT_REMOTE_BATCH_CAP}"
  echo "linux_largefront_source_batch_count=${LINUX_LARGEFRONT_SOURCE_BATCH_COUNT}"
  echo "linux_largefront_scavenge_local_cap=${LINUX_LARGEFRONT_SCAVENGE_LOCAL_CAP}"
  echo "standalone_exact_only=${HZ5_STANDALONE_EXACT_ONLY}"
  echo "linux_p11_speed_core=${LINUX_P11_SPEED_CORE}"
  echo "linux_p25_bridge_attr=${LINUX_P25_BRIDGE_ATTR}"
  echo "linux_p25_bridge_attr_no_cas=${LINUX_P25_BRIDGE_ATTR_NO_CAS}"
  echo "linux_p25_bridge_attr_no_cookie=${LINUX_P25_BRIDGE_ATTR_NO_COOKIE}"
  echo "linux_p25_bridge_attr_readonly_state=${LINUX_P25_BRIDGE_ATTR_READONLY_STATE}"
  echo "enable_linux_p43=${ENABLE_LINUX_P43}"
  echo "linux_p43_prepared_bridge=${LINUX_P43_PREPARED_BRIDGE}"
  echo "linux_p43_trust_wrapper_source=${LINUX_P43_TRUST_WRAPPER_SOURCE}"
  echo "linux_p43_wrapper_token=${LINUX_P43_WRAPPER_TOKEN}"
  echo "linux_p43_wrapper_token_bridge=${LINUX_P43_WRAPPER_TOKEN_BRIDGE}"
} > "$BUILD_CONFIG"

if [[ "$PRELOAD_FREE_MID_FIRST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_FREE_MID_FIRST=1)
fi
if [[ "$PRELOAD_FREE_MIDPAGE_FIRST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_FREE_MIDPAGE_FIRST=1)
fi
if [[ "$PRELOAD_FREE_MIDPAGE_LARGE_FIRST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_FREE_MIDPAGE_LARGE_FIRST=1)
fi
if [[ "$PRELOAD_MIDPAGE_ALLOC_FIRST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_MIDPAGE_ALLOC_FIRST=1)
fi
if [[ "$PRELOAD_MIDPAGE_ALLOC_ABS_FIRST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=1)
fi
if [[ "$PRELOAD_MIDPAGE_SUPERFAST" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_MIDPAGE_SUPERFAST=1)
fi
if [[ "$PRELOAD_MIDPAGE_TAGGED_FREE" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_MIDPAGE_TAGGED_FREE=1)
fi
if [[ "$PRELOAD_TLS_INITIAL_EXEC" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_PRELOAD_TLS_INITIAL_EXEC=1)
  COMMON_FLAGS+=(-ftls-model=initial-exec)
fi

HZ5_SRCS=(
  "${HZ5_DIR}/api/hz5_api.c"
  "${HZ5_DIR}/contract/hz5_contract.c"
  "${HZ5_DIR}/core/hz5_segment.c"
  "${HZ5_DIR}/core/hz5_run.c"
  "${HZ5_DIR}/core/hz5_owner.c"
  "${HZ5_DIR}/core/hz5_remote.c"
  "${HZ5_DIR}/core/hz5_tcache.c"
  "${HZ5_DIR}/core/hz5_stats.c"
  "${HZ5_DIR}/policy/hz5_policy.c"
  "${HZ5_DIR}/policy/hz5_trace.c"
  "${HZ5_DIR}/route/hz5_route.c"
  "${HZ5_DIR}/ownerhub/hz5_ownerhub.c"
  "${HZ5_DIR}/smallfront/hz5_smallfront.c"
  "${HZ5_DIR}/midpagefront/hz5_midpagefront.c"
  "${HZ5_DIR}/midfront/hz5_midfront.c"
  "${HZ5_DIR}/largefront/hz5_largefront.c"
  "${HZ5_DIR}/wrapper/hz5_wrapper.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64_os.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64_p43_segment.c"
  "${HZ5_DIR}/linux/hz5_linux_compat.c"
)

echo "[linux][hz5] arch: ${ARCH}"
echo "[linux][hz5] building library: ${LIB}"
gcc "${COMMON_FLAGS[@]}" -shared "${HZ5_SRCS[@]}" \
  "${SHARED_LINK_FLAGS[@]}" -pthread -ldl -o "$LIB"

echo "[linux][hz5] building preload hybrid library: ${PRELOAD_HYBRID_LIB}"
gcc "${COMMON_FLAGS[@]}" -shared \
  "${HZ5_SRCS[@]}" \
  "${HZ5_DIR}/preload/hz5_preload_hybrid.c" \
  "${SHARED_LINK_FLAGS[@]}" -pthread -ldl -o "$PRELOAD_HYBRID_LIB"

if [[ "$BUILD_PRELOAD_FULL" -eq 1 ]]; then
  echo "[linux][hz5] building preload full library: ${PRELOAD_FULL_LIB}"
  gcc "${COMMON_FLAGS[@]}" -shared \
    "${HZ5_SRCS[@]}" \
    "${HZ5_DIR}/preload/hz5_preload_full.c" \
    "${SHARED_LINK_FLAGS[@]}" -pthread -ldl -o "$PRELOAD_FULL_LIB"
fi

echo "[linux][hz5] building benchmark: ${BENCH}"
BENCH_ALIGNED_FLAGS=()
if [[ "$LINUX_LOCAL2P_EXACT_API" -eq 1 ]]; then
  BENCH_ALIGNED_FLAGS+=(-DBENCHLAB_HZ5_EXACT_LOCAL2P_API=1)
fi
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${SPEED_LINK_COMPILE_FLAGS[@]}" \
  "${BENCH_ALIGNED_FLAGS[@]}" \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_aligned64k.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$BENCH"

echo "[linux][hz5] building remote benchmark: ${REMOTE_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${SPEED_LINK_COMPILE_FLAGS[@]}" \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_remote64k.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$REMOTE_BENCH"

echo "[linux][hz5] building RSS plateau benchmark: ${RSS_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${SPEED_LINK_COMPILE_FLAGS[@]}" \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_rss_plateau.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$RSS_BENCH"

echo "[linux][hz5] building mixed prelude benchmark: ${MIXED_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${SPEED_LINK_COMPILE_FLAGS[@]}" \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_mixed_prelude.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$MIXED_BENCH"

echo "[linux][hz5] building safety benchmark: ${SAFETY_BENCH}"
gcc "${COMMON_FLAGS[@]}" -Werror -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_hz5_standalone_safety.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$SAFETY_BENCH"

echo "[linux][hz5] building MidPage direct benchmark: ${MIDPAGE_DIRECT_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${SPEED_LINK_COMPILE_FLAGS[@]}" \
  -I"${HZ5_DIR}/include" \
  -I"${HZ5_DIR}/midpagefront" \
  "${ROOT_DIR}/bench/bench_hz5_midpage_direct.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$MIDPAGE_DIRECT_BENCH"

mkdir -p "$(dirname "$GENERIC_BENCH")"
echo "[linux][hz5] building generic aligned benchmark: ${GENERIC_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_aligned64k.c" \
  -pthread -o "$GENERIC_BENCH"

echo "[linux][hz5] building generic remote benchmark: ${GENERIC_REMOTE_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_remote64k.c" \
  -pthread -o "$GENERIC_REMOTE_BENCH"

echo "[linux][hz5] building generic RSS plateau benchmark: ${GENERIC_RSS_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_rss_plateau.c" \
  -pthread -o "$GENERIC_RSS_BENCH"

echo "[linux][hz5] building generic mixed prelude benchmark: ${GENERIC_MIXED_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_mixed_prelude.c" \
  -pthread -o "$GENERIC_MIXED_BENCH"

echo "[linux][hz5] done"
