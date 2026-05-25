# HZ5 MidPageFront C7 Results

MidPageFront-C7 is the current Linux ordinary-malloc family for `2049..32768`.
The core result is stable:

```text
same-class MidPage direct API is already tcmalloc-class
mixed-class streams lose throughput under strict classing
coarse classing recovers speed, but RSS determines promotion
```

Raw outputs:

```text
private/raw-results/linux/midpage_c7_direct_rss_sweep_20260525_172138
private/raw-results/linux/midpage_c7_preload_rss_sweep_20260525_172216
```

## Direct API

`ws=4000`, `RUNS=3`.

```text
profile        mix_3_8k ops/RSS      mix_5_32k ops/RSS
strict         53.11M / 76160 KB     30.99M / 86912 KB
band4/8/16/32  52.40M / 74368 KB     34.29M / 84096 KB
band4/8/32     59.27M / 74496 KB     33.67M / 78080 KB
band8/16/32    72.12M / 65408 KB     33.54M / 80384 KB
band8/32       71.95M / 65536 KB     33.20M / 74496 KB
wide32k        40.98M / 67328 KB     39.75M / 67328 KB
```

## Preload

`ws=4000`, `RUNS=3`.

```text
profile        row        r0 ops/RSS        r90 ops/RSS
strict         mix_3_8k   46.89M / 84864    16.29M / 422784
band8/16/32    mix_3_8k   56.42M / 81408    11.11M / 496384
band8/32       mix_3_8k   57.36M / 81280    13.13M / 447616
wide32k        mix_3_8k   36.38M / 77184    21.32M / 196096
tcmalloc       mix_3_8k   66.64M / 80128    43.32M / 140672

strict         mix_5_32k  32.03M / 91392    22.05M / 174208
band8/16/32    mix_5_32k  36.71M / 84608    19.56M / 204032
band8/32       mix_5_32k  37.18M / 78976    22.55M / 208384
wide32k        mix_5_32k  35.92M / 77312    20.49M / 208512
tcmalloc       mix_5_32k  50.96M / 84992    34.94M / 137728
```

## Interpretation

```text
Strict is the low-waste default candidate.
band8/32 and band8/16/32 are real speed/RSS candidates.
wide32k is only a speed upper-bound diagnostic.

The next implementation target is not another mapping table.
It is RSS control for coarse profiles:
  empty/mostly-empty slab release
  bytes-based cache budgets
  retention control on remote-heavy rows
```

## RSS Governor R1 Smoke

Implementation:

```text
flag:
  --linux-midpagefront-empty-slab-release

coarse presets:
  band8/16/32-rssgov
  band8/32-rssgov
```

Direct smoke, `threads=8 iters=1000000 ws=4000 2049..32768 random`:

```text
band8/32 baseline:
  105.05M ops/s, maxrss 75136 KB

band8/32-rssgov cap=64:
   47.10M ops/s, maxrss 54364 KB

band8/32-rssgov cap=512:
   67.20M ops/s, maxrss 62960 KB
```

R2 changed release timing:

```text
free path:
  mark slab retired only

refill/miss path:
  release one retired slab with madvise(DONTNEED)
```

Direct smoke after R2:

```text
band8/32-rssgov cap=512:
   66.14M ops/s, maxrss 70124 KB
```
```

Read:

```text
R1 proves empty-slab madvise can reduce RSS, but runtime madvise cost is too
high for a speed profile. R2 moves release to refill/miss, but the cost profile
is still not a speed-profile candidate. Keep the lane as an RSS diagnostic.
The next design should expose an explicit checkpoint / phase-boundary release
API instead of releasing during allocator hot paths.
```

## RSS Governor R3 Checkpoint Lane

Implementation:

```text
API:
  hz5_midpagefront_release_retired()

build:
  --linux-midpagefront-empty-release-checkpoint

coarse presets:
  band8/16/32-rsscheckpoint
  band8/32-rsscheckpoint

benchmark:
  bench_hz5_midpage_direct ... [release_retired_at_end]
