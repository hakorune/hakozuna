# MediumActiveEmptyLiveRetention-L1

## Change

Same-owner free of the current TLS active medium run keeps an empty payload in
`LIVE` state instead of entering the resident empty budget path. Owner exit is
the hard drain point and decommits empty live payloads.

## Results

Debug medium local0 (`4097..65536`, r0, R5):

```text
throughput median: 12.52M
steady median:     13.06M
empty/retain/reactivate: 0 / 0 / 0
exit_drain:        320
madvise:           320
zero gates:        clean
```

Release quick checks:

```text
medium local0 median:       106.49M
medium local0 steady:       113.80M
medium r50 median:           29.38M
medium r50 steady:           32.26M
small remote90 median:       54.59M
small remote90 steady:       55.99M
```

Debug medium r50 (`4097..65536`, r50, R5):

```text
throughput median:  8.55M
steady median:      9.04M
invalid_owned:      0
writer overlap:     0
owner/list mismatch:0
empty_with_pending: 0
lease shadow gates: 0
```

## Decision

GO. The active local path no longer pays per-operation empty resident
reserve/release/reactivate churn. RSS remains bounded by owner-exit drain.
