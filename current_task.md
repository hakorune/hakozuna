# Current Task

## Active Goal

HZ5 Linux general allocator work is in the MidPage-C7 profile-family phase.

Immediate focus:

```text
MidPage-C7 RSS-bounded coarse profiles
```

## Current Decision

```text
strict:
  low-waste default candidate

band8/32:
  coarse-speed candidate

band8/16/32:
  coarse-pareto candidate

wide32k:
  diagnostic only
```

Promotion rule:

```text
Do not promote a coarse profile until RSS / retention passes.
Next implementation target is RSS governor / empty-slab release.
```

## Active Implementation

```text
MidPage RSS Governor R3:
  empty owner-local 64KiB slabs are purged from local caches,
  returned to the region source list,
  and madvise(DONTNEED)'d outside the allocator hot path.

Candidate presets:
  band8/16/32-rsscheckpoint
  band8/32-rsscheckpoint

R1 smoke:
  RSS improves, but runtime madvise is too expensive for a speed profile.

R2 smoke:
  release on refill/miss avoids direct free-path madvise, but still does not
  make rssgov a speed-profile candidate.

R3:
  add hz5_midpagefront_release_retired() and benchmark-side
  release_retired_at_end so worker TLS retired lists can be released at a
  phase boundary.

R3 smoke:
  cap=512 proves checkpoint release can drop current RSS, but retirement during
  the run still costs throughput.

  cap=4096 avoids retirement on direct r0 and restores band8/32-class speed
  with current RSS around 71MB.

R3 teardown/checkpoint follow-up:
  TLS destructor + explicit checkpoint can release all empty owned slabs at a
  phase boundary. Direct checkpoint rows drop current RSS to roughly 3.7MB, but
  the benchmark times the release inside worker threads, so use checkpoint=0
  for steady-state speed and checkpoint=1 for final-RSS validation.

Remote-heavy r90 smoke:
  r90 is now a remote handoff bottleneck, not an RSS-governor-only problem.
  HZ5 coarse profiles still produce massive benchmark ring overflow while
  tcmalloc barely overflows. Empty-slab teardown reduces RSS but does not fix
  r90 throughput.
```

## Recent Results

Use these for the detailed measurements:

```text
docs/HZ5_MIDPAGEFRONT_C7_LANES.md
docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```

## Current Status

```text
Same-class MidPage direct API is already tcmalloc-class.
Mixed-class streams are where the gap lives.
Class dispersion is a real speed lever, but RSS decides promotion.
```

## Reading Order

```text
1. docs/HZ5_MIDPAGEFRONT_C7_LANES.md
2. docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
3. docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
4. docs/HZ5_LINUX_STATUS.md
```

## Next Step

