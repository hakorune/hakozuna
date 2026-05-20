# Hakozuna HZ5

HZ5 is a sidecar allocator prototype that starts from a page/run-first design.
It lives next to HZ3 (`hakozuna/`) and HZ4 (`hakozuna-mt/`) so the HZ3 S276
default can remain stable while HZ5 explores a different classification model.

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

HZ5 has moved past the initial P0/P3 route proof. The current split is:

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
- `P20`: lazy HZ3 fallback. The LoadLibrary/GetProcAddress fallback loader,
  fallback state machine, and one-shot counters now live in `fallback/`, so the
  BenchLab adapter only keeps pointer-origin wrapper dispatch.
- `route`: exact a8192 route policy, HZ3 medium fallback eligibility, and P12
  route-shadow counters now live in `route/`, so BenchLab adapters call into
  HZ5 for route decisions instead of owning the policy locally.

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
  `hakozuna-hz5/docs/HZ5_P0_ALIGNED_RUN_8192.md`

The private work-log filename is historical. It started as the P0 work order
and now records the implementation lifecycle through P14.

## Planned Layout

```text
hakozuna-hz5/
  include/   public and internal HZ5 headers
  core/      segment, pageheap, run, owner, and remote-free core
  fallback/  P20 lazy HZ3 fallback loader/state/counter module
  lowpage/   P25/P27/P28 low-page 64K raw cache modules
  route/     exact a8192 route policy and P12 route-shadow observation
  src/       reserved for future repo-local shims; currently intentionally empty
  win/       Windows research build/link scripts
  docs/      HZ5 work orders and result notes
```

BenchLab owns the current adapter glue in
`allocator-bench-lab/win/hakozuna_hz5_adapter.c`; the sidecar core and reusable
fallback/low-page/route mechanisms stay in this directory so HZ5 can remain
modular next to HZ3 and HZ4.
