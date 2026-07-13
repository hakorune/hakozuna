# HZ8 Current Task

Updated: 2026-07-13

## Restart Surface

```text
public default:
  HZ8 v2 / Mag16 / KeepRefill balanced

active research line:
  UnifiedMediumDomain

latest commit:
  51b8ebca Test HZ8 generic medium record handoff

next box:
  freeze UnifiedMediumDomain behavior expansion
  retain Page8KRecord and OwnerWitness as opt-in evidence
```

Read first:

- `docs/HZ8_UNIFIED_MEDIUM_DOMAIN_L0.md`
- `docs/HZ8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0.md`
- `docs/HZ8_PAGE8K_R3_INTEGRATED_GATE.md`
- `docs/HZ8_BENCH_GATE.md`

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
  unchanged
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

## Next Box: MediumRecord-L1

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
