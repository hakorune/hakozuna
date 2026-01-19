# S93 / S93-2: Owner Stash PacketBox (ARCHIVED / NO-GO)

## Goal

Replace owner stash “intrusive object list” with “packet (fixed-size pointer array)” to reduce pop-side pointer chasing in `hz3_owner_stash_pop_batch()`.

## Outcome

**NO-GO (archived).** On the scale `r90_pf2` workload, packetization introduced large fixed costs and regressed heavily.

Example A/B (r90, T=8, `bench_random_mixed_mt_remote_malloc`, `iters=2,000,000`):

- Baseline (S112 + S67-2): ~`73M` ops/s
- S93-2 enabled: ~`20M` ops/s (**~-72%**)

## Why it lost

- Workload shape effectively behaves like **`n==1` pushes** most of the time; packetization cannot amortize fixed costs.
- PacketBox adds:
  - packet pool allocation/free traffic,
  - extra metadata loads/stores,
  - packet list management (even with exchange-style draining).

Net: the added fixed costs exceed any reduction in pointer-chase latency.

## Crash note (fixed during triage, but still NO-GO)

When `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`, incoming lists are not guaranteed to be NULL-terminated (`tail->next` may be non-NULL).
Any packet fill must walk **exactly `n`** nodes (never `while(cur)` to NULL).

## Status / Build flags

Archived flags are now blocked via `hakozuna/hz3/include/hz3_config_archive.h`:

- `HZ3_S93_OWNER_STASH_PACKET`
- `HZ3_S93_OWNER_STASH_PACKET_V2`

They remain documented for historical reference but cannot be enabled in mainline.

