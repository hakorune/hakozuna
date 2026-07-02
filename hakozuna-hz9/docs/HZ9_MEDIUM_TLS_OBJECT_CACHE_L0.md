# HZ9 Medium TLS Object Cache L0

## Goal

```text
HZ9MediumTLSObjectCache-L0
```

Test whether HZ9 can improve medium/main local throughput by giving medium
malloc a separate TLS object-cache entry before the HZ8 medium active-run path.

This replaces the failed active-run magazine direction. The active-run magazine
reached its target slots, but `fixed64_local0` stayed around `0.30x`, including
in an unsafe TLS-only ceiling build. The loss is therefore the insertion shape,
not only the `LOCAL_MAG` slot_state transition.

## Identity

```text
HZ8:
  balanced low-RSS line

HZ9:
  throughput-first line
  higher RSS is acceptable only under an explicit cache cap
  HZ8 safety boundary remains a differentiator from HZ3
```

## Core Shape

```text
malloc:
  size -> medium class
  HZ9 TLS cache pop
  hit returns immediately
  miss falls back to HZ8 medium allocator

free:
  HZ8 route / directory / owner / slot decode / pending / slot_state
  same-owner valid medium slot may push to TLS cache
  remote / invalid / non-eligible falls back to HZ8 path
```

The key is that malloc cache hits do not enter:

```text
active run selection
free_mask mutation
allocated_mask mutation
mark_live_on_alloc
owner-list scan
residency transition
```

## Data Structure

Implemented L0 cache is intentionally small and bounded.

```c
typedef struct H9MediumCacheEntry {
    H8MediumRun* run;
    void* ptr;
    uint16_t slot;
    uint16_t class_id;
} H9MediumCacheEntry;

typedef struct H9MediumClassCache {
    uint16_t count;
    uint16_t capacity;
    size_t cached_bytes;
    H9MediumCacheEntry entries[H9_MEDIUM_CACHE_DEPTH_MAX];
} H9MediumClassCache;

typedef struct H9MediumLocalCache {
    H9MediumClassCache by_class[H8_MEDIUM_CLASS_COUNT];
    size_t cached_bytes;
    size_t cached_bytes_peak;
} H9MediumLocalCache;
```

Current L0 depth:

```text
all medium classes:
  depth 4
```

This is a probe cap, not a final HZ9 cache policy. A byte cap or class-specific
depth table can replace it after the direction passes no-regression gates.

## Slot State

Use a FREE-tag sentinel, not TLS-only authority.

```text
ALLOCATED:
  user-visible live slot

LOCAL_CACHE:
  owner-local cached slot
  only reusable through the owning TLS cache
  not acceptable for another free or remote publish

FREE:
  normal HZ8 allocator-visible free slot
```

`LOCAL_CACHE` keeps the slot outside normal free/residency machinery.

```text
LOCAL_CACHE:
  slot_state = LOCAL_CACHE
  allocated_mask bit remains set
  free_mask bit remains clear
  TLS cache owns (run, slot, ptr)
```

Normal free fallback must require `slot_state == ALLOCATED`, because
`allocated_mask` remains set while the slot is cached.

## Flush

Required flush points:

```text
thread shutdown:
  flush all TLS cache entries before owner_exit

owner exit:
  still the lifecycle hard-drain authority

future hardening:
  owner-run scan can verify no LOCAL_CACHE residue
  detach / destroy checks can become hard zero gates
```

Flush:

```text
LOCAL_CACHE -> FREE
allocated_mask bit clear
free_mask bit set
normal empty/residency path may run
```

## Current Counters

Implemented L0 counters are intentionally minimal to keep the source split
small:

```text
h9_medium_cache_pop_hit
h9_medium_cache_push_hit
h9_medium_cache_push_full
h9_medium_cache_state_mismatch
h9_medium_cache_flush_slots
```

Debug fixed64 smoke showed:

```text
push_hit=100000
pop_hit=99999
state_mismatch=0
flush_slots=1
reuse_ratio=1.000
```

Missing hardening counters for the next box:

```text
h9_local_cache_double_free_accepted = 0
h9_local_cache_remote_publish_accepted = 0
h9_owner_exit_local_cache_remaining = 0
h9_detach_local_cache_remaining = 0
h9_destroy_local_cache_remaining = 0
h9_invalid_owned_forwarded_to_platform = 0
```

## Gates

L0 performance:

```text
fixed64_local0:
  >= HZ8 * 1.00

medium_local0:
  >= HZ8 * 1.05

main_local0:
  no regression

medium_r50 / main_r90:
  regression <= 5% for L0
```

## Current Observation

Short R3 probes are not promotion evidence, but they identify the shape.

```text
pure TLS cache:
  local fixed64 can hit near-perfect reuse
  medium r50 regresses materially

pending-gated TLS cache:
  bypasses cache-pop when owner pending exists
  keeps push path close to pure TLS cache
```

Current disposition:

```text
HZ9MediumTLSObjectCache-L0:
  implementation exists
  mechanism works
  not promotion-grade

pure cache:
  near-perfect fixed64 reuse in debug
  medium r50 regresses because cache hits bypass HZ8 collect cadence

pending-gated cache:
  flushes cache on owner-pending malloc
  fixes lifecycle residue risk better than pure cache
  introduces churn and remains weak in short local/r50 probes

remote-class admission:
  implemented as HZ9MediumTLSCacheRemoteClassAdmission-L1 evidence
  after a remote free is successfully published for a medium class, pop/push
  for that class are blocked and the current thread's cached entries for that
  class are flushed
  this tests whether TLS cache can keep local-row reuse without remote-row
  low-hit churn

remote-class result:
  mechanism worked but behavior failed
  medium_local0 kept near-perfect reuse in debug, but release R5 regressed
  medium/main rows:
    medium_local0 0.949
    main_local0 0.966
    medium_r50 0.898
    main_r90 0.924
  decision:
    keep target as evidence
    do not continue TLS admission tuning as the next HZ9 path

next design question:
  if HZ9 wants HZ3-class local speed, it likely needs a local page/slab
  substrate rather than caching objects on top of HZ8 medium runs
```

L0 RSS:

```text
per-thread medium cache cap:
  explicit

post RSS:
  may exceed HZ8 during live threads
  must drain after thread exit / owner exit
```

Correctness:

```text
state_mismatch = 0
double_free_accepted = 0
remote_publish_accepted_on_local_cache = 0
owner_exit_residue = 0
detach_residue = 0
destroy_residue = 0
invalid_owned_to_platform = 0
pending_duplicate_claim_accepted = 0
```

## No-Go

Stop this box if:

```text
cache hit rate is high but medium_local0 still loses materially
fixed64_local0 remains below HZ8
owner-exit residue is nonzero
the design requires weakening slot_state or pending bitmap authority
```

If this fails, HZ9 should move to a new local slab/page design rather than
continuing to use HZ8 medium runs as the local substrate.
