# HZ8 Current Task

Updated: 2026-07-14

## Restart Surface

```text
public default:
  HZ8 / Mag16 / KeepRefill / GeneralMediumPage + EntryBoundary

active research line:
  SmallPartialTransitionDepot-L1 / P1 TransitionOnly recovery
  original depot Windows behavior and remote no-regression gates are GO
  P1 Linux research is GO; cross-platform default promotion remains HOLD

latest commit:
  9a73489f Promote HZ8 general medium default on Linux

next box:
  P1 Windows registration and measurement are complete
  keep P1 as research GO and public default unchanged
  do not reopen P2/P3/P4 or add a capacity/policy ladder
  DynamicCost-L0: complete
  SmallTierMembership-L1: mechanism GO / default NO-GO
  linear Mag16 scanning is not the xorshift common-path blocker
  close this recovery optimization family; seek application-like evidence
```

Read first:

- `docs/HZ8_UNIFIED_MEDIUM_DOMAIN_L0.md`
- `docs/HZ8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0.md`
- `docs/HZ8_WINDOWS_MEDIUM_HOT_PATH_ATTRIBUTION_L0.md`
- `docs/HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`
- `docs/HZ8_WINDOWS_GENERAL_MEDIUM_PAGE_GATE_L1.md`
- `docs/HZ8_PAGE8K_R3_INTEGRATED_GATE.md`
- `docs/HZ8_BENCH_GATE.md`
- `docs/HZ8_SMALL_PARTIAL_TRANSITION_DEPOT_L1.md`

## Fixed Decisions

```text
UnifiedMediumDomain-L0 shadow:
  GO
  native Ubuntu larger_sizes classified every observed free
  medium_hit=309487, page8k_hit=49, mismatch/conflict=0

UnifiedMediumDomainKind-L1:
  NO-GO / frozen reproduction lane
  fixed8K -17.04%, larger_sizes only +2.87%
  kind dispatch still paid the backend classifier

StableRecord-L0:
  GO diagnostic lifetime evidence
  records are type-stable and never reused
  LIVE -> CLOSING -> TOMBSTONE
  immutable geometry, stable mutex, slot/pending mirrors
  turnover and concurrent reader/writer smoke pass

Page8KRecord-L1:
  GO opt-in research candidate
  unified Page8K record bypasses the second Page8K classifier
  generic medium authority unchanged
  speed lane has no diagnostic counters or new atomics

public default:
  PROMOTED cross-platform
  Linux and Windows normal lanes select GeneralMediumPage + EntryBoundary
  old HZ8 v2 remains explicit hz8-v2-rollback

GeneralMediumPageSubstrateExpansion-L1:
  cap64 correctness and exact-size performance GO
  long R10: 8K +68.77%, 16K +215.54%, 32K +97.60%
  larger_sizes -0.09%, but balanced -9.23%
  public/default promotion HOLD

PageCap128:
  page_cap_reject 190221 -> 0 on fixed32K
  direct cap64 comparison: fixed32K +355.18%
  balanced -5.73%, wide_ws -3.39%
  32K specialist evidence only

ColdDispatch-L1:
  branch-hint-only probe
  balanced -10.78%
  NO-GO; flag and build row removed

EntryBoundary-L1A:
  non-small malloc moved to a noinline helper
  long R10: 8K +19.95%, 16K +146.22%, 32K +96.36%
  balanced -0.14%, wide_ws +3.64%, larger_sizes +0.55%
  RSS neutral; same-flag API smoke PASS
  Windows research GO; cross-platform/default HOLD

Linux validation surface:
  Makefile speed/smoke/safety targets added
  WSL GCC -Werror build, API smoke, and safety stress PASS
  WSL performance is too noisy for promotion
  native Ubuntu default/rollback R10 passed all promotion gates
  runner: scripts/run_hz8_page_general_linux_gate.sh

Windows default integration:
  public allocator, MT remote, and Redis-like runners now select hz8
  old v2 is research-only hz8-v2-rollback
  default/rollback long R10 passed all six performance/RSS gates
  fixed8K +21.60%, fixed16K +146.95%, fixed32K +91.79%
  balanced -2.18%, wide_ws +0.24%, larger_sizes -2.83%
  MT remote smoke 121.147M ops/s, peak 19.2 MiB
  Redis-like five-pattern focused smoke PASS

Windows tcmalloc frontier R10:
  remote-small: HZ8 129.292M vs tcmalloc 124.778M (+3.62%)
  remote-small peak: 18.93 MiB vs 728.57 MiB (38.5x lower)
  fixed8K/16K: HZ8 about one third of tcmalloc, but lower peak RSS
  fixed32K: HZ8 41.633M vs 300.840M
  balanced/wide/larger: HZ8 remains 3.0x-7.7x slower
  long mixed peak RSS is a measured HZ8 weakness, not a low-RSS claim row
  detail: docs/HZ8_WINDOWS_DEFAULT_TCMALLOC_R10.md

Small partial-span attribution:
  balanced/wide/larger peak RSS tracks 64 KiB small-span commits
  normal owner-list scan steps remain zero by intentional skip policy
  Mag16 full-preserve events are material across all three rows
  old broad SmallAvailableIndex is not reopened
  next behavior records only FULL -> AVAILABLE local transitions
  Windows AB/BA R5: balanced +435.73%, wide +178.34%, larger +37.34%
  WorkScale=10 candidate peak: 52.95 / 97.29 / 97.82 MiB
  fixed 8K/16K/32K and remote-small controls pass
  Linux trace-parity and P1 recovery gates are complete; default remains HOLD
```

