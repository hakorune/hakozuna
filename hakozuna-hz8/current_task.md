# Current Task

## Now

Current focus:

- `ActiveSpanRedundantStoreElision-L1`

Immediate goal:

- Stop rewriting `owner->active_spans[class]` on every successful fast-path
  allocation.
- Keep active span as a weak hint, not ownership truth.
- Update the hint only when slow path selects a span or local free changes the
  active span.

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
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not README `main_*` (`16..32768`).
- `VmHWM` is process-wide high-water. Exact per-run peak still needs a
  child-process runner, but the current output prevents post-purge RSS from
  being mistaken for peak RSS.

Implementation notes:

- Fast allocation from current active span should not store the same active
  pointer back.
- Slow path should publish a new active hint once after selecting/committing a
  usable span.
- Local free can keep the current behavior only if it actually changes the hint.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `TlsActiveSpanArray-L1`
   - Move `active_spans[]` from `H8OwnerRecord` to `H8ThreadCtx`.

2. `RemoteLookupReuse-L1`
   - Reuse the first local-free classification span/slot when falling into
     remote publish.

3. `SpanHotRemoteSplit-L1`
   - Split local hot, remote hot, and immutable span metadata cache lines after
     slot-state release shape is stable.

4. `ClassFragmentationAudit-L1`
   - Measure requested live bytes, rounded live bytes, rounding ratio, and
     per-class span contribution.
