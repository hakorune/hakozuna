# TLS model fix for the preload shim (+ correction of a prior probe error)

## Correction first: my earlier "front cache gives +17-20% on sh6bench"
## finding (20260707T070000Z notes) was invalid

The `libhz10front.so` used there was a leftover scratch build from
2026-07-06 05:46, predating the owner-record/orphan-adoption work
entirely (`nm -D` shows zero `hz10_public_entry_owner`/`hz10_orphan`
symbols). My actual compile attempt with the current default flag set
+ `HZ10_ENABLE_FRONT_CACHE=1` failed on the `#error` guard that existed
at the time; I mistook the stale artifact for a fresh result. Sorry --
flagging clearly so it isn't cited.

A clean three-way A/B after the guard was removed (default vs
front+fine vs front+coarse, sh6bench 100000/1/1000/4, 3 runs each) shows
NO meaningful difference: 0.81 / 0.82 / 0.82s. This CONFIRMS the other
agent's default-front NO-GO verdict and rules out a fine/coarse
interaction bug.

## Why front cache doesn't touch sh6bench at all (mechanism, not a probe artifact)

Read bench/shbench/sh6bench.c: it holds up to 100,000 pointers alive
simultaneously, sizes cycling 1..1000 via `size_base` / `/=2`. Every
touched class in that range has slot_count in the dozens-to-thousands
(64KiB / <=1000B). Front cache's measured 5-9x win was specifically for
slot_count<=2 classes (24576-65536B) where the page substrate forces a
page-switch every 1-2 ops. sh6bench never enters that regime -- there is
no page-switch pathology here for front cache to remove. Its wall gap is
a different mechanism entirely (below).

## Real mechanism: perf attribution, sh6bench, hz10 vs tcmalloc vs mimalloc

THREADS/config from sh6.in: call_count=100000 min=1 max=1000 threads=4
(binary reports "16 threads" internally -- untouched, pre-existing
script behavior).

```text
allocator  cycles        instructions   cache-misses  L1d-misses   CPU-sec
hz10       25.35B        46.34B         121.3M        286.8M       12.35
tcmalloc    9.32B        18.29B         142.7M        259.6M        4.66
mimalloc    8.49B        13.36B          84.3M        243.8M        4.08
```

hz10 pays ~2.5x tcmalloc's INSTRUCTIONS for the same work, while its
cache-misses are actually LOWER than tcmalloc's. Not a memory/locality
gap -- an instruction-count gap, i.e. per-op algorithmic/code-path cost,
exactly the family the entry-trim work already targets in the static
benches. flat perf profile (perf report -g none), hz10 self%:

```text
44.9% (26.9% self) hz10_free
28.1% (18.6% self) hz10_malloc
17.5% ( 7.2% self) __tls_get_addr           <- general-dynamic TLS access
11.7% ( 7.3% self) doBench (the benchmark itself)
11.3% ( 6.3% self) hz10_public_entry_current_owner
 7.9% ( 3.9% self) hz10_public_entry_current_owner_if_any
 5.4% ( 3.6% self) malloc (shim entry)
 5.1% ( 3.4% self) hz10_shim_mark_thread_for_stats
 3.8% ( 1.8% self) hz10_public_entry_owner_record
 2.6% ( 0.4% self) hz10_freelist_page_create_common.part.0
 2.3% ( 1.3% self) hz10_public_entry_current_owner@plt
```

`__tls_get_addr` (+ its @plt call sites) plus the three owner-lookup
wrapper functions total >20% of self time. This is TLS-ACCESS-MODEL cost,
specific to the SHIM as a shared object: a preloaded `.so`'s TLS
variables default to the slow general-dynamic model (a real function
call per access, because the dynamic linker in general must support the
library being dlopen'd/unloaded later). None of this shows up in the
project's static micro-benches, which link everything into one binary
and already get the fast local-exec model -- this cost was invisible
until the shim/macro lane existed to expose it.

## Fix: -ftls-model=initial-exec on SHIM_CFLAGS

Safe because LD_PRELOAD libraries load at process start and are never
dlopen'd or unloaded during the process lifetime -- exactly
initial-exec's contract. Applied to `SHIM_CFLAGS` in the Makefile (every
`libhz10*.so` variant, not just the default).

