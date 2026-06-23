# HZ8 ActiveHintFullnessShape-L1

Date: 2026-06-23T02:43:22Z

## Result

The active-hint fullness check no longer reads `used_count`.

Instead, active hint exhaustion is inferred from owner-local allocation state:

```text
local_free_head == NONE
and
local_bump_index >= slot_count
```

This keeps `used_count` as lifecycle / accounting authority, but removes the
main non-locked active-hint reader identified by `UsedCountColdReaderProof-L1`.

## Raw Logs

```text
bench_results/20260623T024322Z_active_hint_fullness_debug_local0.log
bench_results/20260623T024322Z_active_hint_fullness_debug_interleaved_r90.log
bench_results/20260623T024322Z_active_hint_fullness_release_local0_r3.log
bench_results/20260623T024322Z_active_hint_fullness_release_interleaved_r90_r3.log
```

## Debug Proof

```text
debug local0:
  used_count_cold active_hint = 0
  owner_scan_locked = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

debug interleaved remote90:
  used_count_cold active_hint = 0
  owner_scan_locked remains nonzero and lock-protected
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean
```

## Release Smoke

```text
local0 RUNS=3:
  noisy run, not used for promotion
  zero gates clean

interleaved remote90 RUNS=3:
  median ~= 37.2M ops/s
  steady ~= 42.0M ops/s
  zero gates clean
```

## Interpretation

The main non-locked cold `used_count` reader is gone.  Remaining non-owner-hot
readers are either lock-protected or cold/quiescent proof paths:

```text
owner_scan_locked:
  owner->owned_lock protected

adoption_locked:
  orphan owner lock / adoption path

owner_exit:
  owner exit after admission closure

verify_quiescent:
  debug / quiescent verifier
```

This moves scalar cutover closer, but the next safe step is still not a blind
plain-field conversion.  First prove or enforce that the lock/cold readers never
race with owner writes in release-relevant paths.