```

Read:

```text
R3 tests phase-boundary release. The direct benchmark calls the release API
inside each worker after final frees, because the retired empty slabs are
thread-local. Report released_retired and current_rss_kb; maxrss alone does not
show post-checkpoint reclamation.
```

Initial direct smoke, `threads=8 iters=1000000 ws=4000 2049..32768 random`:

```text
band8/32-rsscheckpoint cap=512, checkpoint=0:
   94.07M ops/s, current_rss 182784 KB, maxrss 183040 KB,
   released_retired 0

band8/32-rsscheckpoint cap=512, checkpoint=1:
   78.71M ops/s, current_rss 53624 KB, maxrss 181120 KB,
   released_retired 20293

band8/32-rsscheckpoint cap=4096, checkpoint=0:
   98.16M ops/s, current_rss 71168 KB, maxrss 71296 KB,
   released_retired 0

band8/32-rsscheckpoint cap=4096, checkpoint=1:
  105.44M ops/s, current_rss 71296 KB, maxrss 71552 KB,
   released_retired 0

band8/16/32-rsscheckpoint cap=4096, checkpoint=0:
  102.46M ops/s, current_rss 74240 KB, maxrss 74368 KB,
   released_retired 0

band8/16/32-rsscheckpoint cap=4096, checkpoint=1:
   98.23M ops/s, current_rss 74368 KB, maxrss 74496 KB,
   released_retired 0
```

Preload mixed smoke, `bench_mixed_ws_crt 8 500000 4000 2049 32768`:

```text
band8/32-rsscheckpoint cap=4096:
  105.19M ops/s, maxrss 72704 KB

band8/16/32-rsscheckpoint cap=4096:
  104.03M ops/s, maxrss 74624 KB
```

Read:

```text
checkpoint release works when slabs are retired, but cap=512 retires too many
slabs during the run and hurts throughput. cap=4096 avoids retirement in this
direct row, restores band8/32-class speed, and keeps current RSS near the
original direct baseline. Short preload smoke also stays around 104-105M ops/s
with 72-75MB maxrss. The next useful check is r50/r90, where remote-heavy
retention was the real problem.
```

## R3 Teardown And Remote-Heavy Follow-Up

Implementation follow-up:

```text
TLS destructor:
  calls the MidPage release checkpoint for owner-local empty slabs at thread
  teardown.

checkpoint API:
  now releases both retired empty slabs and still-owned fully empty slabs.
```

Direct smoke after full empty-owned release,
`threads=8 iters=1000000 ws=4000 2049..32768 random checkpoint=1`:

```text
band8/32-rsscheckpoint cap=4096:
   58.07M ops/s, current_rss 3716 KB, maxrss 70912 KB,
   released_retired 7138

band8/16/32-rsscheckpoint cap=4096:
   60.56M ops/s, current_rss 3732 KB, maxrss 73984 KB,
   released_retired 6295
```

Read:

```text
checkpoint=1 is a phase-boundary/final-RSS measurement, not a steady-state
speed row. The release is intentionally timed inside the worker in this direct
benchmark, so ops/s drops while current RSS proves reclamation works.
Use checkpoint=0 for steady-state throughput comparisons.
```

Remote-heavy preload smoke,
`bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 90 65536`:

```text
before full empty-owned release:
  band8/32-rsscheckpoint cap=4096:
      561847 ops/s, maxrss 832768 KB, overflow_sent 942872

  band8/16/32-rsscheckpoint cap=4096:
     1050729 ops/s, maxrss 1284096 KB, overflow_sent 744806

after full empty-owned release:
  band8/32-rsscheckpoint cap=4096:
      651284 ops/s, maxrss 599256 KB, overflow_sent 740931

  band8/16/32-rsscheckpoint cap=4096:
      813170 ops/s, maxrss 928864 KB, overflow_sent 961385

tcmalloc:
    29276404 ops/s, maxrss 756352 KB, overflow_sent 5895
```

Interpretation:

```text
The teardown/checkpoint path improves RSS, especially for band8/32, but r90
throughput is still dominated by remote handoff cost. The huge overflow_sent
gap means HZ5 is not draining producer/consumer remote frees fast enough.

Next target is MidPage remote-handoff structure:
  remote packet drain scheduling
  owner inbox starvation under M5 hit-only
  batch/publish cost on remote free

Do not spend the next iteration on more RSS-governor tuning until r90 handoff
is understood.
```

## Remote Drain-On-Hit Diagnostic

Implementation:

```text
flag:
  BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT

