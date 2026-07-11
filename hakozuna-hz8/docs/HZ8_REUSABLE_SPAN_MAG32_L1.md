# HZ8 Reusable Span Magazine Mag32 L1

## Motivation

Post-Mag16 diagnostic counters show that residual small-span commits track
empty magazine pops while owner-list scan steps remain zero. Full-preserve is
frequent, so Mag32 tests one bounded inventory-capacity boundary without
changing replacement policy or remote authority.

The counters are diagnostic-build only. Production speed lanes contain no
counter or atomic update for this experiment.

## Windows attribution

```text
balanced: pop 21,105 / hit 8,625 / reject 435 / commit 12,552
wide_ws:  pop 8,595 / hit 1,924 / reject 61 / commit 6,735
```

## Windows gates

| Row | Mag16 | Mag32 | Change | Mag16 peak | Mag32 peak |
|---|---:|---:|---:|---:|---:|
| balanced | 55.47M | 76.52M | +38% | 790.57 MiB | 450.70 MiB |
| wide_ws | 59.39M | 84.93M | +43% | 399.18 MiB | 343.69 MiB |
| larger_sizes | 23.57M | 26.52M | +13% | 193.41 MiB | 159.01 MiB |

Fixed MT remote R5 was near-neutral: Mag16 130.710M / 18.17 MiB versus
Mag32 133.542M / 18.98 MiB. Effective remote ratios differed, so this is a
regression gate rather than a remote-throughput claim.

## Decision

```text
Windows candidate: GO
HZ8 default: keep Mag16
Mag32 promotion: HOLD pending Linux local/remote/RSS and safety gates
```
