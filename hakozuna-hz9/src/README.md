# HZ9 Source

This directory is now the standalone HZ9 source home.

The initial source is copied from the HZ8 KeepRefill baseline so HZ9 can reuse
the proven boundary mechanisms while changing local throughput architecture in
isolation.

```text
kept from baseline:
  medium route / directory
  slot_state validity authority
  pending bitmap / qstate remote authority
  owner lifecycle / owner-exit hard drain
  lazy/residency accounting

changed by HZ9:
  medium local entry
  TLS object cache
  future local slab/page substrate
  explicit higher-RSS local cache contract
```

Current source-shape rule:

```text
h8_hz9_local_entry.c:
  thin public local-entry seam only

h8_hz9_local_arena.inc:
  LocalArena evidence substrate body

new substrates:
  use new HZ9-owned files or small includes
  do not grow h8_hz9_local_entry.c back into a substrate implementation file

Local slab/page route-boundary work:
  put route result types and shared declarations in a small HZ9-owned header
  split core page allocation, route/free authority, and remote stubs into
  separate translation units
  make free, usable_size, and realloc share one route authority
  allow same-thread exact free to use a TLS pointer-token positive cache before
  route, but route remains the fallback authority for every miss
  do not route public HZ9 free through the HZ8 medium directory as the primary
  local substrate
```

Do not perform a broad `h8_*` to `h9_*` symbol rename in L0. The first HZ9
implementation box should stay mechanically small enough to compare against the
copied baseline.

Local artifact names that still say `hz8` are tolerated only as naming debt.
Build and runtime inputs must resolve inside `hakozuna-hz9/`.

```text
allowed:
  HZ9-only files
  HZ9-only build flags
  small entry-point splits
  counters and tests
  metadata-only shadow scaffolds for the next HZ9 substrate lane

avoid in L0:
  remote pending/qstate rewrite
  owner lifecycle rewrite
  payload-embedded remote freelists
  broad public API rename
```