```text
sh6bench (100000/1/1000/4), 3-5 runs each, wall:
  before: 0.79-0.84s
  after:  0.69-0.73s        (-13 to -15%)
larson (2 8 128 128 1 12345 4), peak RSS, 3 runs each:
  before: 285.8-289.8MB
  after:  282.7-287.9MB     (no change, as expected -- larson's hot loop
                            amortizes TLS lookups across many ops per
                            entry; sh6bench's smaller working set per
                            call hits TLS access far more often per unit
                            of real work)
python_alloc (loops=40), 3 runs each, wall:
  before: 0.51-0.53s
  after:  0.49-0.50s        (small, consistent, within RSS/behavior noise)
```

Gates: smoke-shim-api, smoke-shim-foreign (fail-closed foreign-free
abort still fires correctly), hz10-standalone-check all green after
rebuilding every shim variant with `-B`. RSS unaffected everywhere (pure
codegen change, no behavioral/memory difference) -- correctly zero risk.

## Updated headline

sh6bench moves from ~2.5-3.2x tcmalloc/mimalloc wall to a smaller gap;
rerun the full RUNS=5 macro matrix to get the refreshed table (this
probe used a fixed 3-5 run spot-check, not the full harness).

## Codex verification addendum

Checked on the same checkout after applying the Makefile/docs diff:

```text
make -B -C hakozuna-hz10 \
  preload preload-front preload-coarse preload-base preload-fine \
  preload-orphan-adoption preload-orphan-partial \
  smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

Result: green. The compile lines for every `libhz10*.so` include
`-ftls-model=initial-exec`.

Binary check:

```text
nm -D libhz10*.so | grep __tls_get_addr
  no matches

readelf -r libhz10.so | grep -E 'TLS|TPOFF|DTP|__tls_get_addr'
  R_X86_64_TPOFF64 entries only
```

So the generated shim no longer imports `__tls_get_addr`; TLS access is using
the initial-exec relocation shape.

One full `ALLOCATORS_CSV=glibc,hz10,hz10-coarse,hz10-base,hz10+orphan,tcmalloc,mimalloc`
refresh attempt hit a single SIGSEGV in `hz10` larson run 2. Follow-up checks:

```text
standalone hz10 larson, same args: 5/5 pass
ALLOCATORS_CSV=hz10 RUNS=5 macro matrix: pass
ALLOCATORS_CSV=tcmalloc,mimalloc RUNS=5 macro matrix: pass
```

Treat the failed all-in-one refresh as a watch item, not a blocker for the TLS
codegen fix. The direct HZ10 lane and larson repro both passed after it.

Refreshed split-matrix medians:

```text
workload       hz10 wall/RSS          tcmalloc wall/RSS       mimalloc wall/RSS
python_alloc   0.910s / 106,756 KiB   0.840s / 104,448 KiB   0.680s / 102,508 KiB
redis_setget   0.550s /   8,064 KiB   0.550s /   8,064 KiB   0.550s /   8,064 KiB
larson         4.177s / 282,624 KiB   4.147s / 279,040 KiB   4.148s / 283,980 KiB
xmalloc_test   2.000s /  13,568 KiB   2.030s / 196,096 KiB   2.000s /  22,608 KiB
cache_scratch  1.100s /   3,968 KiB   1.090s /   7,552 KiB   1.100s /   5,724 KiB
mstress        0.220s / 203,472 KiB   0.160s / 216,576 KiB   0.200s / 333,964 KiB
sh6bench       0.690s / 320,896 KiB   0.320s / 271,360 KiB   0.250s / 273,704 KiB
```

Compared with the prior macro-width median, HZ10 sh6bench improved
`0.810s -> 0.690s` with essentially unchanged RSS. The row is still behind
tcmalloc/mimalloc, but the TLS fix is a real product-lane win.

## Next

1. Investigate the one all-in-one matrix hz10 larson SIGSEGV only if it
   repeats. The direct repros passed (standalone larson 5/5, hz10-only macro
   5/5), so do not block the TLS fix on it.
2. `hz10_shim_mark_thread_for_stats` at 5.1% self on EVERY malloc/free
   is suspicious if exit-stats aren't requested for this run -- worth a
   quick look at whether it can be a single cached branch instead of a
   function call, independent of the TLS fix.
3. The owner-lookup wrapper trio (current_owner / current_owner_if_any /
   owner_record, ~20% combined) is the next attribution target if
   sh6bench/mstress still lag materially after the TLS fix and a matrix
   refresh -- likely inlining/flattening candidates now that the bigger
   TLS-model cost is out of the way.
