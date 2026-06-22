# Current Task

## Now

Current focus:

- `LocalInterleavedAttribution-L1` / DONE
- `PendingCountRuntimeDecisionElision-L1` / DONE
- `QuiescentPendingBitmapGate-L1` / MOSTLY DONE, verify before count removal
- `RemotePublishMicrobench-L1` / DONE
- `TailDrainPolicyAudit-L1` / DONE
- `PerfLanePurification-L1` / DONE
- `TlsLeafEntryFastPath-L1` / DONE

Immediate goal:

- Keep `p2-v0` frozen as the v0 default class map.
- Use the new release/debug attribution lanes to decide the next remote
  closeout box without adding production hot counters.
- Treat `h8_bench_release` as the performance lane and
  `h8_bench_release_audit` / `h8_bench` as attribution lanes.
- Remove `pending_count` from release only after quiescent bitmap gates are
  confirmed clean.
- Next allocator box should be chosen from the remaining local leaf shape:
  active-hit validation trust shadow, class lookup width, or used-count/local-hot
  scalar shape.  Do not reopen owner lease or intrusive remote-head until the
  cleaned perf lane says remote protocol is the blocker again.

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
- `LocalInterleavedAttribution-L1` is complete:
  bench now separates release interleaved work from debug allocator counters.
  Release lane adds:
  - `interleaved_phase_ms work_median / tail_median`
  - `interleaved_work remote_enqueue / local_free / drain_calls /
    drain_objects / drain_empty / push_yields / finish_yields`
  Debug lane keeps allocator-heavy counters under `H8_ENABLE_DEBUG_STATS`.
  Short checks:
  - release interleaved remote90, 16 threads x 50k:
    `remote_enqueue=1440306`, `local_free=159694`,
    `drain_objects=1440306`, `push_yields=0`,
    `finish_yields=22095`.
  - debug interleaved remote90, 16 threads x 20k:
    `remote_stage publish_ok=288298`, `validate_fail=0`,
    `duplicate_claim=0`, `remote_lookup enter=0`, `owner_word=288298`.
    This confirms the common remote path reuses the first span/slot
    classification and does not perform the second full pointer lookup.
  - pending density remains high:
    `slots_per_nonzero_word=13.456`, `multi_ratio=0.859`.
  Interpretation: queue push contention is not the current blocker; remote
  closeout should focus on count/mask authority before considering intrusive
  remote lists.
- `PendingCountRuntimeDecisionElision-L1` is complete:
  collector finish no longer reads `pending_count` in release builds.  Runtime
  collection now relies on pending bitmap, pending word mask, and qstate dirty
  handoff; count/mask/bitmap mismatch diagnostics remain debug-only.
  Short checks:
  - release interleaved remote90, 16 threads x 50k:
    `quiescent_pending bitmap_nonzero=0 repair=0`,
    `push_yields=0`.
  - debug interleaved remote90, 16 threads x 20k:
    `publish_ok=288298`, `validate_fail=0`, `duplicate_claim=0`,
    `quiescent_pending bitmap_nonzero=0 repair=0`.
    Live `count_mask0_bitmap1` remains nonzero as an informational race
    counter, not a hard gate.
- `QuiescentPendingBitmapGate-L1` is already mostly represented in code:
  `h8_span_pending_quiescent()` requires `qstate == IDLE`,
  `pending_word_mask == 0`, and a full pending-bitmap scan of all valid words.
  Owner exit, handoff, and adoption use this helper rather than a count-zero
  shortcut.  Before deleting release `pending_count` updates completely, run one
  focused verification pass over owner-exit and adoption paths.
- Observation after count runtime elision:
  - release local0 RUNS=5, 16 threads x 100k: median about `121.6M ops/s`
    with one low outlier; local rows remain noisy on this host.
  - release interleaved remote90 RUNS=3, 16 threads x 100k:
    median about `34.8M ops/s`, peak RSS median about `37.9MiB`,
    `push_yields=0`.
  - release phase-separated remote90 RUNS=3:
    median about `5.85M ops/s`, peak RSS about `1.87GiB`,
    alloc phase about `196.7ms`, remote publish phase about `11.5ms`.
  - debug interleaved remote90, 16 threads x 50k:
    `publish_ok=720429`, `validate_fail=0`, `duplicate_claim=0`,
    `remote_lookup enter=0`, `owner_word=720429`,
    `pending_word_density slots_per_nonzero_word=10.250`,
    `multi_ratio=0.839`, `quiescent_pending bitmap_nonzero=0 repair=0`.
  - debug phase-separated remote90, 16 threads x 50k:
    `slots_per_nonzero_word=42.403`, `multi_ratio=1.000`,
    `quiescent_pending bitmap_nonzero=0 repair=0`.
  Interpretation: pending word drain is dense; intrusive remote-head is still
  not the next obvious win.  Interleaved remote is closer to the v0 `40M` gate,
  but still short.  Queue push contention is negligible; remaining hot cost is
  more likely owner publish lease, pending bit claim, slot-state validation, and
  tail drain/finish behavior.
- Unsafe evidence knobs:
  `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION=1` is not a valid performance
  proxy because it intentionally drops remote-free publication and explodes
  RSS.  `H8_ENABLE_REMOTE_LEASE_ELISION=1` was noisy and slower in a short
  run, so it should not drive design without a safer microbench.
