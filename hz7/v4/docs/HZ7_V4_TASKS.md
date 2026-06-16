# HZ7 V4 Tasks

## Current Board

### RemoteSafeSmoke-L1

- build and run the v4 smoke set
- verify cross-thread free stays safe
- keep route lookup fail-closed

### RouteCorrectness-L1

- keep `MISS / VALID / INVALID` distinct
- ensure retained or inactive HZ7-owned pointers stay `INVALID`
- ensure foreign pointers are not dereferenced before route lookup

### BoundedPressure-L1

- keep cross-thread pressure bounded under the coarse lock
- avoid turning v4 into a remote-fast design
- keep route capacity and retained buckets easy to reason about

### SourceStateSplit-L1

- keep compile-time constants, internal types, globals, route table storage,
  direct-retain storage, and coarse lock state in `hz7_state.inc`
- keep `hz7.c` as a thin wrapper over `hz7/common/*.inc`
- keep each shared fragment below 800 lines
- Linux HZ7 v4 smoke passes after the split

### BenchmarkPlumbing-L1

- wire v4 runners and benchmark summaries
- keep measurements separate from the v3 local-complete lane

## Non-Goals

- owner-aware handoff
- inboxes
- TLS ownership
- lock-free remote queues
- HZ6-style profile forests
