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

`HZ9LocalSlabPageRouteBoundary-L0` showed that route-first public free is too
expensive, but later disassembly also showed that several fused/local body
microbenchmarks were optimized into phantom loops.  Only probes that retain
real bit load/store mutation are valid gate candidates.

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

Assembly gate:

```text
hot loop must retain real allocation-state mutation:
  ctz / slot selection
  free_bits update
  alloc_bits update
  last-token check

if alloc/free mutation is optimized away:
  classify the number as phantom ceiling evidence
  do not use it as a GO gate
```

Honest target band for current same-thread class64 probes is 300M..400M ops/s.
Numbers above that are useful only after asm confirms state mutation remains.

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
The early ptrtoken / inlinebody / lastpublic numbers are no longer accepted as
GO evidence by themselves.  They are useful for code-shape exploration, but asm
must prove the hot loop still mutates allocation state before they can define a
gate.

The first route-fallback debug entry API is not acceptable: ptrentry is slower
than route-first split.  The loss is the helper/TLS/public-shaped boundary, not
the pointer-token idea itself.  The next implementation must keep pointer-token
entry inline or TU-local in the public allocator body and use route only for
miss/invalid/foreign fallback.

The inline/TU-local split shows a sharper distinction:

```text
64-entry direct-mapped ledger:
  about 200M ops/s

one-entry last-token LIFO phantom ceiling:
  about 862M ops/s
```

So the next concrete design should use a last-token first tier, then a bounded
ledger, then route fallback.  The last-token result is a ceiling and not a
general free-order solution; without visible alloc/free mutation it is not a
performance gate.

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
ceiling.  Earlier this was read as helper/code-shape overhead.  After asm
inspection, the safer read is that `lastpublic` is also a phantom ceiling unless
real bit mutation is visible.

The route-boundary TU `lastentry` helper improves over `ptrentry` but is still
far from the phantom ceiling.  This confirms that another debug/helper API is
not enough.  The next real candidate must keep route only as cold fallback and
must be judged by honest mutation asm, not by DCE-friendly fused loops.

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

Unisolated helper follow-up:

```text
fastleaf helper/mode in the same bench TU:
  fastleaf:
    about 256M..429M ops/s

  integrated after adding that helper:
    about 286M..450M ops/s

decision:
  NO-GO as an unisolated helper probe
```

The lesson is not that helper abstraction is inherently bad.  It is that the
benchmark must keep each hot loop mechanically isolated before judging helper
shape.  Adding another loop/mode inside the same large `main()` can change
register allocation and destroy the prior integrated ceiling.

Harness correction:

```text
problem:
  multi-mode main() can change register allocation for unrelated hot loops

fix:
  move integrated / fastleaf hot loops into noinline workers
  keep main() as dispatch/report only for these probes

purpose:
  decide whether helper shape is really slow, or whether prior drops were
  benchmark harness artifacts
```

Worker-isolated result:

```text
command:
  MODE=<mode> CLASS_ID=5 ITERS=30000000 TOUCH=1

integrated R3:
  757.303M
  754.960M
  743.951M

fastleaf R3:
  835.766M
  791.037M
  866.959M

lastpublic R3:
  870.273M
  837.740M
  875.807M

read:
  helper/leaf shape is viable when isolated
  keep Layer0 as a pure leaf or entry-local body
  keep Layer1/Layer2 fallback logic out of the hot worker
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

## Last-Token Authority Entry

```text
box:
  HZ9LastTokenAuthorityEntry-L1

purpose:
  extend the fast last-token entry from free-only to the public local API trio

entry API:
  free:
    last-token exact hit -> inline free slot
    miss/stale/interior/double -> route fallback

  usable_size:
    last-token exact allocated hit -> token slot_size
    miss/stale/interior/freed -> route fallback

  realloc in place:
    last-token exact allocated hit and size <= slot_size -> ptr
    miss/stale/interior/freed/oversize -> route fallback

gate:
  last-token fast counters for free / usable / realloc are nonzero
  fallback counters are nonzero for invalid probes
  owned invalid is not forwarded as MISS
```

This is still single-thread local evidence.  Multi-thread and remote rows stay
closed until the public single-thread authority shape is clean.

Implementation read:

```text
added:
  h9_lsp_debug_lasttoken_usable_size()
  h9_lsp_debug_lasttoken_realloc_in_place()
  MODE=lastusable
  MODE=lastrealloc

smoke:
  exact usable/realloc fast path
  interior usable fallback
  oversize realloc fallback
  exact free
  double/invalid classification

result, CLASS_ID=5 ITERS=30000000 TOUCH=1:
  lastentry:   71.461M
  lastusable:  49.650M
  lastrealloc: 52.078M
  ptrentry:    73.887M
  ptrusable:   55.989M
  lastpublic: 814.455M
  fastleaf:   617.899M

