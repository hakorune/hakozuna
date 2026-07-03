# HZ10 Local Page Substrate Target

HZ10 is the next-design candidate after HZ9 ProductEntry evidence.

## Identity

```text
HZ10 = HZ3-class local speed
     + HZ4/HZ5-class remote resilience
     + HZ8-style fail-closed and bounded-RSS discipline
```

HZ10 is not an HZ9 micro-tune and not a tcmalloc clone.

## Performance Target

Use same-run, same-runner rows only.

```text
minimum GO:
  local:
    >= 2.0x HZ8 on main_local0 / medium_local0
    or >= 250M ops/s on current T16 local0-style rows

  remote:
    >= 1.2x HZ8 on medium_r50 / main_r90-style rows

  RSS:
    post RSS <= 2x HZ8 on local rows
    remote-heavy peak RSS must be capped and reported

good balanced target:
  250-350M ops/s on main/medium local0
  about 40-55% of tcmalloc local rows
  much closer to HZ8 than to tcmalloc/mimalloc on post RSS

not the first target:
  tcmalloc 70%+ local throughput
```

## Architecture

```text
Layer 0: thread-local page + intrusive freelist
  owner thread owns local_free_head and local_free_count
  same-thread malloc/free use plain loads/stores

Layer 1: O(1) pagemap route
  ptr -> page metadata by address-derived pagemap
  exact / interior / misaligned / generation checks stay immediate

Layer 2: remote free stack + owner drain
  foreign free pushes to page remote stack
  owner drains remote stack in batches into local freelist
```

Design rule:

```text
local per-slot state is thread-private
route metadata is static after page publication
remote state is isolated to remote stack / pending authority
```

Forbidden fast-path shapes:

```text
copy local alloc/free bits into shared metadata every operation
use shared per-slot bits as local fast-path authority
check route/pending/generation on every same-thread allocation
```

## Safety Split

Immediate:

```text
unknown address -> MISS / INVALID by pagemap
interior / misaligned / tail slack -> INVALID
generation mismatch -> INVALID
same-thread shallow double free -> reject when cheaply detectable
```

Delayed:

```text
foreign double free -> owner drain detection
remote free reconciliation -> batched owner drain
RSS return -> page transition / pressure / owner exit
```

Hard zero gates:

```text
owned_invalid_forwarded_to_platform = 0
route_interior_valid = 0
route_misaligned_valid = 0
route_generation_stale_valid = 0
remote_lost_publish = 0
remote_duplicate_accepted_as_two_slots = 0
release_while_remote_pending = 0
owner_exit_page_remaining = 0
thread_exit_page_remaining = 0
rss_cap_unbounded_growth = 0
```

## RSS Contract

```text
per-thread:
  per-class active pages
  bounded retired page cache

global:
  bounded page pool
  cap overflow returns/decommits pages

exit:
  thread exit drains local pages and remote stacks
  owner exit releases or returns pages to bounded pool
```

## Implementation Boxes

```text
Box 1: HZ10PageMapRoute-L0
  standalone pagemap
  exact/interior/misaligned/generation smoke
  route cost comparison

Box 2: HZ10ThreadLocalFreelistPage-L0
  one-class intrusive local freelist page
  honest public-shaped malloc/free microbench

Box 3: HZ10RemoteStackDrain-L0
  foreign free stack
  owner drain
  duplicate/stale/pending counters and smokes

Box 4: HZ10BoundedPagePool-L0
  cache caps
  release pressure
  local/remote/RSS matrix
```

## Build Hygiene

```text
-fvisibility=hidden for product/preload artifacts
-ftls-model=initial-exec for preload TLS variables
asm/source-shape honesty checks before performance claims
ASLR-off or alignment-stable reruns for tiny hot-loop attribution
```

