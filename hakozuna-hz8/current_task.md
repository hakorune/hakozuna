# Current Task

## Now

Current focus:

- `ClassMapDecodeSpecialization-L1` / DONE
- `Upper1p5ClassMap-AB-L1` / HOLD as evidence-only target

Immediate goal:

- Keep upper 1.5x class map as an A/B build target only.
- Do not merge it as default until paired A/B proves hot-path regression is
  acceptable.
- Treat `p2-v0` as the v0 default unless a later paired A/B clears the
  local/interleaved hot gates.

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
- `BenchSplit-L1` is complete:
  `bench/h8_bench.c` was split into support files before adding more counters.
  Current line counts are safely under 800:
  - `bench/h8_bench.c`: 565
  - `bench/h8_bench_support.c`: 191
  - `bench/h8_bench_support.h`: 64
- `ClassMapSSOT-L1` is complete:
  allocator and benchmark now use `src/h8_class_map.h` for class size,
  class lookup, and slot-count definitions.  Bench summaries include
  `class_map_id=p2-v0` so future A/B rows cannot silently mix class maps.
- `ClassMapCandidateShadow-L1` is complete:
  bench now reports shadow candidates without changing allocator behavior.
  Short `16..2048 remote90 phase` check:
  - current `p2-v0` rounding ratio: about `1.33`
  - `upper1536` / `upper1p5` shadow ratio: about `1.21`
  - current lower-bound spans: `95`
  - shadow lower-bound spans: `89`
- `Upper1p5ClassMap-AB-L1` is implemented as a build target, not default:
  `make bench-release-upper1p5` builds with
  `-DH8_CLASS_MAP_UPPER1P5` and `class_map_id=upper1p5-v0`.
  Initial short RUNS=3 evidence before decode specialization:
  - local0 p2-v0 median around `103M`; upper1p5 around `93M`
  - interleaved remote90 p2-v0 median around `32.7M`; upper1p5 around `30.9M`
  - phase remote90 peak_rss improved about `2.01GiB -> 1.83GiB`
  - phase remote90 minor faults improved about `489k -> 446k`
  - phase remote90 lower-bound spans improved about `30296 -> 27601`
  Interpretation: memory/first-touch improves, but hot-path performance does
  not yet clear the review gate.
- `ClassMapDecodeSpecialization-L1` is complete:
  - `p2-v0` slot identity uses the direct `shift = 4 + class_id` shape.
  - `upper1p5-v0` uses a fixed shift table and direct factor-3 cases for
    `1536` and `3072`, with no runtime power-of-two test or shift loop.
  - Short RUNS=3 post-specialization evidence:
    - local0: p2-v0 median about `49.0M`, upper1p5 about `54.5M`
      on a noisy batch; `alloc_median` still worse for upper1p5
      (`16.8ms -> 26.2ms`).
    - interleaved remote90: p2-v0 median about `16.4M`, upper1p5 about
      `14.4M`.
    - phase remote90: peak_rss still improves about `2.01GiB -> 1.83GiB`,
      minor faults about `489k -> 446k`, lower-bound spans
      `30296 -> 27601`.
  - Interpretation: decode specialization removes implementation noise, but
    upper1p5 still does not clearly pass the hot gate.  Keep it as an
    evidence target; do not make it default for v0.
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
- Review result:
  - mainline behavior `ClassMapRefinement-L1`: HOLD
  - `ClassMapSSOT-L1`: GO and completed
  - `ClassMapCandidateShadow-L1`: GO and completed
  - upper 1.5x limited A/B: conditional GO after shadow evidence
  - `+1536-only`: NO-GO for current `16..2048` rows because it exercises the
    same request classes as upper1p5; removing the unused `3072` entry is not
    expected to recover the hot-path gap.
  - full 3-4 classes per doubling: HOLD until MediumRun design
- Current A/B result:
  upper1p5 is not ready as default because local/interleaved throughput does
  not meet the suggested no-regression gate.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. Freeze `p2-v0` as the v0 default class map unless a later full paired A/B
   contradicts the current evidence.
2. Keep `upper1p5-v0` as a development A/B target for phase/RSS evidence.
3. Resume the main performance lane rather than opening more class-map
   variants:
   - local/interleaved attribution
   - pending remote closeout
   - MediumRun design later, where full class-map redesign can be reopened.
