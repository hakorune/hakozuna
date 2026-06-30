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
  HZ8 v2 / KeepRefill

recommended default:
  yes

release record:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md

experimental throughput lane:
  docs/HZ8_V2_HZ9_DESIGN.md

windows bring-up lane:
  docs/HZ8_WINDOWS_BRINGUP.md
```

HZ8-v2 keeps the HZ8-v1.1 balanced base and promotes KeepRefill as the default
remote-heavy pressure fix:

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

residency:
  budgeted empty-resident retention
  lazy128 persistent owner-attached reservation
  conservative retained-empty overhead contract of about 212MiB
  owner exit / detach / destroy as hard drain points

compatibility:
  pure LD_PRELOAD malloc/free/calloc/realloc surface
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
```

It keeps owner-local refill-candidate medium runs active-live after remote
collect drains them, avoiding the expensive empty/reactivate loop seen in
remote-heavy rows.  The public cross-allocator matrix confirms it as the
current balanced default.  It is not a claim that HZ8 universally beats
tcmalloc.

## Build

```bash
make smoke
make preload
make bench          # debug/counter build
make bench-release  # release throughput build
make bench-release-mediumkeeprefillempty  # compatibility alias for v2 default
make preload-mediumkeeprefillempty        # compatibility alias for v2 default DSO
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

## Documentation Map

Start here:

```text
docs/HZ8_V1_1_RELEASE.md
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
docs/HZ8_SMALL_V0_RC1.md
docs/HZ8_OWNERSHIP_CONTRACT.md
docs/HZ8_OWNER_LIFECYCLE.md
docs/HZ8_BENCH_GATE.md
docs/HZ8_WINDOWS_BRINGUP.md
docs/ALLOCATOR_MATRIX.md
docs/HZ8_V2_HZ9_DESIGN.md
```

For Japanese, see:

```text
README.ja.md
```
