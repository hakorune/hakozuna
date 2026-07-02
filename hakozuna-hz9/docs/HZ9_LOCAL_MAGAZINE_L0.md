# HZ9 Medium Local Magazine L0

## Status

```text
box:
  HZ9MediumLocalMagazine-L0

state:
  implemented experiment
  HOLD evidence, not promotion-grade
  source cleanup / asm proof before inline work
```

## Purpose

Test whether a TLS active-run magazine can improve HZ8 medium/main local
throughput without changing HZ8's remote authority or balanced default.

## Inherited Contracts

HZ9 L0 inherits these from HZ8:

```text
owned-looking INVALID must not fall through to platform free
slot_state remains validity authority
remote duplicate claim remains pending bitmap authority
qstate / owner queue remain remote notification authority
owner-exit hard drain remains mandatory
lazy/resident/active-live charge leaks remain hard failures
```

## Local Magazine Shape

```c
typedef struct H9MediumLocalMag {
  H8MediumRun* run;
  uint64_t mask;
} H9MediumLocalMag;
```

Placement:

```text
H8ThreadCtx:
  medium_local_mag[H8_MEDIUM_CLASS_COUNT]
```

Eligibility:

```text
same-owner local free
run == ctx->active_medium_runs[class]
slot_state == ALLOCATED
pending bit == 0
mag.run is NULL or mag.run == run
```

## Slot State

HZ9 L0 should represent magazine slots in `slot_state`, not TLS-only metadata.

```text
ALLOCATED:
  user-visible live allocation

LOCAL_MAG:
  owner-local freed slot
  not valid for another free
  only reusable by the local magazine

FREE:
  normal allocator-visible free slot
```

Recommended encoding:

```text
LOCAL_MAG:
  FREE tag + reserved payload sentinel
```

## Mask Semantics

For L0:

```text
LOCAL_MAG:
  allocated_mask bit remains 1
  free_mask bit remains 0
  local magazine mask bit = 1
```

Reason:

```text
avoid mark_empty / mark_live_on_alloc churn
avoid resident/lazy transition on immediate local reuse
keep the run logically LIVE while magazine slots exist
```

Mandatory safety rule:

```text
normal free fallback must require slot_state == ALLOCATED
```

Without that rule, a `LOCAL_MAG` slot could be accepted as a double free because
`allocated_mask` remains set.

## Required Source Prep

Split local free into decode and known-slot mutation:

```c
bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr,
                                       bool keep_empty_live);

bool h8_medium_run_free_local_known_slot(H8MediumRun* run, size_t slot,
                                         bool keep_empty_live);
```

The known-slot helper must not decode again. This prevents magazine miss from
paying duplicate slot-decode cost.

## Malloc Path

Insert magazine pop only after active owner validation and before normal active
allocation:

```text
active owner matches
  -> try local magazine pop
  -> normal active free_mask allocation
  -> existing active miss path
```

Magazine hit:

```text
LOCAL_MAG -> ALLOCATED
clear magazine bit
do not mutate free_mask
do not mutate allocated_mask
do not call mark_live_on_alloc
```

## Free Path

Same-owner active-run free:

```text
ALLOCATED -> LOCAL_MAG
set magazine bit
do not set free_mask
do not clear allocated_mask
do not call mark_empty
do not change residency charge
```

Non-eligible free falls back to known-slot normal free.

## Flush Points

Required:

```text
active switch
owner exit
detach
destroy
```

Flush operation:

```text
LOCAL_MAG -> FREE
clear allocated_mask bit
set free_mask bit
clear magazine bit
caller then reaches existing empty/residency logic at active switch,
owner detach, or run destroy
```

Owner exit must not depend only on TLS state. It must be able to scan owner
medium runs and repair `slot_state == LOCAL_MAG`.

The TLS owner flush is owner-filtered:

```text
ctx->owner == owner:
  flush TLS magazine

ctx->owner != owner:
  skip TLS magazine
  rely on owner run scan repair
```

## Hard Zero Gates

```text
local_mag_state_mismatch = 0
local_mag_double_free_accepted = 0
local_mag_owner_mismatch = 0
local_mag_pending_slot_accepted = 0
local_mag_remote_publish_accepted = 0
local_mag_normal_free_accepted = 0

owner_exit_local_mag_remaining = 0
detach_local_mag_remaining = 0
destroy_local_mag_remaining = 0

invalid_owned_forwarded_to_platform = 0
remote_publish_lost = 0
duplicate_remote_claim_accepted = 0
decommit_while_pending = 0

lazy_charge_leak = 0
resident_charge_leak = 0
active_live_charge_leak = 0
```

