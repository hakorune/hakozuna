# PostClaimCollectorAcceptance-L1

UTC: 2026-06-23T09:55:47Z

## Trigger

After `UsedCountFieldRemoval-L1`, a normal release stability run aborted in
`small_interleaved_remote90`.

Backtrace from an ASan/gdb repro:

```text
h8_collect_bulk_word
  -> old_pending & claimed != claimed
```

The failing line was the collector's pending-word clear check. This exposed the
previously documented post-claim rollback race:

```text
producer:
  validates ALLOCATED
  claims pending bit

collector:
  observes pending bit
  publishes slot_state FREE

producer:
  post-claim state recheck sees FREE
  attempts rollback

collector:
  later sees pending bit missing and aborted
```

## Fix

Post-claim state validation failure is not automatically INVALID.

If the span is `DRAINING` or `DRAINING_DIRTY`, the collector may already own the
claimed pending bit. In that case the producer returns `H8_PUBLISH_OK` and does
not rollback.

If the span is not being drained, rollback remains valid. If rollback finds the
bit already absent, the collector also accepted the publish and the producer
returns OK.

This keeps the pending bitmap as remote/remote duplicate-claim authority while
closing the collector/producer handoff race.

## Verification

Build/smoke:

```text
make bench-release bench smoke safety-stress preload-smoke
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so ./h8_preload_smoke
H8_SMOKE_REGULAR_ADOPTION=1 ./h8_smoke
```

All passed.

Logs:

- `bench_results/20260623T095547Z_post_claim_accept_local0_r10.log`
- `bench_results/20260623T095547Z_post_claim_accept_interleaved_r90_r10.log`
- `bench_results/20260623T095547Z_post_claim_accept_debug_interleaved_r90.log`

Important gates:

```text
release local0:
  throughput median=350.6M
  steady_work median=395.3M
  zero gates clean

release interleaved remote90:
  throughput median=44.38M
  p25=29.46M
  steady_work median=47.74M
  work_median=33.645ms
  tail_median=15.018ms
  quiescent_pending bitmap_nonzero=0 repair=0
  duplicate_claim=0
  slot_shadow used_mismatch=0

debug interleaved remote90:
  remote_stage validate_fail=0
  duplicate_claim=0
  derived_mismatch=0
  mirror_mismatch=0
  quiescent_pending bitmap_nonzero=0 repair=0
```

## Interpretation

Correctness is clean after the fix, but the interleaved remote90 stability gate
is not fully satisfied because p25 is below the current `median * 0.85`
requirement.

Do not reopen owner leases or intrusive remote-head based on this result. The
next evidence box should focus on work/tail stability attribution:

```text
InterleavedTailVarianceAudit-L1
```

The existing bench already reports `work_median`, `tail_median`,
`drain_calls`, `drain_empty`, `push_yields`, and `finish_yields`; the next step
is to explain why p25 drops, not to change the remote ownership protocol.
