# HZ10 Entry Trim Design L0

Design memo for the per-op entry-cost push that HZ10FrontCache-L1's
measurement pass isolated as the remaining main_local0 lever (bench_results/
20260705T041052Z_hz10_front_cache_l1/notes.md, follow-up lever (1) in
current_task.md). Review document, not an implementation patch.

## Problem Statement (measured, not hypothesized)

After HZ10FrontCache-L1, the interleaved public rows still sit at ~0.53x
tcmalloc on main_local0 (~5.9ns/op hz10 vs ~3.1ns/op tcmalloc, where one op
is one malloc OR one free), and the front cache proved page selection is NOT
the cost there: removing page churn moved the row only +3.2%. The cost is
the per-op entry itself. Attribution already exists:

```text
HZ10PublicFreeStageCost-L0 (THREADS=4 PAGES=4096 REPEAT=20000 RUNS=10):
  route      2.37ns   -- ~43% of weighted local free cost
  local_free 3.07ns
  weighted_ns_per_logical_free_corrected = 5.56ns at r50

HZ10FrontCache-L1 same-run: main_local0 0.53, small_local0 0.71 --
  both rows bounded by route + entry overhead, not by page selection.
```

The 2.37ns "route" stage is not one thing. Reading src/hz10_pagemap.c, a
single hz10_pagemap_route() call on the hot VALID path pays, on top of the
two radix loads it actually needs:

```text
R1. a genuine cross-TU call: every production build except the pagemap
    route bench compiles WITHOUT -flto (Makefile builds the allocator as
    plain .o's), so route() is a real call+ret on every free.
R2. return-by-value of a 48-byte H10RouteResult: hz10_pagemap_result()
    stores NINE fields through the sret pointer (kind, reason, page_base,
    slot_size, slot_count, slot_index, generation, owner, flags); the
    local-free caller then reads back only kind, flags, and owner. The
    slot_index division result is computed and thrown away on every
    local free.
R3. a hardware integer division: `offset % slot_size` (interior check)
    plus `offset / slot_size` (slot_index) with a runtime divisor --
    the single most expensive ALU op in the whole free path. Divisors
    are per-page constants known at register() time.
R4. hz10_free() then touches the Hz10FreelistPage struct line for
    owner_thread_token/class_id/slot_size -- a third cache line (record
    line, page struct line, slot line) on every local free even when the
    front cache means the page's freelist itself is never touched.

M1. malloc side: hz10_size_class_for() is also a cross-TU .c function --
    a real call wrapping ~6 ALU ops and a clz.
```

None of these is a contract; all four route validations (tail-slack,
misaligned, interior, generation-stale) and the fail-closed rejection
behavior must survive byte-for-byte. The design constraint from
HZ10_SPEED_ATTACK_PLAN_L0.md stands: "preserve fail-closed route validation
for untrusted pointers" -- this box trims how the checks are computed,
never which checks run.

## Candidates (each its own A/B, in this order)

### E1. HZ10RouteInlineFast-L1: header-inline route fast path

Add `static inline` fast-path routing to hz10_pagemap.h that performs the
same two radix loads and the same four validations, but:

- no call, no ret, no 48-byte sret: results stay in registers, and the
  compiler dead-code-eliminates whatever a given call site does not read
  (the local-free site never materializes slot_index/reason/page_base).
- MISS/INVALID handling falls through to the existing .c function, which
  remains the single authority for diagnostics (reason codes) and keeps
  the header surface small. Production frees essentially never take that
  branch; smokes exercising rejection still go through the same code they
  test today.