build option:
  --linux-midpagefront-m4-remote-drain-hit-interval N

presets:
  band8/32-rsscheckpoint-drainhit
  band8/16/32-rsscheckpoint-drainhit
```

Purpose:

```text
M5 hit-only intentionally moves remote packet drain out of the alloc-hit path.
This is good for r0, but r90 suggested owner inbox starvation. The diagnostic
restores periodic remote packet drain on alloc hits to test whether drain
scheduling is a real lever.
```

Hakmem remote malloc smoke,
`bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 <remote> 65536`:

```text
band8/32-rsscheckpoint cap=4096 baseline:
  r0:  62.35M ops/s, maxrss  76032 KB
  r90:  1.25M ops/s, maxrss 501708 KB, overflow_sent 777625

band8/32-rsscheckpoint-drainhit interval=16:
  r0:  47.87M ops/s, maxrss  75776 KB
  r90:  1.86M ops/s, maxrss 387180 KB, overflow_sent 661633

band8/32-rsscheckpoint-drainhit interval=64:
  r0:  47.64M ops/s, maxrss  76032 KB
  r90:  1.53M ops/s, maxrss 579692 KB, overflow_sent 782977

band8/32-rsscheckpoint-drainhit interval=256:
  r0:  46.14M ops/s, maxrss  75904 KB
  r90:  1.97M ops/s, maxrss 547812 KB, overflow_sent 1066342

band8/32-rsscheckpoint-drainhit every hit, earlier prototype:
  r0:  40.09M ops/s, maxrss  75776 KB
  r90:  1.96M ops/s, maxrss 445332 KB, overflow_sent 824502

band8/16/32-rsscheckpoint-drainhit every hit, earlier prototype:
  r90: 923K ops/s, maxrss 957020 KB, overflow_sent 816952
```

Read:

```text
Remote packet drain scheduling is a real r90 lever: band8/32 improves from
1.25M to roughly 1.5-2.0M ops/s in these smoke rows. However, polling/draining
from the alloc-hit path destroys too much local throughput, even when periodic.

This is not a final lane. It is a diagnostic showing the next implementation
should make remote progress without taxing owner-local alloc hits:
  remote-free-side batching
  owner checkpoint drain outside hit path
  transfer-cache style handoff
```

## M6 Deferred-Free Coarse Smoke

Implementation:

```text
build option:
  --linux-midpagefront-m6-deferred-free

raw cap option:
  --linux-midpagefront-m6-raw-cap N
```

Purpose:

```text
M6 pushes free(ptr) into a classless raw-free quarantine and validates/promotes
in batches. It was originally a direct proof lane; this smoke enables it on the
band8/32 C7 coarse profile to test remote-heavy behavior.
```

Hakmem remote malloc smoke,
`bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 <remote> 65536`:

```text
band8/32-rsscheckpoint + M6 raw cap 64:
  r90: 17.25M ops/s, maxrss 112256 KB, overflow_sent 0

band8/32-rsscheckpoint + M6 raw cap 256:
  r0:  10.99M ops/s, maxrss 112384 KB
  r90: 16.57M ops/s, maxrss 112512 KB, overflow_sent 0

band8/32-rsscheckpoint + M6 raw cap 512:
  r90: 17.07M ops/s, maxrss 112512 KB, overflow_sent 0
```

Interpretation:

```text
M6 solves the producer/consumer overflow symptom in this smoke and is roughly
an order of magnitude faster than immediate remote packet handling on r90.
The cost is severe local-r0 regression, because classless deferred-free also
delays and batch-validates owner-local frees.

Keep M6 as a remote-heavy upper-bound/control. Do not promote it as the default
C7 lane. The next design should preserve owner-local immediate return while
using deferred/batched handoff only for remote frees.
```

## M6 Remote-Only Deferred-Free Smoke

Implementation:

```text
build option:
  --linux-midpagefront-m6-remote-deferred-free

convenience preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote

meaning:
  owner-local frees keep the normal immediate cache return path
  remote frees are deferred through the M6 raw quarantine and batch-promoted
```

Hakmem remote malloc smoke,
`bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 <remote> 65536`:

```text
band8/32-rsscheckpoint + M6 remote-only raw cap 64:
  r0:  48.46M ops/s, maxrss  75648 KB
  r90: 19.13M ops/s, maxrss 161120 KB, overflow_sent 0

