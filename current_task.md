# Current Task

## Active Goal

HZ5 Linux general allocator work is now targeting `tcmalloc`, not `mimalloc`.

Immediate focus:

```text
MidPageFront-M4 owner-local magazine
```

Reason:

```text
shadow is already close to tcmalloc on main r0/r50/r90 and mid_only r90.
The remaining large gap is ordinary mid-size local/front-cache throughput,
especially mid_only_r0. M2/M3 tuning showed the issue is not just slot division,
bitmap checks, or a one-entry/current-page cache.
```

## Branch

```text
codex/hz5-linux-p43-port
```

Recent commits:

```text
5f5c9ff Add MidPageFront freefirst tlslink diagnostic
5d2f414 Record MidPage route matrix results
b13000b Add MidPageFront routefree diagnostic
32b0777 Document HZ5 Linux lane combinations
24e0631 Add MidPageFront M4 freefirst lane
```

## Current Result: Freefirst TLS/Link Matrix

Raw output:

```text
private/raw-results/linux/midpage_freefirst_tlslink_matrix_r5_20260525_093316
```

RUNS=5, threads=8, HZ5_PRELOAD_STATS unset:

```text
main_r0:
  allocfirst  87.62M
  freefirst   89.98M
  tlslink    100.90M
  tcmalloc   226.17M

main_r50:
  allocfirst  31.15M
  freefirst   36.47M
  tlslink     43.22M
  tcmalloc    26.04M

main_r90:
  allocfirst  21.12M
  freefirst   33.05M
  tlslink     30.66M
  tcmalloc    30.43M

mid_only_r0:
  allocfirst  93.51M
  freefirst   92.94M
  tlslink    107.27M
  tcmalloc   226.65M

mid_only_r50:
  allocfirst  30.01M
  freefirst   35.20M
  tlslink     40.68M
  tcmalloc    24.28M

mid_only_r90:
  allocfirst  23.74M
  freefirst   33.25M
  tlslink     31.86M
  tcmalloc    28.61M

cross128_r0:
  allocfirst  55.17M
  freefirst   57.03M
  tlslink     60.20M
  tcmalloc    45.57M

cross128_r50:
  allocfirst  19.60M
  freefirst   20.48M
  tlslink     23.29M
  tcmalloc    21.66M

cross128_r90:
  allocfirst  18.07M
  freefirst   19.89M
  tlslink     17.29M
  tcmalloc    14.37M
```

Read:

```text
tlslink is a strong local/r50 candidate and pushes main/mid_only r0 over 100M,
but r90 still prefers freefirst in main/mid_only/cross128. Do not replace
freefirst as balanced default yet.
```

## Next Diagnostic

```text
MidPageFront / preload path-length upper-bound diagnostics.
```

Purpose:

```text
Measure whether the remaining local-r0 gap is dominated by isolated fixed
costs in the M4 magazine and preload routing path.
```

Rules:

```text
1. Do not mix counters into raw speed runs.
2. Keep unsafe lanes out of candidate/reporting rows.
3. Compare against freefirst-tlslink and tcmalloc first on mid_only/main r0.
4. Treat no-go diagnostics as evidence for the next structural design.
```

Results:

```text
allocelide r0 smoke:
  private/raw-results/linux/midpage_allocelide_r0_smoke_20260525_093736

  main:
    tlslink    105.97M
    allocelide 108.76M
    tcmalloc   220.60M

  mid_only:
    tlslink    100.81M
    allocelide 114.85M
    tcmalloc   225.86M

ptrmag r0 smoke:
  private/raw-results/linux/midpage_ptrmag_r0_smoke_20260525_093901

  main:
    tlslink    103.29M
    ptrmag     104.80M
    tcmalloc   223.57M

  mid_only:
    tlslink    103.15M
    ptrmag     114.58M
    tcmalloc   222.91M

absalloc r0 smoke:
  private/raw-results/linux/midpage_absalloc_r0_smoke_20260525_094053

  main:
    tlslink    100.25M
    absalloc   104.61M
    tcmalloc   220.93M

  mid_only:
    tlslink    107.19M
    absalloc   109.50M
    tcmalloc   225.91M

regcache r0 smoke:
  private/raw-results/linux/midpage_regcache_r0_smoke_20260525_094151

  main:
    tlslink    106.23M
    regcache    97.85M
    tcmalloc   221.30M

  mid_only:
    tlslink    103.86M
    regcache   103.72M
    tcmalloc   224.91M

slotswitch r0 smoke:
  private/raw-results/linux/midpage_slotswitch_r0_smoke_20260525_094257

  main:
    tlslink    102.55M
    slotswitch  97.22M
    tcmalloc   222.33M

  mid_only:
    tlslink    100.71M
    slotswitch 100.00M
    tcmalloc   229.08M
```

