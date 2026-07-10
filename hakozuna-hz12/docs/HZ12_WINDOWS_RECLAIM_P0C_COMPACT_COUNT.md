# HZ12 Windows Reclaim P0-C Compact Per-Span Count

P0-C replaces the diagnostic per-slot bitmap with a compact per-span record:

```text
owner token
class
carved high-water mark
outstanding slot count
rejected transition count
```

Contiguous span bump updates are O(1). Returned-sink reuse and owner-local
flush remain exact, so every returned object is reacquired on refill and
returned again on flush. Automatic reclaim remains disabled.

Small random_mixed round-robin R5:

| lane | median ops/s | delta vs baseline |
| --- | ---: | ---: |
| baseline | 99.781M | - |
| bitmap ledger | 70.540M | -29.3% |
| compact count ledger | 87.415M | -12.4% |

The compact record removes substantial bitmap cost but still fails the -5%
hard gate. The remaining tax comes from maintaining exact continuous state on
frequent returned-sink refill/flush cycles. A smaller record does not solve the
fundamental update-frequency problem.

Decision: P0-C is NO-GO for production promotion. Do not add more continuous
ledger knobs. The next design is retirement-checkpoint reconstruction:

- normal malloc/free/refill performs no reclaim accounting;
- span owner/class/carve metadata remains advisory side-table state;
- a retirement or explicit pressure checkpoint drains the real inbox and
  flushes enrolled caches;
- the checkpoint reconstructs reclaim candidates from stable physical
  cache/sink/route state;
- bitmap and atomic shadows validate the reconstructed set only in diagnostic
  lanes.

This changes policy authority placement, not the route/fail-closed contract.
