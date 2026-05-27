*This is a submission for the [GitHub Finish-Up-A-Thon Challenge](https://dev.to/challenges/github-2026-05-21)*

# Reviving Hakozuna HZ5: From Allocator Sidecar Prototype to Citable Research Artifact

## What I Built

I built and revived **Hakozuna HZ5**, an experimental page/run-first sidecar allocator prototype written in C.

Hakozuna started as a personal allocator research project. Earlier versions focused on the HZ3/HZ4 allocator profiles:

- **HZ3 / ACE-Alloc**: a compact, local-heavy allocation profile using PTAG32-based O(1) pointer-to-bin lookup.
- **HZ4**: a remote-free / message-passing profile for remote-heavy and high-thread-count workloads.

HZ5 is the next experimental branch of that work. Instead of only optimizing the existing small-object paths, HZ5 explores a more structural design:

- page/run-first allocation
- sidecar metadata
- fail-closed ownership checks
- descriptor-owned front-ends
- page-oriented remote free
- profile-specific Linux allocator lanes
- an active experimental Windows native build/benchmark path

The project matters to me because it is not only code. It is a record of trying to turn allocator experiments into something reproducible, understandable, and citable. This finish-up pass was about taking HZ5 from "interesting prototype in the tree" to "documented artifact that another person can inspect, cite, and build on."

Project links:

- GitHub repository: https://github.com/hakorune/hakozuna
- HZ5 Zenodo record: https://zenodo.org/records/20411598
- HZ5 DOI: https://doi.org/10.5281/zenodo.20411598
- HZ5 all-version DOI: https://doi.org/10.5281/zenodo.20411597
- HZ3/HZ4 Zenodo record: https://zenodo.org/records/20411402
- HZ3/HZ4 DOI: https://doi.org/10.5281/zenodo.20411402

## Demo

The main demo is the repository and the archived research artifact.

Recommended screenshots or captures to include:

- Repository top page showing the Hakozuna README and DOI badges.
- `hakozuna-hz5/` directory showing the HZ5 source layout.
- Windows build documentation showing the active native Windows path.
- Zenodo HZ5 record page showing the DOI and uploaded English/Japanese PDFs.
- Benchmark or reproducibility documentation from the HZ5 docs.

If adding a short video walkthrough, I would show:

1. The root repository README and how HZ3/HZ4 and HZ5 are separated.
2. The `hakozuna-hz5/` directory layout.
3. The Linux and Windows documentation entry points.
4. The HZ5 design notes and benchmark documentation.
5. The Zenodo DOI page that archives the HZ5 paper PDFs and artifacts.

Suggested video title:

> Hakozuna HZ5 walkthrough: allocator prototype, docs, benchmark notes, and DOI archive

## The Comeback Story

Before this finish-up pass, HZ5 existed as a promising allocator prototype, but the project was still hard to understand from the outside.

The core ideas were there:

- page/run-first allocation
- sidecar metadata
- descriptor/policy separation
- Linux profile experiments
- Windows native build and benchmark notes
- benchmark and reproducibility notes

But the presentation was not finished. HZ5 was still mixed into the broader Hakozuna story, the top-level README still made the project feel mostly Linux-research oriented, and the relationship between HZ3/HZ4 and HZ5 was not clear enough for a reader landing on the repository for the first time.

The comeback work focused on turning the prototype into a clean research artifact:

- clarified HZ5 as a separate experimental profile next to HZ3/HZ4
- prepared English and Japanese paper PDFs
- published the HZ5 artifact on Zenodo
- assigned a citable DOI to HZ5
- updated README links so HZ3/HZ4 and HZ5 have separate DOI references
- clarified the current Windows HZ5 path as experimental and actively being developed
- documented the artifact contents, source layout, and reproducibility materials
- made the repository easier to navigate for future readers

The most important change was not one single optimization. It was making the project legible.

An unfinished allocator prototype can be valuable, but it is fragile if the design context only exists in the author's head. This pass made HZ5 easier to preserve, cite, and continue.

## My Experience with GitHub Copilot

I used GitHub Copilot as a finishing and review partner, not as a replacement for the allocator design work.

For this project, that distinction matters. HZ5 is low-level C allocator work, so I did not want AI to blindly rewrite memory-management logic or make benchmark claims for me. The core allocator design, implementation direction, and benchmark interpretation were driven by my own work and manually checked.

Copilot helped in the finishing stage:

- polishing the README direction
- reviewing the DEV post narrative
- checking whether the public repository had enough entry points for readers
- helping separate the HZ3/HZ4 and HZ5 artifact story
- keeping the claims scoped to the actual profiles and platforms

I also used other AI tools for discussion and drafting, and I am disclosing that openly. The useful pattern was not "AI writes the project." It was closer to having an extra reviewer asking: is the story clear, are the links findable, is the scope honest, and would a new reader know where to start?

That was exactly the kind of help this project needed. HZ5 did not need to be reinvented. It needed to be made understandable, archived, and easier to continue.

## What Changed Technically

HZ5 is organized as a page/run-first sidecar allocator prototype.

Important areas include:

- `hakozuna-hz5/`: HZ5 allocator source
- `api/`: public allocator API surface
- `contract/`: SpeedLane descriptor ABI and purity contract
- `policy/`: HZ5-native allocation/free dispatch policy
- `lowpage/`, `midpagefront/`, and `largefront/`: experimental page/run front-ends
- Linux benchmark scripts and profile matrices
- Windows native build and benchmark documentation

The HZ5 design direction is different from simply tuning the previous allocator profile. It treats page/run ownership, metadata, and profile dispatch as first-class design concerns.

That makes HZ5 useful as a research branch even when individual benchmark lanes are still experimental. I am intentionally keeping performance claims profile-scoped: the interesting results are tied to specific allocator lanes, workloads, and platforms, not a blanket claim that HZ5 is universally faster everywhere.

## What I Learned

Finishing a research-code project is not only about writing more code.

For this project, "finished enough to share" meant:

- the source is present
- the design intent is written down
- benchmark and reproducibility notes exist
- the artifact is archived
- the DOI is stable
- the README tells readers where to start

The biggest lesson was that a prototype becomes much more useful when its boundaries are clear.

HZ5 is not just "the next allocator folder." It is now a named artifact with its own DOI, its own design story, and its own place next to HZ3/HZ4.

## What's Next

Next steps for HZ5:

- continue Linux allocator profile experiments
- continue the active Windows port and native benchmark path
- improve benchmark coverage
- clarify which HZ5 lanes are stable research artifacts and which are exploratory
- add more reproducibility notes
- keep HZ3/HZ4 and HZ5 documentation separated but cross-linked

The comeback is not the end of HZ5. It is the point where the project becomes easier to continue.
