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

## Initial Scope

The first prototype is `HZ5-P0 AlignedRun8192`:

- Route shadow only, no behavior change.
- Candidate rows:
  - `size == 4096 && align == 8192`
  - `size == 8192 && align == 8192`
  - `size == 65536 && align == 8192`
- Everything else remains HZ3 S276 fallback.

P1/P2 should only start after P0 route counters prove the target rows and guard
rows are classified exactly as expected.

## P1 Status

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

## P2 Status

`HZ5-P2` adds the first owner/remote core:

- `core/hz5_owner.c`: per-thread owner tokens.
- `core/hz5_remote.c`: TLS remote buffer, grouped page flush, and owner drain.
- `core/hz5_run.c`: local/remote free dispatch.

P2 is still not a BenchLab behavior lane. It proves the owner-token and
page-oriented remote-free lifecycle before HZ5 starts routing target workloads.

## P3 Status

`HZ5-P3` adds the first BenchLab behavior adapter:

```text
win/hakozuna_hz5_adapter.c
```

It routes only exact `4K/8K/64K align=8192` to HZ5 and leaves every other row on
HZ3 S276 fallback. Initial smoke passed, but this is still a diagnostic
prototype. The next core feature should be owner-local run reuse after remote
drain.

## Planned Layout

```text
hakozuna-hz5/
  include/   public and internal HZ5 headers
  core/      segment, pageheap, run, owner, and remote-free core
  src/       route, counters, fallback bridge, and shim glue
  win/       Windows research build/link scripts
  docs/      HZ5 work orders and result notes
```
