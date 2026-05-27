# Hakozuna HZ5

HZ5 is a sidecar allocator prototype that starts from a page/run-first design.
It lives next to HZ3 (`hakozuna/`) and HZ4 (`hakozuna-mt/`) so the HZ3 S276
default can remain stable while HZ5 explores a different classification model.

## Archive / DOI

- Zenodo record (3.5-hz5): https://zenodo.org/records/20411598
- DOI (3.5-hz5): https://doi.org/10.5281/zenodo.20411598
- All-version DOI for the HZ5 sidecar allocator series:
  https://doi.org/10.5281/zenodo.20411597

## Direction

HZ5 is intended to combine:

- HZ4-style page/run metadata and address-mask classification.
- HZ3 S276 as the true-large fallback.
- HZ3/HZ4 Windows build, diagnostic, and benchmark infrastructure.

The core rule is:

```text
Small and over-aligned objects are page/run objects, not large objects.
True large keeps S276.
Metadata lives with the segment, not in a direct side-map.
Remote free is page-oriented.
RSS policy is designed in from day one.
```

## Current Status

HZ5 has moved past the initial P0/P3 route proof. On Linux, the current
paper-facing status is a profile family with two scopes:

1. exact `64K/a8192` Local2P appendix profiles for explicit route
   specialization
2. full-preload general profiles that emphasize low RSS, fail-closed ownership,
   and descriptor-owned front-ends

- `hz5-local2p-linkflags`: low-final-RSS local/mixed exact speed profile.
- `hz5-local2p-rssretain2048tls`: retained-cache RSS-throughput profile.
- `hz5-local2p-remotebatch`: producer/consumer remote-free profile.
- `hz5-pagerun64-main` / `hz5-pagerun64-cross128`: MidPage PageRun64 general
  and cross-size profiles.
- `hz5-large128-rss`: low-RSS LargeFront profile.
- `hz5-large128-source16`: broad LargeFront 128K throughput comparison lane.
- `hz5-large128-transfer128`: transfer-cache diagnostic lane, not a default.

These Linux profiles are deliberately separate. HZ5 should be presented as a
low-RSS, fail-closed, descriptor-owned profile family, not as one universal
tcmalloc replacement. Unsupported routes fail closed, and diagnostic lanes stay
out of the paper-facing default unless they beat the active reporting row under
the stated stop rules.

HZ6 is the possible future branch if this work continues toward a
transfer-first allocator: class transfer caches and RSS governance would become
primary design boxes instead of diagnostics layered on top of owner inboxes.

Implementation split:

- `api`: public HZ5 allocator API. It exports `hz5_malloc()`,
  `hz5_aligned_alloc()`, `hz5_free()`, `hz5_owns()`, and
  `hz5_allocator_descriptor_v1()`, so BenchLab can be a thin ABI shim instead
  of the allocator policy owner.
- `contract`: SpeedLane descriptor ABI, feature mask, forbidden mask, and
  compile-time purity checks now live in `contract/`. BenchLab still exports
  the required symbol, but descriptor contents are HZ5-owned.
- `legacy`: historical P17/P24 raw-cache diagnostic lanes now live in
  `legacy/`. They are still available through `Hz5PolicyHooks`, but are no
  longer part of the main adapter body.
- `P11/P9`: current HZ5 seed speed lane. Exact `4K/8K align=8192` route to the
  HZ5 page/run sidecar; exact `64K align=8192` stays on HZ3 fallback.
- `P12 cap1`: diagnostic/no-go evidence lane for exact `64K align=8192` as a
  separate 16-page run family with `owner_cache_cap=1`; repeat-5/repeat-10
  follow-up showed it below P11 fallback and HZ3 speed-default.
- `P13`: diagnostic segment accounting and shutdown-only release snapshots.
- `P14b`: diagnostic retired quarantine. Empty segments are retired and skipped
  for allocation, but memory remains valid until shutdown release.
- `P25/P27/P28`: exact `64K align=8192` low-page raw cache experiments. The
  low-page cache body now lives in `lowpage/` so BenchLab adapters can stay
  thin while HZ5 keeps the HZ4-inspired mechanism inside the sidecar tree.
- `P43`: segment-backed lowpage64 slot source. The P43 segment allocator now
  lives in `lowpage/hz5_lowpage64_p43_segment.c` with its own config header and
  stats snapshot API, keeping the monolithic lowpage cache body ready for the
  next full SlotRef / descriptor-owned list pass.
- `P20`: lazy HZ3 fallback. The LoadLibrary/GetProcAddress fallback loader,
  fallback state machine, and one-shot counters now live in `fallback/`, so the
  BenchLab adapter only keeps pointer-origin wrapper dispatch.
- `route`: exact a8192 route policy, HZ3 medium fallback eligibility, and P12
  route-shadow counters now live in `route/`, so BenchLab adapters call into
  HZ5 for route decisions instead of owning the policy locally.
- `wrapper`: wrapper header layout, cookie generation, initialization, and
  guarded decode now live in `wrapper/`, while the BenchLab adapter keeps only
  source-specific dispatch to the current free paths.
- `policy`: the first HZ5-native allocation/free policy API. It owns route
  ordering, P25 lowpage dispatch, HZ5 P2 dispatch, lazy HZ3 fallback dispatch,
  and wrapper-source free ordering. Legacy P17/P24 raw-cache diagnostics remain
  adapter hooks.

