# HZ8 Bench Gate

HZ8 must use median-based gates and stability gates from the first runnable
version.  Single-row wins are not promotion evidence.

For a design-level allocator comparison matrix, see
[`ALLOCATOR_MATRIX.md`](ALLOCATOR_MATRIX.md).

## Common Rules

Use:

```text
RUNS=10
fresh batches=2 for promotion
run order interleaved when comparing allocators
same machine / same runner for table rows
peak RSS reported with throughput
timeout=0
abort=0
safety zero gates all zero
```

Stability gate:

```text
p25 >= median * 0.85
min_run >= median * 0.60
```

Do not speed-rank diagnostic builds.

## v0 Fusion-Small Scope

v0 supports only:

```text
16B..4KiB
local small allocation/free
remote small free
owner collect
minimal pressure control
boundary safety
```

Rows above 4KiB are explicit fallback rows in v0 and are not HZ8 throughput
claims.

## v0 Performance Gates

Initial v0 gates:

```text
guard_r0:
  T=16
  size=16..2048
  remote_pct=0
  median >= 200M ops/s

small_remote90:
  T=16
  size=16..4096
  remote_pct=90
  median >= 40M ops/s

local small alloc/free micro:
  >= 70% of HZ3 local-small reference

RSS:
  peak <= 96MiB
```

These are bring-up gates, not final performance claims.

## Default Candidate Gates

After v1 MediumRun exists, HZ8 may enter the full default candidate matrix.

Reference rows:

```text
main_r0:
  T=16
  size=16..32768
  remote_pct=0
  target >= 235M ops/s

main_r50:
  T=16
  size=16..32768
  remote_pct=50
  target >= 52M ops/s

main_r90:
  T=16
  size=16..32768
  remote_pct=90
  target >= 54M ops/s

guard_r0:
  T=16
  size=16..2048
  remote_pct=0
  target >= 255M ops/s

cross128_r90:
  T=16
  size=16..131072
  remote_pct=90
  target >= 22M ops/s
  requires MediumRun v1
```

RSS gates:

```text
local0 <= 82MiB
remote90 <= 90MiB
cross128 <= 96MiB
```

## Stretch Goals

To claim HZ8 as a broad throughput leader, the final target must move beyond
the initial candidate floor:

```text
guard_r0:
  close to HZ3, then challenge tcmalloc

main_r0:
  close to HZ3 without losing RSS floor

main_r50/main_r90:
  close to HZ4/tcmalloc band without remote backpressure states

cross128_r90:
  close to HZ4 after MediumRun
```

## Required Report Fields

Every HZ8 benchmark summary should include:

```text
git sha
platform / architecture
compiler
allocator build name
row args
runs
median ops/s
p25
p75
min
max
median peak RSS
safety zero-gate summary
raw result path
```

## v0 Non-Gates

The following must not decide v0 promotion:

```text
cross128_r90 full row
medium retained run rows
large direct rows
real app preload rows
profile-specific wins
diagnostic counter builds
single-run medians
```
