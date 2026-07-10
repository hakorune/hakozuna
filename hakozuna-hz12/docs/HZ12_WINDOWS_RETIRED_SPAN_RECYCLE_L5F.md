# HZ12 Windows Retired Span Recycle L5-F

## Goal

L5-F closes the bounded retired-span lifecycle proof. A span that passed
L5-C detach, L5-D decommit, and depot admission must support real slot carve,
payload access, free, a second ordered detach, and a second decommit.

## Diagnostic Flow

```text
decommitted owner span
-> bounded depot put/take
-> recommit, accounting reset, route attach, current install
-> direct diagnostic carve of every class slot
-> touch one byte in every slot
-> normal hz12_free for every slot
-> existing ordered detach, route last
-> existing Windows decommit
-> return the decommitted span to the bounded depot
```

Direct carve is isolated in `hz12_reclaim_carve_diag.c`. It is linked only by
the diagnostic runner and does not change normal refill order or production
code layout.

## Acceptance

For a fixed 64-span budget:

- initial put/take, exercise, second detach/decommit, and final put are 64/64;
- every slot in every span is carved and touched once;
- accounting reaches a clean whole-span candidate before the second detach;
- all addresses and owner generations remain exact;
- exactly 4.00 MiB is decommitted again;
- the final depot contains exactly 64 spans;
- failures remain zero.

## Boundary

This proves lifecycle completeness, not an automatic reclaim policy. It adds no
hot-path owner lookup, no periodic scanner, and no production depot admission.

## Windows Result

The fixed 64-span lane passed repeat-3:

```text
initial depot put:       64/64
take and recommit:       64/64
spans exercised:         64/64
second ordered detach:   64/64
second decommit:         64/64
final depot put:         64/64
final depot count:       64
slots touched per run:   9,984..10,048
bytes redecommitted:     4.00 MiB
address mismatch:        0
owner mismatch:          0
failures:                0
```

The varying slot count reflects the workload's class mix; every selected span
was exercised to its own full class capacity. This completes the bounded
Windows reclaim lifecycle proof from retired-owner attribution through reuse
and a second reclaim cycle.

## Decision

GO as lifecycle-complete mechanism evidence. Freeze this diagnostic chain.
Automatic candidate selection, pacing, and depot admission remain a separate
policy gate and must not be inferred from this result.
