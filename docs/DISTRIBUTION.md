# Distribution Policy

This repository uses a source-first distribution model.

The public release artifacts are meant to make the allocator variants,
benchmark lanes, and paper-facing claims reproducible. They are not currently
packaged as installers, package-manager feeds, or a universal binary SDK.

## Release Channels

### GitHub Releases

Use GitHub Releases for:

- version tags such as `v3.3.0` and the next source/artifact release
- curated release notes from `docs/releases/GITHUB_RELEASE_*.md`
- the GitHub-generated source archive
- links to paper PDFs, benchmark summaries, and Zenodo records

Do not attach local `out_*` build directories. They are machine-local outputs
and are intentionally excluded from the source archive.

### Zenodo

Use Zenodo for archival snapshots and DOI minting.

Zenodo releases should include:

- the GitHub source archive for the matching tag
- public paper PDFs under `docs/paper/`
- benchmark and reproducibility summaries under `docs/benchmarks/`
- release metadata from `docs/releases/ZENODO_*_DESCRIPTION.md`

Zenodo releases should not include:

- private raw traces
- private working notes
- local `out_*` directories
- prebuilt Ubuntu/Linux, Windows, or macOS binaries unless a release explicitly
  states otherwise

### Local Build Outputs

Local build scripts may create binaries for smoke tests and benchmark runs.
Those outputs are not release artifacts by default.

Examples:

- Linux shared objects and preload smoke binaries from `linux/`
- Windows benchmark executables and DLL copies under `out_win_suite`
- macOS preload smoke artifacts from `mac/`

If a future release publishes prebuilt binaries, it should be a separate,
explicit release tier with platform, compiler, ABI, and safety-mode notes.

## Variant Distribution Scope

### HZ3

`hz3` is the default local-heavy profile and remains the safest public default
for general source distribution.

### HZ4

`hz4` is the remote-heavy / high-thread profile. It remains a public profile
with build and benchmark entrypoints.

### HZ5

HZ5 is distributed as a profile family and research/source artifact track.

Paper-facing HZ5 material should identify the exact profile row being claimed,
for example Linux Local2P appendix rows, MidPage PageRun rows, or LargeFront
rows. Do not present HZ5 as a single default general allocator unless a future
release defines and validates such a profile.

Current HZ5 distribution rules:

- Linux HZ5 profile-family scripts are public source entrypoints.
- Windows HZ5 lanes are experimental/control lanes unless a release note says
  otherwise.
- HZ5 source/artifact releases may include benchmark summaries, lane manifests,
  smoke commands, and paper tables.
- HZ5 releases do not ship prebuilt binaries by default.

## Release Checklist

Before cutting a release:

1. Confirm the working tree is clean.
2. Confirm public paper PDFs exist under `docs/paper/`.
3. Confirm private raw traces and working notes remain under `private/`.
4. Confirm `out_*` directories are ignored and not attached.
5. Update `CHANGELOG.md`.
6. Prepare GitHub Release notes under `docs/releases/`.
7. Prepare Zenodo description and checklist under `docs/releases/`.
8. Record the final DOI and release URL in `README.md` after minting.

## Naming Rules

Use product names consistently:

- `HZ3` / `hz3`
- `HZ4` / `hz4`
- `HZ5`

Current physical paths remain compatibility paths. Do not rename directories as
part of a release unless the migration has its own compatibility plan.
