# HZ11 Windows Span Bump-Batch L1

`HZ11_SPAN_BUMP_BATCH` is an opt-in Windows span-lane probe for the remaining
matrix refill cost. The selected `hz11-span-cache256` row is unchanged.

## Motivation

The random-mixed rows mostly stay in the thread-local cache, while the matrix
and Larson rows repeatedly enter `hz11_thread_cache_refill`. The returned sink
already has a classbatch probe, but a returned miss still falls through to a
single current-span bump. This probe seeds the local cache from the current span
in one bounded refill.

## Contract

The refill order remains:

```text
front cache -> returned sink -> current span -> new span/system fallback
```

When an existing current span has capacity, one slot is returned to the caller
and up to `HZ11_SPAN_BUMP_BATCH_COUNT - 1` slots are pushed into the local cache.
The batch is limited by:

```text
current span remaining slots
per-class cache room plus the returned slot
global cached-byte room plus the returned slot
```

No lock, atomic, route change, or source-lifetime change is added. A newly
carved span still returns its first slot through the existing path; batching
starts on the following refill.

## Probe

```text
HZ11_CLASSIFY_SPAN=1
HZ11_CACHE_CAP=256
HZ11_SPAN_BUMP_BATCH=1
HZ11_SPAN_BUMP_BATCH_COUNT=16
```

The lane name is `hz11-span-cache256-bumpbatch16`. It is diagnostic evidence
until repeat measurements establish that random-mixed and Larson do not regress.

## Acceptance

- Matrix `balanced` and `wide_ws` improve materially over `hz11-span-cache256`.
- `random_mixed` small/medium/mixed regress no more than 2 percent.
- Larson worker/main rows do not regress.
- RSS remains within 5 percent of the selected row.
- Smoke, route safety, and allocation-failure behavior remain unchanged.

## No-go

- Any route, safety, or smoke failure.
- Random-mixed regression greater than 3 percent.
- RSS growth greater than 10 percent.
- Refill batching causes cache overflow flushes or changes the selected row.

This is a Windows/profile-specific probe. It is not a Linux `span-transfer`
replacement and is not a default HZ11 claim.

## Initial Windows scout

The first single-run scout used the existing Windows runners and the same
machine for both rows:

| Workload | cache256 | bumpbatch16 | Reading |
| --- | ---: | ---: | --- |
| Matrix balanced | 16.02M | 16.52M | improvement |
| Matrix wide_ws | 13.13M | 12.25M | regression |
| Matrix larger_sizes | 30.44M | 32.29M | improvement |
| random small, RUNS=3 median | 155.823M | 156.072M | neutral |
| random medium, RUNS=3 median | 152.970M | 154.254M | small improvement |
| random mixed, RUNS=3 median | 152.217M | 154.423M | small improvement |
| Larson worker, 2s scout | 40.54M | 35.01M | regression |
| Larson main, 2s scout | 40.57M | 37.21M | regression |

RSS stayed flat or lower in the matrix rows, but the Larson regression is a
hard blocker for general promotion. The lane remains a matrix-specialist
probe; do not select it as the Windows default. A follow-up should inspect
cache fill/flush interaction in cross-thread Larson before trying larger batch
values.

The diagnostic sibling `hz11-span-cache256-bumpbatch16-diag` reports
`bump_batch_calls`, `bump_batch_items`, and `bump_batch_max` alongside the
existing per-class overflow/flush and matrix attribution counters. Use this
row to determine whether Larson loses time to cache churn after batching; do
not use it for speed ranking.

The same diagnostic build is connected to the Larson runner as
`hz11-span-cache256-bumpbatch16-diag`, so worker-warmup and main-warmup can be
compared without mixing diagnostic counters into the speed rows.

The independent follow-up probe is `hz11-span-cache256-returnedrange`. It keeps
the selected cache shape and changes only flush-side returned insertion: arena
objects are collected locally and spliced into the per-class returned sink with
one lock acquisition. It does not change flush amount, cache cap, or source
lifetime.

## Returned-range scout

The independent returned-range probe improved the same matrix rows without
changing the cache cap or flush amount:

| Workload | cache256 | returnedrange | Reading |
| --- | ---: | ---: | --- |
| Matrix balanced | 17.24M | 26.65M | +54.6% |
| Matrix wide_ws | 14.27M | 19.09M | +33.8% |
| Matrix larger_sizes | 33.37M | 36.60M | +9.7% |
| random small, RUNS=3 median | 154.623M | 156.652M | +1.3% |
| random medium, RUNS=3 median | 153.924M | 152.944M | -0.6% |
| random mixed, RUNS=3 median | 153.600M | 154.586M | +0.6% |

RSS remained nearly flat. These are scout results, not promotion-grade
repeats. The returned-range lane is the stronger next candidate than
bumpbatch16, while the selected row remains unchanged until Larson and repeat
matrix results are collected.

Review outcome: the splice preserves the old lock-protected push/pop, intrusive
link, double-free, and route contracts. The diagnostic sibling adds only
per-class `returned_push_range_calls` and `returned_push_range_items`; these
are absent from speed rows. The next promotion gate is Larson worker/main
repeat-5, not half-flush or additional policy tuning.

Larson R5, T=4, 2-second gate:

| Warmup | cache256 median | returnedrange median | Reading |
| --- | ---: | ---: | --- |
| worker | 40.819M | 39.609M | -3.0%; gate not clean |
| main | 39.895M | 40.202M | +0.8%; neutral |

