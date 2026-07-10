# HZ12 Windows tcmalloc Hot-Path Audit (2026-07-10)

Status: read-only source, assembly, and measurement-provenance audit.

## Comparator Provenance

The Windows reference is `gperftools 2.16` `tcmalloc_minimal.dll` from vcpkg.
Sampling and heap checking are disabled in the minimal target.

| Artifact | SHA-256 |
| --- | --- |
| HZ11 xowner EXE | `D3FADEB76CC15B06FBF399B62D054021F95DBF29430CB5D84E89DBF76E0738CE` |
| HZ12 current xowner EXE | `73264DA954AFE42B6768341CA269800946F1E54AC57B0AB3F6402A9EEAA66E11` |
| tcmalloc xowner EXE | `EF55A27A74D910164CA640516B2C50FBD493DD1D290081B35BADF269F4FB86FC` |
| `tcmalloc_minimal.dll` | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` |

The exact tcmalloc source is available under the vcpkg gperftools 2.16
buildtree. The benchmark imports `tc_malloc` and `tc_free` from the DLL.

## Source-Level Cost Model

### tcmalloc

On a thread-cache hit, tcmalloc performs a direct TLS cache load, compact
size/page-class lookup, and intrusive freelist pop or push. Cross-thread free
does not preserve allocation ownership; it pushes into the freeing thread's
cache. The fast hit uses no lock and no atomic read-modify-write operation.

### HZ11

HZ11's basic cache/classification shape is close to HZ12 core, but the measured
xowner build uses only 32 cache entries per class. Cross-owner objects enter
the freeing consumer's cache. Overflow/refill converges on per-class returned
lists, whose range-push and refill batching options are disabled in that binary.
Assembly confirms per-object `CRITICAL_SECTION` traffic on returned push/pop.

### HZ12 diagnostic inbox lane

The measured HZ12 row is not a counter-free speed lane. Each tracked
cross-owner allocation/free round trip performs at least seven locked RMWs:

```text
owner shadow:
  global allocation counter increment
  span-owner compare/exchange

whole-span accounting allocation:
  allocation increment
  live increment
  optional carved-high-water compare/exchange

consumer defer:
  global deferred-object increment

owner drain accounting:
  live decrement compare/exchange
  release increment
