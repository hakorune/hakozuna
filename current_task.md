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

Pro design decision:
  Implement MidPage-C8 / PageRun-R1 as the last major throughput-chase
  experiment before freezing C7.

Goal:
  Keep C7/M6 remote as saved RSS-efficient profile, but test whether HZ4-style
  page-local run allocation can remove M4 magazine/refill/slot_state2 path
  length.

Scope:
  band8/32 only:
    class 8192
    class 32768
  Linux MidPage only.
  Do not touch Windows P43i/P45, Local2P, SmallFront, LargeFront.

Design:
  Add PageRun state under a compile flag:
    live_bits:
      canonical user-visible live slots
    remote_bits:
      remote-freed slots claimed by non-owner threads
    owner_free_bits:
      owner-local reusable slots
    current_page[class]:
      TLS current page for ctz alloc
    partial_pages[class]:
      owner-local pages with available bits

  Alloc:
    current page owner_free_bits pop
    atomic live_bits set
    return slab_base + slot * class_size
    no M4 magazine pop
    no M4 slot_state2 transition

  Owner-local free:
    page_for_ptr + slot_index
    atomic live_bits clear test
    owner_free_bits |= bit
    partial push if not current
    empty release check

  Remote:
    keep M6 remote deferred boundary
    flush raw ptr -> page/slot
    live_bits clear test
    remote_bits |= bit
    owner inbox gets page pointer
    owner drain exchange(remote_bits) -> owner_free_bits
    no M4 cache_slot
    no M4 REMOTE->CACHE transition

Acceptance:
  weak keep:
    r90 +10% over C7/M6 remote
    r50 +8%
    r0 regression <=5%
    r90 RSS <=200MB
  strong keep:
    r90 near tcmalloc class
    instr/op <180
    branches/op <36
    non_spec_lock/op <0.25
  hard no-go:
    r90 RSS >225MB
    overflow returns
    invalid/double-free can be promoted to owner_free_bits
    empty release can free a page referenced by current/partial/remote

