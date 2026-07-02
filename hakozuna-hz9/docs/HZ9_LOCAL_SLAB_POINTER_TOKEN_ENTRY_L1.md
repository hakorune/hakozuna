# HZ9 Local Slab Pointer Token Entry L1

## Decision

```text
status:
  active implementation probe

intent:
  keep the fast inline local slab body
  stop paying route first for same-thread exact frees
  keep route as canonical fallback authority

default:
  not public
  HZ8 unchanged
```

`HZ9LocalSlabPageRouteBoundary-L0` showed that local bit mutation is not the
primary blocker.  The route-boundary and known-slot split scaffolds stayed near
100M ops/s, while `inlinebody` reached about 750M ops/s with touch enabled.

This box tests the smallest public-entry bridge toward that body:

```text
malloc:
  inline page pop
  record ptr -> page/slot/class/generation in TLS ledger

free:
  first try exact TLS pointer-token hit
  if positive hit, inline free slot and clear token
  otherwise fall back to route authority
```

The ledger is a positive cache only.  A miss proves nothing.

## Authority

```text
positive cache:
  TLS pointer-token ledger

canonical authority:
  route result for MISS / VALID / INVALID

required rule:
  ledger miss / collision / stale / foreign / invalid-looking pointer
  must fall back to route, not platform free
```

Ledger fast free is valid only when all checks pass:

```text
token.live == 1
token.ptr == ptr
token.page != NULL
token.page->generation == token.generation
token.slot < token.page->slot_count
inline_free_slot(page, slot) succeeds
```

Interior, stale, double, tail, and foreign frees must miss or fail the ledger
and be classified by route fallback.

## Gates

Hard zero gates for a behavior candidate:

```text
ledger_route_equiv_mismatch = 0
ledger_interior_accepted = 0
ledger_double_free_accepted = 0
ledger_stale_free_accepted = 0
ledger_foreign_free_accepted = 0
owned_invalid_forwarded_to_platform = 0
usable_size_route_mismatch = 0
realloc_route_mismatch = 0
thread_exit_ledger_remaining = 0
```

Micro gate for this L1 probe:

```text
class64 touch=1 same-thread split malloc/free:
  >=350M ops/s:
    GO

  250M..350M:
    HOLD and inspect asm/counters

  <250M:
    NO-GO for this entry shape
```

Stretch target is 430M ops/s, comparable to prior trusted token-cache API
evidence.

## Initial Observation

Same host, `CLASS_ID=5 TOUCH=1`:

```text
split, ITERS=10000000:
  87.116M ops/s

knownslot, ITERS=10000000:
  132.922M ops/s

inlinebody, ITERS=30000000:
  526.644M ops/s

ptrtoken, ITERS=30000000:
  508.575M ops/s

ptrentry, ITERS=10000000:
  66.377M ops/s

ptrusable, ITERS=5000000:
  43.714M ops/s

ptrrealloc, ITERS=5000000:
  46.926M ops/s

inlinepublic, ITERS=30000000:
  202.176M ops/s

ptrtoken, after inline-public split, ITERS=30000000:
  203.539M ops/s

lastpublic, one-entry LIFO token ceiling, ITERS=30000000:
  861.877M ops/s

lastledger, last-token first then 64-entry ledger, ITERS=30000000:
  231.725M ops/s

live-only clear + compact token, lastledger, ITERS=30000000:
  314.480M ops/s

hot/cold pointer split, ITERS=30000000:
  271.814M ops/s

lastonly generic helper, ITERS=30000000:
  283.922M ops/s

lastentry route-boundary TU helper, ITERS=10000000:
  108.140M ops/s

integrated entry, cold fallback present, ITERS=30000000:
  646.511M..712.279M ops/s

lastentry after cold fallback split, ITERS=10000000:
  98.325M ops/s
```

Read:

```text
ptrtoken clears the 350M L1 gate and exceeds the 430M stretch target.
It preserves almost all measured inlinebody throughput while avoiding the
route-first public-free tax for same-thread exact frees.

The first route-fallback debug entry API is not acceptable: ptrentry is slower
than route-first split.  The loss is the helper/TLS/public-shaped boundary, not
the pointer-token idea itself.  The next implementation must keep pointer-token
entry inline or TU-local in the public allocator body and use route only for
miss/invalid/foreign fallback.

The inline/TU-local split shows a sharper distinction:

```text
64-entry direct-mapped ledger:
  about 200M ops/s

one-entry last-token LIFO ceiling:
  about 862M ops/s