Shape sketch (final signature is the implementation's choice):

```text
static inline int hz10_pagemap_route_local(const void* ptr,
                                           H10RouteLocal* out);
  /* returns 1 only for the exact pointer set the .c route() calls
   * H10_ROUTE_VALID; 0 means "call the slow path if you need to know
   * why". out carries only owner/flags/slot_size/generation. */
```

This requires exporting the root table (or an accessor) from the pagemap
TU. That is a deliberate revision of Box 1's "route stays a boundary .c
function" note: HZ10PublicFreeStageCost-L0 proved route is a PER-OP cost
(43% weighted), which is exactly the header-inline criterion
hz10_freelist_page.h already documents for alloc/free. The lock-free read
semantics (slot_size==0 means absent, slot_size published last) transfer
unchanged -- the inline path must load slot_size first and give up to the
slow path on 0, same as today.

Expected size: call+ret+sret traffic is the bulk of the gap between the
in-TU route bench (~2.4ns contribution measured inside the free composite)
and the raw loads/ALU the pipeline actually needs; expect route's
contribution to drop by roughly 1ns, but measure, do not trust this prose.

### E2. HZ10RouteDivFree-L1: kill the hardware division

The divisor is a per-page constant fixed at register() time, so replace the
runtime div with a per-record reciprocal (libdivide-style magic multiply):

```text
H10PageRecord gains, computed once in register():
  uint64_t slot_inv;   /* magic multiplier for slot_size */
  uint8_t  slot_shift;

route hot path becomes:
  slot_index = (uint32_t)((offset * slot_inv) >> slot_shift);
  interior   = (slot_index * slot_size != offset);
```

Two structural gifts make this smaller than the general case:

- slot_count == 1 (classes 22/23, large_alloc): offset < span == slot_size,
  so interior <=> offset != 0 and slot_index is always 0. One compare, no
  multiply at all. This also covers the multi-quantum large registrations
  where a 64-bit magic would otherwise need care.
- multi-slot slot_size <= 65536 and offset < 65536: 16-bit ranges, so a
  32x32->64 multiply with exactness verified by the multiply-back above is
  trivially correct across the whole input range (still add an exhaustive
  smoke: for every class, every offset in [0, span), fast interior/index
  == slow % and /).

Expected size: one 64-bit div (double-digit cycle latency, unpipelined on
older cores) becomes one imul + shift + imul-compare. On this machine the
div is already partially hidden by surrounding loads, so again: measure.

### E3. HZ10SizeClassInline-L0: size->class in the header

Move hz10_size_class_for() (and the slot_size table lookup) into
hz10_size_class.h as static inline over a shared const table. Pure
mechanical change, no semantics. This is the malloc-side twin of E1 and
probably the cheapest ~0.5-1ns in the whole plan.

### E4. HZ10RecordFatten-L1: owner token + class_id in the page record

Today's local free reads three unrelated cache lines: pagemap record, page
struct (owner_thread_token, class_id, slot_size for the marker), slot.
If the record itself carries the owner token and class_id:

```text
H10PageRecord gains:
  void*    owner_token; /* == Hz10FreelistPage.owner_thread_token */
  uint32_t class_id;
```

then a front-cache local free becomes record line + slot line + TLS line --
the page struct is not touched at all (the flag-off page path still needs
it for local_free_head/free_count, so E4's win is front-cache-lane-specific
and must be A/B'd there).

Costs to design around, not hand-wave:

- record grows 32 -> 48-64 bytes. Leaf virtual size doubles (demand-paged,
  so real RSS grows only with touched records: one 4KiB pagemap page then
  covers 64 instead of 128 quanta). Report RSS columns in the A/B.
- the token is set today by the public entry AFTER registration
  (hz10_freelist_page_set_owner_thread). Either extend the register call
  to carry token+class (one call, preferred -- pooled_page already has an
  owner-carrying variant) or add a small post-registration update under
  the same pagemap lock (extra lock round-trip per page create, slow path).
- release()/generation semantics must stamp the token dead (NULL) so a
  stale record can never token-match a recycled address between release
  and re-register.
  CORRECTED after review #1 (the original store-ordering argument here was
  unsound): H10PageRecord fields are plain non-atomic, so a lock-free
  reader has no acquire edge and the compiler may hoist the token load
  above the slot_size load -- "reader sees slot_size first" is not a
  property today's C can express, and "a NULL-token reader takes the
  non-local path" is NOT conservative (that path dereferences
  route.owner, which can be a page mid-destroy -- a pre-existing L0
  lock-free-read hole this doc must not cite as a safety net). The
  correct safety argument for the false-local-match property is
  different: a token equal to thread T's HZ10_THREAD_TOKEN is only ever
  written by T itself (registration runs on the owner thread), and
  per-location coherence guarantees T never reads a value older than its
  own last write -- so NULL-on-release, written by T when T destroys its
  own page, permanently hides T's stale token from T regardless of store
  order. If E4 lands it must also: make slot_size an _Atomic with
  release-store/acquire-load (token/class relaxed atomics), and state
  the residual pre-existing hole that a dead thread's recycled TLS token
  address could match an orphaned page (exists today through
  page->owner_thread_token; E4 neither fixes nor worsens it).

