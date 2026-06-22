# Current Task

## Now

Current focus:

- `ClassFragmentationAudit-L1`

Immediate goal:

- Measure requested live bytes, rounded live bytes, rounding ratio, and
  per-class span contribution.
- Use that data to decide whether size-class refinement is worth opening.
- Do not change allocation policy in this box.

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
- `TlsActiveSpanArray-L1` is complete:
  active hints now live in `H8ThreadCtx`, not `H8OwnerRecord`.
  Representative quick checks:
  - `local0`: about `154.6M ops/s`
  - interleaved remote90: about `30.1M ops/s`
- `RemoteLookupReuse-L1` is complete:
  `h8_free_inner()` reuses the first `span + slot` classification for the
  common remote publish path and falls back to full pointer retry only for
  owner transition.  Representative checks:
  - interleaved remote90: about `30.8M ops/s`
  - phase-separated remote90: about `5.7M ops/s`
- `SpanHotRemoteSplit-L1` is complete:
  immutable span fields, remote hot fields, and local hot fields are separated;
  remote and local hot groups start on cacheline boundaries, and span metadata
  bundles are allocated at 64B alignment.
  Fresh-process quick checks after the split:
  - `local0`: best quick runs about `141M..152M ops/s`; noisy low runs remain
    possible on this host.
  - interleaved remote90: about `28M..31M ops/s` on clean quick runs.
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not the `docs/HZ8_BENCH_GATE.md` default-candidate `main_*`
  rows (`16..32768`).
- `VmHWM` is process-wide high-water. Exact per-run peak still needs a
  child-process runner, but the current output prevents post-purge RSS from
  being mistaken for peak RSS.

Implementation notes:

- Class fragmentation audit should be counter-only first.
- Current class map is pure power-of-two:
  `16, 32, 64, 128, 256, 512, 1024, 2048, 4096`.
- Phase-separated remote90 already showed span lower-bound ratio near `1.0`
  for the current class map, so the next question is class-map rounding waste,
  not excess span commits.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `RemoteClaimCloseOrdering-L1`
   - Recheck collector ordering before final v0 small remote gate.
   - Pending bitmap is remote/remote duplicate-claim authority, so pending
     clear must not expose stale `ALLOCATED` state to a second producer.
