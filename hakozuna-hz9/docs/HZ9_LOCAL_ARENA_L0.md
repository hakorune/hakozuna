# HZ9 LocalArena L0

`HZ9LocalArena-L0` is the first HZ9-owned medium local substrate. It is
separate from the HZ8 medium-run object cache and from SlabPage. The goal is
to test whether a true local page substrate can pay for the `H9_LOCAL_ENTRY`
seam without changing HZ8's public balanced line.

## Scope

```text
enabled by:
  H9_LOCAL_ENTRY_SPLIT_L1
  H9_LOCAL_ARENA_L0

optional:
  H9_LOCAL_ARENA_REMOTE_DISABLE_L1
  H9_LOCAL_ARENA_REMOTE_SAFE_AFTER_CLASS_L1
  H9_LOCAL_ARENA_MIN_CLASS_ID=N

unchanged:
  HZ8 source tree
  HZ8 default behavior
  small-v0 allocator
  HZ8 medium fallback path
```

The arena reserves a local virtual arena and creates 64KiB pages for medium
classes. Page metadata is HZ9-owned and routeable before HZ8 medium fallback.
The L0 implementation is intentionally a measurement substrate, not a product
default.

## Variants

```text
localarena:
  raw local arena for all medium classes

localarena_adaptive:
  disables future arena allocation for a class after remote frees are seen

localarena_adaptive_min4:
  adaptive arena only for class_id >= 4

localarena_adaptive_min5:
  adaptive arena only for class_id >= 5
```

## Evidence

```text
bench_results/20260702T051705Z_hz9_candidate_gate/
localarena / baseline:
  main_local0                 0.986
  medium_local0               1.112
  medium_r50                  0.552
  main_r90                    0.695
  small_interleaved_remote90  1.046
```

Raw LocalArena proves the local substrate can improve medium local throughput,
but the remote direct-free path is not acceptable.

```text
bench_results/20260702T052046Z_hz9_candidate_gate/
localarena_adaptive / baseline:
  main_local0                 0.796 / p25 0.786
  medium_local0               1.036 / p25 0.987
  medium_r50                  0.990 / p25 0.979
  main_r90                    0.992 / p25 1.000
  small_interleaved_remote90  0.959 / p25 0.959
```

Remote-disable recovers remote rows, but it gives back most local benefit and
hurts non-target rows.

```text
bench_results/20260702T052648Z_hz9_candidate_gate/
localarena_adaptive_min4 / baseline:
  main_local0                 0.969 / p25 0.997
  medium_local0               1.184 / p25 1.169
  medium_r50                  0.946 / p25 0.965
  main_r90                    1.002 / p25 0.989
  small_interleaved_remote90  1.003 / p25 0.993

localarena_adaptive_min5 / baseline:
  main_local0                 1.018 / p25 0.904
  medium_local0               0.969 / p25 0.987
  medium_r50                  0.940 / p25 0.966
  main_r90                    0.994 / p25 0.873
  small_interleaved_remote90  0.983 / p25 0.990
```

`adaptive_min4` is the best current cut: it keeps the medium-local signal and
cleans up main/small rows, but medium_r50 still regresses beyond an acceptable
default gate. `adaptive_min5` removes the useful medium-local bucket.

## Decision

```text
raw localarena:
  HOLD

adaptive:
  HOLD

adaptive_min4:
  best evidence input
  not default

adaptive_min5:
  HOLD / weak

adaptive_class4:
  NO-GO
  class4/48K maps to one slot per 64KiB page and is structurally weak

adaptive_max3:
  NO-GO
  avoids one-slot 48K/64K pages but hurts main local

adaptive_min2_max3:
  NO-GO
  24K/32K-only cut does not recover local rows

dense_adaptive:
  HOLD / profile evidence
  128KiB pages recover medium-local but regress main/remote gates

dense_adaptive_min4:
  NO-GO
  high-class dense pages do not produce a clean mixed-row win

dense_ownerfast:
  evidence only
  proves local rows are dominated by owner-local atomic RMW/CAS cost
  remote rows regress because this is not a safe mixed remote policy

dense_ownerfast_retire:
  NO-GO
  retiring a page after remote free creates page churn and RSS growth on
  remote-heavy rows

dense_ownerfast_admit:
  HOLD
  page-create admission restores part of the medium-local signal but still
  regresses main_r90 and medium_r50

dense_ownerfast_admit_min4:
  HOLD / weak
  removes most main-local damage but gives back the medium-local win

dense_ownerfast_remotesafe:
  NO-GO evidence
  owner-fast pages before class remote history
  remote-safe atomic pages after class remote history
  does not fall back to HZ8 medium for known remote-heavy classes

dense_ownerfast_retire_prob8:
  NO-GO
  fixed-count probation delays page creation but still allows too many
  remote-contaminated pages under remote-heavy rows
```

