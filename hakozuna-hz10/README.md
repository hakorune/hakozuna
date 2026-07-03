# Hakozuna HZ10

HZ10 is the next local-page substrate research line after HZ9 ProductEntry
evidence. It is not a public allocator line yet.

## Position

```text
HZ8:
  public balanced / low-RSS allocator line

HZ9:
  evidence line for ProductEntry, owner-drain, lifecycle, and public-boundary
  tradeoffs

HZ10:
  new substrate candidate
  thread-local intrusive freelist pages
  O(1) pagemap route
  remote stack + owner drain
  bounded RSS page-pool contract
```

The goal is not to expose another allocator choice to users. HZ10 exists to
test whether HZ3-class local speed can be combined with HZ8-style fail-closed
and RSS discipline.

## Target

```text
first GO:
  >= 2.0x HZ8 on main/medium local rows
  or >= 250M ops/s on current T16 local0-style rows

good target:
  250-350M local0
  remote rows >= 1.2x HZ8
  post RSS <= 2x HZ8

not first target:
  tcmalloc 70%+ local throughput
```

## Implementation Order

```text
Box 1: HZ10PageMapRoute-L0
  standalone pagemap
  exact/interior/misaligned/generation smoke
  route cost compared against HZ8/HZ9 directory/hash route

Box 2: HZ10ThreadLocalFreelistPage-L0
  intrusive local freelist page
  honest public-shaped malloc/free microbench

Box 3: HZ10RemoteStackDrain-L0
  foreign free stack
  owner drain
  duplicate/stale/pending counters

Box 4: HZ10BoundedPagePool-L0
  page cache caps
  release pressure
  local/remote/RSS matrix
```

## Source Policy

```text
do:
  keep HZ10 self-contained in hakozuna-hz10/
  keep active source files under 800 lines
  copy only required proven helpers from HZ8/HZ9 when a box needs them
  keep HZ8 and HZ9 behavior frozen unless explicitly working there

do not:
  copy HZ9 bench_results or historical experiment binaries
  import the full HZ9 Makefile as-is
  add another cache around HZ8/HZ9 public paths and call it HZ10
  use fused-loop or DCE-prone microbench numbers as promotion gates
```

## Read First

```text
current_task.md
docs/README.md
docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
```

