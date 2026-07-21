# Hakozuna HZ8 Balanced Allocator Source Release

This release publishes HZ8 as the recommended Hakozuna allocator line.

## Release Scope

- Source-first release by default.
- No prebuilt binaries by default.
- HZ8 is presented as the recommended balanced allocator line.
- The current default combines KeepRefill, remote span-lease publication,
  SmallTransitionInventory, GeneralMediumPage, and EntryBoundary-L1A.
- `hz8-pre-transition-rollback` and `hz8-v2-rollback` preserve immediate and
  broad rollback boundaries.
- HZ8-v1.1 remains the frozen comparison baseline.
- LargeDirectOwned and ShardedHotCache are opt-in evidence lanes, not defaults.
- HZ9 remains archived throughput research evidence, not a release default.
- HZ10-HZ12 remain related research lines. They do not replace HZ8 as the
  first allocator users should choose.

## Zenodo / DOI

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

HZ10-HZ12:
  https://doi.org/10.5281/zenodo.21360690
```

## Suggested GitHub Release Body

```text
This release publishes Hakozuna HZ8 as the recommended balanced allocator line.

HZ8 combines the project lineage from HZ3/HZ4/HZ5/HZ6: local fast-path shape,
owner-stable remote free, pressure / retention control, and fail-closed pointer
ownership. The current default combines KeepRefill, remote span-lease
publication, SmallTransitionInventory, GeneralMediumPage, and
EntryBoundary-L1A.

KeepRefill addresses the previous remote-heavy cliff by keeping owner-local
medium refill candidates active-live after remote collection drains them.  In
the public Ubuntu/Linux matrix, HZ8 is best read as a balanced low-RSS allocator
with practical throughput, not as a universal tcmalloc replacement.

The latest macro-lane hardening is included in this release: the preload shim
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

The repository also includes the HZ10-HZ12 research sequence. HZ10 explores a
lean page substrate, HZ11 develops broad multithreaded transfer behavior, and
HZ12 adds bounded ownership-aware retirement reclaim. HZ8 remains the public
recommendation; these lines are comparative research, not extra product
choices.

Highlights:

- HZ8 source under hakozuna-hz8/
- HZ8 English/Japanese README files
- KeepRefill + SmallTransitionInventory + GeneralMediumPage +
  EntryBoundary-L1A default build and preload targets
- Explicit `hz8-pre-transition-rollback` and `hz8-v2-rollback` controls
- Default remote span-lease publish/backoff fix for xmalloc-style remote storms
- Expanded LD_PRELOAD surface including malloc_usable_size and aligned allocation
- Opt-in LargeDirect / ShardedHot evidence targets
- Fail-closed ownership and route contract documentation
- Public benchmark matrix scripts and release notes
- Related HZ10-HZ12 paper DOI:
  https://doi.org/10.5281/zenodo.21360690
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

Known limitations:

- HZ8 does not claim universal throughput leadership over tcmalloc.
- Some throughput-first local rows remain slower than tcmalloc.
- LargeDirect, Mag32, detached-page, and partial-depot variants remain explicit
  research/profile lanes unless their lane documents say otherwise.
- Windows and Linux use platform-specific selected backends; compare each
  platform with its own baselines rather than comparing absolute numbers
  across operating systems.
```

## Five-Minute Verification

Linux:

```sh
make -C hakozuna-hz8 smoke safety-stress preload-smoke
```

Windows (PowerShell with `clang-cl` available):

```powershell
.\hakozuna-hz8\scripts\build_hz8_win64_release_smoke.ps1
.\win\build_win_allocator_suite.ps1 -OnlyHz8
.\win\build_win_mt_remote_suite.ps1 -OnlyHz8
```

The commands above build the production/default lane. Research and diagnostic
rows require explicit opt-in switches and are not release-speed evidence.

## Pre-Publish Checklist

- [x] HZ8 Zenodo record and DOI are available.
- [x] README badge/link is added after DOI assignment.
- [x] `CITATION.cff` is explicitly left at repository-level citation.
- [x] `hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md` is up to date.
- [x] Public benchmark captions include RUNS, THREADS, ITERS, platform, and RSS.
- [x] Release body avoids "universally beats tcmalloc" style claims.
- [x] Windows and Linux support claims are platform-scoped.
- [x] LargeDirect / ShardedHot wording remains opt-in evidence only.
- [x] Generated binaries are excluded unless attached as explicit platform artifacts.