The class diagnostic confirms that the range path is live: the 1KiB class made
about 90 range calls carrying about 23K objects in each short Larson run. This
supports the lock-round-trip explanation, but the worker result blocks selected
row promotion. Keep returnedrange as a matrix candidate-watch and defer
half-flush or lock-outside-chain work until a longer worker gate determines
whether the three-percent result is persistent.

The lock-outside-chain scout moved private intrusive-chain construction before
the sink lock and kept only the head splice under lock. Larson 2-second R3
medians were worker 42.061M -> 39.386M (-6.4%) and main 38.427M -> 40.105M
(+4.4%). The critical section is smaller, but worker remains the blocker, so
returnedrange is still candidate-watch and half-flush remains deferred.

The new sink-depth diagnostic found `returned_sink_depth_max=12,840` for the
2KiB class in balanced and `60,798` for the 1KiB class in wide_ws. The current
depth also remained nonzero at the benchmark snapshot, showing that the sink
retains a substantial cold tail under pressure. This points to retention and
reuse locality as the next design surface; it does not yet justify a half-flush
policy without a separate RSS/lifetime experiment.

Runtime topology audit H found that returnedrange depth plateaus immediately
and is almost identical to baseline. For class 6 (1KiB), worker depth_max was
about 20.2K at 1s and 19.5K at 10s for returnedrange, versus about 20.2K at 1s
and 19.8K at 5s for baseline; main was about 20.3K at 1s and 20.5K at 10s for
both. Therefore retention is not the direct cause of the Larson worker
regression. The next isolated design is split publication of a range splice;
soft-cap and half-flush remain deferred.

The split-publication probe is `hz11-span-cache256-returnedrange32`. It
publishes each flush range in 32-object chunks, preserving route/lifetime
semantics while restoring finer producer/consumer visibility. It is opt-in
evidence only and is compared against both cache256 and one-shot returnedrange.

Split-publication R3 did not recover Larson: worker was 41.783M baseline vs
40.201M returnedrange32 (-3.8%), and main was 42.432M vs 39.994M (-5.7%). The
row remains safe matrix evidence, but the split-publication hypothesis is
closed as a general promotion path. One-shot and chunked returnedrange both
remain non-default until a separate Larson lifecycle explanation is found.

Long Larson R5 (10 seconds) remains mixed: worker cache256 median 47.305M vs
returnedrange 46.937M (-0.8%), but main cache256 median 46.391M vs
returnedrange 40.417M (-12.9%). The main baseline is noisy, yet the candidate
is consistently near 40M in this gate. Keep returnedrange and returnedrange32
as matrix evidence only; main-warmup lifecycle/pacing is now the next diagnostic
surface.

The longer 5-second R5 reproduced the worker weakness, so general promotion is
still NO-GO. The next narrow probe is the lock-scope trim already isolated by
the design review: build the private intrusive chain outside the sink lock and
perform only the O(1) head splice while locked. It does not change flush amount,
cache cap, route behavior, or source lifetime.

## Paired Larson R5 closeout

The final gate used ten-second runs with five interleaved baseline/candidate
pairs for each warmup mode. The first run in each pair alternated, reducing the
earlier sequential-order and thermal bias.

| Warmup | cache256 median | returnedrange median | Delta | Decision |
| --- | ---: | ---: | ---: | --- |
| worker | 42.503M | 40.241M | -5.3% | NO-GO for general promotion |
| main | 43.019M | 40.429M | -6.0% | NO-GO for general promotion |

The candidate remains materially useful on the Windows allocator matrix, but
it fails the Larson non-regression target in both warmup modes. The result is
now strong enough to close returned-sink batching as a general promotion path:
keep `returnedrange` and `returnedrange32` as matrix-specialist evidence, while
`hz11-span-cache256` remains the selected/default row. No half-flush, retention
cap, or additional returned-sink policy should be added from this experiment.

## Null controls: A/A and inert build

The A/A baseline ABAB R5 showed a substantial Larson noise floor. Worker pair
deltas ranged from -17.8% to +10.3%, with A/A medians differing by only +0.2%.
Main pair deltas ranged from -3.5% to +9.6%, with A/A medians differing by
-0.4%. A single five-percent candidate delta is therefore not decisive on
this machine without a stronger noise-control protocol.

An inert build was compiled with returnedrange code present but runtime forced
through the legacy per-object publication path. In alternating-order R5, the
worker medians were 45.456M inert versus 46.357M returnedrange (+2.0%), and
main medians were 47.169M versus 46.956M (-0.5%). The earlier -5% did not
reappear in the code-layout control. No stable binary-layout regression is
established. Keep default unchanged and do not add another returned-sink
policy; if promotion is revisited, use A/A noise bands rather than a fixed
three-percent Larson threshold.

If this area is revisited, the next experiment should explain Larson lifecycle
or pacing attribution independently. It should not combine another sink policy
with the current returnedrange implementation.

## Lifecycle diagnostic D1

Diagnostic 5-second runs (three per allocator and warmup mode) showed that the
candidate does not change the surrounding lifecycle topology. For the 1KiB
class, refill counts stayed around 29K-31K, overflow counts around 112-118,
and returned sink depth around 19.5K-20.6K for both cache256 and returnedrange.
The candidate published roughly 112-118 ranges containing 28.7K-30.2K items in
each run, as expected from the range splice. Short-run throughput was noisy and
slightly positive for the candidate, but this is not sufficient to overturn the
paired ten-second Larson gate.

This rules out a new source/lifecycle failure as the immediate explanation for
the regression. Existing diagnostic counters are sufficient, and no hot-path
timing counters should be added. If work resumes, isolate benchmark pacing or
lock interaction in a separate diagnostic experiment without changing the
returned-sink policy.
