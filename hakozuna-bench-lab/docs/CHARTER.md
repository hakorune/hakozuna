# Product Charter

## Problem

Allocator results are difficult to approach because setup, lane selection,
measurement conditions, and RSS interpretation are spread across scripts and
documents. Hakozuna Bench Lab makes those boundaries visible without hiding
the underlying command line.

## Audience

```text
primary:
  systems programmers comparing allocator tradeoffs
  application developers investigating memory use
  Hakozuna contributors validating a selected lane

secondary:
  readers who want to reproduce a paper or Zenn result
  allocator authors importing an external DLL
```

## Product Promise

```text
Choose allocators.
Choose one documented workload preset.
Run every allocator under matched conditions.
See throughput and memory together.
Export enough evidence to reproduce the result.
```

## Trust Rules

1. A displayed allocator name must come from a manifest, not a filename guess.
2. Diagnostic and speed binaries are different lanes and cannot share a chart.
3. Failed runs remain visible; they are never dropped from a median silently.
4. Preview samples are not promoted into Verified reports.
5. Raw output is retained with every result bundle.
6. Windows and Linux results are never mixed into one absolute ranking.
7. The GUI is an orchestrator, not part of the timed benchmark process.

## Success Criteria

The Windows MVP is complete when a new user can compare system, HZ8,
mimalloc, and tcmalloc on four presets, understand throughput versus peak/post
RSS, and export a self-contained report without editing a script.
