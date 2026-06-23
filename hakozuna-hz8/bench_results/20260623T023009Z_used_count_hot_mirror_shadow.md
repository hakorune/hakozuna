# HZ8 UsedCountHotMirrorShadow-L1

Date: 2026-06-23T02:30:09Z

## Result

`used_count` remains atomic authority.  A debug-only owner mirror was added to
prove whether owner-side accounting can be kept coherent without changing the
release runtime contract.

## Implementation

```text
release:
  no mirror field
  h8_used_count_mirror_* helpers compile to no-op

DEBUG_STATS:
  H8Span.local_hot.local_used_mirror
  local_used_mirror_mismatch
  local_used_mirror_underflow
```

Mirror updates are attached to the same owner mutation points as atomic
`local_used_count` updates:

```text
local allocation:
  h8_owner_used_add()

same-owner local free:
  h8_owner_used_sub()

owner remote collect:
  h8_used_count_sub()

span init:
  h8_used_count_init()
```

Quiescent verification compares mirror against atomic authority in the slot-state
verifier.

## Checks

Build / smoke:

```text
make smoke safety-stress preload-smoke bench-release
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so ./h8_preload_smoke
```

Debug short runs:

```text
bench_results/20260623T023009Z_used_count_mirror_debug_local0.log
bench_results/20260623T023009Z_used_count_mirror_debug_interleaved_r90.log
```

Observed:

```text
debug local0:
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

debug interleaved remote90:
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  remote validate_fail = 0
  duplicate claim = 0
  quiescent pending clean
```

Release short logs:

```text
bench_results/20260623T023009Z_used_count_mirror_release_local0_r3.log
bench_results/20260623T023009Z_used_count_mirror_release_interleaved_r90_r3.log
```

Observed:

```text
local0 RUNS=3:
  median ~= 150.8M ops/s
  steady ~= 183.7M ops/s
  min outlier ~= 64.9M ops/s
  zero gates clean

interleaved remote90 RUNS=3:
  median ~= 40.6M ops/s
  steady ~= 43.0M ops/s
  zero gates clean
```

The release short local run is noisy and is not used as a promotion decision.

## Interpretation

The mirror evidence is clean for the covered debug workloads, but this still does
not justify plain scalar authority.  Cold readers remain in active-span search,
orphan adoption, owner exit, and slot-state verification.

Next scalar-related step should be a reader-quiescence proof, not a storage
cutover.