DEFERRED per review #1: E4 is NO-GO for this box. Its win is
front-cache-lane-only while HZ10_ENABLE_FRONT_CACHE is default OFF, the
box's own target arithmetic already excludes it, and it is the only
candidate that touches the lock-free reader's correctness envelope.
Rebundle it with the front-cache default-ON push, carrying the corrected
invariant above and the register/release hammer smoke from the gates
section.

### Explicitly out of scope for this box

- Remote publication cost (pending bit + Treiber CAS): frozen per the
  NO-GO ledger; nothing here touches claim/note/publish.
- Slot/page coloring for the 1-slot L1-set-aliasing gap (follow-up lever
  (2)): different mechanism, different box; E1-E4 do not move it and must
  not be judged on the 65536 row's absolute ratio.
- Front-cache capacity policy (HZ10FrontCacheCapacityTune-L0).

## Expected arithmetic (target, to be falsified by measurement)

```text
today, main_local0:      hz10 ~5.9ns/op vs tcmalloc ~3.1ns/op (0.53x)
E1+E2 (free side):       route 2.37 -> ~1.0-1.4ns contribution
E3 (malloc side):        ~0.5-1.0ns off the alloc op
E4 (front lane):         one cache line less per free; value depends on
                         working-set size, near zero on hot microbenches

if E1-E3 land in range:  ~4.3-4.9ns/op -> main_local0 ~0.63-0.72
                         small_local0 from 0.71 -> ~0.85+
```

This box alone probably does NOT clear the 0.75 main_local0 bar; it is the
largest single step toward it, and it compounds with the front cache
(both lanes share the same entry). State that honestly in the A/B rather
than stretching the numbers.

## Box plan and gates

Order revised per review #1 and the LTO calibration (see the review
section below): Step 0 LTO -> E3 -> E1 -> E2; E4 deferred out of this box.
E3 before E1 because it is zero-risk, independent, and validates the
measurement protocol cheaply; E2 after E1 stays as originally reasoned
(the div is partially hidden by call overhead today, so measuring E2
pre-E1 would understate it). E1 and E2 stay separate revertable commits.

