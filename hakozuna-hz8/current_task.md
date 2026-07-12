# HZ8 Current Task

Updated: 2026-07-13

## Restart Surface

```text
public default:
  HZ8 v2 / Mag16 / KeepRefill balanced

active research line:
  UnifiedMediumDomain

latest commit:
  a6c5d845 Add HZ8 stable medium domain records

next box:
  MediumRecord-L1
  stable record -> generic medium same-owner free
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
