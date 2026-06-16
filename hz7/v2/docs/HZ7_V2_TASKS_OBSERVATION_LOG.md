## Hot Path Microbench

`Hz7V2HotPathMicrobench-L1` is diagnostic-only. It measures direct-API
`malloc/free` pairs and standalone `h7_route()` calls without adding allocator
counters or production hot-path instrumentation.

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\hz7/v2\win\run_win_hz7_v2_hotpath.ps1 -Runs 3 -Iters 5000000
```

Observed Windows run:

```text
malloc_free:
  small64    48.310M pairs/s
  span8k     47.536M pairs/s
  direct32k  56.162M pairs/s

route_valid:
  small64   120.945M ops/s
  span8k    120.671M ops/s
  direct32k 121.777M ops/s

route_invalid:
  small64   120.931M ops/s
  span8k    121.037M ops/s
  direct32k 118.798M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_check/
20260611_171114_hz7_v2_hotpath_windows.md
```

Reading:

```text
h7_route() alone is around 120M ops/s.
The direct-API malloc/free pair path is around 47M-56M pairs/s.

This means route lookup is not the only blocker.
The next HZ7 v2 optimization should inspect lock scope and slow-path work
inside the coarse global lock before adding remote-fast machinery.
```

## Next Task: LockScopeTrim / SlowPathOutsideLock

Consultation result:

```text
Remote fast path is not the next best target.
The next target is reducing unnecessary work under the global lock while
preserving HZ7 v2's simple route-safe, remote-free-safe contract.
```

Task plan:

```text
1. Keep HZ7 v2 remote-free safe, but not remote-throughput optimized.
2. Add no production hot-path counters.
3. Identify work currently performed under the global lock.
4. Move pure slow-path preparation outside the lock where correctness allows.
5. Keep route mutation, span free-list mutation, and direct retain mutation
   inside the lock.
6. Validate with smoke tests plus the hot-path diagnostic.
```

Candidate split:

```text
Inside lock:
  route table insert/remove/lookup used by allocation/free
  span free-list mutation
  direct retain bucket mutation
  global accounting visible through h7_stats

Outside lock if safe:
  requested-size classification
  direct allocation size rounding
  OS direct allocation before final route publish
  failed slow-path cleanup after route publish failure
```

Acceptance:

```text
safety:
  Windows and Linux smokes pass
  remote-free smoke stays clean
  route invalid / retained route invariants stay clean

performance:
  hot-path malloc_free improves or stays flat
  random_mixed small/medium/mixed does not regress
  RSS remains near current low-RSS baseline

design:
  no owner inbox
  no lock-free remote queue
  no TLS frontcache yet
  no production counter/atomic in the hot path
```

### LockScopeTrim-L1 observation

Tried direction:

```text
AllocPlan/ClassFast-L1:
  compute small/direct classification outside the lock
  pass class_id/size plan into malloc locked sections
  avoid repeated h7_class_for_size / h7_size_is_small checks under lock
```

Safety result:

```text
Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Performance result:

```text
hotpath:
  baseline small64 malloc_free    48.310M pairs/s
  L1 small64 malloc_free          46.523M pairs/s

  baseline span8k malloc_free     47.536M pairs/s
  L1 span8k malloc_free           46.366M pairs/s

  baseline direct32k malloc_free  56.162M pairs/s
  L1 direct32k malloc_free        53.953M pairs/s

random_mixed:
  baseline small   78.968M-79.859M ops/s depending snapshot
  L1 small         78.080M ops/s

  baseline medium  41.005M-41.186M ops/s
  L1 medium        40.080M ops/s

  baseline mixed   41.477M-42.376M ops/s
  L1 mixed         41.933M ops/s
```

Decision:

```text
AllocPlan/ClassFast-L1:
  no-go / do not promote

Reason:
  safe but not faster.
  HZ7 v2's remaining same-thread cost is not explained by lock-inside
  size-class recomputation alone.

Next:
  keep allocator body at the previous selected implementation
  look for a different hot-path target before changing remote-free design
```

### Component Hot Path Diagnostic

`Hz7V2ComponentHotPath-L1` extends the diagnostic-only hotpath bench. It does
not add allocator counters or change production code.

Added rows:

```text
malloc_batch:
  allocate many objects first, then free them after measurement

free_batch:
  preallocate many objects, then measure only the free phase

free_retained_loop:
  seed one retained object, then measure malloc/free steady reuse
```

Windows repeat-3 observation:

