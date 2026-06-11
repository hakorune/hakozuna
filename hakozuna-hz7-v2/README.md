# Hakozuna HZ7 TinyRoute V2

HZ7 TinyRoute V2 is the growth track for the tiny allocator line. The original
`hakozuna-hz7/` tree stays frozen as the cute v1 baseline; this folder is where
the local-complete v2 shape can grow without importing the full HZ6 profile
matrix.

## Positioning

```text
HZ5:
  low-RSS, fail-closed, descriptor-owned profile families

HZ6:
  route-safe / transfer-first / RSS-aware selected-family allocator

HZ7 v1:
  cute tiny baseline

HZ7 v2:
  tiny reference allocator with route safety, low RSS, coarse MT safety,
  remote-free safety, and stronger local small/medium behavior
```

HZ7 v2 is not HZ6-next. It is HZ6-minimal: a small, readable, single-lane
allocator that keeps only the parts of the Hakozuna line that fit a tiny
implementation.

## Current Contract

```text
API:
  h7_malloc(size)
  h7_calloc(count, size)
  h7_free(ptr)
  h7_route(ptr)
  h7_stats()

Route:
  MISS     foreign pointer / not owned
  VALID    currently active exact pointer
  INVALID  HZ7-owned-looking inactive/interior/retained pointer

Threads:
  coarse global lock
  cross-thread free is safe
  remote free is a safety/control feature, not a throughput claim

Shape:
  small/medium spans through 16KiB
  direct retained buckets for 32KiB and 64KiB
  larger direct OS regions

Lock scope:
  SlowPathOutsideLock-L1 is the current default path
  OS allocation/release happens outside the global lock when route state is safe
```

## Non-Goals

```text
not libc interpose
not remote-performance allocator
not owner-aware remote free
not inbox / TLS ownership
not lock-free remote queue
not HZ6-style profile matrix
not production hot-path diagnostics
```

## Current Measurement

Windows `random_mixed` repeat-5, direct-API row `hz7-v2`:

```text
small   78.337M ops/s, 4,576 KB peak
medium  18.199M ops/s, 5,036 KB peak
mixed   19.491M ops/s, 5,504 KB peak
```

Source:

```text
out_win_random_mixed_hz7v2_routevalidation_module_repeat5/
20260611_152945_paper_random_mixed_windows.md
```

## Build And Smoke

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz7-v2\win\build_win_hz7_smokes.ps1
```

The Windows smoke script builds and runs:

```text
hz7_smoke.exe:
  direct API + route safety smoke

hz7_remote_smoke.exe:
  cross-thread alloc/free safety smoke under the coarse global lock

hz7_mt_smoke.exe:
  coarse-lock multithread safety smoke

hz7_stats_smoke.exe:
  public stats + retained route invariant smoke
```

Linux:

```bash
./hakozuna-hz7-v2/linux/build_hz7_smoke.sh
```

The Linux smoke script runs the same route, remote-free, multithread, and stats
invariant checks with pthreads for the threaded cases.

## Windows Random Mixed Bench

HZ7 v2 is wired into the Windows `random_mixed` runner as a direct-API row named
`hz7-v2`.

Quick HZ7 v2-only build:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_random_mixed_suite.ps1 -OnlyHz7V2
```

Quick HZ7 v2-only run:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1 -Runs 5 -Profiles small,medium,mixed -Allocators hz7-v2
```

Full random_mixed suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_random_mixed_suite.ps1
powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1 -Runs 1 -Profiles small,medium,mixed
```

## Reading Order

```text
docs/HZ7_V2_TASKS.md
docs/HZ7_TINYROUTE_V2.md
docs/HZ7_TINYROUTE_PLAN_V2.md
docs/HZ7_TINYROUTE_PLAN.md
```
