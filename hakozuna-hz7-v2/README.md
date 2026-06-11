# Hakozuna HZ7 TinyRoute V2

HZ7 TinyRoute V2 is the next-stage tiny allocator experiment distilled from HZ6.
This folder is the v2 playground. The original v1 stays in
`hakozuna-hz7/` as the small baseline.

It is not HZ6-next. It is HZ6-minimal: a small, readable, single-lane allocator
that keeps only the parts of the Hakozuna line that fit a tiny implementation.

## Positioning

```text
HZ5:
  low-RSS, fail-closed, descriptor-owned profile families

HZ6:
  route-safe / transfer-first / RSS-aware selected-family allocator

HZ7 TinyRoute:
  tiny-binary / single-shape allocator distilled from HZ6
```

HZ7 v2 should not inherit the HZ6 profile matrix, diagnostic counter forest,
ElasticCapacity machinery, descriptor depot, or workload-specific lanes. The
first prototype should prove that the allocator can stay small and predictable
before adding more shape.

HZ7 v2 is local-performance focused and remote-free safe, but it does not try
to become a remote-performance allocator. Cross-thread frees should stay safe
under the coarse global lock; they are evidence/control, not a throughput
claim.

## TinyRoute-0

TinyRoute-0 is the first prototype.

```text
API:
  h7_malloc(size)
  h7_calloc(count, size)
  h7_free(ptr)
  h7_stats()

Scope:
  direct API only
  no libc interpose
  no foreign pointer fallback
  no route table
  no descriptor table
  single-threaded unless externally synchronized

Shape:
  span:  16B..16KiB, 64KiB aligned spans, bitmap + free list
  big:   >16KiB, direct OS allocation with a small direct header
```

TinyRoute started with only `small` and `big`. TinyRoute-3 MediumLite keeps the
same span machinery and extends span classes to 8K and 16K without adding a
medium central pool. 32K stays direct because it is too close to one retained
slot per 64KiB span.

TinyRoute-3.5 DirectRetain32/64 keeps the two-layer shape but reduces direct
OS churn for the random-mixed medium profile by retaining up to sixteen inactive
direct region for the 32K and 64K direct buckets. The 32K bucket covers
`>16K..32K`, and the 64K bucket covers `>32K..64K`. A retained direct region
remains in the route table as HZ7-owned-looking `INVALID`; it becomes `VALID`
again only when reused by the same bucket.

Windows random_mixed repeat-5 after DirectRetain32/64:

```text
small   79.316M ops/s, 5,136 KB peak
medium  10.928M ops/s, 7,240 KB peak
mixed   11.701M ops/s, 7,600 KB peak
```

## TinyRoute-1

TinyRoute-1 adds optional tiny route safety.

```text
Route result:
  MISS
  VALID
  INVALID

Mechanism:
  fixed segment table
  no heap allocation for route metadata
  valid owned pointer -> VALID
  foreign pointer -> MISS
  owned-looking invalid pointer -> INVALID
```

TinyRoute-1 is where HZ6's route contract returns in miniature. TinyRoute-0
does not claim safe foreign-pointer handling.

## Completion Roadmap

```text
TinyRoute-1:
  fixed region route table
  h7_route(ptr) -> MISS / VALID / INVALID
  h7_free() does not dereference arbitrary foreign pointers first

TinyRoute-2:
  coarse global lock multithread safety smoke
  no speed claim yet

TinyRoute-3:
  MediumLite retained spans for 8K / 16K
  32K and larger stay direct

TinyRoute-3.5:
  DirectRetain32/64 cap=16
  retained direct routes are INVALID, not MISS
  no adaptive medium pool

TinyRoute-3.6:
  SlowPathOutsideLock-L1
  keep coarse global lock semantics
  move OS allocation/release outside the lock when route state is safe
  no owner-aware remote path

TinyRoute-4:
  optional per-thread small span/front cache
  remote free remains global-lock fallback
```

The near-term goal is not to clone HZ6. It is to keep HZ7 tiny while adding
route safety, coarse multithread safety, and the smallest medium coverage that
keeps the code readable.

Remote free is only a fallback/smoke concern here, not a performance track.
Lock-free remote free, owner inboxes, libc interposition, and profile-family
policy are not HZ7 v1/v2 goals. If they become necessary, they belong in a
later HZ8/HZ6-family design rather than TinyRoute.

The next v2 implementation step is `SlowPathOutsideLock-L1`: keep the coarse
global lock model, but avoid holding it across OS allocation and release once
route/list state has been made safe.

## Remote Evidence

```text
Allowed:
  coarse-lock remote smoke
  route-capacity evidence
  safety/fail-closed checks

Not allowed:
  remote throughput claims
  owner-aware remote free
  inbox/TLS ownership
  remote-specific policy matrix
```

## Reading Order

```text
docs/HZ7_V2_TASKS.md
docs/HZ7_TINYROUTE_V2.md
docs/HZ7_TINYROUTE_PLAN_V2.md
docs/HZ7_TINYROUTE_PLAN.md
```

## TinyRoute-0 Smoke

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz7\win\build_win_hz7_smokes.ps1
```

The Windows smoke script builds and runs both:

```text
hz7_smoke.exe:
  direct API + route safety smoke

hz7_remote_smoke.exe:
  cross-thread alloc/free safety smoke under the coarse global lock

hz7_mt_smoke.exe:
  coarse-lock multithread safety smoke
```

Linux:

```bash
./hakozuna-hz7/linux/build_hz7_smoke.sh
```

## Windows Random Mixed Bench

TinyRoute-0 is wired into the Windows `random_mixed` runner as a direct-API
row named `hz7-tinyroute`.

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_random_mixed_suite.ps1
powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1 -Runs 1 -Profiles small,medium,mixed
```

This row uses `h7_malloc()` / `h7_free()` through the benchmark adapter. It is
not a libc interposer row and should not be used as a general allocator claim
until a later HZ7 route/interpose stage exists.

## Non-Goals

```text
not HZ6 replacement
not universal fastest allocator
not profile-family allocator
not benchmark-specific tuning
not libc interpose in v0
not foreign-pointer fallback in v0
not thread-local front caches in v0
not medium retained pool in v0
not production hot-path diagnostics
```