```

So the next concrete design should use a last-token first tier, then a bounded
ledger, then route fallback.  The last-token result is a ceiling and not a
general free-order solution.

The first combined last+ledger probe improves over the 64-entry-only ledger but
still misses the 350M gate.  Treat it as evidence that the last-token fast path
is valuable, while general ledger spill must be cold-path enough to avoid
taxing the local common case.

HotPathTrim result:

```text
live-only clear:
  useful

compact token:
  useful

hot/cold pointer split:
  HOLD, pointer indirection is visible in this micro shape
```

After trimming, `lastledger` improves to about 314M but remains below the 350M
gate.  The next candidate should avoid general ledger work on the common LIFO
case rather than adding another pointer layer.

The generic `lastonly` helper is still far below the hand-shaped `lastpublic`
ceiling.  That points to helper/code-shape overhead rather than the last-token
idea.  The next candidate should carry the `lastpublic` body shape into the
public allocator entry, not wrap it in a generic token helper.

The route-boundary TU `lastentry` helper improves over `ptrentry` but is still
far from the hand-shaped ceiling.  This confirms that another debug/helper API
is not enough.  The next real candidate must integrate the last-token body into
the allocator entry TU itself, with route only as a cold fallback.

IntegratedEntry result:

```text
class64 touch=1:
  integrated:
    646.511M..712.279M ops/s

  lastpublic:
    671.041M ops/s

  lastentry:
    83.785M..98.325M ops/s

  lastledger:
    236.810M ops/s
```

This clears the 350M gate and confirms the GLM read: the winning shape is not a
generic pointer-token helper, but a last-token body integrated into the entry
code with fallback split cold.  The current integrated probe is still LIFO-only;
route fallback remains the correctness authority for miss / invalid / foreign
cases, and non-LIFO coverage must be added without reintroducing hot eviction
tax.

Cold-splitting the existing route-boundary `lastentry` fallback is useful but
not sufficient.  It improves the helper-shaped entry only modestly, so the
remaining loss is still the helper/public-shaped boundary and segment debug
substrate, not merely fallback placement.

Rejected follow-up:

```text
route-capable wrapper co-located with IntegratedEntry:
  integrated LIFO:
    about 188M..247M ops/s

  integrated non-LIFO, one cold VALID fallback per iteration:
    about 94M..97M ops/s

decision:
  NO-GO
```

Even when the fallback is not executed, placing route-classification machinery
in the same hot entry shape pollutes the LIFO path.  More precisely, the losing
shape is the inline second-tier decision layer between the last-token hit and
the cold route.  The next implementation must keep Layer0 as a pure leaf that
returns HIT/MISS only; Layer1 owns the single cold edge; Layer2 contains ledger
and route authority.

Three-layer rule:

```text
Layer0 hot:
  last-token only
  no route header
  no ledger probe
  no cold call
  returns HIT/MISS

Layer1 public entry:
  calls Layer0
  HIT returns
  MISS calls one noinline/cold fallback

Layer2 cold:
  optional ledger
  direct-owned route
  INVALID / MISS / remote classification
```

The failed wrapper does not disprove cold route fallback.  It proves the second
tier must not be inline-expanded into Layer0.

Rejected helper follow-up:

```text
fastleaf helper/mode in the same bench TU:
  fastleaf:
    about 256M..429M ops/s

  integrated after adding that helper:
    about 286M..450M ops/s

decision:
  NO-GO as a helper probe
```

The lesson is stricter than "return HIT/MISS": Layer0 must be the literal hot
entry body or a mechanically isolated leaf.  Adding another helper/mode in the
same benchmark TU changes layout/code shape enough to destroy the prior
integrated ceiling.  Keep the fast probe minimal until the real entry body is
split into separate TUs.
```

## Integrated Entry Candidate

```text
box:
  HZ9LastTokenIntegratedEntry-L1

shape:
  inline local slab pop/free body in the entry TU
  one live last-token only on the common LIFO path
  no malloc-time eviction into the 64-entry ledger
  no generic pointer-token helper call in the hit path
  route fallback is noinline/cold

stats:
  keep cheap fast/fallback counters
  do not make the fast path unobservable
```

This is deliberately narrower than a full public allocator entry.  The point is
to verify that the `lastpublic` body shape survives when the fallback is present
but cold.  A ledger tier may be reintroduced only if non-LIFO workloads need it
and it can remain off the common path.

Expected range:

```text
class64 touch=1:
  >=350M:
    GO

  450M..600M:
    realistic strong range

  lastpublic 800M+:
    ceiling, not product expectation
```

## Commands

```bash
make -C hakozuna-hz9 smoke-hz9localslabrouteboundary
MODE=ptrtoken CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=ptrentry CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastpublic CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastledger CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=hotcold CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastonly CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastentry CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=integrated CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=inlinebody CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
```
