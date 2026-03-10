# Hakozuna v3.1

Source-only release for the latest public repository state.

Highlights

- refreshed public paper PDFs in `docs/paper/`
  - `main_ja.pdf`
  - `main_en.pdf`
- Ubuntu/Linux remains the main evaluation lane in the paper
- Windows native allocator results are now included as supplemental paper tables
  - real Redis balanced comparison
  - memcached external-client representative profiles
  - `hz4` Windows-only knob probe summary
- Windows public bring-up lane now includes:
  - native benchmark entrypoints under `win/`
  - Linux public entrypoints under `linux/`
  - public Windows benchmark summaries under `docs/benchmarks/windows/`

Release contents

- source snapshot only
- no prebuilt binaries
- public PDFs in `docs/paper/`

Paper / archive

- Zenodo record: update after the new version is minted
- DOI: update after the new version is minted
- Citation metadata: `CITATION.cff`

Profile guidance

- `hz3` for local-heavy / redis-like workloads
- `hz4` for remote-heavy / high-thread workloads
- Windows current default judgment also remains `hz3`, while `hz4` stays an investigation profile
