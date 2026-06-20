# Hakozuna HZ8

HZ8 is the SpanOwner Fusion allocator design seed.  It is intended to combine
the strongest parts of the earlier allocator lines without inheriting their
largest cost centers:

```text
HZ8 = HZ4-style span/page-local ownership
    + HZ3-style local leaf fast path
    + HZ5-style slow-path pressure/source control
    + HZ6-style boundary-only safety contract
```

The central rule is that small-object truth lives in span metadata, not in a
per-object descriptor and not in a global exact route table.

## Reading Order

```text
docs/HZ8_ARCHITECTURE.md
docs/HZ8_OWNERSHIP_CONTRACT.md
docs/HZ8_OWNER_LIFECYCLE.md
docs/HZ8_BENCH_GATE.md
docs/HZ8_NO_GO_LEDGER.md
```

## v0 Scope

The first implementation target is `v0-fusion-small`:

```text
platform:
  Linux x86_64 first, Windows x64 after the contract is stable

size:
  16B..4KiB

memory shape:
  reserved small arena
  2MiB aligned segments
  64KiB small spans

local path:
  TLS active_span[class]
  bump allocation
  local free-list

remote path:
  stable span owner
  intrusive per-span MPSC remote free list
  remote-pending bitmap
  owner batch collect

safety:
  MISS / VALID / INVALID boundary result
  INVALID never falls back to platform free

out of v0:
  medium retained runs
  direct large retention
  profile family
  production hot-path diagnostics
```

## Design Laws

```text
1. Small objects have no per-object descriptor.
2. Small object identity is segment + span + slot.
3. Span owner stays fixed for the span lifetime.
4. Remote free is a message to the owner, not ownership transfer.
5. Remote free has no bounded sink.
6. Only the owner mutates local free-list and used_count.
7. Decommit/retire happens only on owner slow paths.
8. A span with pending remote objects cannot retire.
9. MISS can only come from the boundary resolver.
10. INVALID never goes to the platform allocator.
```

## Positioning

HZ8 should start as one balanced allocator.  Future `remote` or `rss` profiles
may exist only as immutable slow-path policy values after the default layout
works.  Profiles must not change segment size, span layout, size classes,
metadata layout, owner model, queue type, or route contract.

For v0, do not expose a public policy/profile selector at all.  Keep collection
budgets and empty-span caps as fixed internal constants until `v0-fusion-small`
passes its default gates.

## Build and Bench

```text
make smoke
make preload
make bench
```

Guard-row bring-up example:

```text
./h8_bench --runs 10 --threads 16 --iters 100000 --min-size 16 --max-size 2048 --remote-pct 0
```
