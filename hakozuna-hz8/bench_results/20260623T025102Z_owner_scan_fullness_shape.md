# HZ8 OwnerScanFullnessShape-L1

Date: 2026-06-23T02:51:02Z

## Result

The owner-list scan fullness check no longer reads `used_count`.

`h8_find_active_span()` now uses the same owner-local allocation-state test as
active hints:

```text
local_free_head == NONE
and
local_bump_index >= slot_count
```

This removes `used_count` from the steady allocation path's cold-reader side.

## Raw Logs

```text
bench_results/20260623T025102Z_owner_scan_fullness_debug_local0.log
bench_results/20260623T025102Z_owner_scan_fullness_debug_interleaved_r90.log
bench_results/20260623T025102Z_owner_scan_fullness_release_local0_r3.log
bench_results/20260623T025102Z_owner_scan_fullness_release_interleaved_r90_r3.log
```

## Debug Proof

```text
debug local0:
  used_count_cold active_hint = 0
  used_count_cold owner_scan_locked = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

debug interleaved remote90:
  used_count_cold active_hint = 0
  used_count_cold owner_scan_locked = 0
  adoption_locked = 0 in this run
  owner_exit / verify_quiescent remain cold proof readers
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean
```

## Release Smoke

```text
local0 RUNS=3:
  median ~= 176.1M ops/s
  steady ~= 244.1M ops/s
  zero gates clean

interleaved remote90 RUNS=3:
  median ~= 41.4M ops/s
  steady ~= 44.3M ops/s
  zero gates clean
```

## Interpretation

The remaining `used_count` cold readers are no longer steady allocation-path
fullness decisions:

```text
adoption_locked:
  adoption side lane

owner_exit:
  owner lifecycle teardown

verify_quiescent:
  debug/quiescent proof
```

This is the cleanest point so far to consider a narrow scalar cutover, but only
if adoption/owner-exit/quiescent readers are either kept atomic or moved behind a
clear cold accessor contract.
