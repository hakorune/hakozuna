# HZ12 Standalone Baseline L1

## Decision

HZ12 is now a standalone code line. The initial Windows span allocator was
forked from the proven HZ11 baseline, renamed from `hz11_*` to `hz12_*`, and
copied into the HZ12 folder.

```text
included in HZ12:
  public API
  size classes
  system allocation
  span and route substrate
  thread cache and live-footprint support

not a dependency:
  HZ11 include files
  HZ11 source files
  HZ11 build inputs
```

## Boundary

The baseline fork is intentionally behavior-preserving. HZ12-specific work
starts above it: advisory owner routing, bounded inboxes, dead-owner adoption,
and verified span reclaim. HZ11 remains frozen as an independent selected
allocator line.

## Verification

The Windows HZ12 build compiles the HZ12 core sources only. A 2 producer / 2
consumer xowner run completed with 100% cross-owner frees, zero unknown-owner
fallback, and all routed objects drained. This verifies the standalone build
boundary and the L1 wrapper contract; it is not a performance promotion.
