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
```

Read:

```text
Remote-only deferred-free is the strongest C7 remote profile signal so far.
It keeps overflow at zero and approaches tcmalloc-class r90 much more closely
than immediate remote packet handling.

Compared with full M6, it avoids applying the classless quarantine to
owner-local frees and improves r90 from roughly 17M to 19M in this smoke.
Compared with the baseline, r0 still drops from 62.35M to 48.46M, so this is a
remote-heavy candidate rather than a universal default.

Next useful work:
  reduce the r0 tax of enabling remote-only M6
  verify r50/r90 medians
  compare against tcmalloc/HZ4 on the same hakmem rows
```