Read:

```text
M4 alloc transition, pointer-only magazine pop, SmallFront-before-MidPage
malloc routing, TLS region cache, and slot-index switch each explain only a
small slice. The remaining tcmalloc gap is structural path length: interposer
routing plus descriptor-owned free classification versus tcmalloc's compact
front cache.
```

## Cleanup Pass

Current cleanup target:

```text
Lane and source-code organization before the next remote/cross-size design.
```

Actions:

```text
1. Keep preset names stable for reproducibility.
2. Consolidate repeated build-script preset bodies into helper functions.
3. Record canonical / diagnostic / archive lane status in:
   hakozuna-hz5/docs/HZ5_LINUX_LANE_CLEANUP.md
4. Do not split MidPageFront into multiple translation units yet; the diagnostic
   flag matrix is still moving.
```

Validation:

```text
bash -n linux/build_linux_hz5_standalone.sh
--linux-hz5-general-midpage-region-shadow-allocfirst build OK
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats build OK
--linux-hz5-general-region-outbox build OK
short MidPage malloc smoke OK
```

## Design Check Before Pro Consultation

Raw output:

```text
private/raw-results/linux/midpage_design_check_20260525_073659
```

Size histogram:

```text
mid_only allocation distribution:
  2304..3072      21,190
  3073..4096      21,396
  4097..8192      85,519
  8193..16384    170,952
  16385..32768   342,543

main allocation distribution:
  <=2048          20,042
  2049..32768   301,179
  32769..65536  320,399

cross128 allocation distribution:
  <=2048           9,992
  2049..32768    150,498
  32769..65536   160,700
  >65536         320,430
```

Unsafe local upper-bound:

```text
mid_only_r0:
  allocfirst   72.83M
  localunsafe  76.79M
  tcmalloc    147.13M
```

Read:

```text
Skipping owner-local MidPage bitmap checks gives only about +5%.
The tcmalloc gap is not primarily local active/free bitmap validation.
```

Alloc+free MidPage-first dispatch:

```text
mid_only_r0:
  allocfirst      69.02M
  allocfreefirst  67.02M
  tcmalloc       139.85M

mid_only_r90:
  allocfirst      31.50M
  allocfreefirst  26.17M
  tcmalloc        47.02M

main_r90:
  allocfirst      25.35M
  allocfreefirst  29.39M
  tcmalloc        19.90M

cross128_r90:
  allocfirst      14.67M
  allocfreefirst  14.71M
  tcmalloc         7.80M
```

Read:

```text
MidPage-first free dispatch can help main_r90, but it hurts mid_only.
Dispatch order alone is not the missing tcmalloc-class local design.
```

Perf note:

```text
perf record/report is noisy but shows MidPage r0 still paying substantial
preload ownership dispatch, including smallfront_page_for_ptr, and MidPage
try_alloc/free. tcmalloc still has much lower instruction/branch footprint in
perf-stat runs.
```

## Read First

```text
hakozuna-hz5/docs/HZ5_LINUX_STATUS.md
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
hakozuna-hz5/docs/HZ5_BENCH_RESULTS_INDEX.md
```

Full chronological log was archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux.md
```

## Current Lead Lanes

```text
--linux-hz5-general-midpage-region-shadow
  current tcmalloc-chase lead for general ordinary malloc rows

--linux-hz5-general-midpage-region
  stable MidPageFront-M2.2 remote-heavy mid-size candidate

--linux-hz5-local2p-linkflags
  exact 64K/a8192 appendix/local speed profile

--linux-hz5-local2p-rssretain2048tls
  exact retained-RSS profile

--linux-hz5-local2p-remotebatch
  exact producer/consumer remote-free profile
```

## Diagnostic / No-Go

```text
--linux-hz5-general-midpage-region-frontfirst
  helps pure mid_only r90 in one check, but regresses main/cross128/guard

--linux-hz5-general-midpage-region-shadow-frontfirst
  helps pure mid_only slightly, but hurts main/cross128