The next design should preserve the medium-local win while reducing the remote
tax. Do not promote any L0 LocalArena variant until medium_r50 is no-regression
and owner-exit/route safety gates remain clean.

## Class Gate Closure

After the first L0 results, LocalArena tried a narrower class gate and an
owner-local active-page rebound on same-owner free. The rebound fixes a real
substrate bug: freed owner-local pages must be made allocation candidates again
instead of forcing continuous page creation.

```text
bench_results/20260702T054034Z_hz9_candidate_gate/
localarena_adaptive_class4 / baseline:
  fixed48_local0              0.360 / p25 0.357
  main_local0                 0.962 / p25 0.947
  medium_local0               0.854 / p25 0.860
  medium_r50                  0.989 / p25 1.009
  main_r90                    0.986 / p25 0.979
  small_interleaved_remote90  0.994 / p25 1.007

localarena_adaptive_max3 / baseline:
  fixed48_local0              0.965 / p25 0.971
  main_local0                 0.787 / p25 0.780
  medium_local0               1.014 / p25 1.024
  medium_r50                  1.019 / p25 1.003
  main_r90                    0.983 / p25 1.000
  small_interleaved_remote90  0.983 / p25 0.979
```

```text
bench_results/20260702T054228Z_hz9_candidate_gate/
localarena_adaptive_min2_max3 / baseline:
  main_local0                 0.809 / p25 0.801
  medium_local0               0.964 / p25 0.895
  medium_r50                  1.002 / p25 1.007
  main_r90                    0.986 / p25 0.972
  small_interleaved_remote90  1.004 / p25 0.992
```

Read: class gating can remove most remote damage, but it does not create a
clean local win. 48K is especially poor because a 64KiB arena page holds only
one 48K slot. Smaller class cuts avoid that shape but still lose on main-local
or fail to improve medium-local enough. Freeze LocalArena as substrate evidence.

## Dense Page Closure

`localarena_dense_adaptive` uses 128KiB pages and 16 metadata slots so 48K and
64K no longer collapse to a one-slot page. The slot decoder also gained exact
2-slot and power-of-two fast paths.

```text
bench_results/20260702T054902Z_hz9_candidate_gate/
localarena_dense_adaptive / baseline:
  main_local0                 0.925 / p25 0.977
  medium_local0               1.172 / p25 1.228
  medium_r50                  0.978 / p25 1.005
  main_r90                    0.956 / p25 0.976
  small_interleaved_remote90  1.016 / p25 1.013
```

Read: dense pages prove the local substrate can buy a strong medium-local
profile win, but not a default-quality HZ9 line. The main local and main remote
regressions are too large, and medium_r50 is still below no-regression. Treat
LocalArena as profile evidence and stop adding class/page variants until a
different local page architecture exists.

## Owner-Fast Mutation Evidence

`localarena_dense_ownerfast` is an evidence build. It keeps the dense 128KiB
page shape but uses owner-local load/store mutation while `remote_seen == false`
instead of CAS/fetch-or on the local path.

```text
bench_results/20260702T055403Z_hz9_candidate_gate/
localarena_dense_ownerfast / baseline:
  main_local0                 1.301 / p25 1.245
  medium_local0               1.299 / p25 1.222
  medium_r50                  0.935 / p25 0.952
  main_r90                    0.954 / p25 0.953
  small_interleaved_remote90  1.009 / p25 0.998
```

Read: the local substrate win is real when owner-local mutation avoids RMW/CAS.
The same evidence build is not a promotion candidate because remote/mixed rows
regress. The next architecture should separate owner-local pages from
remote-seen pages instead of trying to make one page state serve both paths.

`localarena_dense_ownerfast_retire` tried to retire only the remote-seen page
instead of disabling the class. It is not viable:

```text
bench_results/20260702T055746Z_hz9_candidate_gate/
localarena_dense_ownerfast_retire / baseline:
  main_local0                 1.103 / p25 1.131
  medium_local0               1.199 / p25 1.109
  medium_r50                  0.322 / p25 0.332
  main_r90                    0.424 / p25 0.420
  small_interleaved_remote90  1.052 / p25 1.082
```

