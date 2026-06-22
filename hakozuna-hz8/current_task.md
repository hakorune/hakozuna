# Current Task

## Now

Current focus:

- `TlsActiveSpanArray-L1`

Immediate goal:

- Move active span hints from `H8OwnerRecord` to `H8ThreadCtx`.
- Keep owner lists as ownership truth.
- Keep active spans as per-thread weak hints only.

Why this is first:

- `SmallPointerIdentityShift-L1` is complete:
  route/local/remote lookup now reject misaligned/interior slot pointers and
  use shift/mask slot arithmetic.
- `BenchLanePeakRssContract-L1` is complete:
  bench output now reports `post_rss` from `VmRSS` and `peak_rss` from
  process `VmHWM`.
- First peak RSS check shows phase-separated remote90 is a peak-live stress:
  `post_rss ~= 25 MiB` while `peak_rss ~= 1.87 GiB`.
- `TaggedSlotStateReleaseCutover-L1` is complete:
  slot-state authority is release default, release metadata no longer allocates
  live bitmap / `next_free`, and hot slot-state helpers are inline.
- Representative checks after cutover:
  - `local0`: about `143.7M ops/s`
  - interleaved remote90: about `29.5M ops/s`
  - phase-separated remote90: about `5.8M ops/s`
  - smoke passes
- `ActiveSpanRedundantStoreElision-L1` is complete:
  fast-path allocation no longer rewrites the same active hint.  Representative
  `local0` quick check is about `146.7M ops/s`.
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not README `main_*` (`16..32768`).
- `VmHWM` is process-wide high-water. Exact per-run peak still needs a
  child-process runner, but the current output prevents post-purge RSS from
  being mistaken for peak RSS.

Implementation notes:

- `H8ThreadCtx` should own `active_spans[H8_CLASS_COUNT]`.
- Owner-level list removal/exit/adoption must no longer assume active hints
  live on the owner record.
- If a TLS hint is stale, existing owner/span/generation/state validation must
  reject it and fall back to scan/commit.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `RemoteLookupReuse-L1`
   - Reuse the first local-free classification span/slot when falling into
     remote publish.

2. `SpanHotRemoteSplit-L1`
   - Split local hot, remote hot, and immutable span metadata cache lines after
     slot-state release shape is stable.

3. `ClassFragmentationAudit-L1`
   - Measure requested live bytes, rounded live bytes, rounding ratio, and
     per-class span contribution.
