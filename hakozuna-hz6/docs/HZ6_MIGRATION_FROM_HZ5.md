# HZ6 Migration From HZ5

HZ6 should not be a rewrite performed blindly. It should promote the HZ5
lessons that survived measurement and discard the no-go shapes.

## Promote From HZ5

```text
descriptor ownership:
  keep

fail-closed route behavior:
  keep

low-RSS profile discipline:
  keep

Windows Local2P sidecar route:
  keep as RouteLayer seed

Linux MidPage PageRun64:
  keep as MidPage backend evidence

LargeFront transfer128:
  keep as evidence that class transfer can help
```

## Promote From HZ3

```text
thin TLS/local cache:
  promote to FrontCache

simple size-class dispatch:
  promote, but attach to HZ6 route and state contracts
```

Do not copy HZ3 large-cache complexity into HZ6.

## Promote From HZ4

```text
remote grouping:
  promote to TransferLayer

page/span local reuse:
  promote to front backends

separate hot path and control path:
  promote as a core rule
```

Do not copy unsafe header shortcuts into HZ6.

## Keep In HZ5

These should remain HZ5 diagnostics or profile lanes unless revalidated under
HZ6 contracts:

```text
owner-inbox-only remote path
single global transfer128 cache
producer TLS transfer retention
old-owner transfer sharding
unsafe direct header lookup
hot-path policy counters
VirtualQuery guarded hot route
```

## First Bridge Candidates

### Windows Local2P

Current evidence:

```text
HZ5 slots4:
  local and remote-r90/r99 are strong
  RSS is close to tcmalloc

sidecarfast:
  useful RouteLayer prototype
  not necessarily default
```

HZ6 use:

```text
RouteLayer-R1:
  sidecar-first route
  no VirtualQuery hot path

TransferLayer-R1:
  exact-object transfer bin
```

### Linux LargeFront 128K

Current evidence:

```text
transfer128:
  shows class transfer can help

sharded / TLS / owner-shard variants:
  not broad enough as HZ5 patches
```

HZ6 use:

```text
TransferClass128-R1:
  clean consumer-visible transfer design
  source refill checks transfer first
```

### Linux MidPage

Current evidence:

```text
PageRun64:
  strong general profile
  good RSS behavior
```

HZ6 use:

```text
FrontCacheMid-R1:
  thin bins above page-run backend
  route + transfer contracts from the start
```

## Stop Rules

Stop an HZ6 experiment if it violates any of these:

```text
owned invalid pointer falls through to real allocator
double-free can become reusable
transfer cache is unbounded
source refill bypasses transfer cache
RSS is not measured
hot path adds diagnostic counters
platform route requires OS query on hot path
```

## First Milestone Definition

HZ6-R1 is not "faster than tcmalloc everywhere". HZ6-R1 is successful if it
proves the contracts compose:

```text
RouteLayer:
  MISS / VALID / INVALID works on Windows and Linux

TransferLayer:
  bounded transfer cache returns objects safely

SourceLayer:
  OS abstraction builds on Linux and Windows

Profile:
  one speed profile and one strict profile can share the same contracts
```

