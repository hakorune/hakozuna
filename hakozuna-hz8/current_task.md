# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

## Frozen Baselines

```text
small-v0:
  hz8-small-v0-rc1
  docs/HZ8_SMALL_V0_RC1.md

medium-v1 protocol/geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md
```

Small-v0 behavior is frozen unless a hard safety issue appears.

```text
frozen small:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  startup-loaded Linux x86_64 ELF TLS contract
```

## Current Default Shape

```text
medium range:
  4097..65536

medium classes:
  8K / 16K / 32K / 64K

medium geometry:
  q64-run64k2
  64K class uses 128KiB run / 2 slots

medium identity:
  direct registry
  64KiB quantum directory
  power-of-two slot decode
  interior pointers INVALID

medium residency:
  budgeted empty-resident retention
  TLS active empty-live retention
  ctx-aware collector active-empty-live retention
  owner exit is the hard drain point

medium remote:
  owner-attached remote free publishes to owner queue
  pending bit is remote claim authority
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

## Promoted Post-RC1 Local Shape

```text
MediumActiveHitValidationCollapse-L1:
  default

MediumFreeDirectIdentityShape-L1:
  default

MediumMallocInitFastPath-L1:
  default
  h8_malloc_inner no longer calls h8_init before medium malloc;
  h8_thread_ctx_fast slow path remains initialization authority

MediumClassEntryFastPath-L1:
  default
  h8_malloc_inner computes medium class once and calls the class-resolved
  medium allocation entry

MediumActiveOwnerCheckCollapse-L1:
  default
  active-run allocation reuses one owner-token validation within the attempt
```

Evidence:

```text
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
bench_results/hz8_active_hit_ab_20260624T174838Z/
bench_results/hz8_free_identity_ab_20260624T175453Z/
bench_results/hz8_medium_initfast_ab_20260624T182824Z/
bench_results/hz8_medium_classentry_ab_20260624T183430Z/
bench_results/hz8_medium_active_ownercheck_ab_20260624T184605Z/
bench_results/hz8_medium_collect_cadence_attr_20260624T185501Z/
bench_results/hz8_medium_active_slot_attr_20260624T190231Z/
bench_results/hz8_medium_active_empty_fast_ab_20260624T190519Z/
bench_results/20260624T200933Z_medium_codeshape/
```

## Latest Direction

```text
source hygiene:
  bench report output is split into bench/h8_bench_report.c
  medium slot identity/local slot mutation primitives are split into
    src/h8_medium_slots.c
  public stats snapshot is split into src/h8_stats_public.c
  current large files remain below the 800-line working limit
  MediumSlotPtrInlineRestore-L1 restored header-inline slot pointer generation
    after the medium slot primitive split
  MediumMarkLiveInline-L1 removes the remaining release allocation call to
    h8_medium_mark_live_on_alloc while preserving debug counter semantics
```

```text
if optimizing medium/main local speed:
  MediumCollectCadenceAttribution-L1 is recorded
  local0 rows show periodic slow checks are all pending misses
  MediumActiveSlotMutationShape-L1 is recorded
  local0 rows are dominated by active-empty-live allocation reactivation
  MediumActiveEmptyAllocFastPath-L1 was tested and reverted as NO-GO in the
  simple pre-branch shape
  MediumMarkLiveInline-L1 is confirmed:
    medium_i0 +10.8%, main_i0 +3.3%, medium_r50 flat after R10 x4,
    small_i0 -1.9% within quick gate
  MediumActiveOwnerTokenInlineAudit-L1 was tested and reverted as NO-GO:
    asm target achieved, but medium_r50 regressed materially
  next step should be a different asm-visible call site, not owner-token inline

if optimizing RSS / rounded bytes:
  upper48 remains evidence-only until frozen small gates are reworked

if optimizing main stability / first-touch:
  chunk arena remains evidence-only until medium r50 no-regression is solved

do not reopen without new evidence:
  small-v0 behavior
  medium owner queue protocol
  medium owner lease micro-tuning
  sticky armed-set queue design
  broad collect cadence changes
```

## Gates To Preserve

```text
small frozen:
  INVALID platform fallback = 0
  duplicate claim accepted twice = 0
  quiescent pending bitmap = 0
  timeout / abort = 0

medium:
  invalid owned platform fallback = 0
  remote publish lost = 0
  claim accepted twice = 0
  collect duplicate = 0
  idle with pending = 0
  empty with pending = 0
  decommit while pending = 0
  owner/list mismatch = 0
  route authority mismatch = 0
```

## Read First

```text
lane guide:
  docs/HZ8_V1_LANES.md

medium protocol/geometry RC:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

same-run matrix:
  docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

main/medium attribution:
  docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md

benchmark gates:
  docs/HZ8_BENCH_GATE.md
```
