# Zenodo Description Draft: HZ8

Published HZ8 paper record:

```text
https://zenodo.org/records/21084279
https://doi.org/10.5281/zenodo.21084279
```

## Resource Type

```text
Software
```

## Title

```text
Hakozuna HZ8: Balanced Low-RSS Allocator with Fail-Closed Ownership and
Remote-Heavy KeepRefill Control
```

## Authors / Creators

Use the same creator metadata as the existing Hakozuna Zenodo records unless
the author list has changed.

## Description

```text
This record contains the Hakozuna HZ8 allocator source, design notes, benchmark
scripts, and public benchmark summaries.

HZ8 is the recommended Hakozuna allocator line.  It combines ideas from earlier
Hakozuna allocator research: HZ3-style local fast-path structure, HZ4-style
owner-stable remote free, HZ5-style pressure and retention control, and
HZ6-style fail-closed ownership and route safety.

The current default is HZ8-v2 / KeepRefill.  KeepRefill addresses the previous
remote-heavy cliff by keeping owner-local medium refill candidates active-live
after remote collection drains them.  The public benchmark interpretation is
deliberately balanced: HZ8 emphasizes low post-workload RSS, fail-closed pointer
ownership, cross-thread free correctness, and practical throughput.  It is not
claimed to universally outperform tcmalloc on every local-only or throughput
row.

Main components include:

- hakozuna-hz8/: HZ8 allocator source
- hakozuna-hz8/README.md and README.ja.md: English/Japanese entry points
- hakozuna-hz8/docs/: ownership, lifecycle, remote collection, gate, and release
  documentation
- hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md: paper-facing
  Ubuntu/Linux x86_64 public matrix snapshot
- hakozuna-hz8/scripts/: public benchmark matrix helpers
- hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md: release and paper preparation
  checklist

Code repository: https://github.com/hakorune/hakozuna
```

## Keywords

```text
memory allocator
malloc
LD_PRELOAD
RSS
remote free
fail-closed ownership
systems software
benchmarking
```

## Related Identifiers

```text
HZ3/HZ4 DOI:
  https://doi.org/10.5281/zenodo.20753903

HZ5 DOI:
  https://doi.org/10.5281/zenodo.20753950

HZ6 DOI:
  https://doi.org/10.5281/zenodo.20753968

GitHub repository:
  https://github.com/hakorune/hakozuna

HZ8 paper DOI:
  https://doi.org/10.5281/zenodo.21084279
```