```text
steady pair path:
  malloc_free small64       48.938M pairs/s
  malloc_free span8k        47.915M pairs/s
  malloc_free direct32k     56.920M pairs/s

route only:
  route_valid small64      122.091M ops/s
  route_valid span8k       122.136M ops/s
  route_valid direct32k    123.175M ops/s

batch allocate:
  malloc_batch small64      43.462M ops/s
  malloc_batch span8k        0.708M ops/s
  malloc_batch direct32k      0.537M ops/s

batch free:
  free_batch small64        77.011M ops/s
  free_batch span8k          1.921M ops/s
  free_batch direct32k        0.840M ops/s

retained steady loop:
  free_retained_loop small64    49.038M pairs/s
  free_retained_loop span8k     48.349M pairs/s
  free_retained_loop direct32k  56.125M pairs/s
```

Source:

```text
out_win_hz7_v2_hotpath_component_l1/
20260611_172336_hz7_v2_hotpath_windows.md
```

Reading:

```text
Route lookup is not the main steady-state blocker.
Steady reuse is reasonably flat across small/span/direct.

The very slow rows are cold/source-pressure batch rows:
  many live span/direct allocations
  many route registrations
  later route unregister/release or retain-limit pressure

This means HZ7 v2 should not chase remote-fast yet, and it should not treat
random_mixed weakness as a simple route lookup issue.
```

Next candidate:

```text
MixedSizeSteady-L1 diagnostic:
  measure a steady random-size working set with bounded live objects
  separate same-size retained reuse from cross-size churn
  identify whether medium/mixed weakness is class switching, direct retain
  bucket mismatch, route churn, or benchmark RNG/workload overhead
```

### MixedSizeSteady-L1

Added diagnostic-only rows to the hotpath bench:

```text
mixed_steady small_ws400:
  16B..2KiB, live set 400

mixed_steady span_medium_ws400:
  4KiB..16KiB, live set 400

mixed_steady direct_medium_ws400:
  16KiB+1..32KiB, live set 400

mixed_steady medium_ws400:
  4KiB..32KiB, live set 400

mixed_steady mixed_ws400:
  16B..32KiB, live set 400
```

Windows repeat-3 observation:

```text
small_ws400          51.191M ops/s
span_medium_ws400   37.956M ops/s
direct_medium_ws400 54.547M ops/s
medium_ws400        24.280M ops/s
mixed_ws400         23.083M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_mixedsteady_slices_l1/
20260611_172642_hz7_v2_hotpath_windows.md
```

Reading:

```text
Direct-retained medium is not the weak row by itself.
Small steady and direct-medium steady are both around 50M+ ops/s.
Span-medium steady is lower but still far above cross-boundary medium/mixed.

The weak signal appears when the workload crosses the 16KiB span/direct
boundary with a bounded live set. This points at cross-class / cross-source
churn rather than a simple route lookup or direct retain problem.

The peak_kb column in this diagnostic is cumulative within one process and is
not used as RSS evidence for these mixed_steady rows.
```

Next implementation candidates:

```text
BoundaryChurn-L1 diagnostic:
  count how often steady workloads cross small/span/direct source families
  without adding production counters

MediumBoundaryPolicy-L1:
  test whether changing the 16KiB span/direct boundary improves cross-boundary
  steady mixed rows without hurting small/direct-only rows

DirectRetainClass-L1:
  test whether finer direct retain buckets help only after boundary churn is
  proven to be a direct-retain mismatch rather than source-family switching
```

### BoundaryChurn-L1

`BoundaryChurn-L1` adds diagnostic-only source-family switch attribution to
the `mixed_steady` rows. It does not add allocator counters or change HZ7 v2
production code.

Family definition:

```text
span family:
  size <= 16KiB

direct family:
  size > 16KiB
```

Windows repeat-3 observation:

```text
small_ws400:
  median 50.324M ops/s
  switch_rate 0.000000

span_medium_ws400:
  median 37.173M ops/s
  switch_rate 0.000000

direct_medium_ws400:
  median 53.592M ops/s
  switch_rate 0.000000

medium_ws400:
  median 24.176M ops/s
  switch_rate 0.489742

mixed_ws400:
  median 23.206M ops/s
  switch_rate 0.499983
```

Source:

```text
out_win_hz7_v2_hotpath_boundarychurn_l1/
20260611_172849_hz7_v2_hotpath_windows.md
```

Reading:

```text
Boundary churn is now the strongest explanation for the medium/mixed steady
drop.

Rows with switch_rate 0 remain much faster:
  small_ws400 around 50M
  direct_medium_ws400 around 54M
  span_medium_ws400 around 37M

Rows crossing the 16KiB span/direct boundary about half the time drop to
around 23M-24M.
```

Decision:

```text
Next best target:
  MediumBoundaryPolicy-L1

Why:
  The weakness is not direct-retain alone and not route lookup alone.
  It appears when a steady live set alternates between span and direct source
  families.

First experiment:
  raise the span/direct boundary for HZ7 v2 in an explicit lane
  compare:
    span/direct-only steady rows
    medium_ws400
    mixed_ws400
    random_mixed small/medium/mixed
    RSS

No-go:
  RSS loses HZ7 v2's low-memory identity
  small or direct-only rows regress materially
  medium/mixed do not improve
```

### MediumBoundaryPolicy-L1

Implemented as an explicit build lane, not a default change:

```text
H7_SPAN_CLASS_MAX default:
  16KiB

H7_SPAN_CLASS_MAX=32768:
  enables an additional 32KiB span class
  keeps default HZ7 v2 unchanged
```

Build hooks:

```text
hz7/v2/win/run_win_hz7_v2_hotpath.ps1:
  -SpanClassMax 32768

win/build_win_random_mixed_suite.ps1:
  -Hz7V2SpanClassMax 32768

win/run_win_random_mixed_paper.ps1:
  -SuiteDirName <custom suite dir>
```

Safety:

```text
Default Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Default Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Windows span32 observation:

```text
hotpath, H7_SPAN_CLASS_MAX=32768:
  small_ws400          49.930M ops/s
  span_medium_ws400   36.880M ops/s
  direct_medium_ws400 42.476M ops/s
  medium_ws400         2.540M ops/s
  mixed_ws400          2.484M ops/s

random_mixed, H7_SPAN_CLASS_MAX=32768:
  small   78.488M ops/s, 5,016 KB peak
  medium   4.928M ops/s, 5,708 KB peak
  mixed    5.864M ops/s, 6,172 KB peak
```

Sources:

```text
out_win_hz7_v2_hotpath_span32_l1/
20260611_173229_hz7_v2_hotpath_windows.md

out_win_random_mixed_hz7v2_span32_l1/
20260611_173229_paper_random_mixed_windows.md
```

Decision:

```text
MediumBoundaryPolicy-L1:
  no-go / do not promote

Reason:
  raising the span/direct boundary to 32KiB makes medium/mixed dramatically
  worse. Large span slots are too expensive for this tiny allocator design.

Keep:
  H7_SPAN_CLASS_MAX override support as an evidence/control lane.

Default:
  keep 16KiB span boundary.
```

Next:

```text
Do not solve boundary churn by expanding spans to 32KiB.

Next target should preserve the 16KiB boundary and instead reduce cross-family
steady churn:
  direct retain bucket / admission policy
  medium range split policy
  or benchmark/lane separation between span-medium and direct-medium profiles
```

### RandomToggle-L1 correction

`bench_random_mixed_compare` is a toggle workload:

```text
if slot is empty:
  allocate
else:
  free
```

It does not do a free+alloc replacement every operation, and it does not touch
the allocated payload. Earlier `mixed_steady` rows are therefore useful as a
boundary-replacement stress, but they are not the same as the paper
`random_mixed` row.

Added diagnostic rows:

```text
random_toggle fresh_*:
  run before the heavy batch rows so the process is not polluted by batch
  source pressure

random_toggle fresh_*_notouch:
  same toggle workload, but no payload touch on allocation
  closer to bench_random_mixed_compare
```

Windows repeat-3 observation:

```text
fresh_small_ws400:
  touch     86.072M ops/s
  notouch   85.729M ops/s

fresh_medium_ws400:
  touch     21.172M ops/s
  notouch   42.647M ops/s

fresh_mixed_ws400:
  touch     23.276M ops/s
  notouch   43.573M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_toggle_notouch_l1/
20260611_173807_hz7_v2_hotpath_windows.md
```

Corrected reading:

```text
The paper random_mixed medium/mixed row is closer to the notouch toggle row.
The 20M-class medium/mixed result was mostly payload touch/page cost mixed
with allocator cost.

HZ7 v2 still has a real medium/mixed gap:
  small toggle notouch   around 86M
  medium toggle notouch  around 43M
  mixed toggle notouch   around 44M

This gap is no longer explained by payload touch.
It is likely caused by direct-retain capacity/admission and source churn in
the medium range, not by 32KiB span expansion.
```

Next:

```text
DirectRetainCap-L2:
  measure cap 64 / 128 / 256 explicitly with notouch random_toggle and
  random_mixed medium/mixed.

Acceptance:
  medium/mixed improve materially
  small does not regress
  RSS stays close to the low-RSS HZ7 v2 baseline

No-go:
  speed improves only by retaining too much memory
  RSS loses the HZ7 v2 identity
