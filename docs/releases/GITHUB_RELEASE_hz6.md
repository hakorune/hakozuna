# Hakozuna HZ6 Source/Artifact Release Draft

This draft is for the first GitHub source/artifact release that explicitly
publishes HZ6 as an experimental selected-family allocator prototype.

## Release Scope

- Source-only release by default.
- No prebuilt binaries by default.
- HZ3/HZ4 remain the stable practical profiles.
- HZ5 remains the page/run-first sidecar allocator prototype.
- HZ6 is presented as an experimental selected-family allocator line, not as a
  universal replacement for HZ3/HZ4/HZ5 or industrial allocators.

## Zenodo / DOI

- HZ3/HZ4 latest record:
  https://zenodo.org/records/20753903
- HZ3/HZ4 version DOI:
  https://doi.org/10.5281/zenodo.20753903
- HZ3/HZ4 all-version DOI:
  https://doi.org/10.5281/zenodo.18305952

- HZ5 latest record:
  https://zenodo.org/records/20753950
- HZ5 version DOI:
  https://doi.org/10.5281/zenodo.20753950
- HZ5 all-version DOI:
  https://doi.org/10.5281/zenodo.20411597

- HZ6 record:
  https://zenodo.org/records/20753968
- HZ6 version DOI:
  https://doi.org/10.5281/zenodo.20753968
- HZ6 all-version DOI:
  https://doi.org/10.5281/zenodo.20753967

## Suggested GitHub Release Body

```text
This release publishes Hakozuna HZ6 as an experimental selected-family allocator
prototype alongside the existing HZ3/HZ4 allocator profiles and the HZ5 sidecar
allocator prototype.

HZ6 explores route-safe allocation/free dispatch, explicit ownership states,
front-end specialization, SourceLayer / FrontCache contracts, and
profile-specific workload lanes. It should be read as workload- and
platform-specific allocator evidence rather than a universal allocator ranking.

Highlights:

- HZ6 source and profile implementations under hakozuna-hz6/
- selected/default and profile-only lane documentation
- Windows and Ubuntu/Linux benchmark snapshots
- HZ6 paper PDFs are archived on Zenodo:
  https://zenodo.org/records/20753968
- HZ6 DOI:
  https://doi.org/10.5281/zenodo.20753968

Existing paper artifacts:

- HZ3/HZ4 DOI:
  https://doi.org/10.5281/zenodo.20753903
- HZ5 DOI:
  https://doi.org/10.5281/zenodo.20753950

Release shape:

- Source-only release
- No prebuilt binaries
- Raw/private benchmark data remain outside the source archive
- Public benchmark summaries and reproducibility documentation remain in docs/
```

## Pre-Publish Checklist

- [ ] README badge and paper/artifact links point to HZ6 DOI.
- [ ] `README_en.txt` and `README_ja.txt` describe HZ6 as an experimental
      selected-family line, not future work.
- [ ] `CITATION.cff` is reviewed for whether HZ6 should be cited separately or
      only through the Zenodo record.
- [ ] HZ6 paper PDFs are available from the Zenodo record.
- [ ] Release body avoids universal ranking claims.
- [ ] Release tag name is chosen deliberately, for example:
      `hz6-paper-artifacts-2026-06-19` or a project version tag.
