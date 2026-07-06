# HZ10PartialOrphanAdoption-L1 Probe

Command shape:

```text
RUNS=3 THREADS_CSV=4 CHUNKS_CSV=128 LARSON_SECONDS=2
ALLOCATORS_CSV=hz10+orphan,hz10+orphan-partial
HZ10_SHIM_CENSUS_SEC=1 HZ10_SHIM_EXIT_STATS_CLASSES=0
make -C hakozuna-hz10 bench-larson-thread-churn-attribution
```

Verdict: GO as an opt-in sibling lane. Keep default HZ10 and idle-only
`hz10+orphan` unchanged until the broader macro matrix is reviewed.

Median results:

```text
allocator              throughput   current_rss_kb
hz10+orphan            2.069M       2,817,024
hz10+orphan-partial    2.085M         733,568
```

Census median:

```text
bucket                       hz10+orphan             hz10+orphan-partial
orphan_unadopted pages       32,329                  3
orphan_unadopted bytes       2,118,713,344           196,608
orphan_unadopted live slots  32,330                  3
orphan_unadopted free slots  5,465,330               188

class 8 orphan pages         32,324                  1
class 8 orphan bytes         2,118,385,664           65,536
class 8 adopted pages        0                       273
class 8 adopted live slots   0                       32,583
```

Read:

Partial adoption directly attacks the measured residual. The one-live-slot
class-8 orphan set collapses into a few hundred dense adopted pages, and
process RSS falls by roughly 74% while throughput is slightly higher in this
shape. `hidden_free_slots` stays zero for orphan buckets, so the result is not
an artifact of draining previously hidden remote frees.

Guard status:

```text
smoke-public-entry-orphan-adoption: green
smoke-public-entry-orphan-partial: green
hz10-standalone-check: green
git diff --check: green
```
