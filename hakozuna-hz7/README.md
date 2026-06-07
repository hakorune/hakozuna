# Hakozuna HZ7 TinyRoute

HZ7 TinyRoute is a tiny-binary allocator experiment distilled from HZ6.

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

HZ7 should not inherit the HZ6 profile matrix, diagnostic counter forest,
ElasticCapacity machinery, descriptor depot, or workload-specific lanes. The
first prototype should prove that the allocator can stay small and predictable
before adding route-safe mode.

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
  small: 16B..4KiB, 64KiB aligned spans, bitmap + free list
  big:   >4KiB, direct OS allocation with a small direct header
```

TinyRoute-0 intentionally uses only two allocation layers: `small` and `big`.
There is no separate medium retained pool in the first prototype.

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

## Reading Order

```text
docs/HZ7_TINYROUTE_PLAN_V2.md
docs/HZ7_TINYROUTE_PLAN.md
```

## TinyRoute-0 Smoke

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz7\win\build_win_hz7_smokes.ps1
```

Linux:

```bash
./hakozuna-hz7/linux/build_hz7_smoke.sh
```

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
