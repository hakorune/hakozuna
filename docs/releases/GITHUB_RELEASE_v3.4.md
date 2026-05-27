# Hakozuna v3.4 Draft

This draft is for the next source/artifact release after v3.3.

Use v3.4 if the release mainly updates source, build scripts, benchmark
summaries, and HZ5 profile-family artifacts without changing the public paper
revision. Use a larger version bump only if the paper, public API, or default
profile story changes substantially.

## Release Scope

- Source-only release by default.
- No prebuilt binaries by default.
- Public paper PDFs remain tracked in `docs/paper/`.
- HZ3 and HZ4 remain the practical public allocator profiles.
- HZ5 is presented as a profile family and source/artifact research track, not
  as one default general allocator.

## Highlights Since v3.3

- HZ5 Linux profile-family documentation and benchmark summaries.
- HZ5 distribution guidance separating paper-facing profiles from
  experimental/control lanes.
- Windows lane documentation clarifying which HZ5 controls are experimental and
  which HZ3/HZ4 profiles remain practical.
- Release-shape documentation in `docs/DISTRIBUTION.md`.

## Contents

- Source for:
  - `hakozuna/` (`hz3`)
  - `hakozuna-mt/` (`hz4`)
  - `hakozuna-hz5/` (`HZ5` profile-family source)
- Public paper PDFs:
  - `docs/paper/main_ja.pdf`
  - `docs/paper/main_en.pdf`
- Benchmark and reproducibility summaries under `docs/benchmarks/`
- Platform entrypoint documentation:
  - `linux/`
  - `win/`
  - `mac/`
- Distribution policy:
  - `docs/DISTRIBUTION.md`

## Measurement Scope

- HZ3 remains the default local-heavy profile.
- HZ4 remains the remote-heavy / high-thread profile.
- HZ5 rows must be reported with their exact profile names and platform scope.
- Linux HZ5 profile rows are paper-facing only when the matching scripts,
  commit, platform, and benchmark summary are recorded.
- Windows HZ5 rows remain experimental/control unless explicitly promoted in a
  release note.

## Release Shape

- Source-only release.
- No prebuilt Ubuntu/Linux, Windows, or macOS binaries.
- `out_*` directories are not part of the release archive.
- Private raw results, local traces, and working notes stay under `private/`.
