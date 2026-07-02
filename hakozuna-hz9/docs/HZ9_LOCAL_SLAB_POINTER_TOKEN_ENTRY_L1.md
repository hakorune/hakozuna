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
MODE=inlinebody CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
```
