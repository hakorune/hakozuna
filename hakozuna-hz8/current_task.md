# Current Task

## Now

Current focus:

- `TaggedSlotStateReleaseCutover-L1`

Immediate goal:

- Promote slot-state authority from opt-in to release default.
- Remove live bitmap / `next_free` from release metadata.
- Keep live bitmap only as debug shadow.
- Remove runtime authority flag loads from the hot leaf where possible.

Why this is first:

- `SmallPointerIdentityShift-L1` is complete:
  route/local/remote lookup now reject misaligned/interior slot pointers and
  use shift/mask slot arithmetic.
- `BenchLanePeakRssContract-L1` is complete:
  bench output now reports `post_rss` from `VmRSS` and `peak_rss` from
  process `VmHWM`.
- First peak RSS check shows phase-separated remote90 is a peak-live stress:
  `post_rss ~= 25 MiB` while `peak_rss ~= 1.87 GiB`.
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not README `main_*` (`16..32768`).
- `VmHWM` is process-wide high-water. Exact per-run peak still needs a
  child-process runner, but the current output prevents post-purge RSS from
  being mistaken for peak RSS.

Implementation notes:

- Release authority target:
  `slot_state + pending bitmap`.
- Debug authority target:
  slot_state authority with live bitmap shadow checks.
- Boundary target:
  `ALLOCATED && pending==0 -> VALID`; all other arena-internal states are
  `INVALID`.
- Collector ordering target:
  publish `FREE(next)` before clearing pending.

Acceptance:

- `make smoke`
- `make bench-release`
- default and slot-state smoke rows
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `ActiveSpanRedundantStoreElision-L1`
   - Do not rewrite the same active span on every successful fast-path alloc.
   - Update active hint only when slow path selects a new span or free changes
     the active hint.

2. `TlsActiveSpanArray-L1`
   - Move `active_spans[]` from `H8OwnerRecord` to `H8ThreadCtx`.

3. `RemoteLookupReuse-L1`
   - Reuse the first local-free classification span/slot when falling into
     remote publish.

4. `SpanHotRemoteSplit-L1`
   - Split local hot, remote hot, and immutable span metadata cache lines after
     slot-state release shape is stable.

5. `ClassFragmentationAudit-L1`
   - Measure requested live bytes, rounded live bytes, rounding ratio, and
     per-class span contribution.