```

### DirectRetainCap-L2

Tested `H7_DIRECT_RETAIN_CAP` 64 / 128 / 256 using explicit lanes.
`hz7/v2/win/run_win_hz7_v2_hotpath.ps1` now accepts
`-DirectRetainCap`.

Hotpath random_toggle notouch observation:

```text
cap 64:
  fresh_small_ws400_notouch   85.736M ops/s
  fresh_medium_ws400_notouch  56.777M ops/s
  fresh_mixed_ws400_notouch   55.290M ops/s

cap 128:
  fresh_small_ws400_notouch   84.679M ops/s
  fresh_medium_ws400_notouch  57.580M ops/s
  fresh_mixed_ws400_notouch   53.947M ops/s

cap 256:
  fresh_small_ws400_notouch   85.135M ops/s
  fresh_medium_ws400_notouch  57.284M ops/s
  fresh_mixed_ws400_notouch   53.907M ops/s
```

Windows random_mixed repeat-3 observation:

```text
cap 64:
  small   76.577M ops/s, 5,020 KB peak
  medium  53.224M ops/s, 5,144 KB peak
  mixed   51.563M ops/s, 5,672 KB peak

cap 128:
  small   77.146M ops/s, 5,020 KB peak
  medium  53.264M ops/s, 5,176 KB peak
  mixed   51.320M ops/s, 5,728 KB peak

cap 256:
  small   77.510M ops/s, 5,028 KB peak
  medium  53.208M ops/s, 5,188 KB peak
  mixed   51.498M ops/s, 5,736 KB peak
```

Default confirmation after promoting cap 64:

```text
small   78.281M ops/s, 5,016 KB peak
medium  53.657M ops/s, 5,144 KB peak
mixed   51.995M ops/s, 5,672 KB peak
```

Sources:

```text
out_win_hz7_v2_hotpath_retain64_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_hz7_v2_hotpath_retain128_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_hz7_v2_hotpath_retain256_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_random_mixed_hz7v2_retain64_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_retain128_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_retain256_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_default_retain64_confirm/
20260611_174218_paper_random_mixed_windows.md
```

Safety:

```text
Default Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Default Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Decision:

```text
H7_DIRECT_RETAIN_CAP default:
  promote from 32 to 64

Why:
  cap 64 recovers medium/mixed throughput while preserving HZ7 v2's low-RSS
  shape. cap 128/256 do not materially improve speed and retain slightly more
  memory.

Keep as controls:
  cap 128 / cap 256

Default:
  cap 64
```

### BaselineSnapshot-L1

After promoting `H7_DIRECT_RETAIN_CAP=64`, refreshed the Windows
`random_mixed` cross-allocator snapshot using the existing paper-aligned runner.

Command shape:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_random_mixed_suite.ps1 `
  -OnlyHz7V2 `
  -OutDirName out_win_random_mixed

powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1 `
  -Runs 3 `
  -Profiles small,medium,mixed `
  -Allocators hz3,hz4,hz7-v2,mimalloc,tcmalloc `
  -OutputDir .\docs\benchmarks\windows\hz7_v2_baseline_snapshot `
  -SuiteDirName out_win_random_mixed
```

Result:

```text
docs/benchmarks/windows/hz7_v2_baseline_snapshot/
20260611_174745_paper_random_mixed_windows.md
```

HZ7 v2 cap64 row:

```text
small:
  79.741M ops/s
  4,576 KB peak

medium:
  53.353M ops/s
  5,140 KB peak

mixed:
  52.911M ops/s
  5,664 KB peak
```

Decision:

```text
keep:
  HZ7 v2 cap64 default as the current closeout lane

strength:
  lowest-RSS row in small / medium / mixed among the selected allocators
  medium/mixed materially improved over the cap32/emptycap4 rows

limit:
  not a throughput winner against hz3/tcmalloc
  not a remote-throughput allocator

next:
  do not retune blindly
  use this snapshot as the scoreboard before any further OptionalCleanup-L1 work
```

### DirectRetireHelper-L1

This is an `OptionalCleanup-L1` source cleanup, not an allocation policy change.
It makes direct free easier to audit by splitting validation from the retained
vs OS-release decision.

Implementation:

```text
h7_big_is_active_user:
  validates direct region magic / kind / ACTIVE flag / exact user pointer

h7_big_retire_locked:
  pushes to the direct retain bucket when possible
  otherwise unregisters the route and schedules OS release outside the lock
```

Contract:

```text
unchanged:
  H7_DIRECT_RETAIN_CAP default remains 64
  retained direct regions remain route INVALID
  route unregister still happens under the lock before OS release
  final OS release still happens outside the lock
  no owner / inbox / TLS / remote-fast policy is added
```