--linux-hz5-general-midpage-region-shadow-tlscache
  no-go; region lookup cache does not close the tcmalloc gap

--linux-hz5-general-midpage-region-shadow-hotslot
  no-go; one-entry TLS hot object cache does not improve MidPageFront local path

--linux-hz5-general-midpage-region-shadow-activetrust
  diagnostic only; improves r0 slightly but hurts r90

--linux-hz5-general-midpage-region-shadow-allocfirst
  promising diagnostic; removes duplicate MidPageFront class lookup in preload
  via explicit MidPageFront try-alloc dispatch

--linux-hz5-general-midpage-region-shadow-slotswitch
  no-go; removes variable slot-index division but regresses r90 and does not
  improve r0

--linux-hz5-general-midpage-region-shadow-tlslink
  promising diagnostic; preload-wide initial-exec TLS and speed link flags
  improve main/mid, but cross128_r90 regresses versus allocfirst

--linux-hz5-general-midpage-region-shadow-linkonly
  diagnostic only; speed link flags without initial-exec TLS looked promising
  in r3 but r7 does not beat allocfirst on main_r90/mid_only_r90

--linux-hz5-general-midpage-region-shadow-tlsie
  diagnostic split; initial-exec TLS only helps mid_only_r0 but is weaker on
  remote/cross rows

--linux-hz5-general-midpage-region-shadow-nodeless
  diagnostic only so far; M3 nodeless local page-run cache improves mid_only_r0
  slightly, but remote rows regress
```

## Latest Key Results

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
private/raw-results/linux/tcmalloc_shadow_frontfirst_r3_20260525_033543
private/raw-results/linux/tcmalloc_tlscache_r3_20260525_033858
private/raw-results/linux/midpage_hotslot_smoke_r3_20260525_052514
private/raw-results/linux/midpage_activetrust_smoke_r3_20260525_052755
private/raw-results/linux/midpage_allocfirst_r3_20260525_053010
private/raw-results/linux/midpage_allocfirst_r90_verify_r5_20260525_053037
private/raw-results/linux/midpage_allocfirst_tryalloc_r3_20260525_054204
private/raw-results/linux/midpage_slotswitch_r3_20260525_054521
private/raw-results/linux/midpage_tlslink_r3_20260525_054807
private/raw-results/linux/midpage_tlslink_broad_r3_20260525_054859
private/raw-results/linux/midpage_tlslink_cross128_verify_r5_20260525_055003
private/raw-results/linux/midpage_tls_split_r3_20260525_055155
private/raw-results/linux/midpage_linkonly_broad_r3_20260525_055224
private/raw-results/linux/midpage_linkonly_verify_r7_20260525_055344
private/raw-results/linux/midpage_nodeless_r3_20260525_065654
private/raw-results/linux/midpage_perf_allocfirst_nodeless_20260525_070623
```

Current read:

```text
shadow:
  real tcmalloc-chase candidate

frontfirst:
  dispatch order is not the main bottleneck

tlscache:
  region lookup is not the main bottleneck

hotslot:
  one-entry local object bypass is not enough and hurts r90

activetrust:
  alloc-side remote-bit check is not the main bottleneck; r90 becomes unstable

allocfirst:
  duplicate class lookup matters for mid_only r0
  try-alloc cleanup keeps r0 ahead of shadow and r90 near/slightly ahead

slotswitch:
  slot-index division is not the main bottleneck; fixed-class switch removes
  div but loses to allocfirst

tlslink:
  TLS/linkage overhead is real; removes MidPageFront hot-path __tls_get_addr
  and improves main/mid. cross128_r90 still beats tcmalloc but regresses versus
  allocfirst, so do not make it the broad default yet.

tls split:
  linkonly is not promoted after r7. initial-exec TLS helps r0, link flags can
  help short runs, but neither is a broad tcmalloc-chase lead yet.

nodeless:
  first M3 implementation validates the hypothesis only weakly. It removes
  local node->page/node->next traffic and improves mid_only_r0 by ~5%, but
  remote rows regress; remote drain / partial page handling needs redesign
  before promotion.

perf:
  nodeless reduces cache misses but increases branches/instructions, especially
  on r90. The remaining problem is control-flow/state management, not payload
  cache traffic alone.

stats:
  nodeless stats-only run shows very high refill/partial churn even on r0.
  A single current_page[class] is too small for the random mid workload and
  repeatedly round-trips pages through the partial list.

ptrcache:
  per-class TLS pointer cache fixes most r0 partial/refill churn and gives a
  small r0 gain. It is still diagnostic only: mid_only_r90 and cross128_r90
  remain weaker than allocfirst, while main_r90 can improve.

next:
  move to MidPageFront-M4. The next useful design is a descriptor-owned
  owner-local magazine with canonical slot state, followed by page+bitmask
  remote packets if M4a improves local r0.
```

