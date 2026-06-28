# Hakozuna HZ6

HZ6 is the planned transfer-first successor to the HZ5 sidecar allocator
research. It now has an executable R1 family: modular contracts, Windows and
Linux build entrypoints, capacity lanes, and application-like benchmark
runners. Current Windows work is focused on lane discipline and control-plane
evidence rather than a single default promotion.

Japanese README: [READMEjp.md](READMEjp.md)

## Positioning

```text
HZ3:
  thin local/TLS speed ideas

HZ4:
  remote handoff and page/span cache ideas

HZ5:
  descriptor ownership, fail-closed route safety, and low-RSS profiles

HZ6:
  route-safe / transfer-first / RSS-aware allocator family
```

HZ6 should not be a direct port of HZ3, HZ4, or HZ5 internals. It should take
the proven ideas and make them first-class layers from the start.

## Core Direction

HZ6 starts from these rules:

- Route lookup is a first-class layer and returns `MISS`, `VALID`, or
  `INVALID`.
- Remote frees prefer bounded transfer caches over owner inboxes in fast
  profiles.
- Owner inboxes remain for strict, fallback, and owner-bound profiles.
- Hot paths do not update diagnostic counters or read policy state.
- Refill, drain, source, release, and policy decisions live on slow paths.
- Fail-closed ownership remains mandatory, but fast remote profiles may use
  batch-boundary validation when it does not allow corrupted reuse.
- RSS governance is part of the core design, not an afterthought.

## Reading Order

```text
docs/HZ6_BLUEPRINT.md
docs/HZ6_R1_STATUS.md
docs/HZ6_LANE_GUIDE.md
docs/HZ6_R1_BENCHMARK_20260528.md
docs/HZ6_R1_BROAD_TRENDS_20260528.md
docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
docs/HZ6_ARCHITECTURE_DRAFT.md
docs/HZ6_FOLDER_LAYOUT.md
docs/HZ6_MIGRATION_FROM_HZ5.md
docs/current_task.md
```

## R1 Smoke

The first code is intentionally small. It validates module boundaries and
transfer-first state transitions, not performance.

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected output:

```text
hz6-r1-core-contract-smoke ok
hz6-r1-route-smoke ok
hz6-r1-transfer-contract-smoke ok
hz6-r1-source-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-prefill-smoke ok
hz6-r1-sourceblock-smoke ok
hz6-r1-transfer-smoke ok
hz6-r1-reclaim-smoke ok
hz6-r1-safety-smoke ok
```

## Windows Build Seed

HZ6 now also has Windows build entrypoints for the same R1 smoke suite and the
HZ6-only allocator benchmark. This is the Windows build/measurement seed, not a
cross-family benchmark claim.

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\build_win_hz6_r1_smokes.ps1
```

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\run_win_hz6_benchmark.ps1
```

Details:

- Windows source backing uses `source/win_source_virtualalloc.*`.
- The smoke list matches the Linux R1 suite.
- The benchmark runner mirrors the Linux HZ6-only local / remote / reuse
  mode/profile/size matrix.
- Windows RSS output uses peak working set in the `ru_maxrss_kb` TSV column.
- `-SkipRun` builds without executing the smoke binaries.
- `docs/HZ6_WINDOWS_BUILD.md` records the current Windows build seed.

## Benchmark Status

HZ6 now has HZ6-only, legacy allocator matrix, and app-like Windows benchmark
entrypoints. The lane map is intentionally explicit:

```text
route4k:
  current Windows candidate-control lane

control:
  low-capacity / low-RSS baseline

appcap:
  high-capacity completion/control lane, not a default

sourcerun-*:
  source-layer mechanism evidence, not promotion
```

Use `docs/HZ6_LANE_GUIDE.md` before interpreting new HZ6 numbers. No broad
performance claim should be made from a single lane or a single row.

Latest Windows rerun note:

- The 2026-06-29 Larson paper runner capture / rc parsing fix is in place.
- The narrow `-LarsonAppcapOnly` rerun is the current clean Windows HZ6
  baseline.
- The default `hz6-strict`, `hz6-speed`, `hz6-rss`, `hz6-*-broad`, and
  `hz6-*-route4k` lanes still fail warmup on this matrix; keep them as
  control / no-go evidence.
- The strongest HZ6 row in the narrow rerun is
  `hz6-ownerlocality-appcap-speed` at `45.754M ops/s` and `2,250,016 KB`
  peak RSS.

Snapshot:

```text
docs/HZ6_R1_BENCHMARK_20260528.md
docs/HZ6_R1_BROAD_TRENDS_20260528.md
```

Current Ubuntu selected/profile comparison raws are tracked in:

```text
docs/current_task.md
docs/HZ6_UBUNTU_PRELOAD_LANES.md
docs/HZ6_UBUNTU_SELECTED_BALANCE.md
```

Use those docs for the latest HZ3 / HZ4 / HZ5 / mimalloc / tcmalloc comparison
state. Do not infer selected/default promotion from a single profile DSO row.

Current R1 trend:

```text
strict:
  strongest local-only profile in the HZ6-only runner

rss:
  strongest remote/reuse profile in the HZ6-only runner

speed / remote:
  profile intent is documented, but the current R1 knobs are not yet faster
  than rss in the broad single-process sweep
```

## Size Coverage Boundary

R1 is not a complete large-object allocator yet. The current code is an
executable architecture seed with these front boundaries:

```text
toy:
  <= 4KiB contract-validation front

midpage:
  >4KiB..32KiB seed front

local2p:
  exact 64KiB seed front

largespan:
  >32KiB..1MiB except exact 64KiB LargeSpan seed front

largedirect:
  >1MiB..8MiB direct-release coverage seed

>8MiB:
  unsupported in R1
```

The HZ6 large path is split into retained reuse and direct-release coverage:

```text
LargeSpan:
  128K / 256K / 512K / 1M classes
  bytes-aware CentralSpanPool retained reuse

LargeDirect:
  >1M..8M
  descriptor-owned exact route
  OS_PAGED allocation and payload release on free
  no central retain and no transfer cache

Future:
  add full ordinary-malloc preload coverage and compare against HZ3/HZ4/HZ5
```

HZ6 large-size results should be reported as `LargeSpan 128K..1M seed` plus
`LargeDirect >1M..8M coverage seed`, not as a broad large-object allocator
claim.

## Non-Goals

- Do not copy HZ3/HZ4/HZ5 implementation files into HZ6 as a starting point.
- Do not put OS queries such as `VirtualQuery` on a hot free path.
- Do not make a single universal profile the first success criterion.
- Do not weaken owned-invalid / double-free behavior into a silent fallback.