Acceptance:

```text
required:
  Windows smoke passes
  Linux smoke passes

expected:
  no material random_mixed change because this is only readability cleanup
```

### RoutePublicKindHelper-L1

This is an `OptionalCleanup-L1` route readability cleanup, not a route policy
change. It keeps `h7_route_unlocked()` as a tiny lookup wrapper and moves the
region-kind-specific user pointer interpretation into one helper.

Implementation:

```text
h7_region_user_route_kind:
  returns MISS/INVALID directly for non-VALID route lookup results
  validates small span slot pointers with h7_small_slot_index
  validates direct user pointers with h7_big_is_user_ptr
  returns INVALID for unknown route region kinds
```

Contract:

```text
unchanged:
  foreign pointer -> MISS
  active exact pointer -> VALID
  interior/retained/inactive pointer -> INVALID
  h7_free still performs the locked dispatch path separately
  no route table policy change
```

Acceptance:

```text
required:
  Windows smoke passes
  Linux smoke passes
```

### LightFullishBench-L1

This is a closeout snapshot, not a new promotion benchmark. It records one light
Windows pass across HZ7 smoke, HZ7-specific hotpath probes, HZ7 size slices,
cross-allocator `random_mixed`, and the existing MT remote control runner.

Artifacts:

```text
docs/benchmarks/windows/hz7_v2_light_fullish/
```

Result:

```text
completed:
  Windows HZ7 smoke passed
  hotpath probe recorded route/local reuse behavior
  size slices recorded small/span/direct split behavior
  random_mixed small/medium/mixed recorded broad low-RSS snapshot
  mt_remote runner completed a control table

important:
  MT remote is a control/evidence row only
  HZ7 tiny-route failed that high-pressure remote row with route saturation
  this matches the design label: remote-free safe, not remote-throughput optimized

reading:
  HZ7 v2 remains coherent as a tiny low-RSS local allocator
  HZ7 v2 is not a throughput winner against hz3/tcmalloc
  route validation is not the local bottleneck
  span_4k_16k is the main local path to watch if one more narrow experiment is authorized
```

### RemoteNatural-L1

`RemoteNatural-L1` is the next HZ7 v2 continuation lane. It does not turn HZ7
v2 into a remote-throughput allocator. Instead, it makes the existing
coarse-lock, global-route design more natural under bounded cross-thread
pressure.

Implementation:

```text
H7_REMOTE_NATURAL_PRESET:
  widens H7_ROUTE_CAPACITY to 16384 unless the caller overrides it
  keeps the same global lock
  keeps the same MISS / VALID / INVALID contract
  keeps owner / inbox / TLS / lock-free remote queues out of HZ7 v2

h7_stats:
  exposes route_capacity
  exposes empty_span_cap
  exposes direct_retain_cap
  exposes remote_natural_preset

smoke:
  adds hz7_remote_natural_smoke
  compiles that smoke with H7_REMOTE_NATURAL_PRESET=1
  checks cross-thread free correctness under the widened route preset

bench plumbing:
  adds hz7-v2-remote-natural to the Windows mt_remote runner
  labels it as bounded route-pressure evidence, not a remote-fast claim
```

Acceptance:

```text
required:
  Windows HZ7 smoke suite passes
  Linux HZ7 smoke suite passes
  h7_stats reports remote_natural_preset = 1 in the remote-natural smoke
  h7_stats reports route_capacity >= 16384 in the remote-natural smoke
  route_register_fail remains 0 in the remote-natural smoke

design:
  no owner-aware remote free
  no owner inbox
  no TLS ownership
  no lock-free remote queue
  no remote batching
```

Measured:

```text
Windows smoke:
  pass

Linux smoke:
  pass

Windows mt_remote, runs=1, HZ7 v2 remote-natural row:
  ops/s = 4.942M
  actual remote = 87.48%
  fallback = 2.80%
  peak = 520,412 KB
  ALLOC_FAILURES = 0
  route_register_fail = 0
  final active_bytes = 0

reading:
  success as remote-natural safety/pressure evidence
  not a remote-throughput promotion
```

### SourceStateSplit-L1

Cleanup:

```text
hz7_state.inc:
  moved compile-time constants, internal types, globals, route table storage,
  direct-retain storage, and the coarse lock state out of hz7.c

hz7.c:
  remains the behavior implementation as a thin wrapper
  shared body lives in hz7/common/*.inc and each fragment stays below 800 lines
```

Verification:

```text
Linux HZ7 v2 smoke:
  pass
```