after splitting the remote raw quarantine out of the hot MidPage TLS:
  r0:  50.40M ops/s, maxrss  75648 KB
  r90: 19.31M ops/s, maxrss 162392 KB, overflow_sent 0

same current-code r50 comparison:
  baseline band8/32:
      2.36M ops/s, maxrss 427392 KB, overflow_sent 257926

  band8/32 + M6 remote-only:
     22.33M ops/s, maxrss 128332 KB, overflow_sent 0

  tcmalloc:
     31.56M ops/s, maxrss 394240 KB, overflow_sent 0
```

Read:

```text
Remote-only deferred-free is the strongest C7 remote profile signal so far.
It keeps overflow at zero and approaches tcmalloc-class r90 much more closely
than immediate remote packet handling.

Compared with full M6, it avoids applying the classless quarantine to
owner-local frees and improves r90 from roughly 17M to 19M in this smoke.
After the raw quarantine was split out of `Hz5MidPageTls`, r0 is no longer
worse than the current rebuilt baseline in the same hakmem row.

Next useful work:
  verify r50/r90 medians
  compare against tcmalloc/HZ4 on the same hakmem rows
  decide whether remote-only M6 should become the C7 remote profile
```

## C7 Remote Strength Check

Raw output:

```text
private/raw-results/linux/hz5_c7_remote_strength_20260525_184341
```

Benchmark:

```text
bench_random_mixed_mt_remote_malloc
threads=8
iters=500000
ws=4000
size=2049..32768
ring_slots=65536
RUNS=5
```

Median ops/s:

```text
allocator       r0         r50        r90
tcmalloc        116.50M    34.84M     27.65M
hz4              59.51M    39.67M     33.81M
hz5-m6remote     62.15M    26.53M     19.51M
hz3              96.95M    21.71M     16.48M
mimalloc         31.95M     5.18M      3.52M
hz5-baseline     60.66M     3.24M      1.25M
```

Median maxrss:

```text
allocator       r0 KB      r50 KB     r90 KB
hz5-m6remote     75904     127424     160452
hz3              89472     200832     224512
hz4             127232     219520     353920
tcmalloc         87936     524416     725632
mimalloc         94044     601892     884324
hz5-baseline     75776     315312     598188
```

Median overflow_sent:

```text
allocator       r50        r90
hz5-m6remote    0          0
hz4             0          0
hz3             0          0
tcmalloc        0          998
mimalloc        0          90290
hz5-baseline    204753     999587
```

Read:

```text
HZ5 remote-only M6 is now a credible remote-heavy profile:
  r0 is roughly HZ4-class in this row
  r50/r90 are far above HZ5 baseline and mimalloc
  r50/r90 still trail HZ4 and tcmalloc on throughput
  RSS is much lower than HZ4/tcmalloc/mimalloc on remote-heavy rows
  overflow is fixed

This is not yet a clean HZ4/tcmalloc throughput win. The current strength is
RSS-efficient remote robustness, not absolute remote throughput leadership.
```

## M7 RemoteTicket Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m7ticket

design:
  remote free already has page+slot
  sender TLS aggregates page+slot into page+bit batches
  flush claims LIVE->REMOTE in batch and publishes existing remote packets
```

Smoke, same hakmem row:

```text
M6 remote split:
  r0:  50.40M
  r50: 22.33M
  r90: 19.31M

M7 initial:
  r0:  45.32M, maxrss  75904 KB
  r50: 21.25M, maxrss 129728 KB
  r90: 16.98M, maxrss 161496 KB

M7 with per-slot fallback after batch CAS failure:
  r0:  45.80M, maxrss  75904 KB
  r50: 20.22M, maxrss 126716 KB
  r90: 17.19M, maxrss 158284 KB
```

Read:

```text
M7 RemoteTicket does not beat M6 remote-only in this form. The removed
page_for_ptr/slot_index reclassification cost is smaller than the added
page-batch management and batch state-transition cost.

Keep M7 as a no-go diagnostic for now. The saved profile remains C7
band8/32 + M6 remote-only deferred free.
```

## M8 OwnerTransferCache Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m8xfer

