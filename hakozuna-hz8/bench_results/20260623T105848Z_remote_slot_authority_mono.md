# RemoteSlotAuthorityMonomorphic-L1

## Change

- Removed remaining remote collect/publish slot-authority fallback branches.
- Remote collect now always validates tagged slot state and publishes FREE state before clearing pending bits.
- Remote publish now always validates tagged slot state before and after pending claim.
- Span metadata allocation now always allocates slot_state.
- Removed unused next_free metadata field and the obsolete authority helper.
- Pending bitmap, pending_word_mask, qstate dirty handoff, and owner/span lease protocol were intentionally unchanged.
- Debug builds keep live_bits as shadow only.

## Verification

- make bench-release preload smoke safety-stress preload-smoke: pass
- ./h8_smoke: pass
- ./h8_safety_stress: pass
- LD_PRELOAD=libhakozuna_hz8_preload.so /bin/true: pass

## Focused Bench

local r5:

- median: 397.05M ops/s
- p25: 380.19M ops/s
- min: 317.83M ops/s
- steady median: 452.94M ops/s
- log: bench_results/20260623T105848Z_remote_slot_authority_mono_local_r5.log

interleaved remote90 r5:

- median: 58.63M ops/s
- p25: 56.96M ops/s
- min: 42.85M ops/s
- steady median: 61.40M ops/s
- log: bench_results/20260623T105848Z_remote_slot_authority_mono_interleaved_r5.log

## Notes

The low interleaved min remains above the bring-up threshold and is consistent with prior work-phase variance. This box does not reopen remote protocol design.
