# HZ8 Current Task

Updated: 2026-07-14

## Restart Surface

```text
public default:
  HZ8 / SmallTransitionInventory / KeepRefill
  / GeneralMediumPage + EntryBoundary

rollback:
  hz8-pre-transition-rollback
  hz8-v2-rollback

active behavior box:
  MediumTransitionInventory-L1
  cross-platform research GO / shared-default HOLD
  transition-only owner/class inventory replaces deep owner scans
  no production counters or hot-path atomics

active diagnostic box:
  MediumBoundary-L0 complete
  SmallTransitionInventory exonerated
  4097..8192 baseline owner scan averaged 127 steps
  L1 reduces owner scan steps 96.6%; inventory 29,464/29,464 hits
  inventory safety/mismatch counters and exit residue are zero

latest diagnostic closeout:
  SmallHotPathAudit-L0 = complete
  64/128B warmed pairs are about 17% cheaper than tcmalloc
  HZ11 fine128 is about 45% cheaper but has a weaker contract
  SmallEntryTrim-L1 = NO-GO
  SmallMixedTransitionAttribution-L0 = complete
  LCG weakness = class-local empty inventory -> span commit (existing P1 evidence)
  xorshift weakness = 15-21% Mag pop/hit churn, not source commit

latest promotion:
  SmallTransitionInventory-L1
  Linux xorshift/LCG/Redis and trim-control gates pass
  Windows matched-remote Blocks=20 passes 40/40 admissible pairs
  paired throughput +1.63%, post/peak +0.16%, private -0.86%

latest broad check:
  Windows RUNS=1 full matrix + MT remote + Redis-like complete
  balanced/wide/heavy_mixed beat tcmalloc in this run
  fixed 512B..4KiB is strong; 4KiB..64KiB transition and direct-large are weak
  MT remote uses about 1/41 tcmalloc peak RSS at 53.7% throughput

latest medium candidate:
  MediumTransitionInventory-L1 is cross-platform research GO
  Windows WorkScale=10 paired R10 retains large medium gains and RSS gates
  type-stable owner metadata removes the non-medium Redis-like layout tax
  Windows fixed8K repeats disagree: -1.98% then -5.07%
  shared default remains HOLD because the -3% fixed control is not robust

latest Linux medium gate:
  GCC/Clang preload, smoke, safety PASS
  native AB/BA R10: 4097..8192 +157.49%, fixed64K +38.35%
  fixed8/16/32 -1.57%/+1.05%/+2.10%; balanced +0.64%, wide -2.19%
  post/peak RSS and all inventory zero-invariants PASS
  Windows fixed8K -4.50% still blocks shared-default promotion

latest closeout:
  SmallPartialTransitionDepot P1 is research GO / default HOLD
  SmallTierMembership is mechanism GO / default NO-GO
  P2/P3/P4 and further capacity/policy ladders are closed
  require a newly measured public-default weakness before reopening behavior
```

## Current Lane Map

| Bucket | Lane | Disposition |
|---|---|---|
| selected | `hz8` | shared default with SmallTransitionInventory-L1 |
| rollback | `hz8-pre-transition-rollback` | immediate pre-promotion Mag16 control |
| rollback | `hz8-v2-rollback` | explicit comparison and emergency rollback |
| research | `hz8-v2-mag32` | larger/local opt-in; global default HOLD |
| research | `hz8-medium-transition-inventory` | cross-platform medium GO; shared default HOLD on Windows fixed8K variance |
| research | `hz8-small-partial-transition-only` | P1 recovery evidence; default HOLD |
| compatibility | `hz8-small-transition-inventory` | alias of the promoted default |
| research | `LargeDirectOwned` | cross128 profile evidence; not default |
| diagnostic | Page8K/domain/stats shadows | counter-bearing evidence; never speed rows |
| archive | P2/P3/P4, tier membership, owner witness, old cache ladders | NO-GO reproduction only |

Normal Windows runners list only `hz8`. Research rows require
`-IncludeHz8Research`; counter-bearing MT rows additionally require
`-IncludeDiagnostics`. Linux research artifacts use explicit Make targets.

## Fixed Decisions

```text
GeneralMediumPage + EntryBoundary:
  promoted on Linux and Windows
  previous isolated HOLD results are superseded precursor gates

Mag16:
  promoted shared default

Mag32:
  Windows/larger-local opt-in
  small and remote controls block a shared default

SmallPartialTransitionDepot:
  original depot: Windows positive, Linux control negative
  P1 TransitionOnly: cross-platform research GO
  xorshift controls block default promotion
  Redis-like Windows R5 is neutral-to-positive

SmallTierMembership:
  O(1) mechanism works
  xorshift controls fail
  default NO-GO; focused reproduction only

reclaim integration:
  HZ12 retirement contract remains separate evidence
  HZ8 reclaim-adapter behavior is closed
```

## Measured Frontier

```text
strength:
  balanced low post-RSS allocator
  remote-small can reach tcmalloc-class throughput with much lower RSS
  exact 8K/16K/32K improved materially after GeneralMediumPage promotion

remaining measured weakness:
  variable 4KiB..8KiB and fixed 32KiB..64KiB have a strong research fix,
  but the Windows fixed8K control is not robust enough for default promotion
  direct 128KiB..4MiB throughput
  Redis-like mutation throughput
  long mixed traces can still produce high peak RSS

next gate:
  no further policy/capacity ladder for MediumTransitionInventory-L1
  reopen shared-default review only with a stable Windows fixed8K explanation
  keep Redis-like candidate selection available as an application-like control

claim boundary:
  do not claim universal tcmalloc replacement
  do not reopen a closed lane from a microbenchmark-only signal
```

## Reopen Conditions

Open a new HZ8 behavior box only when all are present:

1. A fresh public-default weakness reproduces in paired/fresh-process runs.
2. The weakness has path or counter attribution outside the speed binary.
3. The proposal has one narrow mechanism and explicit safety/RSS gates.
4. Linux and Windows implications are stated before implementation.
5. Existing research or archive lanes do not already answer the question.

## Read First

- [Public position](README.md)
- [Documentation index](docs/README.md)
- [Windows lane registry](docs/HZ8_WINDOWS_LANE_STATUS_L1.md)
- [Linux lane registry](docs/HZ8_LINUX_LANE_STATUS_L1.md)
- [Default integration record](docs/HZ8_GENERAL_MEDIUM_DEFAULT_INTEGRATION_L1.md)
- [P1 recovery closeout](docs/HZ8_SMALL_PARTIAL_TRANSITION_DEPOT_L1.md)
- [Small hot-path audit](docs/HZ8_SMALL_HOT_PATH_AUDIT_L0.md)
- [Small mixed transition attribution](docs/HZ8_SMALL_MIXED_TRANSITION_ATTRIBUTION_L0.md)
- [Small transition inventory design](docs/HZ8_SMALL_TRANSITION_INVENTORY_L1.md)
- [Source module map](src/README.md)
- [Benchmark gate](docs/HZ8_BENCH_GATE.md)

## Maintenance Rules

```text
Keep this file restart-sized; chronology belongs in dedicated decision docs.
Build-target existence is not a support or promotion claim.
Keep diagnostic atomics out of speed/default binaries.
Preserve MISS / VALID / INVALID and generation fail-closed behavior.
```