Keep RSS checkpoint as a phase-boundary/control lane, not the next speed lever.
```

Implementation result:
  Lane:
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun
    --linux-midpagefront-empty-retain-cap 4096

  Build:
    hakozuna-hz5/out/linux/x86_64-hz5-c8-band8-32-pagerun-r1-retain4096/libhakozuna_hz5_preload_full.so

  Implementation:
    PageRun reuses MidPage `active_bits` as live bits.
    PageRun adds `pagerun_owner_free_bits` and owner-local current/partial page
    lists.
    PageRun bypasses M4 magazine/refill/cache_slot/slot_state2 on alloc,
    owner-local free, M6 remote flush, and owner remote drain.
    M4 remote inbox storage is reused only as the owner wakeup/list mechanism.

  RUNS=5, same workload:
    bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 <remote_pct> 65536

    allocator       r0 ops/s    r50 ops/s   r90 ops/s   r90 maxrss
    hz5-pagerun     92.55M      66.24M      66.01M      12.7MB
    hz5-m6remote    61.23M      26.72M      19.74M      162.0MB
    hz4             59.55M      41.30M      36.68M      304.6MB
    tcmalloc        115.50M     32.84M      28.54M      737.7MB

  Smoke:
    preload payload-touch smoke passed
    owner-local double-free-before-reuse did not duplicate a pointer
    remote free smoke passed

Read:
  PageRun-R1 is a strong keep. It beats HZ4 and tcmalloc on r50/r90 throughput
  in this MidPage band8/32 workload while preserving the RSS advantage. The
  likely reason is exactly the intended one: removing M4 magazine/cache_slot and
  packed slot_state2 from the hot owner-reuse path.

Next:
  1. Run perf counters for PageRun vs HZ4/tcmalloc.
  2. Run broader main/mid_only/cross128 and ws sweep.
  3. Check touched-payload RSS rows separately, because this benchmark does not
     necessarily fault every allocated byte.
  4. If broad rows hold, promote as C8 PageRun throughput/RSS profile and keep
     C7 M6 remote as the conservative saved profile.

Follow-up perf:
  RUNS=3, same mid_only r90 shape:

    allocator       ops/s      cycles/op  instr/op  branches/op  br_miss%
    hz5-pagerun     66.54M     295.46     284.97    63.08        1.97
    hz5-m6remote    20.69M     706.43     442.67    90.38        7.41
    hz4             35.80M     371.84     232.09    47.71        7.19
    tcmalloc        26.64M     608.29     373.35    74.76        9.01

Read:
  PageRun did not become as instruction-thin as HZ4, but it cuts M6/tcmalloc
  path cost enough to win throughput. Branch misses are especially low. The
  remaining HZ4 advantage is still branch/path length, not correctness.

Broad result:
  PageRun32 RUNS=3:
    main and mid_only are strong.
    cross128 r90 regresses because the 32769..65536 gap remains on the old
    path and can timeout.

  Triage:
    cross64 r90 has one PageRun32 timeout.
    midgap64 32769..65536 r90 times out in all PageRun32 runs.
    large128 is not the PageRun problem.

Next implementation:
  Add PageRun64 diagnostic:
    keep PageRun32 intact
    add optional 65536 class under `BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K`
    route ordinary malloc 32769..65536 into MidPage only for this lane

PageRun64 result:
  Lane:
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64
    --linux-midpagefront-empty-retain-cap 4096

  RUNS=5:

    case       r0 ops/s   r50 ops/s  r90 ops/s  r90 maxrss
    main       92.35M     60.68M     58.52M     44.7MB
    mid_only   89.19M     66.60M     66.59M     12.0MB
    midgap64   67.32M     17.93M     42.32M     24.1MB
    cross64    80.17M     41.15M     56.94M     32.6MB
    cross128   61.47M     29.08M     22.71M     377.9MB

  Smoke:
    2049/8192/32768/32769/49152/65536 malloc/free/malloc_usable_size ok
    64K double-free-before-reuse did not duplicate a pointer
    64K remote free smoke passed

Read:
  PageRun64 fixes the 32769..65536 timeout gap and makes cross64 strong.
  cross128 r90 recovers versus PageRun32 and beats tcmalloc, but still trails
  HZ4 and is roughly tied with old M6 remote while using slightly more RSS.
  The remaining broad gap is LargeFront/cross-size, not MidPage PageRun.

LargeFront cross-size follow-up:
  Existing PageRun64 config:
    largefront owner inbox: on
    largefront region map: on
    largefront remote batch: off
    largefront drain-take-first: off

  No-code sweep, RUNS=3, cross128 r90, iters=500000:
    base      19.56M, maxrss 466MB
    rb8       17.98M, maxrss 506MB
    rb16      24.29M, maxrss 338MB
    rb32      22.76M, maxrss 394MB
    rb16take 32.70M, maxrss 222MB

  Broad smoke, RUNS=3, PageRun64 vs PageRun64+takefirst:
    cross128 r50: base 31.47M / 181MB, takefirst 36.56M / 129MB
    cross128 r90: base 19.34M / 418MB, takefirst 23.41M / 360MB
    large128 r50: base 22.54M / 309MB, takefirst 21.67M / 340MB
    large128 r90: base 11.95M / 877MB, takefirst 16.56M / 604MB

  Broader allocator comparison, RUNS=3, iters=500000:
    main r90:
      pagerun64 60.65M / 37MB
      pagerun64-takefirst 59.59M / 41MB
      hz4 42.01M / 319MB
      tcmalloc 27.46M / 754MB

    mid_only r90:
      pagerun64 66.48M / 13MB
      pagerun64-takefirst 65.15M / 12MB
      hz4 38.03M / 287MB
      tcmalloc 26.71M / 751MB

    cross64 r90:
      pagerun64 56.64M / 31MB
      pagerun64-takefirst 57.23M / 32MB
      hz4 37.71M / 263MB
      tcmalloc 36.54M / 324MB

    cross128 r90:
      pagerun64 23.68M / 364MB
      pagerun64-takefirst 25.34M / 315MB
      hz4 27.69M / 330MB
      tcmalloc 15.95M / 417MB

    large128 r90:
      pagerun64 18.53M / 514MB
      pagerun64-takefirst 6.33M / 1777MB
      hz4 3.84M / 1805MB
      tcmalloc 16.29M / 525MB

Read:
  LargeFront drain-take-first is a real cross-size lever and now has a named
  preset, but it is not a clean broad default yet. The focused smoke shows it
  can improve cross128 and large128, while the broad matrix still has a bad
  large128 r90 outlier when combined with the full mixed run. Keep it as a
  diagnostic candidate. Do not enable LargeFront remote-batch broadly; rb16 can
  help cross128 but hurts large-only remote rows.

  Verification, RUNS=5, iters=500000, r90:
    cross128:
      pagerun64 21.53M / 421MB
      pagerun64-takefirst 25.00M / 319MB
      hz4 28.01M / 333MB
      tcmalloc 16.16M / 401MB

    large128:
      pagerun64 11.48M / 929MB
      pagerun64-takefirst 13.25M / 800MB
      hz4 3.93M / 1703MB
      tcmalloc 17.35M / 500MB

Read update:
  takefirst survives the focused RUNS=5 check. It improves cross128 and
  large128 versus PageRun64 base and reduces RSS. It still trails HZ4 on
  cross128 and tcmalloc on large128, so the remaining gap is LargeFront's
  128K remote/free path, not MidPage.

LargeFront source batch sweep:
  Added build option:
    --linux-largefront-source-batch-count N

  Reason:
    LargeFront source refill was fixed at 16 spans. For the 128K class this is
    roughly 2MB per refill, which can amplify RSS when remote reuse lags.

  PageRun64+takefirst, RUNS=3, r90, iters=500000:
    cross128:
      batch4  13.44M / 571MB
      batch8  22.12M / 345MB
      batch16 28.70M / 265MB

    large128:
      batch4  18.35M / 420MB
      batch8  11.31M / 864MB
      batch16  9.65M / 1153MB

Read:
  Source batch count is a real lever but splits by workload. batch4 is better
  for pure large128 remote, while batch16 is better for cross128 mixed rows.
  Do not promote a single source-batch default from this alone. The next clean
  design would need phase-aware or pressure-aware LargeFront sourcing, or a
  separate large-only RSS/throughput profile.

LargeFront-L3 adaptive128 first implementation:
  Added:
    --linux-largefront-adaptive128
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-adaptive128

  Implementation:
    only the 128K LargeFront class is adaptive
    source refill count uses mapped 128K bytes on source-refill slow path
      <320MB: batch16
      >=320MB: batch8
      >=512MB: batch4
    no malloc/free hot-path counters
    region map now stores actual refill span_count instead of the fixed macro

  RUNS=5, r90, iters=500000:
    cross128:
      adaptive 13.50M / 567MB
      batch16  27.48M / 288MB
      batch4   17.02M / 455MB

    large128:
      adaptive  8.90M / 1070MB
      batch16  13.04M / 779MB
      batch4    8.20M / 1060MB

Read:
  adaptive128 is no-go in this first form. Mapped-bytes-only pressure is too
  blunt and appears to lower the source batch during phases where cross128
  still wants batch16. Fixed split remains cleaner:
    cross-size diagnostic: PageRun64 + takefirst + batch16
    large-only diagnostic: PageRun64 + takefirst + batch4

  If continuing LargeFront-L3, do not keep chasing source-batch policy alone.
  The next adaptive attempt should add pressure-only retained-payload scavenging
  or a better phase signal; otherwise preserve fixed profiles and move on.