## Performance Gates

L0 viability:

```text
medium_local0:
  >= HZ8 * 1.03

main_local0:
  no regression, preferably >= HZ8 * 1.02

medium_r50:
  regression <= 2%

main_r50 / main_r90:
  regression <= 2%

small rows:
  regression <= 2%
```

L1 continuation:

```text
medium_local0:
  >= HZ8 * 1.05

main_local0:
  >= HZ8 * 1.05
  or no regression with medium_local0 >= HZ8 * 1.10
```

## Current L0 Status

Implemented behind `H8_HZ9_MEDIUM_LOCAL_MAG`.

```text
current L0 scope:
  48K / 64K medium classes only

source:
  hakozuna-hz9/src/h8_hz9_local_mag.c

targets:
  bench-hz9mediumlocalmag
  bench-release-hz9mediumlocalmag
  bench-release-hz9mediumlocalmag-min2
  bench-release-hz9mediumlocalmag-min5
  bench-release-hz9mediumlocalmag-disabled

class gate:
  hakozuna-hz9/scripts/run_hz9_local_mag_class_gate.sh

known-slot prep:
  h8_medium_run_free_local_scaffold()
    -> decode only
  h8_medium_run_free_local_known_slot()
    -> mutation helper
```

Quick sanity:

```text
debug local0:
  free_push=42824
  alloc_hit=42808
  state_mismatch=0
  flush_slots=16

debug r50:
  alloc_probe=29384
  alloc_no_mag=8185

dedicated smoke:
  free_push=9
  alloc_hit=2
  invalid_state=2
  flush_slots=3
  state_mismatch=0

release R10:
  medium_local0 ~= 0.86x
  medium_r50   ~= 0.85x
  main_local0  ~= 1.06x

release asm:
  try_alloc / try_free_known calls are inlined away
```

Short release R3 after inline cleanup:

```text
fixed48_local0:
  HZ8 median 134.79M
  HZ9 median 150.05M
  ratio 1.11

medium_local0:
  HZ8 median 86.60M
  HZ9 median 105.51M
  ratio 1.22

medium_interleaved_r50:
  HZ8 median 12.65M
  HZ9 median 12.92M
  ratio 1.02

main_local0:
  HZ8 median 133.19M
  HZ9 median 96.51M
  ratio 0.72

small_interleaved_remote90:
  HZ8 median 22.91M
  HZ9 median 29.20M
  ratio 1.27
```

This is short-run evidence only. It is enough to keep L0 on HOLD because
`main_local0` regresses while the target 48K row improves.

Focused R10 follow-up:

```text
main_local0:
  HZ8         156.38M
  HZ9 min4    156.99M
  HZ9 min6    155.70M
  HZ9 min2    153.46M

medium_local0:
  HZ8         143.21M
  HZ9 min4    140.22M

medium_r50:
  HZ8          34.74M
  HZ9 min4     33.98M

small_remote90:
  HZ8          49.99M
  HZ9 min4     50.95M

fixed48_local0:
  HZ8         175.15M
  HZ9 min4    188.57M
```

Interpretation:

```text
main_local0:
  the earlier R3 regression did not reproduce at R10

fixed48:
  local magazine helps the direct target

medium aggregate:
  target benefit is diluted or offset by other medium classes

remote/small:
  no clear immediate regression in this short follow-up
```

Class-gate exploratory run:

```text
script:
  hakozuna-hz9/scripts/run_hz9_local_mag_class_gate.sh

variants:
  hz8
  hz9_min2
  hz9_min4
  hz9_min5
  hz9_disabled

rows:
  fixed24_local0
  fixed32_local0
  fixed48_local0
  fixed64_local0
  main_local0
  medium_local0
  medium_r50
  small_remote90
```

Latest short RUNS=3 read:

```text
fixed rows:
  very noisy; do not promote from R3

main_local0:
  disabled ~= neutral
  min2/min5 around -2%
  min4 around -6%

medium_local0:
  min5 closest to neutral
  min2/min4 roughly flat to slightly negative

medium_r50:
  all variants negative in R3

small_remote90:
  min4 / disabled neutral
  min2 / min5 negative in R3
```

