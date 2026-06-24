# MediumQuantumDirectoryCap-L1

## Scope

Increase the medium 64KiB quantum directory from 4096 entries to 65536 entries.

This closes the observed direct-directory capacity fallback in phase rows. It
does not implement chunk carving or remove per-run `mmap`.

## Results

```text
short debug phase r90:
  throughput median: 17.654K ops/s
  alloc median:      1101.378 ms
  remote median:     3.926 ms
  global_skip_foreign=0
  free_steps=0
  peak RSS median:   159.2MiB

debug medium r50:
  throughput median: 3.198M ops/s
  global_skip_foreign=0
  free_steps=0

release medium r50:
  throughput median: 7.590M ops/s
  steady median:     7.895M ops/s

small interleaved remote90 quick:
  throughput median: 53.244M ops/s
```

## Verification

```text
make bench bench-release
make smoke safety-stress
./h8_smoke
./h8_safety_stress
```

## Decision

```text
directory-capacity fallback:
  closed

remaining medium phase costs:
  per-run mmap / VMA count
  first touch
  madvise / run lifecycle
  64K one-slot density

next:
  MediumChunkArenaCarve-L1 or MediumUpper48KSizePolicyShadow-L1
```
