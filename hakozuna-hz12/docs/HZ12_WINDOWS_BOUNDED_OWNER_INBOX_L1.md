# HZ12 Windows Bounded Owner Inbox L1

L1 is a behavior prototype over the standalone HZ12 span substrate. That core
began as a renamed HZ11-derived baseline, but HZ12 builds from its own source
tree and does not modify HZ11 source or its selected Windows row.

## Behavior

```text
consumer free:
  append to a consumer-local deferred batch
  no owner lookup per object until the batch flushes

consumer flush:
  read advisory span owner
  group objects by owner
  publish one bounded intrusive chain per owner inbox
  if the inbox is full or owner is unknown, free through the existing
  ownerless HZ12 path

producer drain:
  periodically detach its inbox under a mutex
  free the detached objects on the owner thread
  drain again after all consumers finish
```

The current L1 drain cadence is a benchmark-wrapper stand-in for a future
allocator slow path. It is not a claim that HZ12 has a production hot path.

## Safety Contract

```text
owner metadata is advisory only
unknown owner -> ownerless HZ12 free
inbox overflow -> ownerless HZ12 free
each deferred object is either routed once or freed once
producer lifetime extends through final consumer flush and inbox drain
```

## L1 Acceptance

```text
route_objects > 0
owner_drain_objects > 0
fallback_unknown = 0 under xowner smoke
fallback_overflow is bounded and explained by the configured cap
no hang during producer/consumer teardown
no HZ11 selected/default behavior change
```

L1 throughput is diagnostic evidence only. It must be compared with the
ownerless HZ11 xowner lane and tcmalloc before any claim about recovery.

## Initial Windows R3

The first 4 producer / 4 consumer xowner L1 run, with 5-second repetitions and
sizes 8..1024, produced a median 21.458M ops/s. All objects were cross-owner;
`fallback_unknown=0`. The owner inbox reached its 1024-object cap and used the
ownerless fallback for only 0.001-0.003% of deferred objects.

This is strong L1 mechanism evidence, not a selected allocator lane. The next
question is whether the cap/drain policy can remain bounded while preserving
this recovery, followed by dead-owner adoption and verified span state in L2.

## Same-Session Cross-Owner Compare R3

The dedicated comparison runner measured all three rows with 4 producers, 4
consumers, a 4096-object ring, sizes 8..1024, and 5-second R3:

| Allocator | Median ops/s | Reading |
| --- | ---: | --- |
| HZ11 ownerless | 9.138M | ownerless reference |
| HZ12 owner inbox L1 | 24.516M | 2.68x HZ11; bounded handoff evidence |
| tcmalloc | 28.553M | reference |

HZ12 L1 reaches about 86% of tcmalloc on this deliberately 100% cross-owner
pipeline. `fallback_unknown=0`; occasional overflow remains bounded by the
ownerless fallback. A 10-run teardown smoke passed without a hang or unknown
owner. This is GO for the mechanism, but not a public/default promotion.

## Cap Policy R3

The cap sweep used the same 4 producer / 4 consumer xowner pipeline, 5-second
repetitions, a 256-object drain interval, and R3 medians.

| Inbox cap | Median ops/s | Peak RSS MiB | Final RSS MiB | Overflow objects | Inbox max |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 512 | 24.217M | 18.32 | 18.18 | 281,076 | 512 |
| 1024 | 24.612M | 17.04 | 16.89 | 4,007 | 1,021 |
| 2048 | 24.680M | 18.01 | 17.86 | 256 | 1,829 |

`HZ12_INBOX_CAP=1024` remains the balanced L1 control. The 2048 cap adds only
0.3% median throughput while increasing peak RSS by about 1 MiB. The 512 cap
is slower and produces materially more ownerless fallback. All runs retained
zero unknown-owner fallback; overflow is a bounded correctness-preserving
fallback, not a dropped-object path.

This result narrows L2: investigate dead-owner adoption and verified
whole-span reclaimability, rather than widening the inbox or treating this
wrapper cadence as a production policy.
