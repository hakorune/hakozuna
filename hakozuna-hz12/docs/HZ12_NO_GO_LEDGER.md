# HZ12 No-Go Ledger

| Proposal | Status | Reason |
| --- | --- | --- |
| Put owner lookup on every free | NO-GO | Violates the HZ12 hot-path contract and duplicates a per-free remote allocator shape. |
| Use owner metadata as pointer/free safety authority | NO-GO | Route validity must remain independent and fail-closed. |
| Add lock-free remote queues in L0/L1 | NO-GO | No evidence yet justifies their lifetime and teardown complexity. |
| Modify HZ11 selected Windows row | NO-GO | HZ12 must prove its contract in a separate line. |
| Reclaim a span from inbox counts alone | NO-GO | Whole-span state needs an explicit verified authority. |
| Unlimited owner inboxes | NO-GO | Overflow must downgrade to bounded ownerless recycling. |
| Promote FlushTimeOwnerRouting-L1 | HOLD | It doubles HZ11 xowner throughput, but loses about 7% locally and remains 28% behind tcmalloc in the fixed xowner R5. |
## Windows per-free owner lookup

Status: NO-GO for public/default integration.

The Windows same-owner random_mixed R5 compared HZ12 core with advisory owner
mapping on allocation and owner lookup on every free. The combined path lost
10.6% on small, 9.7% on medium, and 10.4% on mixed. Allocation-only mapping
lost only 1.6..2.4%, so the dominant rejected cost is the free-time lookup.

Decision:

- do not read owner metadata on normal free;
- keep normal free as a local class-cache push;
- inspect and group ownership only on cache overflow/flush;
- keep OwnerFastLoad selected only in the xowner mechanism lane until that
  flush-time behavior has its own lifecycle and broad-regression proof.

Evidence:

- `bench_results/windows_ownermap_local_ab_r5/20260710_202926_paper_random_mixed_windows.md`
- `bench_results/windows_allocmap_local_ab_r5/20260710_203111_paper_random_mixed_windows.md`

## Windows FlushTimeOwnerRouting-L1

Status: HOLD as opt-in mechanism evidence; not a default/profile promotion.

The safe owner-by-class inbox reached 26.128M ops/s in the fixed 100%
cross-owner R5, versus HZ11 ownerless at 12.939M and tcmalloc at 36.318M.
Local random_mixed lost about 7% against HZ12 core. The mechanism works, but
its Pareto point does not justify promotion or another cap/drain knob ladder.

The original attempt to reuse the deferred-free inbox is rejected: drained
objects were already free, and sending them through `hz12_free` again could
recurse on overflow and terminate with stack overflow. Flush routing owns a
separate class-aware inbox instead.
