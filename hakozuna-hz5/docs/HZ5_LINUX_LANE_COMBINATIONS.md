# HZ5 Linux Lane Combinations

This document is the short map for which Linux HZ5 lanes can be combined, which
ones are comparison baselines, and which ones should not be promoted.

## Rules

```text
1. Keep exact Local2P profiles separate from general malloc profiles.
2. Keep speed lanes free of runtime counters.
3. Do not combine unsafe diagnostics with candidate rows.
4. Do not combine OwnerHub-R2/R3 with MidPage M4 unless the lane explicitly
   says so; previous generic OwnerHub attempts had fixed-cost failures.
5. Treat unsupported/libc passthrough as attribution failure, not a HZ5 win.
```

## Exact 64K/a8192 Profiles

These are standalone appendix profiles. Do not mix them with general malloc
front-end claims.

| Preset | Components | Status |
| --- | --- | --- |
| `--linux-local2p-speed-linkflags` | Local2P fast exact route + speed link flags | accepted exact local/mixed speed profile |
| `--linux-local2p-rssretain2048tls` | Local2P retained-cache TLS profile | accepted retained RSS-throughput profile |
| `--linux-local2p-remotebatch` | Local2P remote batch profile | accepted producer/consumer remote-free profile |

## General Malloc Baseline

| Preset | Components | Status |
| --- | --- | --- |
| `--linux-hz5-general-region-outbox` | SmallFront-S1 + MidFront-M1 rb16 + LargeFront-L1 region/inbox | broad general baseline |

Use this as the broad ordinary malloc baseline when a MidPage experiment is not
being tested.

## Saved PageRun64 Profile Aliases

These aliases are the current HZ5 Linux profile family. Prefer them in new
scripts and reports instead of copying the long historical flag chains.

### Human-Facing LargeFront Names

Use these names in new benchmark command lines and reports. The historical
`pagerun64-large128-*` aliases remain available for reproducibility.

| Human name | Build alias | Runner allocator | Meaning | Status |
| --- | --- | --- | --- | --- |
| `large128-rss` | `--linux-hz5-profile-large128-rss` | `hz5-large128-rss` | saved source-batch4 profile; low RSS large128 baseline | saved fixed profile |
| `large128-source16` | `--linux-hz5-profile-large128-source16` | `hz5-large128-source16` | source-batch16 throughput diagnostic | diagnostic only |
| `large128-r50-drain` | `--linux-hz5-profile-large128-r50-drain` | `hz5-large128-r50-drain` | source16 + drain budget 1; r50 drain diagnostic | diagnostic only |
| `large128-r50-hold` | `--linux-hz5-profile-large128-r50-hold` | `hz5-large128-r50-hold` | source16 + drain budget 1 + RemoteHold cap4; r50 candidate/diagnostic | diagnostic only |
| `large128-r50-hold8` | `--linux-hz5-profile-large128-r50-hold8` | `hz5-large128-r50-hold8` | source16 + drain budget 1 + RemoteHold cap8; wider r50 diagnostic | diagnostic only |
| `large128-global-remote` | `--linux-hz5-profile-large128-global-remote` | `hz5-large128-global-remote` | 128K remote frees go to global recycle instead of owner inbox | diagnostic only |
| `large128-remote-first` | `--linux-hz5-profile-large128-remote-first` | `hz5-large128-remote-first` | source16 + owner inbox drain before local free-list reuse | diagnostic only |
| `large128-remote-first-gated` | `--linux-hz5-profile-large128-remote-first-gated` | `hz5-large128-remote-first-gated` | source16 + owner inbox drain before local reuse only if inbox nonempty | diagnostic only |
| `large128-chunk16` | `--linux-hz5-profile-large128-chunk16` | `hz5-large128-chunk16` | source16 + remote batch16 using 128K owner chunk inbox | diagnostic only |
| `large128-policy-l7` | `--linux-hz5-profile-large128-policy-l7` | `hz5-large128-policy-l7` | first remainder-size rule policy; no-go diagnostic | diagnostic only |
| `large128-policy-l8-shadow` | `--linux-hz5-profile-large128-policy-l8-shadow` | `hz5-large128-policy-l8-shadow` | Policy-L0 plus owner-drain shadow classification; no behavior change | observation only |

