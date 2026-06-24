# MediumUpper48KSizePolicyAB-L1

## Scope

Evidence-only build target.

```text
default:
  q64-run64k
  classes: 8K / 16K / 32K / 64K

candidate:
  q64-upper48
  classes: 8K / 16K / 32K / 48K / 64K
  build macro: H8_MEDIUM_UPPER48_CLASS
```

The default allocator behavior is unchanged.

The empty-run resident budget remains fixed to the default four-class budget, so
the candidate does not receive extra retained-RSS budget from the fifth class.

## Build / Smoke

```bash
make smoke smoke-mediumupper48 bench-release bench-release-mediumupper48
./h8_smoke
./h8_smoke_mediumupper48
```

Both smoke binaries passed.

## Short Release R50

Command:

```bash
./h8_bench_release \
  --runs 3 --threads 2 --iters 20000 \
  --min-size 4097 --max-size 65536 \
  --remote-pct 50 --interleaved 1
```

Result:

```text
default q64-run64k:
  median        6.784M ops/s
  steady median 7.191M ops/s
  peak RSS      4.10MiB
  minor faults  420

candidate q64-upper48:
  median        7.108M ops/s
  steady median 7.576M ops/s
  peak RSS      3.76MiB
  minor faults  484
```

## Short Phase R90

Command:

```bash
./h8_bench_release \
  --runs 2 --threads 2 --iters 4000 \
  --min-size 4097 --max-size 65536 \
  --remote-pct 90 --interleaved 0
```

Result:

```text
default q64-run64k:
  median        114.5K ops/s
  peak RSS      64.9MiB
  minor faults  15546
  alloc median  84.744ms

candidate q64-upper48:
  median        170.6K ops/s
  peak RSS      64.3MiB
  minor faults  15479
  alloc median  70.445ms
```

## Interpretation

```text
upper48 build is functional and smoke-clean
short release measurements show no immediate hot-path blocker
phase RSS improvement is small at this sample size
```

Decision:

```text
HOLD as default
run paired R10 x2 before any promotion decision
keep p2 medium map as current default
```
