# MediumCollectorActiveEmptyLiveShadow-L1

## Change

Behavior is unchanged. The medium owner collector now receives `H8ThreadCtx*`
for steady-state maintenance paths and records whether an empty collected run
could have remained `LIVE` when it is the current TLS active run.

The box also adds active-empty-live byte shadow accounting and demotes the old
active run when the TLS active hint is replaced.

## Results

Debug medium r50 (`4097..65536`, R5):

```text
throughput median: 7.67M
steady median:     8.46M
collect_active_would_keep: 168347
ctx_owner_mismatch:        0
active_not_owned:          0
empty_live_not_active:     0
owner_exit_active_live:    0
active_live_peak:          2818048
empty/retain/reactivate:   411272 / 411272 / 409992
```

Debug medium local0 (`4097..65536`, R5):

```text
throughput median: 12.31M
steady median:     12.68M
active_live_peak:  5242880
owner_exit_active_live: 0
empty/retain/reactivate: 0 / 0 / 0
```

## Decision

GO for `MediumCollectorActiveEmptyLive-L1`. Shadow shows substantial collector
opportunity and the active-live boundary is clean.
