# Current Task

## Active Goal

HZ5 Linux general allocator work is now targeting `tcmalloc`, not `mimalloc`.

Immediate focus:

```text
MidPageFront local object topology
```

Reason:

```text
shadow is already close to tcmalloc on main r0/r50/r90 and mid_only r90.
The remaining large gap is pure local ordinary mid-size allocation, especially
mid_only_r0.
```

## Branch

```text
codex/hz5-linux-p43-port
```

Latest commits:

```text
f3b287e Add MidPageFront nodeless pointer cache diagnostic
1450c76 Add MidPageFront nodeless stats observation
afc5317 Add MidPageFront nodeless run diagnostic
dbf751e Document MidPageFront M3 nodeless run
3e766ee Add MidPageFront tcmalloc consultation prompt
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
  do not promote nodeless/ptrcache broadly. The next useful design must handle
  remote/cross-size pressure without reusing remote object payload lists or
  destabilizing main_r90.
```

## Next Step

Refine MidPageFront-M3 only as a diagnostic lane. The first nodeless run did
not meet promotion criteria.

```text
--linux-hz5-general-midpage-region-shadow-nodeless
--linux-hz5-general-midpage-region-shadow-nodeless-stats
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats
```

Design intent:

```text
Keep HZ4's page/header-owned and owner-aware remote principles.
Borrow tcmalloc's local run/cache shape.
Keep HZ5 fail-closed descriptor ownership.
Do not mutate Windows P43i/P45 or Local2P.
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