Read: page-local retire preserves local speed but creates severe page churn and
RSS growth under remote-heavy rows. The next design must route remote-heavy
classes away from local-owner pages before page creation, not after a remote
free has already contaminated the page.

## Remote-Shape Counters

`HZ9LocalArenaRemoteShapeCounters-L1` adds class-wise debug counters without
changing LocalArena behavior.

```text
tracked by class:
  page_create
  alloc_hit
  free_local
  free_remote
  remote_first
  remote_disable
  remote_retire
  ownerfast_alloc
  ownerfast_free
  create_before_remote
  create_after_remote
  class_remote_first
  remote_first_alloc_age
```

Short debug probes after the counter patch:

```text
local medium row:
  page_create=[2,2,2,2,2,2]
  alloc_hit=[73,126,143,139,252,267]
  free_local=[73,126,143,139,252,267]
  free_remote=[0,0,0,0,0,0]
  remote_first=[0,0,0,0,0,0]

medium r50-shaped row:
  page_create=[37,56,63,62,123,104]
  free_remote=[38,64,71,65,140,115]
  remote_first=[36,55,61,60,123,102]
  remote_retire=[35,54,61,60,121,102]

admission shadow on a repeated debug r50-shaped row:
  create_before_remote=[1,1,1,1,2,2]
  create_after_remote=[36,53,64,62,119,99]
  class_remote_first=[1,1,1,1,1,1]

bench_results/20260702T064334Z_localarena_remote_first_age_probe/
remote-first age debug probe on a medium r50-shaped row:
  page_create=[86,141,146,131,264,256]
  remote_first=[86,141,146,131,264,256]
  remote_first_alloc_age=[0:0,1:324,2_3:494,4_7:196,8p:10]
```

Read: in the remote-shaped row, first remote contamination is close to page
creation by class, and retire follows it. This supports the current closure:
retiring after contamination is too late. The next HZ9 L1 should test
admission/probation before local-owner page creation, or a separate remote-safe
substrate, instead of adding more LocalArena class/page variants.

The admission shadow is intentionally coarse and global by class. It is not a
default policy, but it shows the next useful behavior experiment: after a class
has observed remote frees, stop immediately creating fresh owner-fast local
pages for that class unless a probation signal proves it has returned to a
local-only phase.

The age probe sharpens that design: remote-contaminated pages are usually
contaminated after only one to three local allocations. A future probation
candidate should therefore test whether delaying local-owner page creation can
avoid most remote churn without permanently taxing local-only rows.

## Remote Admission Behavior

`localarena_dense_ownerfast_admit` is the first behavior form of the admission
idea:

```text
if class has observed any remote free:
  do not create another LocalArena page for that class
  fall back to HZ8 medium
```

Direction-finding R3:

```text
main_local0                 1.183
medium_local0               1.027
medium_r50                  0.978
main_r90                    0.973
small_interleaved_remote90  1.020
```

Follow-up R10:

```text
main_local0                 0.850
medium_local0               1.007
medium_r50                  0.996
main_r90                    0.970
small_interleaved_remote90  1.015
```

Reverse-order `main_local0` R5:

```text
main_local0                 0.867
```

After moving the admission check from the malloc entry to the page-create path,
active local pages no longer pay the class-history load on every allocation:

```text
localarena_dense_ownerfast_admit / baseline, R10:
  main_local0                 0.938
  medium_local0               1.177
  medium_r50                  0.989
  main_r90                    0.968
  small_interleaved_remote90  1.016
```

The high-class-only admission cut avoids most main-local damage, but also gives
back the medium-local win:

```text
localarena_dense_ownerfast_admit_min4 / baseline, R10:
  main_local0                 0.989
  medium_local0               0.997
  medium_r50                  0.981
  main_r90                    0.972
  small_interleaved_remote90  1.021
```

Read: remote admission is useful evidence because it removes the catastrophic
remote-heavy page churn seen in `dense_ownerfast_retire`. It is not a default
candidate: page-create admission can recover medium-local signal, but `main_r90`
and `medium_r50` still miss no-regression gates, while `min4` loses the local
payoff. Keep this lane as HOLD evidence. The next design needs either a lower
fixed local-entry cost or a different page-local substrate; admission alone is
not enough.

## Remote Probation Behavior

`localarena_dense_ownerfast_retire_prob8` tested fixed-count probation after a
class had observed remote frees:

```text
after remote history:
  block 8 page-create attempts through HZ8 fallback
  then allow one LocalArena page
```

```text
bench_results/20260702T064641Z_hz9_candidate_gate/
localarena_dense_ownerfast_retire_prob8 / baseline, R3:
  main_local0                 1.050 / p25 0.975
  medium_local0               1.170 / p25 1.169
  medium_r50                  0.583 / p25 0.557
  main_r90                    0.617 / p25 0.617
  small_interleaved_remote90  1.011 / p25 0.997
```

Debug readout:

```text
bench_results/20260702T064811Z_localarena_prob8_debug/
page_create=[67,141,146,132,269,269]
remote_first=[67,141,146,132,269,269]
probation_block=[14150,28448,28318,28096,56228,56508]
probation_allow=[1764,3551,3536,3508,7023,7059]
```

Read: fixed-count probation is not selective enough. It blocks many attempts,
but remote-heavy rows still eventually create many owner-fast pages and then
contaminate them. Close this as NO-GO; do not tune the probation count further.
The next design should either use a different local/remote split or a stronger
thread-local phase signal.

## Thread Admission Counters

`HZ9LocalArenaThreadAdmissionCounters-L1` adds debug-only distribution counters.
Each LocalArena page records the creator thread/class page sequence and local
allocation history at creation time. When that page later sees its first remote
free, the same buckets are recorded for comparison.

```text
bench_results/20260702T_thread_admit_probe_r1/
medium_r50 debug probe:
  page_create=[82,143,142,134,263,260]
  remote_first=[82,143,142,134,263,260]
  create_seq=[0:48,1:48,2_3:96,4_7:190,8p:642]
  remote_seq=[0:48,1:48,2_3:96,4_7:190,8p:642]
  create_alloc_history=[0:48,1:13,2_3:39,4_7:67,8p:857]
  remote_alloc_history=[0:48,1:13,2_3:39,4_7:67,8p:857]
```

Read: for this remote-heavy shape, every created LocalArena page later becomes
remote-first. Thread-local page sequence and local allocation history exactly
match the remote-first distribution, so they are not sufficient admission
signals by themselves. The next substrate should separate owner-local and
remote-safe capacity before allocation instead of tuning fixed probation/history
thresholds.

## Page Mix Counters

`HZ9LocalArenaPageMixCounters-L1` adds debug-only counters for the local/remote
free mix within a page. It records how many local frees happened before the
first remote free, remote frees after any local free, and local frees after the
page was already remote-observed.

```text
bench_results/20260702T_localarena_page_mix_probe_r1/
medium_r50 debug probe:
  remote_first_local_free=[0:421,1:284,2_3:244,4_7:70,8p:5]
  remote_after_local=[50,109,111,102,193,198]
  local_after_remote=[0,3,2,3,8,19]
```

Read: remote-heavy pages become remote-observed very early. Many pages see the
first remote free before any local free, and almost all see it by three local
frees. Once a page is remote-observed, subsequent local frees are rare. This
supports a remote-safe page mode after class remote history rather than fixed
probation, HZ8 fallback, or continued owner-fast page creation.

## Remote-Safe Page L1

`localarena_dense_ownerfast_remotesafe` tested that hypothesis directly.

```text
before class remote history:
  create dense owner-fast pages

after class remote history:
  keep creating LocalArena pages
  mark them remote-safe
  use the atomic free/alloc path from the first allocation
  do not fall back to HZ8 medium
```

R1 release probe:

```text
bench_results/20260702T072016Z_hz9_candidate_gate/
localarena_dense_ownerfast_remotesafe / baseline:
  main_local0                 0.674
  medium_local0               0.944
  medium_r50                  0.480
  main_r90                    0.542
  small_interleaved_remote90  0.958
```

Debug mechanism probe:

```text
bench_results/20260702T_remotesafe_debug_r1.log
remote_safe page_create=[7,7,8,14,501,473]
remote_safe alloc_hit=[14068,28164,23950,23927,62510,62918]
remote_safe free_local=[7060,13961,12150,12103,31272,31617]
remote_safe free_remote=[7008,14203,11800,11824,31238,31301]
```

Read: the mechanism worked, but the shape is wrong. Remote-safe pages become
mixed local/remote atomic pages, which removes the owner-fast local advantage
and increases remote-row page/RSS pressure. Keep the target as evidence, but
do not promote or keep retuning LocalArena page modes without a different
substrate idea.
