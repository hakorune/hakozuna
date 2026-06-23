# ActiveHintStoreShape-L1

## Change

- Changed local free active-span hint update from conditional store to unconditional store:
  `ctx->active_spans[span->class_id] = span`.
- Ownership, slot-state authority, pending protocol, and remote protocol unchanged.

## Verification

- `make bench-release smoke safety-stress`: pass
- `./h8_smoke`: pass
- `./h8_safety_stress`: pass

## Focused Bench

local r5:

- median: 395.63M ops/s
- p25: 391.56M ops/s
- min: 304.39M ops/s
- steady median: 453.36M ops/s
- log: bench_results/20260623T123736Z_active_hint_store_local_r5.log

interleaved remote90 r5:

- median: 58.30M ops/s
- p25: 57.02M ops/s
- min: 53.74M ops/s
- steady median: 60.93M ops/s
- log: bench_results/20260623T123736Z_active_hint_store_interleaved_r5.log

## Decision

Adopt. Focused runs remain above the current bring-up gates and do not show a protocol concern.
