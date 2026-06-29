# HZ8 SmallRemotePressureCollect L1

Status:

```text
implemented in working tree / next weak-row candidate
target: small_interleaved_remote90 first
secondary target: main_interleaved_r90
```

## Why This Box Exists

The latest same-run matrix shows that HZ8-v1.1 is already useful on local rows
because it keeps RSS low while retaining decent throughput.

```text
guard_local0:
  hz8 264.15M ops/s, RSS 6.14 MiB

fixed24_local0:
  hz8 323.23M ops/s, RSS 6.05 MiB

fixed48_local0:
  hz8 340.68M ops/s, RSS 6.10 MiB
```

The weak rows are remote/interleaved:

```text
small_interleaved_remote90:
  hz8 0.91M ops/s, RSS 868.28 MiB

main_interleaved_r90:
  hz8 2.68M ops/s, RSS 144.06 MiB

medium_interleaved_r50:
  hz8 2.93M ops/s, RSS 168.01 MiB
```

Do not start with HZ9 local magazine work for this problem. The first HZ8
weak-row box should stay inside the existing HZ8 small remote protocol.

## Hypothesis

`small_interleaved_remote90` likely spends too much time with remote-freed
small spans waiting for the owner to collect them.

The first hypothesis is:

```text
remote publish is functioning
pending spans are present
owner-side collect is too thin or too late for remote90 pressure
active span refill then misses useful pending work
```

Therefore L1 changes only owner-side collect timing and budget at active
miss/full points.

## Scope

Allowed:

```text
small only
owner-side collect budget
active full / active miss / pending-exists paths
diagnostic counters outside local active-hit success
```

Preserved:

```text
slot_state authority
pending bitmap authority
pending_word_mask authority
qstate protocol
remote publish protocol
owner lifecycle
medium behavior
residency policy
local active-hit success path
```

## Proposed Behavior

Add a remote-pressure collect budget for owner active-miss situations.

```text
pending == 0:
  collect nothing

pending <= 2:
  collect pending

pending <= 8:
  collect 4

pending <= 32:
  collect 8

pending <= 128:
  collect 16

otherwise:
  collect 32
```

Use it only when:

```text
active small span allocation failed
or active span search is about to run
and owner pending_span_count is nonzero
```

The point is to thicken remote-pressure recovery without adding work to the
ordinary local active-hit success path.

## Minimum Counters

Diagnostic-only counters:

```text
small_remote_pressure_collect_call
small_remote_pressure_collect_budget
small_remote_pressure_collect_spans
small_remote_pressure_collect_pending_before
small_remote_pressure_collect_pending_after
small_active_full_pending_nonzero
small_active_full_collect_helped
small_active_full_collect_no_help
```

The key question:

```text
after active full + pending nonzero,
did collect return a usable span?
```

If the answer is yes, this box has a clear mechanism. If not, the next box is
remote publish cost attribution.

## Acceptance

Experimental accept:

```text
small_interleaved_remote90:
  +5% or better

guard_local0:
  regression <= 2%

main_local0:
  regression <= 2%

safety:
  invalid fallback = 0
  duplicate claim = 0
  quiescent pending bitmap = 0
  timeout / abort = 0
```

Strong accept:

```text
small_interleaved_remote90:
  +15% or better

main_interleaved_r90:
  no regression or improvement

RSS:
  no unexplained increase
```

## No-Go

```text
local active-hit success path grows materially
guard_local0 regresses more than 2%
main_local0 regresses more than 2%
remote publish protocol changes
pending/qstate authority changes
RSS grows without throughput recovery
safety zero gates fail
```

## If L1 Fails

If owner-side collect budget does not move `small_interleaved_remote90`, the
next box should be:

```text
SmallRemotePublishCostAttribution-L1
```

That box should measure:

```text
owner admission / lookup
slot validation
pending claim
pending_word_mask update
qstate notify
queue push / retry
```

Do not jump directly to cross-owner handoff or residency unless the attribution
points there.
