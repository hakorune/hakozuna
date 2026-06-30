# HZ8 RemotePressureCollect Attribution L2

Status:

```text
diagnostic attribution box / no behavior change
tracks source of remote-pressure collect
```

## Why This Box Exists

`SmallRemotePressureCollect-L1` moved the weak rows:

```text
small_interleaved_remote90:
  0.91M -> 1.06M ops/s
  peak RSS 868.28 -> 455.47 MiB

main_interleaved_r90:
  2.68M -> 4.54M ops/s
  peak RSS 144.06 -> 113.75 MiB

medium_interleaved_r50:
  2.93M -> 8.19M ops/s
  peak RSS 168.01 -> 122.10 MiB
```

That means the question is no longer "does collect help at all?"
The new question is:

```text
which trigger source is doing the work?
```

## Source Attribution

The L2 implementation tags the collect source as:

```text
ACTIVE_HIT_FULL
ACTIVE_MISS
OWNER_EXIT
```

The first two are the only current small-path sources.
The third is reserved for later attribution around owner shutdown drain.

## Counters Added

```text
remote_pressure_collect_source_active_hit_full_count
remote_pressure_collect_source_active_miss_count
remote_pressure_collect_source_owner_exit_count

small_remote_pressure_collect_call_count
small_remote_pressure_collect_budget_count
small_remote_pressure_collect_span_count
small_remote_pressure_collect_pending_before_count
small_remote_pressure_collect_pending_after_count
small_active_full_pending_nonzero_count
small_active_full_collect_helped_count
small_active_full_collect_no_help_count
```

## Scope

Allowed:

```text
diagnostic-only attribution
source-tagged collect calls
bench report visibility
current_task ledger visibility
```

Preserved:

```text
remote publish protocol
slot_state authority
pending bitmap authority
qstate protocol
owner lifecycle
local active-hit success path
```

## What We Want To Learn

```text
ACTIVE_HIT_FULL dominates:
  the row is mostly paying for "span got full while pending existed"

ACTIVE_MISS dominates:
  the row is mostly paying for "miss -> collect -> retry"

OWNER_EXIT becomes non-zero later:
  shutdown drain is contributing to the same family
```

## Acceptance

```text
local rows:
  source attribution remains low or zero

small/main/medium interleaved rows:
  one of active_full or active_miss should explain the gain
  helped count should stay non-zero on the weak rows
  no safety regressions
```

## Next Box

If this attribution shows that `ACTIVE_MISS` is the main driver, the next
behavior box should be:

```text
MainRemoteCollectBudget-L1
```

If `ACTIVE_HIT_FULL` dominates instead, keep the same mechanism but tighten the
budget trigger and measure `MediumRemoteCollectBudget-L1` separately.

Current L2 diagnostic showed `ACTIVE_HIT_FULL` dominance on the small row, so
the next narrow behavior lane is:

```text
docs/HZ8_ACTIVE_FULL_REMOTE_COLLECT_BUDGET_L1.md
```

That budget-thickening lane did not improve the small row. A follow-up bucket
probe showed that most active-full pending counts are small, so the current
small-side candidate is:

```text
docs/HZ8_ACTIVE_FULL_DEFER8_L1.md
```