## Next Step

Implement MidPageFront-M4 as a diagnostic lane. Keep M2/M3 lanes available for
reproduction, but do not promote them.

```text
--linux-hz5-general-midpage-region-shadow-m4mag
```

Design intent:

```text
Keep HZ4's page/header-owned and owner-aware remote principles.
Borrow tcmalloc's per-class front-cache shape.
Keep HZ5 fail-closed descriptor ownership.
Do not mutate Windows P43i/P45 or Local2P.
```

M4a scope:

```text
64KiB slab unchanged
class table unchanged: 3072, 4096, 8192, 16384, 32768
magazine entry: { page, slot }
slot_state2 canonical states: LIVE/CACHE/REMOTE/DEAD
alloc pop: CACHE -> LIVE, no page lookup or slot division
local free: validate ptr -> page -> slot, LIVE -> CACHE, push magazine
remote path: existing object-list handoff is allowed for first M4a smoke
```

Acceptance:

```text
Keep if mid_only_r0 >= 90M or allocfirst best +15%.
No-go if mid_only_r90/main_r90 regress more than 8% or cross128_r90 more than
10%.
Do not mix speed runs with runtime stats/counters.
```

First M4a smoke:

```text
private/raw-results/linux/midpage_m4mag_smoke_20260525_080112
private/raw-results/linux/midpage_m4mag_localfast_smoke_20260525_080203
private/raw-results/linux/midpage_m4mag_broad_smoke_20260525_080228
private/raw-results/linux/midpage_m4mag_attrib_20260525_080241
```

Result:

```text
mid_only_r0:
  allocfirst 93.53M
  m4mag      90.92M
  tcmalloc  230.97M

mid_only_r90:
  allocfirst 23.83M
  m4mag      37.22M
  tcmalloc   26.23M

main_r90:
  allocfirst 23.65M
  m4mag      39.36M
  tcmalloc   29.96M

cross128_r90:
  allocfirst 17.33M
  m4mag      14.98M
  tcmalloc   14.65M
```

Read:

```text
M4 magazine is not yet the local r0 tcmalloc chase answer. It improves
remote-heavy mid/main rows strongly, but cross128 regresses versus allocfirst.
The next M4 work should either add page+bitmask remote packets to remove
object-list remote payloads, or stop local-r0 chasing here and inspect
free-route/classifier fixed cost.
```

First M4b page-packet smoke:

```text
private/raw-results/linux/midpage_m4packet_smoke_20260525_082809
private/raw-results/linux/midpage_m4packet_attrib_20260525_082826
```

Result:

```text
mid_only_r90:
  allocfirst 27.53M
  m4mag      23.25M
  m4packet   36.12M
  tcmalloc   24.31M

main_r90:
  allocfirst 24.33M
  m4mag      34.07M
  m4packet   36.28M
  tcmalloc   31.43M

cross128_r90:
  allocfirst 15.02M
  m4mag      15.94M
  m4packet   16.72M
  tcmalloc   15.60M
```

Read:

```text
M4b page-descriptor packet is a real remote-heavy candidate. It fixes the M4a
cross128_r90 regression in the short smoke and beats tcmalloc on mid_only_r90,
main_r90, and cross128_r90. It still does not address local r0.
```

M4b gated-drain verify:

```text
private/raw-results/linux/midpage_m4packet_gated_smoke_20260525_084328
private/raw-results/linux/midpage_m4packet_gated_verify_r5_20260525_084409
private/raw-results/linux/midpage_m4packet_gated_attrib_20260525_084433
```

Result:

