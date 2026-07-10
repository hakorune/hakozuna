# HZ12 Windows ColdSpanOwner-L2 Thread Churn (2026-07-10)

Status: GO as bounded owner-lifetime evidence. Still opt-in.

## Lifetime Contract

ColdSpanOwner-L2 adds a generation to each advisory owner slot and registers a
Windows FLS destructor. Thread exit performs this order:

```text
flush every local class cache
drain the current generation inbox
detach the owner slot
release the thread-cache object
```

An old span token cannot publish into a reused slot generation. A stale or
inactive target downgrades to the existing ownerless returned sink. Owner
metadata remains advisory and is not a route-safety authority.

## Thread-Churn Smoke

The smoke created and joined 128 threads sequentially. Each thread allocated
and freed 512 mixed-size objects before exit.

```text
threads=128
attach_success=128
attach_reuse=127
attach_full=0
detach_success=128
stale_fallback=0
```

The 64-slot table therefore remained bounded and reusable without exhaustion.

## Performance Recheck

Local random_mixed R5 after generation/FLS integration:

| Profile | HZ12 core | ColdSpanOwner-L2 | Delta |
| --- | ---: | ---: | ---: |
| small | 154.408M | 151.847M | -1.7% |
| medium | 152.785M | 151.107M | -1.1% |
| mixed | 153.934M | 151.327M | -1.7% |

Cross-owner R3 after integration:

| Allocator | Median ops/s | Peak RSS median | Final RSS median |
| --- | ---: | ---: | ---: |
| HZ12 integrated flushroute | 21.881M | 9.69 MiB | 9.54 MiB |
| HZ12 ColdSpanOwner-L2 | 29.194M | **12.01 MiB** | **11.64 MiB** |
| HZ12 OwnerFast upper bound | 35.967M | 11.91 MiB | 11.75 MiB |
| tcmalloc | **38.089M** | 14.19 MiB | 13.60 MiB |

## Decision

The fixed owner-table exhaustion blocker is closed for the Windows opt-in
candidate. The next gate is concurrent retirement: a publisher must either
complete under the matching inbox generation or fall back ownerlessly while
the owner thread exits. After that, use a stable MT broad row before considering
profile promotion.

Evidence:

- `bench_results/windows_xowner_roundrobin_coldspanowner_generation_fastlocal_high_r3.md`
- `win/bench_hz12_owner_churn.c`
