# HZ5 Linux Lane Cleanup

This is the cleanup-facing view of the Linux HZ5 lane set. It is intentionally
shorter than the historical benchmark logs.

## Cross-Platform Positioning

Linux HZ5 lanes are backend implementations of the shared HZ5 core contract,
not a separate allocator product. The BenchLab-side contract lives at:

```text
docs/hakozuna-hz5-cross-platform-contract.md
```

Use these role mappings when moving results into papers or cross-platform
reports:

```text
hz5-core-speed:
  --linux-local2p-speed-linkflags

hz5-core-rss:
  --linux-local2p-rssretain2048tls

hz5-core-remote:
  --linux-local2p-remotebatch

hz5-general-backend-candidate:
  MidPageFront-C7
  SmallFront-S1 + MidFront-M1 + LargeFront-L1
  PageRun64 / LargeFront 128K profile families
```

Do not collapse these Linux names onto Windows P43i/P45 aliases. Windows
P43i/P45 remains the compatibility/control-plane backend family until a
dedicated Windows port of the Linux Local2P/RemoteBatch/general frontend roles
exists.

## Keep As Reporting / Candidate Rows

```text
MidPageFront-C7 profile family:
  See docs/HZ5_MIDPAGEFRONT_C7_LANES.md for the active strict/coarse/wide32k
  lane matrix. C7 supersedes most older MidPage local-r0 chase diagnostics.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide
  C7 strict baseline: 3K/4K/8K/16K/32K. Keep as the low-waste default
  candidate until a coarse profile passes RSS thresholds.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32
  C7 coarse-pareto candidate. Improves local mixed-mid speed; not promoted
  until r90 RSS is controlled.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32
  C7 coarse-speed candidate. Strong r0 speed profile candidate, not default.

--linux-local2p-speed-linkflags
  Exact 64K/a8192 low-final-RSS local/mixed profile.

--linux-local2p-rssretain2048tls
  Exact 64K/a8192 retained-cache RSS-throughput profile.

--linux-local2p-remotebatch
  Exact 64K/a8192 producer/consumer remote-free profile.

--linux-hz5-general-region-outbox
  Current broad general allocator baseline:
  SmallFront-S1 + MidFront-M1 + LargeFront-L1.

--linux-hz5-general-midpage-region
  MidPageFront-M2 region-array candidate for 2049..32768 ordinary malloc.

--linux-hz5-general-midpage-region-shadow-allocfirst
  Current MidPage tcmalloc-chase lead diagnostic. Keep selectable because most
  later MidPage diagnostics compare against it.

--linux-hz5-general-midpage-region-shadow-m4mag
  MidPageFront-M4 magazine candidate. First smoke improves mid/main r90 but does
  not solve mid_only r0 and regresses cross128_r90 versus allocfirst.

--linux-hz5-general-midpage-region-shadow-m4packet
  MidPageFront-M4b remote packet candidate. Gated-drain RUNS=5 wins
  mid_only/main r50/r90 and cross128 r0/r50; cross128 r90 remains below
  allocfirst and local r0 remains unsolved.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst
  M4packet plus MidPageFront-first preload free dispatch. Cleaner incremental
  candidate than crossdrain: improves local r0 and slightly improves
  main/cross128 r90, but does not close the tcmalloc local-r0 gap.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink
  M4packet-freefirst plus initial-exec TLS and speed link flags. RUNS=5 shows
  strong local/r50 gains and main/mid_only r0 above 100M, but r90 still prefers
  freefirst. Keep as local/r50 candidate-watch, not the balanced default.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-absalloc
  M4packet-freefirst-tlslink plus MidPageFront absolute-first malloc routing.
  Small r0 gain only; keep as diagnostic.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m5hit
  FrontCache-M5a diagnostic: M4/freefirst/tlslink with hit-only MidPage cache
  entries and remote drain moved to miss/refill. No-go for local-r0: RUNS=5
  stayed around 110M mid_only_r0, far below the 135M threshold.

--linux-hz5-general-midpage-region-shadow-m4packet-routefree
  M4packet plus MidPageFront -> LargeFront -> SmallFront -> MidFront free
  dispatch. Candidate-watch: improves local r0 and cross128 r0, but does not
  beat freefirst on cross128 r90.
```

## Keep As Diagnostics