```text
mid_only:
  r0   allocfirst 90.20M, m4packet 88.07M, tcmalloc 221.24M
  r50  allocfirst 30.96M, m4packet 37.34M, tcmalloc  24.01M
  r90  allocfirst 24.79M, m4packet 33.82M, tcmalloc  25.46M

main:
  r0   allocfirst 92.54M, m4packet 88.49M, tcmalloc 210.87M
  r50  allocfirst 29.64M, m4packet 35.28M, tcmalloc  26.30M
  r90  allocfirst 23.77M, m4packet 31.50M, tcmalloc  29.86M

cross128:
  r0   allocfirst 53.77M, m4packet 56.83M, tcmalloc 46.12M
  r50  allocfirst 21.63M, m4packet 23.52M, tcmalloc 21.37M
  r90  allocfirst 18.64M, m4packet 14.62M, tcmalloc 14.68M
```

Read:

```text
Gated M4b is a strong mid/main remote-heavy candidate and beats tcmalloc on
main/mid r50/r90. It is not a local-r0 answer. It is not a broad cross128_r90
default yet because allocfirst remains better there.
```

M4b cross-drain diagnostic:

```text
private/raw-results/linux/midpage_m4packet_crossdrain_smoke_20260525_085453
private/raw-results/linux/midpage_m4packet_crossdrain_r0_smoke_20260525_085539
private/raw-results/linux/midpage_m4packet_crossdrain_attrib_20260525_085513
```

Change:

```text
Add a separate m4packet-crossdrain lane.
Remote packet publish sets a MidPageFront owner/class pending mask.
SmallFront/MidFront/LargeFront alloc miss can opportunistically drain a small
MidPageFront budget without enabling generic OwnerHub-R2/R3.
```

Result:

```text
main r90:
  m4packet    11.02M
  crossdrain  11.52M
  tcmalloc    11.83M

mid_only r90:
  m4packet    11.23M
  crossdrain  12.79M
  tcmalloc    11.92M

cross128 r90:
  m4packet     7.11M
  crossdrain   5.88M
  tcmalloc    12.70M

r0:
  m4packet and crossdrain are similar; both remain far below tcmalloc.
```

Decision:

```text
Cross-drain is useful as a diagnostic signal for mid_only/main r90, but it is
not a broad default. It worsens cross128 r50/r90, likely because other-front
alloc miss drain adds fixed work while cross128 is dominated by cross-size
traffic. Keep m4packet as the better current candidate and treat cross-drain as
a no-promote lane.
```

M4packet free-dispatch A/B:

```text
private/raw-results/linux/midpage_m4packet_freefirst_smoke_20260525_085727
private/raw-results/linux/midpage_m4packet_freefirst_attrib_20260525_085745
```

Result:

```text
main r0:
  m4packet   33.09M
  freefirst  36.33M
  tcmalloc  117.42M

mid_only r0:
  m4packet   35.33M
  freefirst  37.66M
  tcmalloc  102.98M

cross128 r0:
  m4packet   19.24M
  freefirst  19.60M
  tcmalloc   60.62M

main r90:
  m4packet   11.50M
  freefirst  11.73M
  tcmalloc   10.50M

cross128 r90:
  m4packet    6.30M
  freefirst   6.76M
  tcmalloc   10.83M
```

Decision:

```text
MidPageFront-first free dispatch is a cleaner candidate than cross-drain. It
improves local r0 and slightly improves main/cross128 r90 without attribution
regression. It still leaves a large tcmalloc local-r0 gap, so this is an
incremental lane, not the final answer.
```

Latest M3 stats:

```text
private/raw-results/linux/midpage_nodeless_stats_20260525_070957

mid_only_r0:
  refill=463246
  refill_partial_hit=461678
  refill_remote_hit=0
  refill_new_page=1568
  partial_push=463165
  remote_drained=0

mid_only_r90:
  refill=321442
  refill_partial_hit=293643
  refill_remote_hit=8051
  refill_new_page=19748
  partial_push=305066
  remote_drained=525393
```

Latest M3.2 ptrcache:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_r3_20260525_071403

mid_only_r0:
  allocfirst 70.63M
  ptrcache   76.48M
  tcmalloc  139.49M

mid_only_r90:
  allocfirst 35.31M
  ptrcache   25.82M
  tcmalloc   41.78M

main_r90:
  allocfirst 25.23M
  ptrcache   27.15M
  tcmalloc   21.72M

cross128_r90:
  allocfirst 21.58M
  ptrcache   10.26M
  tcmalloc    7.88M