```

Additional work includes owner lookup for every object, building intrusive
owner chains, mutex-protected batch publication, periodic owner drain, and two
calls per drained object: accounting release followed by full `hz12_free`.

## Assembly Findings

- HZ12 producer code is materially larger and has three extra steady-state
  calls after allocation: shadow, accounting, and periodic drain.
- HZ12 flush uses a `0x538`-byte frame and clears about `0x500` bytes of owner
  head/tail/count arrays for each 256-object flush.
- HZ12 drain detaches a chain under the lock, but processes each object
  separately afterward.
- HZ11 returned push/pop takes a per-class critical section per object in the
  measured control.
- All three harnesses pay runtime ring division and a full `mfence` on push and
  pop. This is a common absolute cost, not the relative HZ12/tcmalloc cause.

## Measurement-Drift Audit

The grouped R5 result was:

| Allocator | Grouped R5 median |
| --- | ---: |
| HZ11 | 5.828M |
| HZ12 | 10.634M |
| tcmalloc | 27.440M |

The runner executes every HZ11 run, then every HZ12 run, then every tcmalloc
run. It sets no affinity or priority, uses no cooldown, rebuilds only missing
binaries, and records no binary hashes.

A read-only alternating null control did not reproduce the apparent HZ12
binary regression:

| HZ12 binary | Alternating R3 median |
| --- | ---: |
| preserved early cap1024 binary | 18.538M |
| current binary | 18.937M |

The same early binary moved from 24.593M to about 18.52M inside one sequence.
Unchanged binaries rerun later produced HZ11 8.585M, HZ12 18.937M, and
tcmalloc 28.877M. Therefore 10.634M is not a stable current headline and the
post-R3 source change is not sufficient to explain the drop.

## Ranked Causes

1. Uncontrolled Windows run order, scheduler placement, and run state explain
   much of the R3/R5 movement.
2. Per-object diagnostic/accounting atomics are the strongest verified HZ12
   cost relative to tcmalloc.
3. Owner routing, grouping, mutex publication, and per-object drain replay are
   real structural costs absent from tcmalloc's ownerless fast hit.
4. The HZ12 comparison uses a non-speed frontend: TLS fast path and SOA are
   off, byte accounting is on, and the AoS class cache is enlarged to 256.
5. HZ12's power-of-two classes waste more space for uniform 8..1024 requests
   than tcmalloc's denser size map.

## Next A/B Order

1. Fix measurement provenance first: rotate allocator order by round, use a
   fixed affinity mask, and record EXE/DLL hashes in every result.
2. Compile out only whole-span accounting in the HZ12 owner-inbox lane. This
   removes four steady-state RMWs plus the high-water CAS while preserving
   owner routing.
3. Compile out only diagnostic counters, especially globally shared
   `alloc_span_seen` and `deferred_objects` cache lines.
4. Run bare HZ12 malloc/free through the common xowner harness to establish the
   allocator-core ceiling without shadow, inbox, or accounting.
5. Only after those controls, test frontend flags or redesign owner drain.

No production allocator change is justified before steps 1-4 separate
measurement noise, diagnostic tax, owner-routing tax, and allocator-core tax.

## Completed A/B Result

The corrected runner and first two compile-time controls are complete:

| Row | Median ops/s | Interpretation |
| --- | ---: | --- |
| diagnostic L1 | 18.420M | accounting and counters enabled |
| accounting off | 25.351M | owner-inbox behavior retained |
| accounting + counters off | 28.477M | speed mechanism lane |

The final speed comparison measured HZ11 at 13.046M, HZ12 speed at 28.896M,
and tcmalloc at 36.833M. Thus most of the earlier gap was diagnostic tax, while
a real 21.6% median gap to tcmalloc remains. Bare core is the next control; only
after that result should owner-map CAS/lookup or drain replay be redesigned.

The subsequent controls resolved that decision. Bare ownerless HZ12 reached
only 13.434M versus 29.274M for the batch inbox, so inbox removal is a no-go.
Replacing the steady-state owner CAS with a relaxed same-owner load raised the
median to 36.052M (+23.1%). A direct R5 measured this candidate at 35.542M
versus tcmalloc at 37.597M. The remaining median gap is about 5.5%, close to
the measured A/A median-position noise; broad validation is required before
any stronger claim.

## Review Synthesis

The audit changes the interpretation of the existing L1 number:

```text
HZ12 owner-inbox L1 as measured:
  allocator core
  + owner routing
  + shadow ownership diagnostics
  + whole-span atomic accounting
  + inbox diagnostic counters

HZ12 speed lane:
  not measured yet
```

The seven locked RMWs are not all intrinsic owner-inbox cost. At least four
come from whole-span accounting and its high-water witness, while additional
globally shared RMWs come from shadow/inbox diagnostics. Therefore the first
speed candidate must preserve inbox behavior while compiling both accounting
and diagnostic counters out in separately attributable steps.

The corrected experiment order is:

1. runner provenance: round-robin order, fixed affinity, binary hashes, build
   flag manifest, and a newly measured A/A noise band;
2. owner inbox with accounting off but diagnostics retained;
3. owner inbox with accounting and diagnostic counters off;
4. bare HZ12 core in the common harness as a ceiling control;
5. only then decide whether owner grouping/drain itself needs redesign.

Cooldown is secondary to round-robin order. Grouped allocator runs are no
longer eligible for a headline result.

## Production Reclaim Authority

L5 correctly used atomic accounting as a diagnostic judge, but that judge must
not become production hot-path authority. A production candidate should use an
owner-local batch ledger:

```text
allocation/carve:
  owner updates its local span ledger

same-owner cache flush:
  owner applies a class/span batch delta on the cold path

foreign free:
  producer publishes objects only; no shared span counter update

owner drain:
  owner applies the returned batch delta locally

retirement checkpoint:
  enrolled producers acknowledge
  local caches flush
  inbox becomes empty
  owner ledger reaches a stable snapshot

reclaim:
  stable owner ledger says zero live
  existing cache/route/generation gates also open
```

Only the owner writes the production ledger, so normal allocation/free requires
no shared atomic accounting. Generation remains advisory locality metadata;
route safety remains independent. The existing atomic shadow stays in a
diagnostic sibling and compares its per-span result with the owner-local ledger
at L5 acceptance. Any mismatch is a hard no-go for production reclaim.
