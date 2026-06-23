# SlotAuthorityMonomorphic-L1

## Change

- Removed dead local slot-authority fallback branches from malloc/free leaf.
- Local allocation now always decodes free-list next from tagged slot state.
- Local free now always validates and publishes tagged slot state.
- Release local path no longer touches live bitmap; debug builds keep live bitmap as shadow.
- Removed deprecated slot-state authority global state and env parsing.
- Route classification now uses slot-state authority directly.

## Verification

- make bench-release preload smoke safety-stress preload-smoke: pass
- ./h8_smoke: pass
- ./h8_safety_stress: pass
- LD_PRELOAD=libhakozuna_hz8_preload.so /bin/true: pass

## Focused Bench

local r5:

- median: 386.79M ops/s
- p25: 274.94M ops/s
- min: 226.63M ops/s
- steady median: 459.18M ops/s
- log: bench_results/20260623T104207Z_slot_authority_mono_local_r5.log

interleaved remote90 r5:

- median: 58.64M ops/s
- p25: 58.59M ops/s
- min: 55.62M ops/s
- steady median: 60.52M ops/s
- log: bench_results/20260623T104207Z_slot_authority_mono_interleaved_r5.log

## Notes

Remote protocol intentionally unchanged. The remaining remote-side slot-authority fallback branches are a separate monomorphization box.
