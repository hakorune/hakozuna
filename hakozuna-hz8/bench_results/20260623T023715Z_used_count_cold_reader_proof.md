# HZ8 UsedCountColdReaderProof-L1

Date: 2026-06-23T02:37:15Z

## Result

`used_count` remains atomic authority.  This box classifies every non-owner-hot
`used_count` load so the remaining scalar-cutover blockers are explicit.

## Reader Classes

```text
active_hint:
  TLS active span fullness check before slow path

owner_scan_locked:
  owner owned_by_class scan under owner->owned_lock

adoption_locked:
  permanent orphan list scan under orphan->owned_lock / adoption path

owner_exit:
  owner exit span walk after owner admission close / publisher drain

verify_quiescent:
  slot-state quiescent verifier
```

Raw logs:

```text
bench_results/20260623T023715Z_used_count_cold_reader_debug_local0.log
bench_results/20260623T023715Z_used_count_cold_reader_debug_interleaved_r90.log
bench_results/20260623T023715Z_used_count_cold_reader_release_local0_r3.log
bench_results/20260623T023715Z_used_count_cold_reader_release_interleaved_r90_r3.log
```

## Debug Observations

```text
debug local0:
  active_hint = 0
  owner_scan_locked = 0
  adoption_locked = 0
  owner_exit = 128
  verify_quiescent = 128
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0

debug interleaved remote90:
  active_hint and owner_scan_locked are nonzero
  adoption_locked = 0
  owner_exit is small relative to hot path
  verify_quiescent tracks quiescent proof work
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
```

## Release Smoke

```text
local0 RUNS=3:
  median ~= 180.5M ops/s
  steady ~= 219.9M ops/s
  zero gates clean

interleaved remote90 RUNS=3:
  median ~= 43.1M ops/s
  steady ~= 46.0M ops/s
  zero gates clean
```

## Interpretation

The cold-reader split is now visible.  The two readers still relevant to the
steady allocation path are:

```text
active_hint
owner_scan_locked
```

The owner scan reader is lock-protected.  The active-hint fullness check remains
the main non-locked reader that blocks a plain scalar authority cutover.

Next safe direction:

```text
ActiveHintFullnessShape-L1
```

Goal:

```text
avoid using used_count as the active-hit fullness authority, or prove a stable
owner-only scalar read for active hints before removing atomic used_count.
```
