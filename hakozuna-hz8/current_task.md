# Current Task

## Now

Current focus:

- `ClassMapPolicyQuestion-L1`

Immediate goal:

- Decide whether to ask for a size-class policy change after the remote safety
  closeout.
- Use `ClassFragmentationAudit-L1` evidence: `16..2048` rounding ratio is
  around `1.33`, with most rounded bytes in the 2048B class.
- Do not implement class-map changes before design review.

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
- `ClassFragmentationAudit-L1` is complete:
  bench now reports requested bytes, rounded bytes, rounding ratio, and
  class contribution.  Short checks for `16..2048` show rounding ratio around
  `1.33`, with the 2048B class dominating rounded bytes.  This is a real
  future optimization lane, but it changes size-class policy and should wait
  until the remote safety closeout is clean.
- `RemoteClaimCloseOrdering-L1` is complete:
  remote publish now revalidates slot/live authority after the pending bit
  claim.  If collector publication made the slot non-allocated before the
  producer claimed, the producer rolls back the pending bit and returns
  `INVALID`.
  Short debug interleaved check:
  - `remote_stage validate_fail=0`
  - `duplicate_claim=0`
  - `quiescent_pending bitmap_nonzero=0 repair=0`
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not the `docs/HZ8_BENCH_GATE.md` default-candidate `main_*`
  rows (`16..32768`).
- `VmHWM` is process-wide high-water. Exact per-run peak still needs a
  child-process runner, but the current output prevents post-purge RSS from
  being mistaken for peak RSS.

Implementation notes:

- `slot_shadow_used_mismatch` can still appear in live remote-collect debug
  snapshots because `h8_slot_shadow_verify_span()` is called outside a closed
  publish gate.  Treat quiescent checks as the hard safety authority.
- Class-map policy affects compatibility, active span array size, fragmentation,
  and all matrix comparability.  Do not change it silently.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `BenchSplit-L1`
   - `bench/h8_bench.c` is now 794 lines.
   - Split benchmark helpers before adding more counters.