## Page8K Record Evidence

WSL fixed8K R5:

```text
TargetDispatch: 506.65M
Page8K record:   535.79M
delta:           +5.75%
```

Windows ABAB R10:

| Row | Delta vs TargetDispatch |
|---|---:|
| fixed8K | -1.68% |
| balanced | +3.86% |
| wide_ws | +0.46% |
| larger_sizes | -0.07% |

Safety:

```text
WSL smoke/safety: PASS
owner exit:       8
remote free:      8192
duplicate claim:  1 rejected
invalid free:     7 rejected
Windows suite:    build PASS
```

## Archived Box: MediumRecord-L1

Goal:

```text
Remove the duplicate generic-medium directory lookup after a unified RUN hit.
Do not weaken ownership, generation, exact-slot, pending, or fail-closed rules.
```

Initial scope:

```text
same-owner generic-medium free only
stable record supplies immutable geometry and lifetime lock
record must be LIVE before and after lock acquisition
implementation pointer is consumed only while the stable lock is held
foreign owner / transition / unavailable record -> complete legacy path
generic remote publication remains unchanged
```

Required ordering:

```text
1. unified directory lookup
2. stable record lock
3. confirm LIVE
4. load implementation
5. confirm record geometry and exact slot
6. run existing same-owner free authority
7. unlock

On any uncertainty:
  release record lock
  run the complete old medium directory path
```

No-go:

```text
raw H8MediumRun handoff without stable lock
per-free reader refcount or diagnostic atomic in speed lane
foreign-owner behavior rewrite
owned-invalid converted to MISS/system free
record mutex destroyed or record reused
implementation used after unlock
```

Performance gate against Page8KRecord-L1 / TargetDispatch:

```text
larger_sizes: target >= +5%
balanced:     >= -3%
wide_ws:      >= -3%
fixed8K:      >= -3%
RSS:          <= max(+3%, +1MiB)
safety:       all existing gates pass
```

If the same-owner box is neutral or negative, close generic record handoff.
If it is positive, add a separate remote/transition box; do not silently widen
L1.

Result:

```text
WSL fixed8K:      -1.29%
WSL larger_sizes: -46.51%
safety:           PASS
decision:         NO-GO / frozen evidence
cause:            stable mutex replaced a lockless same-owner free path
```

Next admissible experiment:

```text
MediumOwnerWitness-L0 shadow only
mirror owner identity/generation into stable control metadata on cold changes
prove matching current owner implies implementation lifetime
no behavior, mutex, refcount, CAS, or production counter
```

Initial L0 evidence:

```text
owner sync:                 7
owner final sync:           20000
owner mirror mismatch:      0
owner update after CLOSING: 0
current-owner match:        4
status:                     GO synchronization evidence
```