Naming rule:

```text
saved profiles:
  large128-rss

source/refill diagnostics:
  large128-source16

r50-specific diagnostics:
  large128-r50-*

policy experiments:
  large128-policy-*
```

| Alias | Components | Primary rows | Status |
| --- | --- | --- | --- |
| `--linux-hz5-profile-pagerun64-main` | PageRun64 + M6 remote + empty retain cap 4096 | main / mid_only / cross64 | default candidate |
| `--linux-hz5-profile-pagerun64-cross128` | PageRun64 + LargeFront takefirst + source batch16 + exact-base LargeFront map + empty retain cap 4096 | cross128 remote-heavy | saved fixed profile |
| `--linux-hz5-profile-pagerun64-large128` | PageRun64 + LargeFront takefirst + source batch4 + exact-base LargeFront map + Large-first free route + empty retain cap 4096 | large128 remote-heavy | saved fixed profile |
| `--linux-hz5-profile-pagerun64-large128-batch8` | same as `large128` but source batch8 | large128 r50/r90 source-batch diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-batch16` | same as `large128` but source batch16 | large128 r50/r90 source-batch diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-b16-drain1` | same as `large128-batch16` but alloc-miss drain local budget 1 | large128 r50 drain-budget diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-b16-drain1-hold8` | same as `large128-batch16` but drain budget 1 + RemoteHold cap8 | large128 r50 hold-width diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-b16-rb32` | same as `large128-batch16` but LargeFront remote batch enabled, cap 32 | large128 remote publish diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-b16-rb64` | same as `large128-batch16` but LargeFront remote batch enabled, cap 64 | large128 remote publish diagnostic | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-policy-l0` | saved large128 profile + slow-path LargeFront Policy-L0 counters | control-plane feature observation | observation only |
| `--linux-hz5-profile-pagerun64-large128-policy-l1a` | PageRun64 + LargeFront takefirst + 128K source batch selector 4/8/16 | source-batch policy candidate | diagnostic only |
| `--linux-hz5-profile-pagerun64-large128-policy-l1b` | policy-l1a + 128K remote batch cap selector 16/32/64 | source + remote-cap policy candidate | diagnostic only |

Use the fixed cross128 and large128 aliases as separate profiles. The source
batch optimum reverses between those workloads, so a single fixed value is not
the current design claim.

Latest large128 focused check:

```text
private/raw-results/linux/hz5_large128_largefirst_fastmap_r5

large128 r0:
  improved strongly after exact-base lookup + Large-first free route.

large128 r90:
  HZ5 wins t=8 with lower RSS than tcmalloc/HZ4.

large128 r50:
  still the next LargeFront gap.
```

Use `hz5-large128-rss`, `hz5-large128-source16`,
`hz5-large128-r50-drain`, `hz5-large128-r50-hold`, and
`hz5-large128-r50-hold8` in new `linux/run_hz5_hakmem_compare.sh`
invocations. Keep the older `hz5-pagerun64-large128-*` names only when
reproducing old result directories. Do not include diagnostics in broad default
matrices unless they replace the saved batch4 profile.

L4 first read:

```text
source batch sweep:
  private/raw-results/linux/hz5_large128_l4_batch_sweep_r3
  private/raw-results/linux/hz5_large128_l4_batch16_confirm_r5
  private/raw-results/linux/hz5_large128_l4_batch16_r0_r5

drain budget:
  private/raw-results/linux/hz5_large128_l4_drain1_r3

decision:
  batch16 remains high-remote/high-thread candidate-watch.
  batch8 is not clear.
  b16-drain1 is no-go for broad promotion.
  saved large128 stays batch4 until a lane improves r50/r90 without losing
  t=8 r50 or r0.
```

Remote batch cap sweep:

```text
private/raw-results/linux/hz5_large128_l4_remote_batch_cap_r3
private/raw-results/linux/hz5_large128_l4_remote_batch_cap_fixed_r3

decision:
  first result invalidated because the aliases did not enable LargeFront remote
  batching.
  fixed rerun keeps rb32/rb64 as phase-sensitive diagnostics:
    rb64 wins t=4/t=8 r90.
    rb32 wins t=8 r50.
    lower-thread r50 remains weak.
```

Policy-L0 observation:

```text
Use:
  --linux-hz5-profile-pagerun64-large128-policy-l0
  HZ5_LARGEFRONT_POLICY_L0=1

Purpose:
  collect source/refill/remote/drain features for a future L1 selector.

Rule:
  observation only.
  no speed medians.
  no malloc/free hot-path counter updates.

Smoke:
  private/raw-results/linux/hz5_policy_l0_smoke.log
```

Policy-L1a candidate:

```text
Use:
  --linux-hz5-profile-pagerun64-large128-policy-l1a

Purpose:
  choose LargeFront 128K source refill batch 4/8/16 on source refill slow path.

Limits:
  no remote batch cap adaptation.
  no malloc/free hot-path policy state.
  diagnostic until it beats fixed split rows without broad regressions.
```

Policy-L1b candidate:

```text
Use:
  --linux-hz5-profile-pagerun64-large128-policy-l1b

Purpose:
  add 128K remote batch cap adaptation on top of Policy-L1a.

Limits:
  cap is TLS-cached and updates only at remote flush boundaries.
  diagnostic until low-thread rows are safe.
```

Policy-L8 shadow classifier:

```text
Use:
  --linux-hz5-profile-large128-policy-l8-shadow

Purpose:
  observe whether owner-drain slow-path shape separates r50-like and r90-like
  phases before another runtime policy is attempted.

Counters:
  l8_heavy_drain
  l8_sparse_drain
  l8_local_like
  l8_hold_like
  l8_republish_like
  l8_mixed_like

Rule:
  observation only.
  no behavior change.
  no malloc/free hot-path counter updates.
```

## MidPageFront Combination Chain

MidPageFront is the current tcmalloc chase track for ordinary malloc
`2049..32768`.

| Preset | Adds | Primary use | Status |
| --- | --- | --- | --- |
| `--linux-hz5-general-midpage-region` | MidPageFront-M2 region-array slabs | M2 stable control | candidate/control |
| `--linux-hz5-general-midpage-region-shadow` | remote-shadow + owner-local fast state | state representation control | control |
| `--linux-hz5-general-midpage-region-shadow-allocfirst` | MidPage try-alloc before can-handle dispatch | local-r0 comparison baseline | keep |
| `--linux-hz5-general-midpage-region-shadow-m4mag` | descriptor-owned owner-local magazine | M4 magazine A/B | remote-heavy candidate, not broad default |
| `--linux-hz5-general-midpage-region-shadow-m4packet` | page-descriptor remote packet | M4b remote handoff | current remote-heavy candidate |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst` | M4packet + MidPage-first preload free dispatch | balanced MidPage candidate | current lead |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink` | M4packet-freefirst + initial-exec TLS/speed link flags | TLS overhead reduction | local-r0 candidate-watch |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-absalloc` | freefirst-tlslink + MidPage absolute-first malloc routing | preload routing diagnostic | small r0 gain only |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-regcache` | freefirst-tlslink + MidPage TLS region lookup cache | free classification diagnostic | no-go |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-slotswitch` | freefirst-tlslink + fixed-class slot-index dispatch | slot arithmetic diagnostic | no-go |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m5hit` | freefirst-tlslink + FrontCache-M5a hit-only MidPage cache | front-cache contract diagnostic | no-go for local-r0 |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast` | freefirst-tlslink + unsafe pointer magazine, alloc elision, hit-only, and preload fast bypass | local-r0 physical upper bound | no-go; unsafe diagnostic only |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide` | superfast + unsafe owner-local free-side slot-state elision | free-side state upper bound | no-go; unsafe diagnostic only |
| `--linux-hz5-general-midpage-region-shadow-m4packet-routefree` | M4packet + MidPage/Large/Small/Mid free dispatch | free-route fixed-cost diagnostic | mid_only r90 candidate-watch; not broad default |
| `--linux-hz5-general-midpage-region-shadow-m4packet-crossdrain` | M4packet + MidPage pending drain from other-front misses | fixed-cost diagnostic | no-promote |

