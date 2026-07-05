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
  and re-register. The generation check already guards the re-register
  case; NULLing the token on release closes the release window without
  adding a fast-path branch. Ordering rule if E4 lands: registration writes
  owner_token/class_id before publishing `slot_size`, and release writes
  owner_token=NULL before clearing `slot_size` to 0. A reader that sees
  `slot_size==0` bails out; a reader that sees a half-release with a NULL
  token takes the conservative non-local path instead of falsely matching.

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

```text
HZ10EntryTrim-L0 (measurement prep, first):
  - extend the stage-cost bench with a "route_fast" stage so E1/E2 are
    measured by the same protocol as the 2.37ns baseline
    (THREADS=4 PAGES=4096 REPEAT=20000 RUNS=10, medians).
  - add the differential route smoke BEFORE any fast path exists:
    for a matrix of registered pages (every class, plus large_alloc,
    plus released/re-registered generations), compare slow-path route()
    against the fast path over valid pointers, interior/misaligned/
    tail-slack/stale probes, and unregistered addresses. This smoke is
    the fail-closed gate for every E-box; it must pass byte-for-byte
    (same accept set, same owner/flags/generation for accepted ptrs).

E1 -> E2 -> E3 as separate commits, each gated on:
  - differential smoke + full existing smokes + ASan/UBSan, flag-off AND
    front lanes
  - stage-cost route (and route_fast) medians
  - local-path per class (the 5-class table) both lanes
  - same-run matrix RUNS=10: gate is "main_local0 and small_local0 move
    toward tcmalloc, remote rows unchanged"; keep per-box deltas honest,
    no combined-run attribution
  - rollback: each box is a small, revertable commit; E1/E2 keep the .c
    slow path as the semantic authority so reverting is deleting the
    inline path, not restoring semantics

E4 last, front lane only, judged on same-run + RSS columns
  (record-size growth) + the record-lifetime release/token rule above.
```

## Open questions for reviewers

```text
1. E1's header surface: export the root array directly (fastest, ugliest)
   or a static-inline accessor around an extern pointer? Any measurable
   difference under -fPIC for the eventual LD_PRELOAD .so build?
2. E2 exactness: is the multiply-back verification (slot_index * slot_size
   != offset) kept permanently as the interior check (self-verifying, no
   correctness dependence on the magic constants), or replaced by a
   proven-magic scheme? Recommendation: keep multiply-back -- it makes the
   magic constants performance-only, never correctness-bearing.
3. E4 token lifetime: is NULL-on-release sufficient, or does a token need
   its own generation? (Claim: sufficient if register publishes
   token/class before slot_size, and release clears token before slot_size.
   Verify this ordering argument in review against the existing lock-free
   route reader model.)
4. Should E3 also inline hz10_size_class_slot_size() for the front-cache
   init/refill paths, or is that slow-path-only and not worth header
   surface?
5. Does -flto on the production .so build (not just benches) make E1
   partially redundant? Worth one calibration run before implementing:
   if LTO alone recovers most of R1/R2, E1 becomes "enable LTO for the
   preload build" plus a much smaller header change.
```

## Relationship to the standing plans

HZ10_SPEED_ATTACK_PLAN_L0.md's "route result fastening / route/local-free
specialization" candidate (Branch 2C) is exactly this box, now with the
front-cache measurement (20260705T041052Z) proving it is the binding
constraint on the aggregate local rows. The remote-side designs
(HZ10OwnerTokenFastState, HZ10RemotePublicationV2) stay deferred; nothing
in E1-E4 forecloses them, and E4's record-carried owner token is a strict
subset of what an owner-state design would need anyway.
