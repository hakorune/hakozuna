# HZ8 Current Task

Updated: 2026-07-14

## Restart Surface

```text
public default:
  HZ8 / Mag16 / KeepRefill / GeneralMediumPage + EntryBoundary

rollback:
  hz8-v2-rollback

active behavior box:
  SmallTransitionInventory-L1 research sibling
  L1-A local behavior and L1-B normal-owner remote collect are implemented
  correctness smoke GO; performance/default gate pending

active diagnostic box:
  none

latest diagnostic closeout:
  SmallHotPathAudit-L0 = complete
  64/128B warmed pairs are about 17% cheaper than tcmalloc
  HZ11 fine128 is about 45% cheaper but has a weaker contract
  SmallEntryTrim-L1 = NO-GO
  SmallMixedTransitionAttribution-L0 = complete
  LCG weakness = class-local empty inventory -> span commit (existing P1 evidence)
  xorshift weakness = 15-21% Mag pop/hit churn, not source commit

next gate:
  SmallTransitionInventory-L1
  paired fresh-process default/candidate LCG and xorshift rows
  fixed-small and remote controls
  Windows first, then native Linux confirmation

latest closeout:
  SmallPartialTransitionDepot P1 is research GO / default HOLD
  SmallTierMembership is mechanism GO / default NO-GO
  P2/P3/P4 and further capacity/policy ladders are closed
  require a newly measured public-default weakness before reopening behavior
```

## Current Lane Map

| Bucket | Lane | Disposition |
|---|---|---|
| selected | `hz8` | shared Linux/Windows public default |
| rollback | `hz8-v2-rollback` | explicit comparison and emergency rollback |
| research | `hz8-v2-mag32` | larger/local opt-in; global default HOLD |
| research | `hz8-small-partial-transition-only` | P1 recovery evidence; default HOLD |
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
  broad balanced/wide/larger throughput remains below tcmalloc
  long mixed traces can produce high peak RSS

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