```text
Switch from RSS-governor tuning to MidPage remote-handoff diagnostics.

First candidate:
  test a remote-drain-on-hit lane for M4 packet profiles. The M5 hit-only path
  intentionally skips remote drain on alloc hits, which is good for r0 but may
  starve owner inboxes under r90.

Remote-drain-on-hit smoke:
  restoring remote packet drain on alloc hits improves r90 throughput, but it
  damages r0 too much. Periodic intervals still pay a noticeable local cost.

  band8/32 baseline on hakmem remote malloc:
    r0 62.35M, r90 1.25M

  drainhit interval=16:
    r0 47.87M, r90 1.86M

  drainhit interval=64:
    r0 47.64M, r90 1.53M

  drainhit interval=256:
    r0 46.14M, r90 1.97M

  every-hit earlier:
    r0 40.09M, r90 1.96M

Read:
  remote packet drain scheduling is a real lever, but alloc-hit polling is not
  the final shape. The next design should move remote progress out of the local
  alloc hit path, likely via remote-free-side batching or owner-side drain
  checkpoints.

M6 deferred-free coarse smoke:
  M6 classless raw-free quarantine can be enabled on the C7 coarse profile.
  It is not a local-speed lane, but it is a strong remote-heavy upper bound.

  band8/32 + M6 raw cap 64:
    r90 17.25M, overflow 0, maxrss 112MB

  band8/32 + M6 raw cap 256:
    r0 11.0M, r90 16.57M, overflow 0, maxrss 112MB

  band8/32 + M6 raw cap 512:
    r90 17.07M, overflow 0, maxrss 112MB

Read:
  deferred free fixes the producer/consumer overflow problem but destroys r0.
  This supports a split design: local-speed profile keeps immediate local cache
  return, while remote-heavy profile needs deferred/batched handoff. The next
  real design should avoid applying classless deferred-free to owner-local r0.

Remote-only M6 smoke:
  keeping owner-local immediate free and deferring only remote frees is the
  first strong C7 remote profile candidate.

  band8/32 + M6 remote-only raw cap 64:
    first smoke: r0 48.46M, r90 19.13M, overflow 0, maxrss 161MB

  after splitting remote raw quarantine out of Hz5MidPageTls:
    r0 50.40M, r90 19.31M, overflow 0, maxrss 162MB

  same current-code comparison on hakmem remote malloc:
    baseline band8/32: r0 46.87M, r50 2.36M, r90 3.92M
  remote-only M6:    r0 50.40M, r50 22.33M, r90 19.31M
  tcmalloc:          r50 31.56M

RUNS=5 strength check:
  hakmem remote malloc, threads=8, iters=500000, ws=4000,
  size 2049..32768, ring=65536.

  r0 median ops/s:
    tcmalloc 116.50M
    hz3 96.95M
    hz5-m6remote 62.15M
    hz5-baseline 60.66M
    hz4 59.51M
    mimalloc 31.95M

  r50 median ops/s:
    hz4 39.67M
    tcmalloc 34.84M
    hz5-m6remote 26.53M
    hz3 21.71M
    mimalloc 5.18M
    hz5-baseline 3.24M

  r90 median ops/s:
    hz4 33.81M
    tcmalloc 27.65M
    hz5-m6remote 19.51M
    hz3 16.48M
    mimalloc 3.52M
    hz5-baseline 1.25M

  r90 median maxrss:
    hz5-m6remote 160MB
    hz3 225MB
    hz4 354MB
    tcmalloc 726MB
    mimalloc 884MB
    hz5-baseline 598MB

Read:
  remote-only deferred-free keeps the key M6 r90 benefit without applying the
  classless quarantine to owner-local frees. With the remote raw buffer split
  out of the hot MidPage TLS, r0 is no longer worse than the current baseline.
  This is now the main remote-heavy candidate. It is not yet HZ4/tcmalloc-class
  on r50/r90 throughput, but it is already a strong RSS-efficient remote profile
  and a large win over HZ5 baseline/mimalloc.

Next design decision:
  Move from M6 raw pointer remote quarantine to M7 RemoteTicket.

  Reason:
    free_page already has page and slot. M6 remote throws that away, queues a
    raw pointer, then page-lookups and slot-indexes again during flush.

  Target:
    remote free queues page+slot as a page+bitmask ticket batch.
    flush performs batch LIVE->REMOTE claim and publishes existing remote
    packet pages to the owner inbox.

  Keep:
    owner-local immediate free path
    low RSS checkpoint/release behavior
    fail-closed ownership

  First acceptance:
    r0 no worse than m6remote by >5%
    r50/r90 +10% over m6remote or clear instruction/overflow improvement
    r90 RSS stays below 225MB

M7 smoke:
  M7 lazy RemoteTicket was implemented as a diagnostic:
    remote free stores page+slot into sender TLS page+bit batches
    flush performs batch LIVE->REMOTE and existing remote-packet publish

  Smoke against M6 remote:
    M6 remote split: r0 50.40M, r50 22.33M, r90 19.31M
    M7 initial:      r0 45.32M, r50 21.25M, r90 16.98M
    M7 fallback:     r0 45.80M, r50 20.22M, r90 17.19M

Read:
  M7 did not beat M6 remote. Avoid replacing M6 remote with M7 ticket in the
  current design. The likely issue is that batch state transition / sender
  page-batch management costs more than the raw pointer reclassification it
  removes. Keep M7 as no-go diagnostic unless a later owner transfer cache
  changes the drain side.

Next diagnostic:
  M8 OwnerTransferCache.

  Reason:
    M4 remote packet drain currently expands each remote bit immediately into
    the owner magazine/local list. When the magazine is full, that spills into
    object-payload local nodes and loses the page+bitset compactness that made
    the remote packet path attractive.

  Target:
    After REMOTE->CACHE, keep drained slots as owner-local page+bitset transfer
    entries. On alloc refill, move only enough transfer bits into the magazine.
    This tests whether owner-side drain/local-list churn, not producer-side
    ticketing, is the remaining remote-heavy cost.

  Keep:
    M6 remote raw pointer split as producer-side profile
    existing M4 remote packet publish path
    exact slot state transitions
    RSS checkpoint/release controls

  First acceptance:
    r0 no worse than m6remote by >5%
    r50/r90 improves over m6remote smoke, or at least retains r90 RSS with
    lower drain/list churn
    no stale xfer entries when empty slabs are released

M8 smoke:
  M8 owner transfer cache was implemented as a diagnostic:
    M6 remote producer path stays unchanged
    drained REMOTE->CACHE slots can be kept as owner-local page+bitset xfer
    refill moves xfer bits into the M4 magazine on demand
    xfer cache lives in separate TLS so the hot MidPage TLS does not grow

  Fair smoke against M6 remote split, with empty_retain_cap=4096:
    M6 remote split: r0 47.08M, r50 23.43M, r90 18.80M
    M8 xfer:         r0 45.76M, r50 21.09M, r90 17.55M

  RSS:
    M8 xfer stayed in the same RSS band:
      r0 ~75.9MB
      r50 ~129.7MB
      r90 ~160.7MB

Read:
  M8 owner transfer cache does not beat current M6 remote. Keeping drained
  slots as page+bitset xfer entries adds enough refill/cache-management cost
  that it loses throughput while preserving RSS. The current M4 remote drain
  behavior remains the better default for this row.

Worker audit after M7/M8:
  HZ5 C7/M6 saved profile path:
    malloc hit:
      preload -> MidPage try_alloc -> M4 magazine pop -> pointer-mag return
    owner-local free:
      preload free -> MidPage first -> page_for_ptr + slot_index
      freeelide skips slot-state transition, then caches slot
    remote free:
      free_page already knows page/slot, but M6 queues raw ptr in separate TLS
      flush redoes page_for_ptr + slot_index, then LIVE->REMOTE and packet push
      owner drain does REMOTE->CACHE per bit, then cache_slot per bit

  Why M7/M8 lost:
    M7 removed reclassification but added linear page-batch search and batch
    state transition cost.
    M8 avoided local-list expansion but added xfer indirection/refill management;
    direct M4 drain is cheaper on this workload.

  Next candidate ranking:
    1. Budgeted owner drain using existing remote_packet_bits:
       drain only enough slots to fill the magazine, leave remaining bits on
       page and requeue. Acceptance: r50/r90 +10%, r0 regression <=3%,
       overflow zero, r90 RSS <=225MB.
    2. Flat M6 remote slot ring:
       store {page,slot} from free_page without page aggregation. Acceptance:
       r50/r90 +5%, r0 regression <=3%, RSS unchanged.
    3. M6 remote cap/flush scheduling sweep:
       no-code/low-code signal using cap 32/64/128/256 and no-refill-flush.

M6 cap/flush sweep:
  Built cap 32/64/128/256 with refill and no-refill-flush variants.
  Single-run scan:
    all variants kept overflow_sent=0 and overflow_pending=0
    r50/r90 differences were small
  RUNS=5 candidates:
    cap32_refill:
      r0 62.80M, r50 26.47M, r90 19.59M
    cap64_refill:
      r0 63.25M, r50 26.57M, r90 19.68M
    cap128_refill:
      r0 60.49M, r50 26.72M, r90 19.46M
    cap64_norefill:
      r0 61.94M, r50 26.27M, r90 19.58M

Read:
  M6 cap/flush scheduling is not the main remaining lever. cap64/refill remains
  the simplest saved default. Proceed to budgeted owner drain.

M9 budgeted owner drain:
  Implemented diagnostic:
    remote_packet_bits are drained only up to current magazine capacity
    remaining bits are restored to the page and requeued

  Single smoke:
    r0 46.50M, r50 20.86M, r90 17.44M
    RSS stayed in band, overflow zero

  Read:
    no-go. Budget/requeue management loses to existing drain-all behavior.

M10 flat remote slot ring:
  Implemented diagnostic:
    remote free stores {page, slot} in a flat TLS ring
    no M7 page aggregation / linear page-batch search
    flush uses page+slot directly, avoiding M6 raw pointer reclassification

  Single smoke:
    r0 45.62M, r50 21.32M, r90 17.49M
    RSS stayed in band, overflow zero

  Read:
    no-go. Removing second page_for_ptr/slot_index does not recover throughput.
    Producer-side queue representation is unlikely to be the next big lever.

M11 remote direct-cache:
  Implemented diagnostic:
    M6 remote flush transitions remote slots LIVE->CACHE directly
    remote packet only notifies owner
    owner drain checks CACHE and skips REMOTE->CACHE transition

  Single smoke:
    r0 60.20M, r50 23.13M, r90 18.19M
    RSS stayed in band, overflow zero

  Read:
    no-go versus M6 cap64/refill medians:
      cap64/refill r50 26.57M, r90 19.68M
    Removing the owner-side REMOTE->CACHE transition alone does not close the
    gap. The remaining loss is broader path length / refill/free structure,
    consistent with perf showing high instructions/op and branches/op.

Perf plan:
  Run speed-lane perf separately for HZ5 M6 remote, HZ4, and tcmalloc on
  r0/r50/r90. Do not enable HZ5 stats/counters in these medians.
  Useful counters:
    cycles, instructions, branches, branch-misses
    cache-references/cache-misses
    dTLB-loads/dTLB-load-misses
    ls_locks.*, l2_request_g2.*, l2_cache_req_stat.*, l2_latency.*
  Read:
    high instructions/branches with similar misses => dispatch/state path
    high ls_locks or L2 exclusive requests => atomics/remote handoff
    high branch miss ratio => route/class dispatch
    high cache/TLB misses => footprint/locality issue

Perf result:
  Core counters, RUNS=3, r0/r50/r90:
    HZ5 M6 remote r90:
      20.66M ops/s
      271.85 cycles/op
      219.46 instructions/op
      44.53 branches/op
      2.41% branch miss
    HZ4 r90:
      30.98M ops/s
      212.85 cycles/op
      123.93 instructions/op
      24.93 branches/op
      3.31% branch miss
    tcmalloc r90:
      25.61M ops/s
      266.61 cycles/op
      128.01 instructions/op
      23.96 branches/op
      4.36% branch miss

  Deep r90 counters:
    HZ5 M6 remote:
      non_spec_lock/op 0.396
      cache_misses/op 3.43
      L2 wait cycles/op 76.25
      L2 exclusive req/op 0.689
    HZ4:
      non_spec_lock/op 0.083
      cache_misses/op 3.53
      L2 wait cycles/op 54.14
      L2 exclusive req/op 0.121
    tcmalloc:
      non_spec_lock/op 0.154
      cache_misses/op 4.28
      L2 wait cycles/op 67.47
      L2 exclusive req/op 0.328

Read:
  HZ5's RSS win is not a cache-miss win/loss story here. Cache misses are not
  worse than HZ4/tcmalloc. The gap is path length plus atomic/exclusive state
  traffic. Next target should reduce per-slot state transitions on remote drain,
  not producer queue representation or RSS release policy.

Keep RSS checkpoint as a phase-boundary/control lane, not the next speed lever.
```
