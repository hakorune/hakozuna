# Hakozuna HZ8 Source/Artifact Release Draft

This draft is for the first GitHub source/artifact release that publishes HZ8
as the recommended Hakozuna allocator line.

## Release Scope

- Source-first release by default.
- No prebuilt binaries by default.
- HZ8 is presented as the recommended balanced allocator line.
- HZ8-v2 / KeepRefill is the current default.
- HZ8-v1.1 remains the frozen comparison baseline.
- HZ9 remains an opt-in throughput research lane, not a release default.

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
ownership.  The current default is HZ8-v2 / KeepRefill.

KeepRefill addresses the previous remote-heavy cliff by keeping owner-local
medium refill candidates active-live after remote collection drains them.  In
the public Ubuntu/Linux matrix, HZ8 is best read as a balanced low-RSS allocator
with practical throughput, not as a universal tcmalloc replacement.

Highlights:

- HZ8 source under hakozuna-hz8/
- HZ8 English/Japanese README files
- HZ8-v2 / KeepRefill default build and preload targets
- Fail-closed ownership and route contract documentation
- Public benchmark matrix scripts and release notes
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
```

## Pre-Publish Checklist

- [x] HZ8 Zenodo record and DOI are available.
- [x] README badge/link is added after DOI assignment.
- [x] `CITATION.cff` is explicitly left at repository-level citation.
- [x] `hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md` is up to date.
- [ ] Public benchmark captions include RUNS, THREADS, ITERS, platform, and RSS.
- [ ] Release body avoids "universally beats tcmalloc" style claims.
- [ ] Windows HZ8 remains described as bring-up/evidence only.
