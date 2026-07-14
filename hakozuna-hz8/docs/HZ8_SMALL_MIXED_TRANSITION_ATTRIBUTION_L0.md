# HZ8 Small Mixed Transition Attribution L0

## Question

The warmed same-owner small pair is already cheaper than tcmalloc, while the
public mixed rows remain slower. This diagnostic asks which cold transition
separates those two observations.

No behavior was changed. `hz8-speed-attribution` is a counter-bearing sibling
and its throughput is not used for ranking.

## Existing Signals

The existing diagnostic surface is sufficient:

```text
local_active_hit / local_active_miss
local_freelist_pop / local_bump_alloc
reusable_mag_pop_attempt / hit / reject / push
reusable_mag_empty_pop_by_class
local_span_commit
pending_collect_call_count
```

No counter or atomic is added to the public speed binary.

## Windows LCG Observation

| Row | Small active allocs | Active miss/commit | Mag pop | Mag hit | Dominant empty class |
|---|---:|---:|---:|---:|---|
| balanced | 1,044,160 | 12,552 | 21,105 | 8,625 | c7: 12,187 |
| wide_ws | 832,951 | 6,735 | 8,595 | 1,924 | c6: 5,669 |
| larger_sizes | 153,605 | 3,887 | 6,478 | 2,611 | c8: 3,753 |

LCG reproduces the already documented partial-span visibility failure. The
misses concentrate in one class per row and fall through to 64 KiB span
commit. `SmallPartialTransitionDepot P1` is the existing research answer; its
xorshift regression blocks default promotion. This result does not reopen the
closed P2/P3/P4 or capacity ladders.

## Windows Xorshift Observation

| Row | Small active allocs | Commit | Mag pop | Mag hit | Mag push |
|---|---:|---:|---:|---:|---:|
| balanced | 1,024,362 | 443 | 150,582 | 150,211 | 191,207 |
| wide_ws | 846,425 | 781 | 143,321 | 142,604 | 195,319 |
| larger_sizes | 149,848 | 217 | 31,456 | 31,259 | 43,795 |

The xorshift public-control shape is not source-starved. Commits are below
0.15% of small allocations, while magazine pops occur for about 15%, 17%, and
21% of the small allocation path. Magazine hits are near-perfect, so another
capacity increase or replacement policy cannot remove the fixed transition
cost. Random local frees also publish magazine candidates frequently.

This reconciles the hot-pair audit with the mixed matrix:

```text
one live object:
  active span remains hot
  HZ8 is cheaper than tcmalloc at 64/128 B

mixed working set:
  frees are distributed across many spans
  allocation repeatedly leaves the active span and validates a Mag candidate
  source commit is rare under xorshift
```

## Decision

```text
SmallEntryTrim: closed
Mag capacity/replacement ladder: closed
P1/P2/P3/P4 recovery ladder: remains closed
source admission/OS commit optimization for xorshift: no-go
```

The next design question is narrower:

```text
Can same-owner local objects be reused without paying a full span-candidate
transition on 15-21% of small allocations, while preserving slot-state,
owner-generation, and remote-publication authority?
```

A possible research box is `SmallLocalObjectReuse-L0`, but it must begin with a
contract/design review. It is not permission to add another broad cache:

```text
required:
  same-owner local only
  slot state remains authority
  remote frees remain on the existing pending path
  stale generation is fail-closed
  bounded per-class storage
  diagnostic/speed siblings remain separate

no-go:
  trusted free that skips classification
  owner/generation weakening
  per-free atomic or global lock
  Mag32/Mag64 retry
  default promotion from a synthetic row alone
```

The design must explain why it is not merely a second representation of Mag16
and how duplicate availability between an object cache, active span, and Mag16
is prevented. Until that contract exists, behavior remains unchanged.

## Reproduction

```powershell
pwsh win/build_win_allocator_suite.ps1 -OnlyHz8 -IncludeHz8Research

pwsh win/run_win_allocator_matrix.ps1 `
  -Profiles balanced,wide_ws,larger_sizes `
  -Allocators hz8-speed-attribution `
  -IncludeHz8Research -IncludeDiagnostics

# Add final trace argument 1 for direct xorshift diagnostic invocations.
```