read:
  authority trio is wired for correctness
  out-of-line debug/public boundary remains too expensive
  next performance candidate must be entry-local free/usable/realloc trio,
  not the debug route-boundary functions
```

## RouteLeaf Cold Fallback Probe

```text
box:
  HZ9RouteLeafColdFallbackProbe-L1

purpose:
  prove the clean Layer0/Layer1/Layer2 split without using debug public
  route-boundary functions as the hot body

shape:
  Layer0:
    routeable segment-local last-token helper
    no route call on exact LIFO hit

  Layer1:
    dispatch calls one cold fallback only on miss

  Layer2:
    noinline/cold direct-owned route
    handles intentional non-LIFO VALID fallback

bench modes:
  routeleaf:
    LIFO hit path

  routeleafnonlifo:
    alloc A, alloc B, free A through cold route VALID fallback, free B fast

result, CLASS_ID=5 ITERS=30000000:
  routeleaf touch=1 R3:
    279.212M
    247.680M
    248.926M

  routeleafnonlifo touch=1 R3:
    188.355M
    200.843M
    206.108M

  routeleaf touch=0:
    286.999M

  routeleafnonlifo touch=0:
    186.634M

read:
  cold edge works:
    route_valid == fallback count on non-LIFO
    state_mismatch == 0

  LIFO ceiling is not preserved:
    routeable segment-backed hot state is far below lastpublic / fastleaf
    payload touch is not the main cause

  decision:
    HOLD as design input
    keep Layer0 helper freedom, but do not put full routeable segment state
    directly on the hot local body
```

## Compact RouteLeaf Probe

```text
box:
  HZ9CompactRouteLeafProbe-L1

shape:
  hot path:
    H9LspInlinePage bits embedded in the entry
    last-token exact free checks entry-local state

  cold path:
    authority segment pointer lives at the entry tail
    cold fallback syncs entry-local bits into the routeable segment before
    direct-owned route classification

result:
  routeleafcompact CLASS_ID=5 ITERS=30000000 TOUCH=1 R3:
    314.142M
    302.418M
    334.129M

  routeleafcompactnonlifo CLASS_ID=5 ITERS=30000000 TOUCH=1 R3:
    200.751M
    202.339M
    203.968M
    route_valid = 30000000
    ptr_fast = 30000000
    ptr_fallback = 30000000
    state_mismatch = 0

  routeleafcompact CLASS_ID=5 ITERS=100000000 TOUCH=0:
    330.798M ops/s

  routeleafcompact CLASS_ID=5 ITERS=100000000 TOUCH=1:
    248.459M ops/s

comparison:
  routeleaf segment-backed TOUCH=1 in the same build:
    218.846M ops/s

  inlinebody TOUCH=0:
    412.369M ops/s

  fastleaf / lastpublic 800M+ family:
    DCE phantom unless asm proves alloc/free bit mutation remains

  knownslot / allocslotonly TOUCH=0:
    about 97M / 98M ops/s

read:
  entry-local bits are better than segment-backed mutable bits, but they do
  not restore the lastpublic / fastleaf phantom ceiling.

  disassembly shows fastleaf/lastpublic/inlinebody-style cyclic loops can
  collapse to same-pointer touch or counter loops in LIFO mode. Treat 800M+
  numbers as phantom ceiling artifacts, not as product-state mutation targets.

  next honest target:
    beat segment-backed routeleaf
    keep real bit mutation visible in asm
    move from the 300M floor toward 400M by removing only genuine checks
    keep cold route out of Layer0
```

Honest expected range:

```text
class64 touch=1:
  300M..400M with visible bit mutation:
    GO

  250M..300M:
    HOLD and inspect remaining hot loads/checks

  800M+:
    assume phantom until asm proves real alloc/free mutation remains
```

## Honest ASM Audit

```text
script:
  hakozuna-hz9/scripts/run_hz9_local_slab_honest_asm_audit.sh

latest:
  bench_results/20260702T224632Z_hz9_local_slab_honest_asm_audit

classification:
  integrated_worker:
    phantom-ceiling
    slot_select = 0

  fastleaf_worker:
    phantom-ceiling
    slot_select = 0

  routeleaf_segment:
    honest-candidate
    slot_select = 1

  routeleaf_compact:
    honest-candidate
    slot_select = 1

read:
  gate candidates must keep visible slot selection and bit mutation in asm.
  fused cyclic loops without visible slot selection are code-shape ceilings only.
```

## Commands

```bash
make -C hakozuna-hz9 smoke-hz9localslabrouteboundary
hakozuna-hz9/scripts/run_hz9_local_slab_honest_asm_audit.sh
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
MODE=lastusable CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastrealloc CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=routeleaf CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=routeleafnonlifo CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=routeleafcompact CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=routeleafcompactnonlifo CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=integrated CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=fastleaf CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=inlinebody CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
```