Current comparison set for MidPage work:

```text
allocfirst
m4packet
m4packet-freefirst
m4packet-freefirst-tlslink
routefree
tcmalloc
```

After RUNS=5, use `m4packet-freefirst` as the balanced lead. Add
`m4packet-freefirst-tlslink` when testing local-r0 path length. Add `routefree`
only when testing MidPage-heavy r90 or free-route ordering. Add
`m4packet-crossdrain` only when explicitly testing cross-front drain policy.
Do not include crossdrain in broad candidate matrices by default.

## Diagnostic-Only MidPage Lanes

| Preset | Reason |
| --- | --- |
| `--linux-hz5-general-midpage-region-shadow-localunsafe` | unsafe upper bound only; not safety-preserving |
| `--linux-hz5-general-midpage-region-shadow-slotswitch` | slot arithmetic was not the main bottleneck |
| `--linux-hz5-general-midpage-region-shadow-tlscache` | region lookup cache did not close the gap |
| `--linux-hz5-general-midpage-region-shadow-hotslot` | one-entry object cache was too narrow |
| `--linux-hz5-general-midpage-region-shadow-nodeless` | M3 run-cache diagnostic, not broad lead |
| `--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache` | M3 ptrcache diagnostic, not broad lead |
| `--linux-hz5-general-midpage-region-shadow-nodeless-stats` | observation only; contains counters |
| `--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats` | observation only; contains counters |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-allocelide` | unsafe M4 alloc-state upper bound only |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-ptrmag` | unsafe pointer-only M4 magazine upper bound only |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast` | unsafe physical upper-bound lane; no-go for 150M local-r0 bar |
| `--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide` | unsafe free-side state upper-bound lane; no-go |

## Do Not Combine

```text
localunsafe + remote/paper rows:
  unsafe state shortcut; not a reportable allocator profile

nodeless-stats / ptrcache-stats + speed medians:
  runtime atomic counters contaminate timing

routesplit + current MidPage M4 claims:
  routesplit was a no-go and moves 8K..64K into a heavy LargeFront path

OwnerHub-R2/R3 + MidPage M4 candidate:
  previous generic OwnerHub variants had timeout/fixed-cost failures

m4packet-crossdrain + broad default:
  improves MidPage-heavy r90 rows but hurts cross128 r50/r90

routefree + broad default:
  routefree is useful for mid_only r90, but RUNS=5 keeps freefirst as the
  balanced lead.

tlslink + final claim:
  broad RUNS=5 confirms local/r50 gains, but main/mid_only/cross128 r90 still
  prefer freefirst. Keep as a local/r50 candidate-watch.

allocelide / ptrmag + paper rows:
  unsafe upper-bound diagnostics. They weaken slot-state attribution and must
  not be used as reportable allocator profiles.

regcache / slotswitch + default:
  latest M4/freefirst/tlslink smokes did not improve local-r0. Keep for
  reproducibility only.
```

## Next Matrix

Use this small set before designing another allocator change:

```text
lanes:
  --linux-hz5-profile-pagerun64-main
  --linux-hz5-profile-pagerun64-cross128
  --linux-hz5-profile-large128-rss
  optional: --linux-hz5-profile-large128-source16
  optional: --linux-hz5-profile-large128-r50-hold
  HZ4
  tcmalloc
  mimalloc
  system

workloads:
  main r0/r50/r90
  mid_only r0/r50/r90
  cross128 r0/r50/r90
  large128 r0/r50/r90

policy:
  RUNS=5 minimum
  HZ5_PRELOAD_STATS unset for timing
  separate attribution smoke with HZ5_PRELOAD_STATS=1
```

Only add diagnostic HZ5 lanes when the question is specific. `adaptive128`,
payload scavenging, observe, no-go M4/M5/M6 variants, and stats lanes must stay
out of reportable speed medians.
