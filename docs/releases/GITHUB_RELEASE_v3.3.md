# Hakozuna v3.3

This release contains v3.3 of the Hakozuna allocator source, paper PDFs, and
public benchmark artifacts.

Paper note:

- the public paper PDFs in `docs/paper/` are unchanged from the v3.2 paper
  revision
- v3.3 is mainly a source / artifact release for Linux arm64 coverage,
  ownership-routing fixes, and cross-platform follow-up lanes

Highlights since v3.2:

- Linux arm64 benchmark and profiling lanes are now public
- the Linux arm64 preload ownership-routing bug is fixed
- Windows HZ4 now keeps the foreign-pointer safety boundary while recovering the
  duplicated free-route ownership cost through a one-shot ownership box
- Mac follow-up lanes now include the foreign-pointer safety promotion and the
  current `malloc-large` research box

Contents include:

- `docs/paper/main_ja.pdf` / `docs/paper/main_en.pdf`
- benchmark tables and reproducibility docs
- public benchmark summaries for Ubuntu/Linux, Windows native, macOS, and Linux
  arm64 follow-up lanes
- source for:
  - `hakozuna/` (`hz3`)
  - `hakozuna-mt/` (`hz4`)

Profiles:

- `hz3` (ACE-Alloc): local-heavy workloads and low RSS
- `hz4` (message-passing variant): remote-heavy / high-thread scalability

Measurement scope:

- the paper's main reported measurements remain Ubuntu-native with fixed build
  flags
- Windows native remains the supplemental/public app-bench lane
- Mac and Linux arm64 are public development / cross-platform follow-up lanes
- see `docs/benchmarks/` for commands, parameters, and environment details

Release shape:

- source-only release
- no prebuilt binaries
- `out_*` directories are not part of the release archive
- public paper PDFs remain tracked in `docs/paper/`