```text
Step 0 (DONE 20260705): LTO calibration -- see review section for
  numbers. Follow-through: make -flto the default for the production/
  bench allocator builds (a Makefile change, its own tiny commit,
  measured with the standard same-run protocol) BEFORE E1, so E1's A/B
  baseline already includes the free win. Scope the Makefile change to
  the normal optimized allocator/bench lane; sanitizer/TSan lanes remain
  explicit compiler-mode gates and must be re-run after the flag change
  instead of assumed equivalent.

HZ10EntryTrim-L0 (measurement prep, before E1/E2):
  - extend the stage-cost bench with a "route_fast" stage so E1/E2 are
    measured by the same protocol as the 2.37ns baseline
    (THREADS=4 PAGES=4096 REPEAT=20000 RUNS=10, medians).
  - add the differential route smoke BEFORE any fast path exists:
    for a matrix of registered pages, compare slow-path route() against
    the fast path; it must pass byte-for-byte (same accept set, same
    owner/flags/generation for accepted ptrs) -- BOTH directions: no
    false accepts AND no false rejects (review finding 2: a valid
    pointer rejected as INTERIOR by an inexact reciprocal is a leak or
    abort in an LD_PRELOAD shim; this smoke is the primary gate for
    that, not redundancy). Matrix must cover:
      every class; arbitrary non-class slot_size values (register()
      accepts any uint32 divisor and large_alloc registers rounded
      sizes); slot_count==1 multi-quantum registrations INCLUDING
      probes into quanta >= 2 (must reproduce today's MISS, and must
      take the offset!=0 branch, never the 32-bit magic path, for
      64-bit offsets); released/re-registered generations; NULL;
      root-absent addresses; offset == span exact boundary; interior/
      misaligned/tail-slack/stale probes; unregistered addresses.

E3, then E1, then E2, as separate commits, each gated on:
  - differential smoke + full existing smokes + ASan/UBSan, flag-off AND
    front lanes
  - TSan run of the existing multi-thread smokes (new gate, review
    finding 6): E1 inlines the record loads into caller loops and
    deletes today's call-boundary compiler barrier, a race class the
    single-threaded differential smoke cannot see. E1 REQUIREMENT, not
    implementer's choice: the inline path loads slot_size with
    atomic_load_explicit(acquire) and the remaining record fields
    relaxed -- on x86 this compiles to the same plain loads plus
    compiler ordering, zero-cost insurance. (Reconfirm TSan runs in
    this environment first; current_task.md records a prior TSan
    runtime failure here.)
  - stage-cost route (and route_fast) medians
  - local-path per class (the 5-class table) both lanes
  - same-run matrix RUNS=10: gate is "main_local0 and small_local0 move
    toward tcmalloc, remote rows unchanged, large rows not regressed";
    keep per-box deltas honest, no combined-run attribution
  - rollback: each box is a small, revertable commit; E1/E2 keep the .c
    slow path as the semantic authority so reverting is deleting the
    inline path, not restoring semantics

E1 stated decisions (review findings 3, 8):
  - FLAG_LARGE routes return 0 from the inline path and fall through to
    the slow .c route (large frees are rare and munmap-bound); the out
    struct therefore does NOT carry page_base. Gate: large-row
    non-regression in the same-run matrix.
  - the inline path serves GENERATION_ANY callers only (both real call
    sites); expected_generation != ANY stays slow-path-only, stated in
    the signature contract.

E2 stated decision (review finding 5): the fast path snapshots
  slot_size/slot_inv/slot_shift ONCE per route and never re-reads --
  today's %-and-/ are self-consistent off one slot_size load; E2
  introduces multi-field consistency for the first time and a
  release+re-register race between reads must at worst reject (never
  mis-accept), which single-snapshot plus multiply-back guarantees.

E4: deferred out of this box entirely (see the DEFERRED note in the E4
  section). When rebundled with front-cache default-ON, its gates are
  the same-run + RSS columns plus a NEW register/release/route hammer
  smoke under TSan+ASan (threads continuously register/release/
  re-register a fixed base set while readers route stale pointers) --
  the static differential smoke cannot exercise token lifetime at all.
```

## Open questions (status after review #1 + LTO calibration)

```text
1. OPEN. E1's header surface: export the root array directly (fastest,
   ugliest) or a static-inline accessor around an extern pointer? Any
   measurable difference under -fPIC for the eventual LD_PRELOAD .so
   build? Implementer measures both, picks by number.
2. ANSWERED-with-correction (review finding 2): keep multiply-back, but
   the original claim was half wrong -- multiply-back prevents false
   ACCEPTS only; an inexact magic still causes false REJECTS of valid
   pointers. The magic constants ARE correctness-bearing in that
   direction, and the exhaustive differential smoke is the primary gate
   for it, not belt-and-suspenders.
3. SUPERSEDED (review finding 1): the ordering claim was unsound; see
   the corrected token-uniqueness/coherence argument and the atomics
   requirement in the E4 section. E4 itself is deferred out of this box.
4. OPEN, low stakes. E3 may inline hz10_size_class_slot_size() too if
   the header stays small; slow-path-only callers do not justify it on
   their own.
5. ANSWERED by measurement, 20260705 (Step 0): LTO alone is worth
   +5.6-7.6% on the local public-entry rows. See the review section.
```

