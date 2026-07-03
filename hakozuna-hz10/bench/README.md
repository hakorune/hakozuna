# HZ10 Bench

Benchmarks must be public-shaped and honest:

```text
no fused-only promotion gates
no DCE-prone alloc/free loops
record THREADS / ITERS / RUNS
compare same-run HZ8 / HZ9 / HZ10 where possible
```

`HZ10ThreadLocalFreelistPage-L0` bench compares hz10_freelist_page against
plain libc malloc/free doing the identical fixed-size alloc/free/touch loop
(system malloc needs no external checkout, so this comparison is always
available; the opt-in HZ10_EXT_ROOT/HZ9 comparison from Box 1 remains the
path for a same-run HZ9 number). Covers both a LIFO (immediate alloc-then-free,
matching HZ9's "local0" bench pattern) and a non-LIFO (alloc a batch, free in
shuffled order) mode, since a LIFO-only bench cannot distinguish a real
freelist from a depth-1 last-pointer cache -- see current_task.md's box 2
note on HZ9 ProductEntry's free-cache mistake.

`HZ10RemoteStackDrain-L0` bench is genuinely multi-threaded (the first box
that is): THREADS producer threads concurrently remote-free disjoint slices
of a fully-allocated page, timed separately from the single owner-thread
drain that follows, repeated REPEAT cycles. A single-threaded `local_free`
row (Box 2's hz10_freelist_page_free, same total op count) runs alongside
in the same invocation as the "cost of staying local" reference point,
mirroring HZ8/HZ9's local-vs-remote-row comparisons.