```

## Operating Rules

```text
Do not change Windows P43i/P45 behavior.
Do not promote diagnostic lanes without broad matrix evidence.
Do not count unsupported or libc-passthrough routes as HZ5 wins.
Keep detailed run output under private/raw-results/linux/.
Update HZ5_BENCH_RESULTS_INDEX.md after meaningful measurements.
```

## Lane Combination Cleanup

New source of truth:

```text
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
```

Current MidPage comparison set:

```text
allocfirst:
  local-r0 comparison baseline

m4packet:
  current remote-heavy candidate

m4packet-freefirst:
  cleaner incremental candidate for next matrix

routefree:
  candidate-watch; improves local r0/cross128 r0 but not cross128 r90

tcmalloc:
  target allocator
```

No-promote:

```text
m4packet-crossdrain:
  improves MidPage-heavy r90 rows but hurts cross128 r50/r90

localunsafe:
  unsafe upper bound only

nodeless-stats / ptrcache-stats:
  observation counters only; not speed lanes

OwnerHub-R2/R3:
  historical generic cross-front drain experiments with fixed-cost failures
```

FreeRoute-C1 smoke:

```text
private/raw-results/linux/midpage_m4packet_routefree_smoke_20260525_090830
private/raw-results/linux/midpage_m4packet_routefree_attrib_20260525_090850
```

Result:

```text
main r0:
  m4packet 32.95M, freefirst 33.39M, routefree 36.18M, tcmalloc 116.96M

mid_only r0:
  m4packet 36.14M, freefirst 36.39M, routefree 36.89M, tcmalloc 119.44M

cross128 r0:
  m4packet 19.85M, freefirst 21.19M, routefree 21.68M, tcmalloc 64.50M

cross128 r90:
  m4packet 5.52M, freefirst 6.45M, routefree 6.15M, tcmalloc 15.08M
```

Decision:

```text
Routefree proves free-route ordering still matters, especially local r0 and
large-heavy cross128 r0. It is not the broad answer because cross128 r90 is
below freefirst. Keep it as candidate-watch and run the next RUNS=5 matrix with
allocfirst / m4packet / freefirst / routefree / tcmalloc.
```

RUNS=5 MidPage route matrix:

```text
private/raw-results/linux/midpage_route_matrix_r5_20260525_091054
private/raw-results/linux/midpage_route_matrix_attrib_20260525_091125
```

Result:

```text
main r0:
  allocfirst 35.62M, m4packet 35.79M, freefirst 36.23M, routefree 36.21M, tcmalloc 116.63M

main r90:
  allocfirst  7.78M, m4packet 11.27M, freefirst 11.65M, routefree 11.25M, tcmalloc 10.29M

mid_only r0:
  allocfirst 36.48M, m4packet 36.19M, freefirst 37.57M, routefree 35.64M, tcmalloc 115.21M

mid_only r90:
  allocfirst  6.68M, m4packet 12.44M, freefirst 11.86M, routefree 12.82M, tcmalloc  6.09M

cross128 r0:
  allocfirst 20.50M, m4packet 20.57M, freefirst 20.68M, routefree 20.17M, tcmalloc 61.49M

cross128 r90:
  allocfirst  5.10M, m4packet  6.43M, freefirst  6.57M, routefree  5.78M, tcmalloc 12.17M
```

Decision:

```text
Freefirst is the balanced MidPage lead. Routefree is only a candidate-watch for
mid_only r90. The local-r0 and cross128 gaps to tcmalloc are structural; free
dispatch order is not enough.
```

Perf / worker-audit phase:

```text
private/raw-results/linux/perf_midpage_freefirst_r0_20260525_091532
private/raw-results/linux/perf_midpage_freefirst_tlslink_20260525_091619
private/raw-results/linux/perf_midpage_freefirst_tlslink_record_20260525_091650
```

Read:

```text
mid_only r0 perf stat:
  freefirst cycles 108.72M, instructions 223.67M, branches 48.79M
  tcmalloc   cycles  52.27M, instructions  82.64M, branches 14.59M

perf report:
  freefirst shows visible __tls_get_addr cost plus
  hz5_midpagefront_try_alloc/free hot samples.

freefirst -> tlslink:
  median ops/s 33.51M -> 38.97M
  instructions 221.28M -> 178.47M
  branches 48.83M -> 37.03M
```

Decision:

```text
Add m4packet-freefirst-tlslink as candidate-watch. TLS/linkage overhead is a
real component. It is not enough alone; after broad verification, the next
structural experiment should test descriptor slot_state2 transition cost or a
pointer-only local magazine upper bound.
```