design:
  keep M6 remote producer-side raw quarantine
  after remote packet drain, store CACHE slots as owner-local page+bitset
  transfer entries
  refill moves only enough transfer bits into the M4 magazine
  transfer cache is in separate TLS to avoid growing hot MidPage TLS
```

Smoke, same hakmem row and empty_retain_cap=4096:

```text
M6 remote split:
  r0:  47.08M, maxrss  75776 KB
  r50: 23.43M, maxrss 127784 KB
  r90: 18.80M, maxrss 163008 KB

M8 owner xfer:
  r0:  45.76M, maxrss  75904 KB
  r50: 21.09M, maxrss 129744 KB
  r90: 17.55M, maxrss 160724 KB
```

Read:

```text
M8 owner transfer cache is no-go in the current design. It preserves the RSS
band but loses throughput. For this workload, current M4 remote packet drain
to magazine/local cache is cheaper than holding drained slots as a compact
page+bitset transfer cache.

Keep M8 as diagnostic only. The saved remote profile remains C7 band8/32 +
M6 remote-only deferred free.
```

## M6 Cap / Flush Sweep

RUNS=5 candidates, same hakmem row:

```text
cap32_refill:
  r0 62.80M, r50 26.47M, r90 19.59M

cap64_refill:
  r0 63.25M, r50 26.57M, r90 19.68M

cap128_refill:
  r0 60.49M, r50 26.72M, r90 19.46M

cap64_norefill:
  r0 61.94M, r50 26.27M, r90 19.58M
```

Read:

```text
M6 raw cap / refill scheduling is not the remaining large lever. cap64/refill
remains the simplest saved default. All tested variants kept overflow at zero.
```

## M9 Budgeted Owner Drain Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m9budget

design:
  drain remote_packet_bits only until the current magazine can be filled
  restore leftover remote bits to the page and requeue it
```

Smoke:

```text
M9 budget drain:
  r0:  46.50M, maxrss  75904 KB
  r50: 20.86M, maxrss 128088 KB
  r90: 17.44M, maxrss 158660 KB
```

Read:

```text
M9 is no-go. Budgeting/requeueing the packet drain costs more than the current
drain-all path on this workload.
```

## M10 Flat Remote Slot Ring Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m10slot

design:
  remote free stores {page, slot} in a flat TLS ring
  no M7 page aggregation
  flush uses page+slot directly, removing M6 raw pointer reclassification
```

Smoke:

```text
M10 flat slot ring:
  r0:  45.62M, maxrss  75520 KB
  r50: 21.32M, maxrss 128872 KB
  r90: 17.49M, maxrss 161856 KB
```

Read:

```text
M10 is no-go. Removing the second page_for_ptr/slot_index is not enough to win;
producer-side queue representation is unlikely to be the next large lever.
```

## M11 Remote Direct-Cache Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-m11direct

design:
  M6 remote flush changes remote slots LIVE->CACHE directly
  remote packet only wakes the owner
  owner drain checks CACHE and skips REMOTE->CACHE transition
```

Smoke:

```text
M11 remote direct-cache:
  r0:  60.20M, maxrss  76032 KB
  r50: 23.13M, maxrss 128044 KB
  r90: 18.19M, maxrss 160684 KB
```

Read:

```text
M11 is no-go versus the saved M6 cap64/refill median
(r50 26.57M, r90 19.68M). Removing owner-side REMOTE->CACHE transition
alone does not close the gap. The remaining cost is broader path length and
free/refill structure, matching perf evidence.
```

## Perf Snapshot: HZ5 M6 vs HZ4/tcmalloc

Core counters, RUNS=3:

```text
allocator       r90 ops/s   cycles/op  instr/op  branches/op  br_miss%
hz5-m6remote    20.66M      271.85     219.46    44.53        2.41
hz4             30.98M      212.85     123.93    24.93        3.31
tcmalloc        25.61M      266.61     128.01    23.96        4.36
```

Deep r90 counters:

```text
allocator       lock/op  cache_miss/op  l2_wait/op  l2_x_req/op
hz5-m6remote    0.396    3.43           76.25       0.689
hz4             0.083    3.53           54.14       0.121
tcmalloc        0.154    4.28           67.47       0.328
```

Read:

