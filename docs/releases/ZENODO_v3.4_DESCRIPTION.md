# Zenodo v3.4 Draft Description

This draft describes the next Hakozuna source/artifact release after v3.3.

The release is source-first. It archives allocator source, public paper PDFs,
benchmark summaries, and reproducibility documentation. It does not publish
prebuilt binaries by default.

## Release Scope

- HZ3 remains the default local-heavy public profile.
- HZ4 remains the remote-heavy / high-thread public profile.
- HZ5 is included as a profile family and source/artifact research track.
- HZ5 measurements must be interpreted by profile, platform, benchmark shape,
  and commit.

## Contents

- `docs/paper/main_ja.pdf` / `docs/paper/main_en.pdf`
- benchmark tables and reproducibility docs
- platform entrypoint docs for Ubuntu/Linux, Windows native, and macOS
- source for:
  - `hakozuna/` (`hz3`)
  - `hakozuna-mt/` (`hz4`)
  - `hakozuna-hz5/` (`HZ5` profile-family source)
- distribution policy:
  - `docs/DISTRIBUTION.md`

## Measurement Scope

- Linux HZ5 profile rows may be paper-facing when their scripts, platform,
  commit, and benchmark summaries are recorded.
- Windows HZ5 lanes remain experimental/control unless a release explicitly
  promotes them.
- HZ5 is not presented as one default general allocator in this release.

## Release Shape

- source-only release
- no prebuilt binaries
- `out_*` directories are not part of the release archive
- private raw results, traces, and working notes remain under `private/`
