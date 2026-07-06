# Hakozuna HZ8

HZ8 is the recommended Hakozuna allocator line.

It is designed as a balanced allocator rather than a throughput-only clone:

```text
HZ8 = HZ3-style local fast path
    + HZ4-style owner-stable remote free
    + HZ5-style pressure and retention control
    + HZ6-style fail-closed boundary safety
```

Use HZ8 when you want one allocator with:

```text
low post-workload RSS
fail-closed pointer ownership
bounded medium-run retention
cross-thread free safety
pure LD_PRELOAD compatibility
decent general throughput
```

HZ8 is not intended to beat tcmalloc on every local-only hot-path benchmark.
Its value is the combination of safety, RSS recovery, remote-free correctness,
and practical throughput.

## Status

```text
current public line:
  HZ8 v2 / KeepRefill + remote span-lease publish

recommended default:
  yes

release record:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  docs/HZ8_PRELOAD_SHIM_SURFACE_F1.md
  docs/HZ8_REMOTE_SPAN_LEASE_PUBLISH_L0.md
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md
  docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md

paper DOI:
  https://doi.org/10.5281/zenodo.21084279

paper Zenodo record:
  https://zenodo.org/records/21084279

experimental throughput lane:
  docs/HZ8_V2_HZ9_DESIGN.md

windows bring-up lane:
  docs/HZ8_WINDOWS_BRINGUP.md
```

HZ8-v2 keeps the HZ8-v1.1 balanced base and promotes KeepRefill as the default
remote-heavy pressure fix. The current source default also includes the
post-macro hardening fixes that made the preload surface and xmalloc-style
remote storm usable:

```text
small:
  small-v0 frozen behavior

medium:
  4097..65536 bytes
  8K / 16K / 24K / 32K / 48K / 64K classes
  q64-v12-48k2 geometry
  64KiB quantum directory

remote free:
  owner-attached medium pending queue
  pending bitmap as remote-claim authority
  qstate owner collector protocol
  active-full Defer4 remote-pressure collection
  medium capacity collect budgeting
  owner-local refill-candidate empty-run keep-live
  span-lease remote publish instead of owner-wide publish lease
  bounded transition backoff under OWNER_TRANSITION retry storms

residency:
  budgeted empty-resident retention
  lazy128 persistent owner-attached reservation
  conservative retained-empty overhead contract of about 212MiB
  owner exit / detach / destroy as hard drain points

compatibility:
  pure LD_PRELOAD malloc/free/calloc/realloc surface
  malloc_usable_size + aligned allocation entrypoints for common preload hosts
```

## Which HZ Should Users Pick?

Users should normally pick only HZ8.

The older names are design lineage and benchmark references, not product
choices:

| Line | Role |
|---|---|
| HZ3 | Local-speed reference |
| HZ4 | Owner-stable remote-free reference |
| HZ5 | Pressure / RSS-control reference |
| HZ6 | Fail-closed boundary-safety reference |
| HZ7 | Route-safe transition prototype |
| HZ8 | Recommended balanced allocator |
| HZ9 | Experimental throughput research lane, not a release default |

HZ9 work should remain opt-in until it proves a simple, distinct public
promise.  Do not expose HZ3..HZ9 as a menu of allocator choices to ordinary
users.

## Design Contract

HZ8 keeps these contracts:

```text
1. Owned-looking INVALID pointers never fall through to platform free.
2. MISS / VALID / INVALID are distinct route outcomes.
3. slot_state remains the allocation-validity authority.
4. Remote duplicate free is claimed through pending bits.
5. Owner lifecycle protects against stale owner reuse.
6. Owner exit hard-drains pending, retained, and active-live medium state.
7. Medium retention is bounded and explicitly documented.
8. Slow-path pressure policy must not become a hot-path profile selector.
```

The remaining known weakness is throughput relative to throughput-first
allocators on some local-only rows.  That is now treated as HZ9 research input,
not an HZ8 correctness issue.

## HZ8 v2 Default

The current HZ8 default includes:

```text
MediumKeepRefillEmpty-L1
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  make bench-mediumkeeprefillempty
  make bench-release-mediumkeeprefillempty
  make preload-mediumkeeprefillempty

RemoteSpanLeasePublish-L0
  docs/HZ8_REMOTE_SPAN_LEASE_PUBLISH_L0.md
  default preload fix for the xmalloc remote publish livelock

PreloadShimSurface-F1
  docs/HZ8_PRELOAD_SHIM_SURFACE_F1.md
  malloc_usable_size / aligned allocation surface for LD_PRELOAD hosts
```