Interpretation:

```text
current best hypothesis:
  fixed48 benefit is real but too narrow
  class-wide magazine gating creates mixed effects
  min5 is the least disruptive medium-local variant

next measurement before behavior changes:
  RUNS=10 for hz8 vs min5 vs disabled on
    medium_local0
    medium_r50
    main_local0
    small_remote90
```

Class-specific counters:

```text
medium_hz9_local_mag_class:
  free_push=[24k,32k,48k,64k]
  alloc_hit=[24k,32k,48k,64k]
  flush=[24k,32k,48k,64k]
```

Read:

```text
push high, alloc_hit high:
  the magazine mechanism is reaching the class

push high, alloc_hit low:
  local frees are captured but not reused before active switch / fallback

alloc_hit high, flush high:
  the class benefits but active switching is discarding material capacity

push/hit concentrated in one fixed row only:
  keep as evidence; do not promote broad HZ9 behavior
```

Focused R10 with min5 / disabled:

```text
historical pre-split output:
  bench_results/20260701T174900Z_hz9_min5_disabled_r10_hz9_local_mag_class_gate/

medium_local0:
  hz8          116.02M
  hz9_min5      93.24M
  hz9_disabled 116.86M

medium_r50:
  hz8           25.65M
  hz9_min5      26.32M
  hz9_disabled  26.99M

main_local0:
  hz8          122.06M
  hz9_min5     121.97M
  hz9_disabled 121.35M

small_remote90:
  hz8           50.76M
  hz9_min5      51.11M
  hz9_disabled  50.96M
```

Read:

```text
min5:
  remote/small neutral-to-positive
  medium_local0 regression is too large

disabled:
  validates that most source-prep/code-shape changes are neutral

decision:
  keep HZ9 L0 behavior on HOLD
  keep source-prep and counters
  do not promote or broaden magazine behavior without a new mechanism
```

HZ3 reference:

```text
borrow:
  TLS-local reuse first
  batch/cache thinking
  keep remote work out of the common local hit when possible

do not copy directly:
  payload/TLS-only free authority as the only validity source
  remote freelist replacement for pending bitmap/qstate
  weakening owned INVALID fail-closed behavior
```

Why:

```text
HZ3 local speed:
  thin TLS bins / batch refill / less per-slot authority in the local hit

HZ8/HZ9 required contracts:
  slot_state validity
  remote duplicate-claim prevention
  owner-exit hard drain
  bounded RSS / lazy128
  MISS vs owned INVALID separation
```

Unsafe TLS-only ceiling:

```text
build:
  bench-release-hz9mediumlocalmag-unsafe-tlsonly

purpose:
  test whether removing LOCAL_MAG slot_state transitions restores 64K speed

result:
  fixed64_local0:
    hz9_min5           0.297x
    hz9_unsafe_tlsonly 0.296x
    hz9_disabled       0.999x

  medium_local0:
    hz9_min5           0.834x
    hz9_unsafe_tlsonly 0.853x
    hz9_disabled       1.009x

read:
  slot_state transition is not the dominant loss
  weakening slot_state authority does not rescue this L0 design
  the active-run magazine insertion shape is the problem
```

Code-shape audit:

```text
script:
  hakozuna-hz9/scripts/run_hz9_code_shape_audit.sh

current result:
  default_hz9_behavior_symbols = 0
  default_hz9_try_calls        = 0
  hz9_flush_symbols            = 1
  hz9_try_symbols              = 0
  hz9_try_alloc_calls          = 0
  hz9_try_free_calls           = 0

interpretation:
  HZ8 default symbol isolation is clean
  HZ9 magazine hot push/pop are no longer out-of-line calls
  performance gates can now test mechanism cost without call overhead
```

Interpretation:

```text
mechanism:
  clean

throughput signal:
  target 48K/medium rows improve after inline cleanup
  main_local0 still regresses in short R3

promotion:
  NO

reason:
  target medium rows regress despite clean mechanism

next:
  keep HZ8 default isolated
  rerun performance gates
  do not change remote authority or HZ8 balanced default contracts
```

## Stop Conditions

```text
remote authority changes are required for speed
slot_state authority must be weakened
owner-exit flush becomes complex or leaky
medium/main remote rows regress by more than 2%
medium_local0 gain is less than 3% after clean branch isolation
```
