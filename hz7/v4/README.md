# Hakozuna HZ7 TinyRoute V4

HZ7 v4 is the remote-free safe research fork of the tiny allocator line.
It is intentionally not remote-fast. The point of v4 is to keep the allocator
small and readable while proving that cross-thread free stays correct under a
coarse global lock and a route-first free path.

## Positioning

```text
HZ7 v1:
  frozen cute baseline

HZ7 v2:
  tiny direct-API / remote-safe phase

HZ7 v3:
  local-complete / performance-growth fork

HZ7 v4:
  remote-free safe allocator fork
  route safety first
  bounded cross-thread pressure
  no owner inboxes, no TLS ownership, no lock-free remote queue
```

## Contract

```text
API:
  h7_malloc(size)
  h7_calloc(count, size)
  h7_free(ptr)
  h7_route(ptr)
  h7_stats()

Threads:
  coarse global lock
  cross-thread free is safe
  remote free is correctness / control evidence, not a throughput claim

Route:
  MISS     foreign pointer / not owned
  VALID    currently active exact pointer
  INVALID  HZ7-owned-looking inactive/interior/retained pointer

Shape:
  small/medium spans through 16KiB
  direct retained buckets for 32KiB and 64KiB
  larger direct OS regions
```

## What v4 tries to prove

```text
1. route-first free stays safe under cross-thread pressure
2. coarse-lock remote free remains bounded and easy to reason about
3. the allocator stays tiny enough to remain readable
4. ownership complexity does not need to grow into inbox / TLS / queue land
```

## Non-Goals

```text
not remote-performance allocator
not owner-aware remote handoff
not inbox / TLS ownership
not lock-free remote queue
not HZ6-style profile matrix
not production hot-path diagnostics
```

## Current Entry Points

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\hz7\v4\win\build_win_hz7_smokes.ps1
powershell -ExecutionPolicy Bypass -File .\hz7\v4\win\run_win_hz7_v4_remote_safe.ps1 -Runs 3
powershell -ExecutionPolicy Bypass -File .\hz7\v4\win\run_win_hz7_v4_size_slices.ps1 -Runs 3
```

Linux:

```bash
./hz7/v4/linux/build_hz7_smoke.sh
```

## Initial Tasks

- `docs/HZ7_V4_TASKS.md`
- `docs/HZ7_V4_REMOTE_SAFE.md`

The task board is split so the safety contract stays explicit before any
throughput discussion.