```text
--linux-hz5-general-midpage-region-shadow
  Remote-shadow control for MidPageFront state representation.

--linux-hz5-general-midpage-region-shadow-frontfirst
--linux-hz5-general-midpage-region-frontfirst
  Free-dispatch-order diagnostics.

--linux-hz5-general-midpage-region-shadow-tlscache
--linux-hz5-general-midpage-region-shadow-hotslot
--linux-hz5-general-midpage-region-shadow-activetrust
--linux-hz5-general-midpage-region-shadow-slotswitch
--linux-hz5-general-midpage-region-shadow-tlslink
--linux-hz5-general-midpage-region-shadow-tlsie
--linux-hz5-general-midpage-region-shadow-linkonly
  No-go or non-promoted MidPage diagnostics. Keep for reproducibility, but do
  not use as default candidates.

--linux-hz5-general-midpage-region-shadow-nodeless
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
  M3 diagnostics. They validate partial/refill churn but are not broad leads.

--linux-hz5-general-midpage-region-shadow-nodeless-stats
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats
  Observation-only builds. Never use for speed medians.

--linux-hz5-general-midpage-region-shadow-localunsafe
  Unsafe r0 upper-bound diagnostic. Skips owner-local MidPage bitmap state
  checks. Never use for safety, remote, or paper rows.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-allocelide
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-ptrmag
  Unsafe upper-bound diagnostics for M4 alloc-side slot-state transition and
  pointer-only magazine pop. They do not close the tcmalloc gap and must not be
  used for safety, remote, or paper rows.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast
  Unsafe physical upper-bound diagnostic. Combines pointer magazine,
  alloc-state elision, M5 hit-only, absolute MidPage malloc routing, and a
  preload MidPage fast bypass. Use only for local-r0 ceiling checks; never use
  for safety, remote, RSS, or paper rows. First smoke is no-go for the 150M
  local-r0 upper-bound bar.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide
  Unsafe free-side state upper-bound diagnostic. Skips owner-local M4 free-side
  slot-state transition/check on top of superfast. First smoke is no-go; it
  does not close the tcmalloc local-r0 gap. Retained only as the C7 strict
  speed/RSS baseline because later class-dispersion diagnostics are built on
  this preset.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-wide32k
  C7 speed upper-bound diagnostic. It proves class dispersion is a major direct
  speed limiter, but it is too wasteful and unstable under high working set for
  default or broad speed-profile promotion.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-16-32
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-16-32
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-32
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band16-32
  C7 coarse-pareto diagnostics. Keep for reproducibility; current lead
  candidates are band8/16/32 and band8/32.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-tagfree
  No-go RouteTag wrapper diagnostic. It calls page_for_ptr before free_tagged,
  so it does not remove the actual ownership lookup.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-overarray
  No-go M4 overflow-array diagnostic. Avoiding payload node overflow did not
  beat the baseline.

--linux-hz5-general-midpage-m6-deferred-free-direct
  No-go deferred-free ThreadCache diagnostic. Delaying validation increased
  refill/raw-buffer pressure and did not close the tcmalloc gap.

--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-regcache
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-slotswitch
  M4/freefirst/tlslink combination diagnostics. Latest r0 smokes are no-go or
  neutral; keep for reproducibility only.

--linux-hz5-general-midpage-region-shadow-m4packet-crossdrain
  MidPageFront owner/class pending drain from other-front misses. Diagnostic
  only: improves MidPage-heavy r90 rows but hurts cross128 r50/r90.

--linux-ownerhub-r1
--linux-ownerhub-r2
--linux-ownerhub-r3
  OwnerHub diagnostics. R2/R3 had timeout / fixed-cost issues and are not
  promoted.
```

## Archive / No-Go Interpretation

Do not remove these presets yet because old raw-result directories and docs
refer to them, but treat them as historical:

```text
--linux-local2p-fast
--linux-local2p-fastcookie
--linux-local2p-exactapi
--linux-local2p-slot1
--linux-local2p-free-first
--linux-local2p-freefirst-fastcookie
--linux-local2p-rssretain256
--linux-local2p-rssretain512
--linux-local2p-rssretain1536
--linux-local2p-rssretain2048
--linux-local2p-rssretain2048array
--linux-hz5-general-midfirst
--linux-hz5-general-midcache
--linux-hz5-general-midrbuf
--linux-hz5-general-routesplit
--linux-hz5-general-directfree
--linux-hz5-general-midoutbox
```

Current read:

```text
local2p-fast/freefirst/rssretain variants:
  historical evolution toward the three kept Local2P rows. Keep for
  reproducibility, but do not add more public aliases.

midfirst/midcache:
  dispatch and lookup-cache changes are not enough.

midrbuf/midoutbox:
  selected cross-size signals but not broad defaults.

routesplit:
  explicit no-go. Moving 8K..64K into LargeFront is too heavy.

directfree:
  state-machine diagnostic, not a broad candidate.
```

## Build Script Organization

`linux/build_linux_hz5_standalone.sh` now uses helper functions for the repeated
general/MidPage preset bases:

```text
enable_general_region_outbox_base
enable_midpage_region_base
enable_midpage_region_shadow_base
enable_midpage_shadow_allocfirst_base
enable_midpage_nodeless_base
enable_midpage_m4mag_base
```

Rules for new lanes:

```text
1. Add a helper only when two or more presets share the same base.
2. Keep old preset names stable while raw-results and docs reference them.
3. Use `*-stats` only for observation counters; never mix stats into speed
   medians.
4. Prefer adding one delta flag to a base helper over copying a full preset
   block.
5. If a lane is no-go, document it here and in HZ5_BENCH_RESULTS_INDEX.md
   rather than deleting the preset immediately.
```

For exact route/profile naming, use:

```text
docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

For allowed combinations, use:

```text
docs/HZ5_LINUX_LANE_COMBINATIONS.md
```

## Source Cleanup Boundaries

Low-risk:

```text
midpagefront:
  keep M2/M3 diagnostics under compile flags
  keep nodeless stats isolated behind NODELESS_STATS
  add helpers when a path is shared by multiple diagnostics

build script:
  consolidate repeated preset flag bundles
  keep compatibility aliases
```

Avoid for now:

```text
Splitting hz5_midpagefront.c into multiple translation units while the
diagnostic flags are still changing. The compile-time matrix is already dense;
file splitting before lane freeze would make A/B verification harder.
```
