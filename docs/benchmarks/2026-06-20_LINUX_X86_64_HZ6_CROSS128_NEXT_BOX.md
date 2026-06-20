# Linux x86_64 HZ6 Cross128 Next Box, 2026-06-20

Follow-up direction after `Cross128RouteCostAudit-L1`.

## Current Evidence

- `cross128_r90` is a fixed 128B Toy-size remote-heavy row.
- HZ6 is far behind HZ4/tcmalloc/HZ3 on this row in the allocator compare.
- Diagnostic counters show the row is dominated by remote transfer pressure and
  transfer-cache scan/full cost, not source allocation.
- `toy_source_alloc` is about 1K, while `returned_backpressure` is about 125K.
- `small_class_shard` reduces `transfer_pop` and scan work, but does not remove
  the full/backpressure tail.

## Next Box

```text
ToyCross128TransferFastPath-L1
```

Boundary:

- Toy/front small only.
- Prefer exact 128B class first; do not affect MidPage or large paths.
- Default-off flag, production A/B safe.
- No profile mixing with `transfer_presence`.

First implementation candidate:

```text
Toy128OriginTransferSpecialize-L1
```

Goal:

- Reduce origin-transfer full/scan cost for fixed 128B remote frees.
- Keep correctness authority unchanged.
- Avoid remote-thread frontcache mutation and broad draining.

Promotion evidence:

- `cross128_r90` production R3/R10 improves.
- `remote50` and `remote90` do not regress materially.
- Integrity gates stay zero.
- `returned_backpressure` and `transfer_reserve_full` decrease on cross128.
