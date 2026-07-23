# Changelog

All notable changes to this repository are documented in this file.

This project follows a release-tag workflow (`vX.Y.Z`) and keeps BREAKING changes explicit.

## [Unreleased]

### Added
- HZ5 distribution guidance for the Linux profile family and Windows
  experimental/control lanes.
- Draft v3.4 GitHub Release and Zenodo templates for the next source/artifact
  release.
- `docs/DISTRIBUTION.md` as the release-shape hub for source archives,
  paper/artifact snapshots, and optional local build outputs.
- HZ8 public release wording and Zenodo/GitHub release drafts for the current
  recommended balanced allocator line.
- HZ8 preload hardening notes covering the expanded LD_PRELOAD surface and the
  default remote span-lease publish/backoff fix for xmalloc-style remote storms.
- HZ8 balanced source-release notes, five-minute verification commands,
  explicit rollback boundaries, and links to the HZ8 and HZ10-HZ12 papers.
- A Windows HZ8 release smoke that compiles the selected default flags without
  research or diagnostic lanes and runs outside non-executable source drives.

### Changed
- Clarified that HZ5 is a profile family and paper-facing source/artifact
  track, not a default general allocator or prebuilt binary package.
- Clarified that HZ8 is the current public recommendation, while HZ9/HZ10
  remain research/candidate lines unless a later release promotes them.
- Updated the HZ8 release contract to the current KeepRefill +
  SmallTransitionInventory + GeneralMediumPage + EntryBoundary-L1A default.

### BREAKING
- None.

## [3.3.0] - 2026-05-26

### Added
- Source/artifact release for Linux arm64 coverage and cross-platform
  follow-up lanes.
- Public HZ5 Linux profile-family summaries, including exact Local2P appendix
  rows and broader full-preload profile rows.
- Windows HZ4 ownership-routing fix notes and Mac follow-up lane notes.

### Changed
- Public paper PDFs remain aligned with the v3.2 paper revision; v3.3 is a
  source/artifact release, not a new paper revision.

### BREAKING
- None recorded for this release.

## [3.0.0] - 2026-02-18

### Added
- Paper/artifact update for v3.0 (JA/EN PDFs, benchmark snapshot, release notes).
- Public repository split guidance (`hakozuna/` for `hz3`, `hakozuna-mt/` for `hz4`).

### BREAKING
- None recorded for this release.

[Unreleased]: https://github.com/hakorune/hakozuna/compare/v3.3.0...HEAD
[3.3.0]: https://github.com/hakorune/hakozuna/releases/tag/v3.3.0
[3.0.0]: https://github.com/hakorune/hakozuna/releases/tag/v3.0.0
