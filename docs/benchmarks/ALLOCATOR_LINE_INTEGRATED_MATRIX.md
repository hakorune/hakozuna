# Allocator Line Integrated Matrix

Status: public benchmark entrypoint / source-of-truth for cross-line quick
comparison.

## Purpose

The root README used to carry a large `MT lane x remote%` table. That table is
useful, but adding every new allocator line directly to it makes the README hard
to scan and can mix different benchmark contracts.

Use this document and the same-run runner below as the stable place for
cross-line allocator snapshots. The README should link here instead of growing
another wide table.

## Runner

```bash
ALLOCATORS=hz8,hz10,hz3,hz4,mimalloc,tcmalloc,system \
ROWS=guard_local0,main_local0,main_interleaved_r50,main_interleaved_r90 \
RUNS=10 THREADS=16 ITERS=50000 \
  hakozuna-hz8/scripts/run_hz8_v11_same_run_matrix.sh
```

For a cheaper smoke:

```bash
ALLOCATORS=hz8,hz10,system \
ROWS=guard_local0 \
RUNS=1 THREADS=2 ITERS=1000 \
  hakozuna-hz8/scripts/run_hz8_v11_same_run_matrix.sh
```

## What This Measures

All rows use the same `bench_matrix_malloc` harness and swap allocators through
`LD_PRELOAD`, except `system`, which uses no preload.

Current built-in allocator names:

```text
hz8
hz8_legacy64k2
hz8_keeprefill
hz10
hz3
hz4
mimalloc
tcmalloc
system
```

The `hz10` column builds and uses `hakozuna-hz10/libhz10.so`. Treat this as a
same-harness measurement column, not as a public recommendation. HZ8 remains
the current public allocator line unless a later release promotes HZ10
separately.

## Recommended Rows

```text
guard_local0              small local fixed-cost guard
main_local0               16..32768 local row
main_interleaved_r50      16..32768 interleaved remote 50%
main_interleaved_r90      16..32768 interleaved remote 90%
small_interleaved_remote90
medium_local0
medium_interleaved_r50
fixed24_local0
fixed48_local0
```

## Output

The runner writes:

```text
README.log   environment, allocator DSO paths, run settings
samples.csv  raw per-run samples
summary.md   ranked median ops/s and RSS table per row
```

Use `summary.md` as the convenient bench list. Use `samples.csv` when variance
matters or when updating public claims.

## README Policy

Do not paste a full wide matrix into the root README. Keep the README as the
allocator selection guide:

```text
HZ8  public recommended balanced allocator
HZ10 research/macro-shim candidate, measured here but not replacement yet
older HZ lines historical/profile evidence
```

When a snapshot becomes worth publishing, put the result file under
`docs/benchmarks/` or the relevant `bench_results/` directory and link it from
the README.
