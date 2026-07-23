# GUI Cross-Allocator Comparison

The Run tab is for one allocator and one workload preview. The Compare tab is
the entry point for a reproducible multi-allocator scorecard.

## Flow

```text
Compare tab
  -> choose suite and score profile
  -> start benchlab compare batch in an isolated child process
  -> read scorecard JSON
  -> show recommendation and per-allocator summary
```

The GUI never loads allocator DLLs. It locates the repository's `benchlab.exe`
through `HAKOZUNA_BENCHLAB_EXE` or `HAKOZUNA_BENCHLAB_ROOT`, starts the CLI with
the repository root as its working directory, and captures the scorecard. The
CLI remains authoritative for suite execution, raw JSONL output, and scoring.

## User Choices

| Choice | Values |
|---|---|
| Suite | `presubmit`, `integration` |
| Profile | `balanced`, `local-fast`, `remote-heavy`, `low-rss` |

The first connected slice shows the recommended allocator, average speed,
average memory, and profile score. Workload rows remain in the generated JSON
scorecard for later charts and exports.

The current `presubmit` suite contains system, mimalloc, and tcmalloc. HZ8 is
shown in the Run tab's dedicated Windows Preview because its prepared
`bench_mixed_ws_hz8.exe` uses a different runner contract from the Rust CLI
manifest workers; it is not silently represented as a scorecard row.

## Boundaries

- Compare is Preview/Research until provider identity and run hashes are
  surfaced in the GUI.
- A CLI timeout or malformed scorecard is shown as an error; it is not rendered
  as a zero score.
- The GUI does not invent a fallback command when `benchlab.exe` is missing.
- Raw JSONL and scorecard files remain on disk for audit and reruns.
