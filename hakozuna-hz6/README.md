# Hakozuna HZ6

HZ6 is the planned transfer-first successor to the HZ5 sidecar allocator
research. It now has an R1 executable seed: modular contracts, a toy
contract-validation front, and a first Large128 transfer-first front seed.

## Positioning

```text
HZ3:
  thin local/TLS speed ideas

HZ4:
  remote handoff and page/span cache ideas

HZ5:
  descriptor ownership, fail-closed route safety, and low-RSS profiles

HZ6:
  route-safe / transfer-first / RSS-aware allocator family
```

HZ6 should not be a direct port of HZ3, HZ4, or HZ5 internals. It should take
the proven ideas and make them first-class layers from the start.

## Core Direction

HZ6 starts from these rules:

- Route lookup is a first-class layer and returns `MISS`, `VALID`, or
  `INVALID`.
- Remote frees prefer bounded transfer caches over owner inboxes in fast
  profiles.
- Owner inboxes remain for strict, fallback, and owner-bound profiles.
- Hot paths do not update diagnostic counters or read policy state.
- Refill, drain, source, release, and policy decisions live on slow paths.
- Fail-closed ownership remains mandatory, but fast remote profiles may use
  batch-boundary validation when it does not allow corrupted reuse.
- RSS governance is part of the core design, not an afterthought.

## Reading Order

```text
docs/HZ6_BLUEPRINT.md
docs/HZ6_R1_STATUS.md
docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
docs/HZ6_ARCHITECTURE_DRAFT.md
docs/HZ6_FOLDER_LAYOUT.md
docs/HZ6_MIGRATION_FROM_HZ5.md
docs/current_task.md
```

## R1 Smoke

The first code is intentionally small. It validates module boundaries and
transfer-first state transitions, not performance.

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected output:

```text
hz6-r1-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-safety-smoke ok
```

## Non-Goals

- Do not copy HZ3/HZ4/HZ5 implementation files into HZ6 as a starting point.
- Do not put OS queries such as `VirtualQuery` on a hot free path.
- Do not make a single universal profile the first success criterion.
- Do not weaken owned-invalid / double-free behavior into a silent fallback.