It keeps owner-local refill-candidate medium runs active-live after remote
collect drains them, avoiding the expensive empty/reactivate loop seen in
remote-heavy rows.  The public cross-allocator matrix confirms it as the
current balanced default.  It is not a claim that HZ8 universally beats
tcmalloc.

## LargeDirect and ShardedHot Evidence

The HZ8 paper line keeps the public default conservative:

```text
default:
  HZ8-v2 / KeepRefill balanced default

opt-in evidence:
  LargeDirectOwned
  LargeDirectShardedHotCache-L1
```

LargeDirectOwned shows that the `cross128_r90` weakness is largely a
large/direct-boundary issue.  It can improve the cross128 row by a large factor,
but the RSS tradeoff is significant, so it remains an opt-in profile and paper
evidence rather than the default allocator behavior.

ShardedHotCache-L1 is also kept as evidence only.  The mechanism works, and the
128 MiB total / 32 MiB per-shard cap is the best observed shape in the current
sweep, but the measured throughput/RSS Pareto point is not default-quality yet.
Do not present it as a release default.

## Paper-Ready Matrix Highlights

Primary paper snapshot:

```text
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
Ubuntu 22.04.5 / Linux 6.8.0-90 / x86_64
RUNS=10, THREADS=16, ITERS=50000
```

Representative rows:

| Row | HZ8 KeepRefill | mimalloc | tcmalloc |
|---|---:|---:|---:|
| `small_interleaved_remote90` ops/s | 12.023M | 10.960M | 23.900M |
| `small_interleaved_remote90` post RSS | 2.91 MiB | 50.98 MiB | 32.94 MiB |
| `main_interleaved_r90` ops/s | 6.048M | 4.715M | 12.178M |
| `main_interleaved_r90` post RSS | 4.57 MiB | 183.12 MiB | 90.31 MiB |
| `medium_interleaved_r50` ops/s | 8.128M | 4.151M | 15.870M |
| `medium_interleaved_r50` post RSS | 3.81 MiB | 162.54 MiB | 79.06 MiB |

Read these as throughput/RSS tradeoff rows.  tcmalloc remains the stronger raw
throughput baseline on several rows; HZ8's claim is practical throughput with
very low post-workload RSS.

## Build

```bash
make smoke
make preload
make bench          # debug/counter build
make bench-release  # release throughput build
make bench-release-mediumkeeprefillempty  # compatibility alias for v2 default
make preload-mediumkeeprefillempty        # compatibility alias for v2 default DSO
make bench-release-largedirectdefault      # opt-in LargeDirect evidence
make bench-release-largedirectshardedhotcache  # opt-in ShardedHot evidence
```

Common local checks:

```bash
./h8_smoke
./h8_bench_release --runs 10 --threads 16 --iters 100000 \
  --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0
```

Pure preload matrix:

```bash
RUNS=5 THREADS=16 ITERS=50000 scripts/run_hz8_v11_same_run_matrix.sh
ALLOCATORS=hz8,system \
  ROWS=small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50 \
  RUNS=3 THREADS=16 ITERS=50000 scripts/run_hz8_v11_same_run_matrix.sh
MIMALLOC_SO=/path/to/libmimalloc.so \
TCMALLOC_SO=/path/to/libtcmalloc_minimal.so \
  scripts/run_hz8_keeprefill_public_matrix.sh
```

Paper-ready matrix snapshot:

```text
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
```

HZ8 MT lane x remote% snapshot:

```text
docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md
```

## Documentation Map

Start here:

```text
docs/HZ8_V1_1_RELEASE.md
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
docs/HZ8_SMALL_V0_RC1.md
docs/HZ8_OWNERSHIP_CONTRACT.md
docs/HZ8_OWNER_LIFECYCLE.md
docs/HZ8_BENCH_GATE.md
docs/HZ8_PUBLIC_RELEASE_PREP.md
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md
docs/HZ8_WINDOWS_BRINGUP.md
docs/ALLOCATOR_MATRIX.md
docs/HZ8_V2_HZ9_DESIGN.md
```

For Japanese, see:

```text
README.ja.md
```
