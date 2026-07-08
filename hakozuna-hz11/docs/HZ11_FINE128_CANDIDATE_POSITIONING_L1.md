# HZ11Fine128CandidatePositioning-L1

Status: GO for positioning. No allocator policy change. No default promotion.

## Box

Document the final HZ11 lane positioning after the fine128 macro gate,
current-RSS semantics check, and remote/mixed final confirmation.

Boundary:

```text
documentation and lane naming only
no allocator policy changes
no default path change
no new performance claim
```

## Final Lane Positions

Recommended opt-in macro speed-lane candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

This lane combines:

```text
thread-exit current-span handling for larson thread churn
central-cap correctness for python_alloc and mstress
batch32 transfer granularity for sh6bench wall
selective fine128 size classes for sh6bench RSS/class packing
documented current-RSS gate semantics
RUNS=10 remote/mixed final confirmation
```

Remote/mixed microbench lane:

```text
libhz11_span_transfer.so
```

This remains the cleanest transfer-cache result for the remote/mixed matrix.
Fine128 is acceptable for the macro candidate, but it does not supersede
span-transfer on pure remote/mixed rows.

Research-only lanes:

```text
libhz11_span_transfer_thread_exit_cap_batch32.so
libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so
```

`batch32` remains an intermediate attribution/candidate step. It is useful for
showing the sh6bench wall lever, but it is not the final candidate.

Global fineclass remains a sh6bench RSS research lane only. It improves
sh6bench RSS but has material remote/mixed throughput/RSS/span-create tradeoffs.

Default allocator path:

```text
unchanged
```

Do not promote fine128 or span-transfer to the default path from these boxes.

## Claim Boundary

Allowed:

```text
fine128 is the recommended opt-in HZ11 macro speed-lane candidate backed by
macro-gate, current-RSS, and remote/mixed evidence.

span-transfer remains the clean HZ11 remote/mixed microbench speed lane.
```

Not allowed:

```text
HZ11 generally beats tcmalloc.
fine128 is the default allocator.
fine128 supersedes span-transfer on remote/mixed microbench rows.
global fineclass is a general candidate.
batch32 is the final candidate.
```

## Evidence Chain

```text
HZ11_TRANSFER_PROMOTION_MATRIX_L1:
  span-transfer GO for remote/mixed microbench lane

HZ11MacroSpeedLaneGateWithBatch32-L1:
  batch32 improves sh6bench wall but is not enough for macro promotion

HZ11SelectiveFineclassRange-L1:
  fine128 selected over fine256/global fineclass as the next selective candidate

HZ11MacroSpeedLaneGateFine128Reclassify-L1:
  fine128 macro gate reclassified under the documented current-RSS rule

HZ11Fine128RemoteMixedFinalConfirm-L1:
  fine128 confirmed as acceptable versus batch32 and tcmalloc, while
  span-transfer remains cleaner for remote/mixed-only rows
```

## Next Step

If the next work is a promotion box, it must be explicit:

```text
HZ11Fine128DefaultPromotionGate-L1
```

That box must define rollback, default-path target names, public claim wording,
and any additional macro/platform evidence required before changing defaults.