- `RemotePublishMicrobench-L1` is complete:
  added `bench/h8_remote_micro.c` and Makefile targets
  `remote-micro-release` / `remote-micro`.  The microbench uses a distributed
  owner ring: each thread allocates its own objects, the neighboring thread
  remote-frees them, and each owner then collects its own pending spans.
  Short results:
  - release, RUNS=5, 16 threads x 100k:
    publish median about `128.0M ops/s`, publish phase about `12.5ms`,
    owner collect about `0.7ms`.
  - debug, RUNS=1, 16 threads x 50k:
    `publish_ok=800000`, `lease_ok=800000`, `pending_claim_ok=800000`,
    `validate_fail=0`, `duplicate_claim=0`, `quiescent_repair=0`.
  Interpretation: distributed remote publish alone is not the blocker; it is
  already in the same high-throughput band as the phase-separated remote
  publish phase.  The remaining interleaved gap is more likely mixed
  alloc/free/drain scheduling and tail behavior than owner lease CAS retry.
- `TailDrainPolicyAudit-L1` is complete:
  added `--live-window N` to the benchmark.  `0` preserves the previous
  effectively-unbounded interleaved inbox, while a positive value bounds the
  SPSC queue capacity for live-window experiments.
  Short release interleaved remote90 checks:
  - `live_window=0`: `push_yields=0`, tail about `27ms` in a noisy batch.
  - `live_window=4096`: peak RSS lower, but `push_yields=26801` and tail about
    `76ms`.
  - `live_window=1024`: peak RSS lower again, but `push_yields=108388` and
    throughput remains noisy.
	  Interpretation: reducing the benchmark live window trades RSS for producer
	  stalls and does not expose a clear allocator win.  Keep this as a bench
	  classification knob, not an allocator policy.
- `PerfLanePurification-L1` is complete:
  `bench-release` no longer compiles per-op attribution or fragmentation
  candidate accounting.  `bench-release-audit` preserves the release-build
  attribution lane, and debug `bench` keeps both allocator debug counters and
  bench attribution.  Bench summaries now print `bench_attribution=0/1` and
  `steady_work throughput_median` separately from end-to-end throughput.
  Short verification:
  - `bench-release` interleaved remote90, RUNS=3, 16 threads x 100k:
    end-to-end median about `46.7M ops/s`, steady-work median about
    `47.9M ops/s`, tail median about `15.0ms`, `push_yields=0`,
    `fragmentation attribution=disabled`.
  - `bench-release-audit` short run reports `bench_attribution=1` and keeps
    fragmentation output (`rounding_ratio` about `1.33` for `16..2048`).
  Interpretation: prior interleaved rows mixed allocator work with benchmark
  attribution.  Future performance comparisons should use `bench-release`;
  use audit lanes only when attribution is explicitly needed.
- `TlsLeafEntryFastPath-L1` is complete:
  `h8_thread_ctx_fast()` reads the `_Thread_local` context directly and calls the
  slow helper only on first use.  The slow helper owns `h8_init`,
  `pthread_getspecific`, owner creation, and `pthread_setspecific`.  Small malloc
  no longer calls `h8_init()` before the TLS fast check; free keeps the arena gate
  initialization but uses the fast context lookup after classification.
  Short checks:
  - `make smoke bench-release bench-release-audit bench`
  - smoke passes
  - `bench-release` local0, RUNS=3, 16 threads x 100k:
    end-to-end median about `251.9M ops/s`, steady-work median about
    `274.7M ops/s`
  - `bench-release` interleaved remote90, RUNS=3, 16 threads x 100k:
    end-to-end median about `50.4M ops/s`, steady-work median about
    `52.5M ops/s`, `push_yields=0`, tail median about `13.7ms`
  - debug `bench` short interleaved remote90:
    `publish_ok=72050`, `validate_fail=0`, `duplicate_claim=0`,
    `quiescent_pending bitmap_nonzero=0 repair=0`
  Interpretation: the repeated init/TLS getter cost was a real primary blocker.
  First RUNS=10 confirmation:
  - local0: median about `265.8M ops/s`, p25 about `254.0M ops/s`,
    steady-work median about `285.7M ops/s`
  - interleaved remote90: median about `47.6M ops/s`, p25 about
    `40.9M ops/s`, steady-work median about `49.5M ops/s`, `push_yields=0`
  - phase-separated remote90 lifecycle/RSS stress, RUNS=5: median about
    `6.70M ops/s`, alloc phase about `172.5ms`, remote publish phase about
    `11.3ms`, peak RSS about `1.87GiB`, post-purge RSS about `30MiB`
  Interpretation: local and interleaved remote now clear the v0 small gates in
  a first stable batch.  Interleaved p25 is just over `40M`, so a second fresh
  batch is still needed before freezing the gate.
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
- Current attribution result:
  interleaved producer queues are not backpressuring (`push_yields=0` in the
  short release run).  Remaining remote work is in publish admission,
  pending claim/mask/count, collect finish, and tail drain behavior.

Acceptance:

- `make smoke`
- `make bench-release`
- smoke
- quick `local0`, interleaved remote90, and phase-separated remote90 checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. Run the second fresh stable confirmation batch for the cleaned perf lane:
   - `local0`
   - interleaved remote90
   - phase-separated remote90 as lifecycle/RSS stress
2. Verify `QuiescentPendingBitmapGate-L1` with owner-exit and adoption stress.
3. Consider `TlsActiveHintTrustShadow-L1`
   - shadow whether an active TLS span can skip owner/class/state revalidation
   - no behavior change until mismatch counters are proven zero
4. `PendingWordMaskAuthority-L1`
   - release: no `pending_count` update/load in remote publish or collect
   - debug: keep `pending_count` as a shadow and quiescent consistency check
5. If confirmed interleaved remains below `40M`, ask design review whether to attack:
   - owner publish lease shape
   - slot-state validation duplication
   - tail drain/finish policy
   - or a dedicated remote publish microbench
6. MediumRun and full class-map redesign stay HOLD until small v0 gates are
   closer.
