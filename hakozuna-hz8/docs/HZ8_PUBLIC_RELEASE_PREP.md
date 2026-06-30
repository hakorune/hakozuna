# HZ8 Public Release Preparation

Status: public-release preparation / paper input.

HZ8 is the recommended Hakozuna allocator line.  The current default is
HZ8-v2 / KeepRefill, with HZ8-v1.1 kept as the frozen comparison baseline.

## Public Claim

Use this wording:

```text
HZ8 is a balanced allocator line combining low post-workload RSS,
fail-closed ownership, cross-thread free correctness, and practical throughput.
The HZ8-v2 KeepRefill default removes the prior remote-heavy cliff while
staying far below mimalloc RSS on the reported remote-heavy rows.
```

Do not use these claims:

```text
HZ8 universally beats tcmalloc.
HZ8 is the fastest allocator on every local-only row.
HZ8 replaces all HZ3/HZ4/HZ5/HZ6 research lines as evidence.
Windows HZ8 has Linux parity.
```

## Release Scope

```text
include:
  hakozuna-hz8/ source
  HZ8 docs under hakozuna-hz8/docs/
  README.md / README.ja.md
  public benchmark summaries and scripts

exclude by default:
  generated binaries
  private raw benchmark dumps
  profile-only or HZ9 claims
```

## Paper Tables

Primary HZ8 table:

```text
record:
  bench_results/20260630T191745Z_hz8_keeprefill_public_matrix/

rows:
  small_interleaved_remote90
  main_interleaved_r90
  medium_interleaved_r50
  guard_local0
  main_local0
  medium_local0

allocators:
  hz8
  hz8_keeprefill
  mimalloc
  tcmalloc
  system
```

Required caption details:

```text
RUNS=10
THREADS=16
ITERS=50000
  median ops/s
  peak RSS
  Ubuntu/Linux x86_64 environment
  prepared mimalloc / tcmalloc LD_PRELOAD DSOs
```

Paper-ready snapshot:

```text
hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
```

Interpretation:

```text
small_interleaved_remote90:
  HZ8 removes the old remote-heavy RSS/throughput cliff.

main_interleaved_r90:
  HZ8 keeps RSS very low and beats mimalloc on throughput in the reported row,
  but tcmalloc remains faster.

medium_interleaved_r50:
  HZ8 improves over the prior default, but do not overclaim this as a universal
  medium-size win.
```

## Pre-Publish Checklist

```text
[ ] README mentions HZ8 as the recommended allocator line.
[ ] README does not say HZ8 universally beats tcmalloc.
[ ] HZ8 Zenodo description says "balanced allocator", not "fastest allocator".
[ ] HZ8 paper/table captions include RUNS, THREADS, ITERS, platform, and RSS.
[ ] HZ8 Windows text remains bring-up/evidence only.
[ ] CITATION.cff is reviewed once a Zenodo DOI exists.
[ ] GitHub release body links the HZ8 Zenodo record once available.
[ ] Generated binaries are excluded from the source release unless explicitly
    attached as separate artifacts.
```

## Release Names

Suggested tag names:

```text
hz8-v2-keeprefill
hz8-paper-artifacts-2026-07
```

Suggested Zenodo title:

```text
Hakozuna HZ8: Balanced Low-RSS Allocator with Fail-Closed Ownership and
Remote-Heavy KeepRefill Control
```
