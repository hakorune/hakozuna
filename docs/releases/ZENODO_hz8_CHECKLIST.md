# Zenodo Checklist: HZ8

Use this when creating the HZ8 Zenodo record.

## Metadata

```text
Resource type:
  Software

Title:
  Hakozuna HZ8: Balanced Low-RSS Allocator with Fail-Closed Ownership and
  Remote-Heavy KeepRefill Control

Creators:
  match the existing Hakozuna Zenodo records

License:
  Apache-2.0

Language:
  English
```

## Files

Recommended source archive contents:

```text
include:
  hakozuna-hz8/
  docs/release drafts for HZ8
  README.md
  CITATION.cff

exclude:
  generated binaries
  private raw result directories
  local build outputs
```

If attaching benchmark summaries separately, include:

```text
hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md
hakozuna-hz8/docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
hakozuna-hz8/docs/HZ8_BENCH_GATE.md
```

## Claims Check

Before publishing, verify:

```text
[ ] Description says balanced allocator, not fastest allocator.
[ ] Description mentions low RSS, fail-closed ownership, and remote-heavy
    KeepRefill control.
[ ] Description does not claim universal tcmalloc superiority.
[ ] Windows is described as bring-up/evidence only.
[ ] RUNS / THREADS / ITERS / platform / RSS are present in paper-facing tables.
[ ] Related identifiers include HZ3/HZ4, HZ5, HZ6, and GitHub repository.
```

## After Publish

```text
[ ] Add HZ8 DOI badge/link to README.md.
[ ] Fill TODO fields in docs/releases/GITHUB_RELEASE_hz8.md.
[ ] Update docs/releases/ZENODO_hz8_DESCRIPTION.md with the final record URL.
[ ] Review CITATION.cff if Zenodo provides a preferred HZ8 citation.
[ ] Create or update the GitHub release body from GITHUB_RELEASE_hz8.md.
```

