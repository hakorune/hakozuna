# Hakozuna HZ10-HZ12 Integrated Paper Release

This release note accompanies the first public HZ10-HZ12 integrated research
paper. The paper treats the three allocator lines as one architectural arc,
not as three independent public allocator products.

## Published Paper

```text
Title:
  Hakozuna HZ10-HZ12: From Ownerless Recycling to Bounded Span Reclamation

Zenodo record:
  https://zenodo.org/records/21360690

Version DOI:
  https://doi.org/10.5281/zenodo.21360690

All-version DOI:
  https://doi.org/10.5281/zenodo.21360689

Published:
  2026-07-15

Version:
  2026-07-15 v0.2 draft
```

The record contains English and Japanese PDFs:

- `main_en.pdf`
- `main_ja.pdf`

## Scope

- HZ10 establishes route, lifecycle, and measurement boundaries.
- HZ11 provides the ownerless speed-first recycling baseline.
- HZ12 adds advisory ownership at batch/retirement boundaries and bounded
  whole-span reclamation.
- HZ8 remains the recommended balanced allocator for normal use.
- The paper does not claim that HZ11 or HZ12 universally replaces tcmalloc,
  mimalloc, or HZ8.

## Suggested GitHub Release Body

```text
This release publishes the first integrated Hakozuna HZ10-HZ12 research paper
in English and Japanese.

HZ10-HZ12 are presented as one architectural progression. HZ10 fixes route,
lifecycle, OS-backing, and measurement boundaries. HZ11 studies lean ownerless
front-cache, transfer-cache, central-span, and recycling paths. HZ12 introduces
advisory ownership only at cold batch and retirement boundaries so complete
spans can be reclaimed with bounded depot capacity and zero-limbo rollback,
without production per-operation diagnostic atomics on the malloc/free hot
path.

The paper includes bounded HZ11 synthetic and real-application evidence and an
HZ12 Linux retirement-turnover result: 512 spans and 32 MiB reclaimed across
eight owner generations, retirement p99 1.144 ms, and zero limbo spans.

HZ8 remains the recommended balanced public allocator. This paper studies the
speed/reclaim tradeoff and does not claim a universal tcmalloc replacement.

Zenodo:
  https://zenodo.org/records/21360690

DOI:
  https://doi.org/10.5281/zenodo.21360690
```

## Repository Pointers

- `hakozuna-hz10/`: route/lifecycle/measurement substrate research
- `hakozuna-hz11/`: ownerless speed-first recycling research
- `hakozuna-hz12/`: advisory ownership and bounded reclamation research
- `docs/releases/ZENODO_hz10_hz12_DESCRIPTION.md`: published metadata summary

## Release Checklist

- [x] English and Japanese PDFs are published.
- [x] Version DOI and all-version DOI are recorded.
- [x] Root English/Japanese repository guides link to the record.
- [x] Public wording keeps HZ8 as the recommended balanced allocator.
- [x] Public wording avoids universal throughput or replacement claims.
