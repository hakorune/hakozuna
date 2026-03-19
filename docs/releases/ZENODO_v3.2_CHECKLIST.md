# Zenodo v3.2 Checklist

Goal

- mint the next Zenodo version from the latest public repository state
- keep the repository head current, while Zenodo preserves the archived snapshot

Release shape

- source-only archive
- no `out_*` directories
- no prebuilt Windows binaries
- include public paper PDFs via `docs/paper/`

Preflight

1. Confirm `main` is pushed
2. Confirm public paper PDFs exist:
   - `docs/paper/main_ja.pdf`
   - `docs/paper/main_en.pdf`
3. Confirm `out_*` remains ignored
4. Confirm private assets stay under `private/`
5. Confirm release notes are prepared from `docs/releases/GITHUB_RELEASE_v3.2.md`

Zenodo metadata to verify

- title
- creators
- description
- version `v3.2`
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
   - release notes
5. Done for this version:
   - record: `19120414`
   - DOI: `10.5281/zenodo.19120414`