Remaining blocker:

```text
Audit every detach/unregister caller.
Behavior is allowed only if a matching current owner excludes concurrent
teardown for the complete free operation without a new hot-path primitive.
```

Caller audit result:

```text
published medium runs are process-lifetime metadata today
owner exit detaches to ownerless pool; it does not unregister/destroy
destroy_scaffold callers are pre-registration creation failures only
```

L1 initial WSL R5:

```text
larger_sizes:            +22.42%
fixed8K control:          -3.36%
larger interleaved r90:   -3.35%
smoke/safety:             PASS
status:                   HOLD pending native AB/BA
```

Windows lock/lifetime correction:

```text
stable-record lookup on owner attach/detach:
  O(1) H8MediumRun backpointer, linear scan only as fallback

OwnerWitness run locking:
  existing embedded run lock
  stable mutex is not substituted into the allocation path

minimal 16K probe:
  previous apparent hang removed
```

Windows rotated R10 against Page8KRecord-L1:

| Row | Delta |
|---|---:|
| fixed8K | -2.77% |
| balanced | -7.05% |
| wide_ws | +2.26% |
| larger_sizes | -1.72% |

Diagnostic-only attribution:

```text
fixed8K generic-medium witness:
  attempt=268124 valid=268124 invalid=0 fallback=0

larger_sizes witness:
  attempt=160080 valid=160080 invalid=0 fallback=0

balanced / wide_ws witness:
  attempt=0
```

Decision:

```text
MediumOwnerWitness-L1:
  correctness/research GO
  cross-platform performance promotion NO-GO
  opt-in evidence only

The witness reaches the intended path at 100% in the medium rows, but Windows
does not recover the WSL larger_sizes gain. Do not add more owner-witness
knobs. The balanced regression cannot be attributed to the behavior because
that row never invokes it.
```

Claude layout review correction:

```text
The balanced row does not execute witness behavior, but the conditional
stable_domain_record field was inserted before hot run state fields. Its
presence can therefore change cache-line placement even when attempt=0.

Next single experiment:
  move stable_domain_record to the cold struct tail
  make the backpointer atomic with release/acquire publication
  align MEDIUM_RECORD_L1 owner-mirror maintenance
  rerun the identical Windows rotated R10

Interpretation:
  controls return to noise band -> previous balanced loss was layout artifact
  controls remain outside gate -> archive OwnerWitness with a real presence cost
```

Layout null-control result:

| Windows rotated R10, 10x duration | Delta |
|---|---:|
| fixed8K | -1.09% |
| balanced | -3.40% |
| wide_ws | +2.11% |
| larger_sizes | -6.13% |

```text
The short-duration R10 was rejected because ~30ms samples moved controls by
more than 8% in opposite directions.

The 10x run shows that cold-tail placement removes much of the control-row
distortion. The medium-heavy larger_sizes row still fails the -3% gate.

Final decision:
  atomic/cold-tail lifetime hygiene: GO
  OwnerWitness correctness evidence: GO
  OwnerWitness performance promotion: NO-GO / closed
  public HZ8 v2 default: unchanged
```

## Archived Box: WindowsMediumHotPathAttribution-L0

Goal:

```text
Explain the remaining native Windows exact-medium cost before adding behavior.
Audit fixed 16K and 32K in addition to the existing fixed8K control.
Keep production malloc/free free of diagnostic counters and atomics.
```

Decision boundary:

```text
alloc-side current-run/refill dominates:
  open one bounded current-run/refill box

free-side classify/authority dominates:
  evaluate the existing HZ10-style O(1) page substrate through an HZ8 contract

fail-closed checks dominate:
  record the contract cost; do not remove ownership/generation/state checks

unclear or layout-sensitive:
  stop behavior work and add a cycle-level microbench only
```

The audit is an independent diagnostic script, not a normal allocator matrix
row. OwnerWitness remains buildable only as closed speed evidence plus a
separate diagnostic sibling.

Native Windows release repeat-5:

| Size | HZ8 v2 | HZ10 substrate | tcmalloc |
|---:|---:|---:|---:|
| 8K | 63.190M | 179.180M | 204.940M |
| 16K | 20.760M | 45.330M | 30.200M |
| 32K | 5.520M | 13.770M | 15.100M |

