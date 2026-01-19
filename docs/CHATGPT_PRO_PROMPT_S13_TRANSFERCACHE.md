# ChatGPT Pro Prompt: hz3 TransferCache + BatchChain Review

You are reviewing a design for hz3 (a C allocator) that adds a TransferCache
and batch-chain interface to reduce shared-path cost. Please focus on:

- correctness invariants
- concurrency / locking / memory ordering
- hot path impact (must stay unchanged)
- integration points (slow/event paths only)
- risks and test coverage

## Design Doc

Primary reference:
`hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_DESIGN.md`

## Constraints (must follow)

- Box Theory: define boxes and keep boundaries in one place.
- No ENV knobs in hot path.
- Compile-time flags only.
- Fail-fast only in debug builds.
- No per-op stats in hot path.
- A/B gating is mandatory.

## Current hz3 Structure (context)

- Hot path: TLS bin push/pop only.
- Slow path: central/inbox/outbox/segment alloc.
- v2-only path uses PTAG; classification already fast.
- Current bottleneck: shared path O(n) intrusive lists and central lock cost.

## Questions to Answer

1) Are the invariants sufficient to avoid corruption or leaks?
2) Is the TC pop/push ordering safe under contention?
3) Are there better integration points to keep hot path clean?
4) What minimal extra counters would help validate TC without hot-path cost?
5) Any corner cases in bin overflow / epoch trim?

Please suggest improvements if there is a safer or faster arrangement.
