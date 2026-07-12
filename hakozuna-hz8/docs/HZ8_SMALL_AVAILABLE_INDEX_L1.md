# HZ8 SmallAvailableIndex L1

L0 found an average of about 2,061 usable same-owner 4KiB spans hidden at each
source commit. L1 tests one O(1) advisory index for class 8 only.

```text
push:
  local free reaches Mag16 full-preserve
  span is class 8 and not already indexed

pop:
  after active and Mag16 miss
  before pending collection and source commit
  validate owner, generation, class, state, and usable slots

exit:
  clear every intrusive membership before owner lifecycle handoff
```

The index is not route or slot-state authority. Stale entries are discarded on
pop. The speed lane has no counters; the diagnostic sibling owns push/pop/hit/
reject/reset counters and existing speed attribution.

While a span is `OWNED_ACTIVE`, the index reuses its otherwise inactive
`next_orphan_class` link. Thread exit clears every membership before orphan
handoff, so the candidate adds only an advisory membership flag rather than a
second pointer to every `H8Span`.

Initial gate:

```text
fixed 4KiB local >= +30%
balanced / wide_ws >= -3%
small local >= -3%
allocation failure = 0
safety smoke passes
```

Remote and cross-platform gates are required only after the initial Windows
local gate passes.

## Windows Result

```text
fixed 4KiB local, repeat-10 throughput:
  HZ8 v2 median:       about 14.17M ops/s
  SmallAvailableIndex: about 136.90M ops/s
  improvement:         about 9.7x

fixed 4KiB peak RSS, repeat-5 median:
  HZ8 v2:              about 1.08GiB
  SmallAvailableIndex: about 69.9MiB

diagnostic:
  span commit: 17,548 -> 1,008
  index push: 17,616
  index pop hit: 16,540
  index reject: 72

broad controls:
  balanced: about -1.7%
  wide_ws repeat-10 after metadata trim: about +1.2%
  larger_sizes: about +36%

fixed 4KiB remote90, equal effective ratio, repeat-10:
  HZ8 v2 median: about 71.65M ops/s
  candidate median: about 84.09M ops/s
  allocation failure: 0
```

The remote diagnostic records no index hits, so remote throughput is only a
no-regression result. The mechanism's supported claim is same-owner 4KiB
reuse visibility and source-growth reduction.

## Decision

```text
Windows candidate: GO
public / cross-platform default: HOLD
production counters: disabled
```

The first candidate added another pointer to every `H8Span` and produced a
noisy wide_ws regression. Reusing `next_orphan_class` while the span is
`OWNED_ACTIVE` removed that metadata expansion. Membership is cleared before
owner exit, before the link regains its orphan-list meaning.

## WSL Linux Gate

WSL Ubuntu on the same checkout passed GCC `-Werror`, preload, smoke, and
safety stress. Alternating repeat-5 medians were:

| Row | HZ8 v2 | Candidate | Delta |
|---|---:|---:|---:|
| fixed 4KiB | 555.11M | 512.61M | -7.7% |
| balanced | 535.00M | 498.96M | -6.7% |
| wide_ws | 470.98M | 495.93M | +5.3% |
| larger_sizes | 213.80M | 191.72M | -10.3% |
| fixed 4KiB remote90 | 35.74M | 38.81M | +8.6% |

The Linux baseline already finds/reuses 4KiB spans efficiently enough that the
Windows index does not pay for itself. WSL is sufficient to block a shared
default promotion; native Ubuntu is required only if Linux promotion is ever
reopened.

```text
Windows: candidate GO
Linux/WSL: NO-GO
cross-platform default: HOLD
```