Batched single-thread phase attribution:

| Size | HZ8 alloc/free ns | HZ10 alloc/free ns | tcmalloc alloc/free ns |
|---:|---:|---:|---:|
| 8K | 26.00 / 11.07 | 7.72 / 2.94 | 4.91 / 2.48 |
| 16K | 94.86 / 11.10 | 23.60 / 3.00 | 5.48 / 2.46 |
| 32K | 356.44 / 12.90 | 83.87 / 2.86 | 6.00 / 2.91 |

Diagnostic-only residency/scan result:

```text
budget reject:       0 at 8K/16K/32K
global scan:         initial run creation only
owner scan avg step: about 16 / 32 / 64 at 8K / 16K / 32K
resident reuse:      succeeds; decommit churn is not the cause
```

Decision:

```text
WindowsMediumHotPathAttribution-L0: complete
primary cost: alloc-side owner-run discovery plus the common medium substrate
free/classifier-only optimization: not the next box
MediumAvailableIndex revival: NO-GO; it already removed discovery without a
  clean medium_r50 gain
next: GeneralMediumPageSubstrateExpansion-L0 over the existing HZ8-native R3 contract
```

Correction after contract inventory:

```text
exact 8K R3 contract:
  already complete as an HZ8-native research substrate

do not create:
  a duplicate MediumPageSubstrateContract-L0

active P3 box:
  GeneralMediumPageSubstrateExpansion-L0
  exact 16K/32K immutable geometry shadow over the existing contract
```

P3 L0 gate:

```text
sizes:              exact 8K / 16K / 32K, fresh process per size
run geometry:       64KiB with 8 / 4 / 2 slots
geometry mismatch: 0
run mismatch:      0
state mismatch:    0
interior/duplicate: non-reusable
speed/default:     unchanged; shadow is diagnostic-only
```

Only after L0 passes may a behavior sibling replace fixed Page8K constants
with immutable per-class geometry. Remote publication, owner exit, orphan
residency, RSS, and native Linux gates remain mandatory before promotion.

P3 L0 Windows result:

```text
8K:  geometry 3/0, run mismatch 0, state mismatch 0
16K: geometry 3/0, run mismatch 0, state mismatch 0
32K: geometry 3/0, run mismatch 0, state mismatch 0
interior/duplicate: fail-closed
status: GO
```

L1 behavior design:

```text
page metadata:
  immutable class_id / slot_size / slot_count

owner inventory:
  active_page[class]
  available_head[class]

adoption:
  class-preserving only

shared unchanged contract:
  64KiB registry classification
  exact slot and live-state authority
  bounded pending bitmap and inbox
  owner close/adopt publication fence
  bounded orphan residency and decommit
```

L1 initial Windows result:

```text
API exact 8K/16K/32K: PASS
remote claims/drains: 14 / 14
remote reject/lost:   0 / 0

directional local:
  8K:  106.7M -> 135.7M
  16K: 54.8M  -> 151.1M
  32K: 18.8M  -> 33.5M

status:
  correctness GO
  performance HOLD
```

The exact32 control uses 256 live objects per worker, which needs 128 two-slot
pages. The initial per-class page cap is 64, so diagnostic attribution reports
only `211405/400384` allocations served by the page substrate; the rest use the
legacy medium fallback. Despite that boundary, the directional row improves
about 1.78x. Measure RSS and paired controls before changing the cap.

## Other Lanes

```text
hz8-v2:
  public balanced low-post-RSS default

hz8-v2-mag32:
  larger/local opt-in; global default HOLD

LargeDirectOwned:
  cross128 profile evidence; not default

LargeDirect HotCold/ShardedHot:
  HOLD / evidence only

HZ8 reclaim adapter:
  waits for a proven HZ12-compatible reclaim contract
```

## Rules

```text
Keep this file below 800 lines.
Move chronology and raw results to docs/ or bench_results/.
Do not mix diagnostic counters into production speed lanes.
Do not promote profile-only lanes without paired speed, RSS, and safety gates.
Do not weaken MISS / VALID / INVALID or owned-invalid fail-closed behavior.
```
