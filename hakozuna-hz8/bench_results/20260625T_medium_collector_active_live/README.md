# MediumCollectorActiveEmptyLive-L1

## Change

When owner-thread medium collection empties the current TLS active run and no
pending bits remain, the collector keeps the payload `LIVE` instead of entering
the resident empty budget path. Owner exit remains the hard drain point.

## Debug Results

Medium r50 (`4097..65536`, R5):

```text
throughput median: 8.77M
steady median:     9.35M
collect_active_would_keep: 177937
empty/retain/reactivate:   236460 / 236460 / 235274
empty_with_pending:        0
empty_live_not_active:     0
owner_exit_active_live:    0
active_live_peak:          4063232
```

## Release Quick Results

```text
medium r50 median:       33.38M
medium r50 steady:       35.66M
medium local0 median:   104.61M
main remote90 median:    25.01M
small remote90 median:   52.62M
```

## Decision

GO. Collector-side active empty live retention reduces medium empty churn
without breaking zero gates or small frozen rows in quick checks.
