# MediumChunkArenaCarve-L1

## Scope

Add an evidence-only medium chunk-carve build.

```text
default:
  per-run mmap remains default

candidate:
  H8_MEDIUM_CHUNK_CARVE
  16MiB chunks
  64KiB quantum bump carve
```

This does not change default allocator behavior.

## Results

```text
release medium r50:
  default median: 7.512M ops/s
  chunk median:   7.394M ops/s
  ratio:          0.984

chunk debug phase r90 short:
  throughput median: 18.647K ops/s
  remote median:     4.111 ms
  free_steps=0
  peak RSS median:   159.4MiB
```

Earlier default-on probe showed similar phase behavior but a small r50
regression. The candidate does not justify default promotion.

## Verification

```text
make bench bench-release bench-mediumchunk bench-release-mediumchunk
make smoke safety-stress
./h8_smoke
./h8_safety_stress
```

## Decision

```text
MediumChunkArenaCarve-L1:
  HOLD as default

reason:
  directory fallback is already fixed
  chunk carve gives no clear medium r50 win
  phase row is now dominated by first-touch/madvise/run geometry

next:
  MediumUpper48KSizePolicyShadow-L1
```
