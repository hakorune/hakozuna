# Zenodo v3.4 Draft Checklist

Goal

- mint the next Zenodo version from the matching GitHub release tag
- preserve the source/artifact snapshot for HZ3, HZ4, and the HZ5 profile-family
  documentation

Release shape

- source-only archive by default
- no `out_*` directories
- no prebuilt Ubuntu/Linux, Windows, macOS, or arm64 binaries
- include public paper PDFs via `docs/paper/`
- include release and distribution docs

Preflight

1. Confirm `main` is pushed.
2. Confirm the tag name and GitHub release version.
3. Confirm public paper PDFs exist:
   - `docs/paper/main_ja.pdf`
   - `docs/paper/main_en.pdf`
4. Confirm `out_*` remains ignored.
5. Confirm private raw traces and working notes stay under `private/`.
6. Confirm HZ5 rows in README / benchmark summaries include profile names and
   platform scope.
7. Confirm release notes are prepared from:
   - `docs/releases/GITHUB_RELEASE_v3.4.md`
   - `docs/releases/ZENODO_v3.4_DESCRIPTION.md`

Zenodo metadata to verify

- title
- creators
- description
- version `v3.4`
- GitHub source archive linkage
- DOI minting
- related identifiers if needed

Post-mint follow-up

1. Replace placeholder Zenodo lines in the public docs.
2. Create or update the matching GitHub release/tag.
3. Paste the curated source-only release body.
4. Record the final DOI and record URL in:
   - `README.md`
   - `README_en.txt` if present
   - `README_ja.txt` if present
   - `docs/paper/README.md` if present
   - release notes
