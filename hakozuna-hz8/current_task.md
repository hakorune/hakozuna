# Current Task

## Now

Current focus:

- `SmallPointerIdentityShift-L1`

Immediate goal:

- Treat interior and misaligned small pointers as `INVALID`.
- Replace runtime class-size division in pointer-to-slot classification with
  power-of-two shift/mask arithmetic.
- Keep `MISS` limited to pointers outside the HZ8 arena.

Why this is first:

- HZ8's contract says owned-looking interior pointers must not fall through to
  the platform allocator.
- The current benchmark rows are `guard_*`-equivalent because they use
  `16..2048`, not README `main_*` (`16..32768`).
- Current RSS readings are post-owner-exit/post-purge `VmRSS`, not peak RSS.
  Peak RSS needs a separate `VmHWM`/child-process contract.

Implementation notes:

- Add one checked slot helper used by route, local free lookup, and remote
  publish lookup.
- Class sizes are powers of two, so `shift = 4 + class_id`.
- Reject `offset & ((1 << shift) - 1)` before computing `slot`.
- Default non-slot-state route must also reject `pending` slots.
- Default remote publish must validate live state before claiming pending.

Acceptance:

- `make smoke`
- `make bench-release`
- quick `local0` and `remote90` sanity checks
- no source file over 800 lines
- worktree clean after removing generated binaries

## Next

1. `BenchLanePeakRssContract-L1`
   - Split reported RSS into peak RSS (`VmHWM`/`ru_maxrss`) and post-purge
     `VmRSS`.
   - Document `guard_*`, phase-separated, and interleaved lanes separately.

2. `TaggedSlotStateReleaseCutover-L1`
   - Make slot-state authority the release default.
   - Remove live bitmap / `next_free` from release metadata.
   - Keep live bitmap only as debug shadow.
   - Inline slot-state helpers in the hot leaf.

3. `ActiveSpanRedundantStoreElision-L1`
   - Do not rewrite the same active span on every successful fast-path alloc.
   - Update active hint only when slow path selects a new span or free changes
     the active hint.

4. `TlsActiveSpanArray-L1`
   - Move `active_spans[]` from `H8OwnerRecord` to `H8ThreadCtx`.

5. `RemoteLookupReuse-L1`
   - Reuse the first local-free classification span/slot when falling into
     remote publish.

6. `SpanHotRemoteSplit-L1`
   - Split local hot, remote hot, and immutable span metadata cache lines after
     slot-state release shape is stable.

7. `ClassFragmentationAudit-L1`
   - Measure requested live bytes, rounded live bytes, rounding ratio, and
     per-class span contribution.
