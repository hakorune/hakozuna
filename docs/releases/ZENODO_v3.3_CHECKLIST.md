# Zenodo v3.3 Checklist

Goal

- mint the next Zenodo version from the latest public repository state
- keep the repository head current, while Zenodo preserves the archived snapshot

Release shape

- source-only archive
- no `out_*` directories
- no prebuilt Ubuntu/Linux, Windows, Mac, or arm64 binaries
- include public paper PDFs via `docs/paper/`

Preflight

1. Confirm `main` is pushed
2. Confirm public paper PDFs exist:
   - `docs/paper/main_ja.pdf`
   - `docs/paper/main_en.pdf`
3. Confirm `out_*` remains ignored
4. Confirm private assets stay under `private/`
5. Confirm release notes are prepared from:
   - `docs/releases/GITHUB_RELEASE_v3.3.md`
   - `docs/releases/ZENODO_v3.3_DESCRIPTION.md`

Zenodo metadata to verify

- title
- creators
- description
- version `v3.3`
- GitHub source archive linkage
- DOI minting
- related identifiers if needed

Post-mint follow-up

1. Replace placeholder Zenodo lines in the public docs
2. Create the matching GitHub release/tag
3. Paste the curated source-only release body
4. Record the final DOI and record URL in:
   - `README.md`
   - `README_en.txt`
   - `README_ja.txt`
   - `docs/paper/README.md`
   - release notes
