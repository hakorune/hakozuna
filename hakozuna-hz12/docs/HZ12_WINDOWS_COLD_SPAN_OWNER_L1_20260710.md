# HZ12 Windows ColdSpanOwner-L1 (2026-07-10)

Status: GO as an opt-in integration candidate. Not a public/default promotion.

## Design

ColdSpanOwner-L1 keeps normal free owner-blind and removes owner recording from
each successful allocation. It assigns advisory ownership once when a 64 KiB
span becomes a current source. Owner inbox drain is also moved from paced
refill polling to the cold current-span replacement boundary.

Ownership remains advisory. Route validity and fail-closed free behavior do
not depend on the owner table.

## Local Random Mixed R5

| Profile | HZ12 core | ColdSpanOwner | Delta | Peak RSS |
| --- | ---: | ---: | ---: | ---: |
| small | 155.766M | 151.803M | -2.5% | 4.55 MiB |
| medium | 152.118M | 150.036M | -1.4% | 5.74 MiB |
| mixed | 153.545M | 150.053M | -2.3% | 5.85 MiB |

All three rows remain inside the 3% local acceptance gate.

## Cross-Owner Pipeline R5

Conditions: Windows, affinity `0xFF`, High priority, 4 producers, 4 consumers,
sizes 8..1024, 100% cross-owner frees, rotated order, five-second runs.

| Allocator | Median ops/s | Peak RSS median | Final RSS median |
| --- | ---: | ---: | ---: |
| HZ12 integrated flushroute | 26.169M | 14.20 MiB | 14.04 MiB |
| HZ12 ColdSpanOwner | 29.064M | **11.32 MiB** | **11.16 MiB** |
| HZ12 OwnerFast upper bound | 35.096M | 11.88 MiB | 11.72 MiB |
| tcmalloc | **36.911M** | 15.13 MiB | 14.54 MiB |

ColdSpanOwner is 11.1% faster than the first integrated flushroute and uses
25.2% less peak RSS than tcmalloc in this compact pipeline. It reaches 78.7%
of tcmalloc throughput. This is a useful low-RSS Pareto point, not a claim of
universal throughput parity.

## Decision

Keep ColdSpanOwner-L1 as the selected HZ12 integration candidate. Do not
promote it until owner lifetime is bounded: the current process-lifetime owner
IDs are not reused, and thread churn beyond the fixed owner table falls back to
ownerless behavior. The next gate is a thread-churn/retirement smoke followed
by generation-safe owner-slot reuse outside normal malloc/free.

Evidence:

- `bench_results/windows_xowner_roundrobin_coldspanowner_high_r5.md`
- local R5 generated under `bench_results/windows_coldspanowner_local_r5/`