Linux Local2P notes:

- `Local2P` is a Linux-only exact `64K/a8192` path. It is closer to an HZ4-like
  TLS span-cache profile than to a direct Windows P43i/P45 port.
- `P25` remains the Linux HZ5 control path for exact lowpage64 behavior.
- P43 token/source lanes remain remote/RSS/source-control candidates and are not
  the local-only speed default.
- Current benchmark entrypoint: `linux/run_linux_hz5_local2p_focus.sh`.

Linux general allocator notes:

- `SmallFront`, `MidPageFront`, `MidFront`, and `LargeFront` are experimental
  Linux full-preload front-ends. They are separate from the exact 64K Local2P
  appendix profiles.
- `MidPageFront-C8 PageRun64` is the current strong keep for ordinary malloc
  through 64K. It fixes the old `32769..65536` route gap and keeps main,
  mid_only, and cross64 rows healthy.
- `LargeFront` remains the active design target for 128K remote/free behavior.
  The current evidence favors saved fixed profiles over the first
  mapped-bytes-only adaptive source-batch policy.
- Current Linux profile registry:
  `docs/HZ5_LINUX_PROFILE_MATRIX.md`.
- Detailed MidPage lane matrix:
  `docs/HZ5_MIDPAGEFRONT_C7_LANES.md`.

P13/P14 are not speed lanes. P11/P9 should stay counter-free when used for
throughput measurements; P12 cap1 should now be included only when comparing
negative run16 evidence.

## Bootstrap Milestones

### P1

`HZ5-P1` adds a local aligned-run allocator:

- 2 MiB aligned segment.
- 64 KiB segment header/metadata region.
- Page metadata for `FREE`, `RUN_START`, and `RUN_CONT`.
- Alignment-aware page bitmap scan.
- Local free for HZ5-owned run-start pointers.

Smoke:

```text
pwsh -NoProfile -File hakozuna-hz5/win/build_win_hz5_p1_smoke.ps1
```

Expected output:

```text
HZ5-P1/P2 aligned-run smoke passed
```

### P2

`HZ5-P2` adds the first owner/remote core:

- `core/hz5_owner.c`: per-thread owner tokens.
- `core/hz5_remote.c`: TLS remote buffer, grouped page flush, and owner drain.
- `core/hz5_run.c`: local/remote free dispatch.

P2 is still not a BenchLab behavior lane. It proves the owner-token and
page-oriented remote-free lifecycle before HZ5 starts routing target workloads.

### P3

`HZ5-P3` added the first BenchLab behavior adapter:

```text
win/hakozuna_hz5_adapter.c
```

It routes only exact `4K/8K/64K align=8192` to HZ5 and leaves every other row on
HZ3 S276 fallback. It is now superseded by the narrower P11/P9 seed and the P12
run16 follow-up.

## Documentation

- BenchLab-facing status, lane policy, and result timeline:
  `allocator-bench-lab/docs/hakozuna-hz5-sidecar-design.md`
- BenchLab HZ5 lane registry:
  `allocator-bench-lab/docs/hakozuna-hz5-lane-registry.md`
- Private implementation work log:
  `hakozuna-hz5/docs/HZ5_P0_ALIGNED_RUN_8192.md`. The full lifecycle history
  is archived under
  `hakozuna-hz5/docs/archive/HZ5_IMPLEMENTATION_LIFECYCLE_HISTORY_2026-05.md`.

The private work-log filename is historical. It started as the P0 work order
and now records the implementation lifecycle through P14.

Current P43 lowpage64 status:

- `lowpage/hz5_lowpage64_p43_segment.c` owns the 2 MiB segment / 128 KiB slot
  raw-source experiments.
- P43b.1 descriptor-owned SlotRef dry run removes committed-list traversal from
  data-page intrusive metadata.
- P43b.2 PAGE_NOACCESS is diagnostic-only and verifies inactive slots are not
  read through data-page wrapper/header/list state before real slot decommit.
- Real slot MEM_DECOMMIT is currently safety/RSS evidence only; speed recovery
  should come from descriptor-owned segment-local reuse before any promotion.

## Planned Layout

```text
hakozuna-hz5/
  api/       public HZ5 allocator API and descriptor forwarding
  contract/ SpeedLane descriptor ABI and purity contract
  include/   public and internal HZ5 headers
  core/      segment, pageheap, run, owner, and remote-free core
  fallback/  P20 lazy HZ3 fallback loader/state/counter module
  legacy/   historical P17/P24 diagnostic raw-cache modules
  lowpage/   P25/P27/P28/P43 low-page 64K raw cache and segment-slot modules
  policy/    HZ5-native alloc/free policy API and dispatch ordering
  route/     exact a8192 route policy and P12 route-shadow observation
  wrapper/   HZ5 wrapper header/cookie/decode helpers
  src/       reserved for future repo-local shims; currently intentionally empty
  win/       Windows research build/link scripts
  docs/      HZ5 work orders and result notes
```

BenchLab owns the current adapter glue in
`allocator-bench-lab/win/hakozuna_hz5_adapter.c`; the sidecar core and reusable
api/contract/fallback/legacy/low-page/policy/route/wrapper mechanisms stay in
this directory so HZ5 can remain modular next to HZ3 and HZ4.