## Review #1 verdict and applied changes, 2026-07-05

Independent agent review (fresh context, instructed to attack):

```text
overall plan:  GO-with-changes
E1 route inline:      GO-with-changes (atomics + large-path decision)
E2 reciprocal:        GO-with-changes (false-reject correction + smoke
                      matrix expansion + single-snapshot rule)
E3 size-class inline: GO as written
E4 record fatten:     NO-GO for this box; defer to the front-cache
                      default-ON push with the corrected invariant
```

All findings applied to this doc in place: the E4 safety argument was
replaced (the original store-ordering claim was unsound for plain
non-atomic fields; the sound argument is token uniqueness + per-location
coherence + NULL-on-release), open question 2 corrected (multiply-back
does not make magics correctness-free -- false rejects), E1 gained the
acquire-load requirement and the FLAG_LARGE fall-through decision, the
differential smoke matrix gained arbitrary-slot_size / multi-quantum /
boundary cases, and TSan plus a register/release hammer smoke entered
the gates. Review also confirmed: no Load-Bearing Constraint and no
NO-GO ledger entry is touched by E1-E3.

Step 0 LTO calibration, measured same day (interleaved RUNS=6 medians,
front lane, plus stage-cost RUNS=5):

```text
stage-cost route:      2.41ns with -flto vs 2.37-2.44 without -- the
                       isolated stage does not move (call overhead
                       pipelines away in a route-only loop)
main_local0:           168.8M -> 178.3M ops/s (+5.6%)
small_local0:          147.2M -> 158.4M ops/s (+7.6%)
slot_count1_local0:    183.3M -> 185.7M ops/s (+1.3%)
```

Read: in the real entry paths (not the isolated stage), cross-TU call
removal is worth +5-8% on local rows for one build flag. Follow-through
is a Makefile commit making -flto the default optimized allocator build,
landed and measured BEFORE E1 so E1's baseline includes it; E1's
remaining headroom (forcing the inline + lean result shape + acquire
ordering) is then measured honestly against that.

## Implementation Results, 2026-07-05

```text
Step 0 LTO: landed.
E3 size-class inline: landed.
HZ10EntryTrim prep: landed; route_fast stage added and differential route
  smoke added.
E1 route inline fast path: landed. Standard stage-cost
  (THREADS=4 PAGES=4096 REPEAT=20000 RUNS=10) measured route median
  ~2.46ns vs route_fast median ~1.57ns.
E2a slot_count==1 division skip: landed.
E2b multi-slot reciprocal fields: NO-GO. Prototype was correct under
  exhaustive fast-vs-slow span smoke and ASan/UBSan/TSan, but route_fast
  regressed to ~1.81ns median and H10PageRecord grew 32B -> 48B. See
  docs/HZ10_NO_GO_LEDGER.md.
E4 record fatten: still deferred.
```

Read: this box delivered the call/sret/local route win, but not the
division-free multi-slot shape. Future route work should avoid adding hot
record fields; otherwise it risks paying more in loads/cache footprint than
it saves in ALU latency.

## Relationship to the standing plans

HZ10_SPEED_ATTACK_PLAN_L0.md's "route result fastening / route/local-free
specialization" candidate (Branch 2C) is exactly this box, now with the
front-cache measurement (20260705T041052Z) proving it is the binding
constraint on the aggregate local rows. The remote-side designs
(HZ10OwnerTokenFastState, HZ10RemotePublicationV2) stay deferred; nothing
in E1-E4 forecloses them, and E4's record-carried owner token is a strict
subset of what an owner-state design would need anyway.
