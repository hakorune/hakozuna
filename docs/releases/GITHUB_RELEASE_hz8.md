# Hakozuna HZ8 Source/Artifact Release Draft

This draft is for the first GitHub source/artifact release that publishes HZ8
as the recommended Hakozuna allocator line.

## Release Scope

- Source-first release by default.
- No prebuilt binaries by default.
- HZ8 is presented as the recommended balanced allocator line.
- HZ8-v2 / KeepRefill plus preload-surface and remote span-lease publish
  hardening is the current default.
- HZ8-v1.1 remains the frozen comparison baseline.
- LargeDirectOwned and ShardedHotCache are opt-in evidence lanes, not defaults.
- HZ9 remains archived throughput research evidence, not a release default.
- HZ10 is included as an active speed/RSS-aware research line, but it is not
  promoted as the HZ8 public recommendation.

## Zenodo / DOI

Fill after the HZ8 record is created:

```text
HZ8 archived Zenodo record:
  https://zenodo.org/records/21084279

HZ8 version DOI:
  https://doi.org/10.5281/zenodo.21084279

HZ8 all-version DOI:
  https://doi.org/10.5281/zenodo.21084278
```

Existing paper artifacts:

```text
HZ3/HZ4:
  https://doi.org/10.5281/zenodo.20753903

HZ5:
  https://doi.org/10.5281/zenodo.20753950

HZ6:
  https://doi.org/10.5281/zenodo.20753968
```

## Suggested GitHub Release Body

```text
This release publishes Hakozuna HZ8 as the recommended balanced allocator line.

HZ8 combines the project lineage from HZ3/HZ4/HZ5/HZ6: local fast-path shape,
owner-stable remote free, pressure / retention control, and fail-closed pointer
ownership.  The current default is HZ8-v2 / KeepRefill plus the
preload-surface and remote span-lease publish hardening fixes.

KeepRefill addresses the previous remote-heavy cliff by keeping owner-local
medium refill candidates active-live after remote collection drains them.  In
the public Ubuntu/Linux matrix, HZ8 is best read as a balanced low-RSS allocator
with practical throughput, not as a universal tcmalloc replacement.

The latest macro-lane hardening is included in this draft: the preload shim
exports `malloc_usable_size` and aligned allocation entrypoints needed by common
LD_PRELOAD hosts, and the default remote publish path uses span-level publish
leasing plus bounded transition backoff. The latter fixes the observed
`xmalloc-test` 15s livelock in the default preload lane while preserving the
low post-RSS public guard band.

LargeDirectOwned is included as opt-in/profile evidence for the known
`cross128_r90` weakness: it shows that the weakness is largely at the
large/direct boundary, but it has a larger RSS tradeoff and is not the default.
ShardedHotCache-L1 is kept as HOLD/future-work evidence; it does not yet provide
a default-quality throughput/RSS Pareto point.

The repository also includes the HZ10 research line.  HZ10's lazy page-init
default improves local-row throughput and RSS substantially in the integrated
matrix, but HZ8 remains the release recommendation for balanced low post-RSS.
Use the integrated HZ10 snapshot as comparative evidence, not as a replacement
claim:
docs/benchmarks/20260707_allocator_line_integrated_hz10_bump_default/README.md

Highlights:

- HZ8 source under hakozuna-hz8/
- HZ8 English/Japanese README files
- HZ8-v2 / KeepRefill default build and preload targets
- Default remote span-lease publish/backoff fix for xmalloc-style remote storms
- Expanded LD_PRELOAD surface including malloc_usable_size and aligned allocation
- Opt-in LargeDirect / ShardedHot evidence targets
- Fail-closed ownership and route contract documentation
- Public benchmark matrix scripts and release notes
- Integrated HZ-line comparison including HZ10:
  docs/benchmarks/20260707_allocator_line_integrated_hz10_bump_default/README.md
- Paper-facing Ubuntu/Linux x86_64 matrix snapshot:
  hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
- HZ8 public release preparation notes:
  hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md
- HZ8 paper Zenodo record:
  https://zenodo.org/records/21084279
- HZ8 paper DOI:
  https://doi.org/10.5281/zenodo.21084279

Release shape:

- Source-first release
- No prebuilt binaries by default
- Generated benchmark outputs are not part of the source archive unless
  attached separately
- HZ9 remains an opt-in research lane
- LargeDirect / ShardedHot are not promoted as release defaults
```

## Pre-Publish Checklist

- [x] HZ8 Zenodo record and DOI are available.
- [x] README badge/link is added after DOI assignment.
- [x] `CITATION.cff` is explicitly left at repository-level citation.
- [x] `hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md` is up to date.
- [ ] Public benchmark captions include RUNS, THREADS, ITERS, platform, and RSS.
- [ ] Release body avoids "universally beats tcmalloc" style claims.
- [ ] Windows HZ8 remains described as bring-up/evidence only.
- [ ] LargeDirect / ShardedHot wording remains opt-in evidence only.
- [ ] Generated binaries are excluded unless attached as explicit platform artifacts.
