# UsedCountFieldRemoval-L1

UTC: 2026-06-23T09:49:41Z

## Scope

Remove the release `local_hot.local_used_count` field after
`UsedCountReleaseElision-L1` proved release hot paths no longer update or read
it.

The remaining debug count is the existing `local_used_mirror` shadow. Release
cold paths continue to derive allocated count from `slot_state`.

## Code Shape

- `H8Span.local_hot.local_used_count` removed.
- `h8_used_count_load_owner_relaxed()` removed.
- `h8_used_count_store_owner_relaxed()` removed.
- debug alloc/free/remote-collect update only `local_used_mirror`.
- debug cold readers read `local_used_mirror`.
- release cold readers scan slot-state allocation tags.

The `used_count_detail` counter names are retained for bench output
compatibility, but in debug they now count mirror-shadow operations rather than
atomic `local_used_count` field accesses.

## Verification

Build and smoke:

```text
make bench-release bench smoke safety-stress preload-smoke
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so ./h8_preload_smoke
H8_SMOKE_REGULAR_ADOPTION=1 ./h8_smoke
```

All passed.

Line budget:

```text
src/h8_internal.h = 795 lines
```

## Debug Proof

Logs:

- `bench_results/20260623T094941Z_used_count_field_removal_debug_local0.log`
- `bench_results/20260623T094941Z_used_count_field_removal_debug_interleaved_r90.log`

Important gates:

```text
debug local0:
  used_count_cold derived_mismatch=0 derived_scan=256
  used_count_detail mirror_mismatch=0 mirror_underflow=0
  slot_shadow used_mismatch=0
  quiescent_pending bitmap_nonzero=0 repair=0

debug interleaved remote90:
  used_count_cold derived_mismatch=0 derived_scan=27550
  used_count_detail mirror_mismatch=0 mirror_underflow=0
  slot_shadow used_mismatch=0
  remote_stage validate_fail=0
  queue_contention duplicate_claim=0
  quiescent_pending bitmap_nonzero=0 repair=0
```

## Release Proof

Logs:

- `bench_results/20260623T094941Z_used_count_field_removal_release_local0_r3.log`
- `bench_results/20260623T094941Z_used_count_field_removal_release_interleaved_r90_r3.log`

Important gates:

```text
release local0:
  used_count_detail load_alloc=0 store_alloc=0
  used_count_detail load_free=0 store_free=0
  used_count_cold derived_mismatch=0 derived_scan=0
  steady_work throughput_median=219081506.262

release interleaved remote90:
  used_count_detail load_alloc=0 store_alloc=0
  used_count_detail load_free=0 store_free=0
  used_count_cold derived_mismatch=0 derived_scan=0
  steady_work throughput_median=34784582.343
```

The release data proves the used-count field is no longer present or touched.
This short run is not a final performance stability gate.

## Interpretation

`used_count` is now fully removed from release span layout. The lifecycle count
authority is:

```text
release:
  slot_state quiescent derivation

debug:
  local_used_mirror shadow + slot_state derivation checks
```

Do not reintroduce a plain hot used-count authority without a new design
decision; that path was explicitly held because of adoption/collect reader
synchronization.
