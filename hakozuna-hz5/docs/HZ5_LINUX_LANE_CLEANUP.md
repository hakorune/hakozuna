# HZ5 Linux Lane Cleanup

This is the cleanup-facing view of the Linux HZ5 lane set. It is intentionally
shorter than the historical benchmark logs.

## Keep As Reporting / Candidate Rows

```text
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
