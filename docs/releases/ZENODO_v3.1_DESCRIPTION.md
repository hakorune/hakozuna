# Zenodo v3.1 Description

This release contains v3.1 of the Hakozuna allocator paper and artifacts.

Contents include:

- `main_ja.pdf` / `main_en.pdf`
- benchmark tables and reproducibility docs
- scripts and result summaries for SSOT (`T=16/R=90`), T/R sweeps, `dist_app`,
  RSS snapshots, the mimalloc-bench subset, redis-like workloads, and the
  current Windows-native supplemental benchmark lanes

Profiles:

- `hz3` (ACE-Alloc): local-heavy workloads and low RSS
- `hz4` (message-passing variant): remote-heavy / high-thread scalability

Measurement scope:

- the paper's main reported measurements were collected on Ubuntu (native) with
  fixed build flags
- Windows-native allocator results are included as supplemental tables and
  public benchmark summaries, while Ubuntu remains the main evaluation lane
- see `docs/benchmarks/` for commands, parameters, and environment details

Source code is provided as:

- `hakozuna/` (`hz3`)
- `hakozuna-mt/` (`hz4`)

This release also documents a practical case study of rapid allocator R&D using
LLM-assisted design, implementation, and benchmarking.

Release shape:

- source-only release
- no prebuilt binaries
- public paper PDFs are tracked in `docs/paper/`

Archive references:

- Zenodo record: https://zenodo.org/records/18939227
- DOI: https://doi.org/10.5281/zenodo.18939227