```text
HZ5 is not losing because of cache-miss footprint. It has similar or lower
cache misses than HZ4/tcmalloc. It is paying more instructions, branches,
non-spec locks, and L2 exclusive traffic per op. The next real design target is
not producer queue representation; it is reducing free/refill path length.
```

## C8 PageRun-R1 Diagnostic

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun
  --linux-midpagefront-empty-retain-cap 4096

design:
  band8/32 only
  bypass M4 magazine/refill/cache_slot/slot_state2 for MidPage hot path
  alloc pops owner-local page owner_free_bits with ctz
  owner-local free clears active_bits and returns the bit to owner_free_bits
  M6 remote flush clears active_bits, sets remote_bits, and wakes owner inbox
  owner drain exchanges remote_bits into owner_free_bits
```

RUNS=5:

```text
workload:
  bench_random_mixed_mt_remote_malloc 8 500000 4000 2049 32768 r 65536

allocator       r0 ops/s    r50 ops/s   r90 ops/s   r90 maxrss
hz5-pagerun     92.55M      66.24M      66.01M      12.7MB
hz5-m6remote    61.23M      26.72M      19.74M      162.0MB
hz4             59.55M      41.30M      36.68M      304.6MB
tcmalloc        115.50M     32.84M      28.54M      737.7MB
```

Smoke:

```text
preload payload-touch smoke passed
owner-local double-free-before-reuse did not duplicate a pointer
remote free smoke passed
```

Read:

```text
PageRun-R1 is a strong keep. It removes the M4 magazine/cache_slot/slot_state2
path from owner-local reuse and turns the previous RSS-efficient remote profile
into a throughput winner on this band8/32 r50/r90 workload.

The very low maxrss should be interpreted carefully: this benchmark does not
necessarily fault every allocated byte. Keep touched-payload RSS rows separate
before making a final memory-efficiency claim.
```

Perf compare, RUNS=3 on the same r90 shape:

```text
allocator       ops/s      cycles/op  instr/op  branches/op  br_miss%
hz5-pagerun     66.54M     295.46     284.97    63.08        1.97
hz5-m6remote    20.69M     706.43     442.67    90.38        7.41
hz4             35.80M     371.84     232.09    47.71        7.19
tcmalloc        26.64M     608.29     373.35    74.76        9.01
```

Read:

```text
PageRun is not yet as instruction-thin as HZ4, but it dramatically cuts the M6
path cost and has much lower branch misses. The design win is real: removing M4
magazine/refill/cache_slot from owner reuse was the right lever.
```

## C8 PageRun64 Diagnostic

Reason:

```text
PageRun32 fixed MidPage band8/32, but broad cross rows exposed an unrecovered
32769..65536 ordinary malloc gap. In cross64/cross128 r90, that old path could
timeout or dominate the row.
```

Implementation:

```text
preset:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64
  --linux-midpagefront-empty-retain-cap 4096

design:
  keep PageRun32 intact
  add optional 65536 class with one slot per 64KiB slab
  route 32769..65536 ordinary malloc into MidPage only for this diagnostic
```

RUNS=5:

```text
workload:
  bench_random_mixed_mt_remote_malloc 8 500000 4000 min max r 65536

case       r0 ops/s   r50 ops/s  r90 ops/s  r90 maxrss
main       92.35M     60.68M     58.52M     44.7MB
mid_only   89.19M     66.60M     66.59M     12.0MB
midgap64   67.32M     17.93M     42.32M     24.1MB
cross64    80.17M     41.15M     56.94M     32.6MB
cross128   61.47M     29.08M     22.71M     377.9MB
```

Smoke:

```text
2049/8192/32768/32769/49152/65536 malloc/free/malloc_usable_size ok
64K double-free-before-reuse did not duplicate a pointer
64K remote free smoke passed
```

Read:

```text
PageRun64 is a strong diagnostic keep. It fixes the 32769..65536 timeout gap
and makes cross64 strong. cross128 r90 recovers versus PageRun32 and beats
tcmalloc in this run, but it still trails HZ4 and is roughly tied with old M6
remote while using slightly more RSS.

Remaining broad work is no longer the MidPage owner-local path. It is
LargeFront/cross-size behavior in 16..131072 remote-heavy rows.
```
