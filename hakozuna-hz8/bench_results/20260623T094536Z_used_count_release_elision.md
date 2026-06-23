# UsedCountReleaseElision-L1

UTC: 2026-06-23T09:45:36Z

## Scope

Release builds no longer maintain `local_used_count` on the hot path.

The field remains present for now, but its runtime role changed:

- release alloc/free/remote-collect: no used-count load/store
- debug alloc/free/remote-collect: keep atomic shadow and mirror
- cold release readers: derive allocated count from `slot_state`
- cold debug readers: compare atomic shadow with slot-state derivation

This avoids the rejected hybrid model where separate hot and cold authorities
can disagree. The release authority for lifecycle emptiness is the quiescent
slot-state scan.

## Code Shape

- `h8_owner_used_add()` is debug-only except for attribution counters.
- `h8_owner_used_sub()` is debug-only except for attribution counters.
- remote collect `h8_used_count_sub()` is debug-only.
- `h8_used_count_load_cold_acquire()` returns:
  - debug: atomic shadow
  - release: `h8_slot_allocated_count_quiescent()`
- `h8_used_count_init()` initializes the atomic shadow only in debug builds.

## Verification

Build:

```text
make bench-release bench smoke safety-stress preload-smoke
```

Smoke:

```text
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so ./h8_preload_smoke
H8_SMOKE_REGULAR_ADOPTION=1 ./h8_smoke
```

All passed.

## Debug Proof

Logs:

- `bench_results/20260623T094536Z_used_count_release_elision_debug_local0.log`
- `bench_results/20260623T094536Z_used_count_release_elision_debug_interleaved_r90.log`

Important gates:

```text
debug local0:
  used_count_detail load_alloc=480000 store_alloc=480000
  used_count_detail load_free=480000 store_free=480000
  used_count_cold derived_mismatch=0 derived_scan=256
  slot_shadow used_mismatch=0
  quiescent_pending bitmap_nonzero=0 repair=0

debug interleaved remote90:
  used_count_detail load_alloc=480000 store_alloc=480000
  used_count_detail load_free=47760 store_free=47760
  used_count_cold derived_mismatch=0 derived_scan=25619
  slot_shadow used_mismatch=0
  remote_stage validate_fail=0
  queue_contention duplicate_claim=0
  quiescent_pending bitmap_nonzero=0 repair=0
```

Debug still updates the atomic shadow intentionally.

## Release Proof

Logs:

- `bench_results/20260623T094536Z_used_count_release_elision_release_local0_r3.log`
- `bench_results/20260623T094536Z_used_count_release_elision_release_interleaved_r90_r3.log`

Important gates:

```text
release local0:
  used_count_detail load_alloc=0 store_alloc=0
  used_count_detail load_free=0 store_free=0
  used_count_cold derived_mismatch=0 derived_scan=0
  slot_shadow used_mismatch=0
  steady_work throughput_median=252441981.733

release interleaved remote90:
  used_count_detail load_alloc=0 store_alloc=0
  used_count_detail load_free=0 store_free=0
  used_count_cold derived_mismatch=0 derived_scan=0
  slot_shadow used_mismatch=0
  steady_work throughput_median=32618153.144
```

The release counters prove used-count hot updates are eliminated. The local0
end-to-end median was noisy in this short run, so the saved data should be
used as a correctness and code-shape proof rather than a final performance
gate.

## Interpretation

`UsedCountReleaseElision-L1` is implemented as a behavior-preserving authority
shift:

- VALID/INVALID remains `slot_state + pending bitmap`.
- lifecycle empty/nonempty in release is derived from quiescent `slot_state`.
- debug keeps the old atomic count as a shadow and verifies it.

Next useful box:

```text
UsedCountFieldRemoval-L1
```

Only attempt this after another clean pass, because removing the field changes
layout and will require replacing remaining debug reports with shadow-only
wording.
