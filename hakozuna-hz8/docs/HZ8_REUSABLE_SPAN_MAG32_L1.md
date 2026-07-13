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

The follow-up per-class diagnostic on balanced attributes empty pops as:

```text
class 4:      8
class 5:     29
class 6:    233
class 7: 12,187
other active classes: 0
```

Class 7 accounts for about 98% of empty pops. Its depth low-water reaches zero;
classes without demand retain the initialized low-water of 16. This supports a
class-specific phase-amplitude interpretation rather than owner scanning or a
generic replacement-policy failure.

## Design review disposition

```text
root cause:
  bounded inventory capacity / phase amplitude

replacement policy:
  CLOSED / NO-GO
  reusable empty spans are fungible; FIFO/LRU/ring does not add information

full-preserve count:
  event evidence only, not the capacity proof

completed experiment:
  Linux Mag16/Mag32 A/A-calibrated R5
  effective remote percentage was matched before interpreting throughput

future bridge:
  magazine tail decommit may become the HZ8-native receiver of the HZ12
  bounded reclaim contract, but only after Mag32 promotion is decided
```

Do not test Mag64 unless Mag32 still shows residual class-attributed empty pops
after passing the cross-platform gate.

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
Windows larger/local opt-in: GO
HZ8 default: keep Mag16
Mag32 promotion: HOLD after Linux local/remote/RSS and safety gates
```

## Linux Gate

GCC and Clang smoke/safety pass for Mag16 and Mag32. Local paired repeat-5 is
strongly positive for 16..2048 and 16..4096, with large peak-RSS reductions,
but 16..256 regresses 9.4%. A deterministic remote repeat-5 with identical
90.07925% effective remote work measures -2.1%, near-equal post RSS, and a
small peak-RSS increase.

```text
Linux correctness: GO
larger local ranges: GO
global Mag32 promotion: HOLD
HZ8 default: keep Mag16
```

Full results: [Linux Mag32 gate](../../docs/benchmarks/linux/HZ8_REUSABLE_SPAN_MAG32_20260711.md).
The class-7 detached-sidecar follow-up is NO-GO and Mag64 is closed untested.
Capacity tuning is closed; Mag32 remains an explicit larger/local opt-in.
