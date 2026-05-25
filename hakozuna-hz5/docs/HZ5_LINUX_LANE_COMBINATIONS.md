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

| Alias | Components | Primary rows | Status |
| --- | --- | --- | --- |
| `--linux-hz5-profile-pagerun64-main` | PageRun64 + M6 remote + empty retain cap 4096 | main / mid_only / cross64 | default candidate |
| `--linux-hz5-profile-pagerun64-cross128` | PageRun64 + LargeFront takefirst + source batch16 + empty retain cap 4096 | cross128 remote-heavy | saved fixed profile |
| `--linux-hz5-profile-pagerun64-large128` | PageRun64 + LargeFront takefirst + source batch4 + empty retain cap 4096 | large128 remote-heavy | saved fixed profile |

Use the fixed cross128 and large128 aliases as separate profiles. The source
batch optimum reverses between those workloads, so a single fixed value is not
the current design claim.

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
  --linux-hz5-profile-pagerun64-large128
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
