# HZ10 Orphan Registry Drain Potential Design L0

Design memo for HZ10OrphanRegistryDrainPotential-L0: a behavior-measurement
box that drains the hidden pending remote frees on orphan-registry pages and
counts how many become fully idle (reclaimable) versus how many stay
live-pinned even after draining. This determines whether the remote-heavy
post-RSS residual is drain-recoverable ( -> a quiescent drain+destroy policy
unlocks it) or genuinely live ( -> trim/drain cannot help; the fix is on the
partial-adoption-reach / object-lifetime side). Review document; measurement
only, no allocator behavior change in the default lane.

Status: implemented + measured; GO to HZ10ExplicitQuiescentOrphanPurge-L0.

## Review update: owner proof for the drain probe

`hz10_page_drain_remote()` is explicitly documented as owner-thread-only in
`src/hz10_remote_stack.h`. Therefore this probe must NOT drain an EXITED
owner page "ownerless", even at process exit.

The implementation must reuse the partial-adoption ownership proof:

```text
1. walk the orphan registry under hz10_orphan_lock
2. confirm the old owner record is EXITED
3. temporarily set page->owner_thread_token to a probe/sentinel live owner
4. call hz10_page_drain_remote(page)
5. classify the page
6. restore the old owner token (this box measures only; it does not adopt)
```

This keeps the measurement inside the existing owner-only drain contract
while preserving default allocator behavior. Pages may be drained by the
opt-in exit probe, but they are not destroyed or adopted.

## Measurement result (20260707)

Implemented:

```text
HZ10_SHIM_ORPHAN_REGISTRY_DRAIN_PROBE=1
```

HZ8 public harness, HZ10 preload, THREADS=16 ITERS=50000 RUNS=3 median:

```text
row                         post RSS   depth  already  drain-idle  drain-capacity  truly-live  pending  merged
small_interleaved_remote90   44.2MB      642     16        626           0              0       42,494  42,494
main_interleaved_r90        104.7MB    2,951      7      2,945           0              0       23,170  23,170
medium_interleaved_r50       76.0MB    3,317     27      3,295           0              0        5,095   5,095
main_local0                  35.5MB      570    570          0           0              0            0       0
medium_local0                 6.3MB      160    160          0           0              0            0       0
```

Verdict:

```text
GO: remote-heavy residual is drain-recoverable.
```

The prior "live-pinned" classification was mostly freed-but-undrained
capacity. After a temporary-owner drain, remote-heavy rows are almost entirely
`drain-idle`, with zero `truly-live` pages in this run. This is the missing
proof for `HZ10ExplicitQuiescentOrphanPurge-L0`: at an explicit quiescent
boundary, drain orphan pages, destroy the pages that become fully idle, and
keep any non-idle pages registered.

Record:

```text
bench_results/20260707T_hz10_orphan_registry_drain_potential_l0/
```

## Measured motivation (HZ10OrphanRegistryIdleAgeProbe-L0, 20260707)

The idle-age probe already classified orphan-registry pages on the HZ8-style
public rows (THREADS=16 ITERS=50000 RUNS=3,
`hakozuna-hz8/bench/bench_matrix_malloc.c`), median:

```text
row                         post RSS   depth  idle  live-pinned  hidden pending  age<100ms
small_interleaved_remote90   44.2MB      646    16      630          42,813        646
main_interleaved_r90         96.1MB    2,763     6    2,756          22,359      2,763
medium_interleaved_r50       72.6MB    3,139    29    3,107           4,822      3,139
main_local0                  35.1MB      570   570        0               0        570
medium_local0                 6.3MB      160   160        0               0        160
```

Two facts decide the next move:

1. Remote-heavy rows are overwhelmingly *live-pinned*, not fully idle (16/646,
   6/2763, 29/3139 idle). A blind idle-destroy trim would touch almost none
   of the residual.
2. Hidden pending frees are large on exactly those rows (42.8k / 22.4k /
   4.8k). Pending = remote frees that landed in a (now-dead) owner's stripes
   but were never drained. They show as "live" (`free_count < slot_count`)
   but are actually freed-and-waiting-to-merge.

The probe could not distinguish "freed-but-undrained" from "genuinely handed
out." This box makes that distinction, by draining.

## Hypothesis and central question

Hypothesis: most of the remote-heavy live-pinned population is
freed-but-undrained (drain-recoverable), not genuinely live. If so, draining
turns those pages fully idle and a drain+destroy policy reclaims the residual.

Central question:

```text
Of orphan-registry pages on the remote-heavy rows, how many become
fully idle (free_count == slot_count) after their hidden pending is
drained? How many recover only partial capacity? How many are truly
live-pinned (no drain benefit)?
```

## Classification and accounting

Per orphan page, read `slot_count`, `free_count`, and the pending slot count
in its stripes. Then:

```text
truly_live       = slot_count - free_count - pending
after_drain_free = free_count + pending

already-idle      : free_count == slot_count                (today's "idle")
drain-idle        : free_count < slot_count AND truly_live == 0
                                                    -- drain -> idle -> reclaimable
drain-capacity    : truly_live > 0 AND pending > 0
                                                    -- drain recovers some capacity
                                                       (adoptable, like partial adoption)
truly-live-pinned : pending == 0 AND free_count < slot_count
                                                    -- no drain benefit; out of trim/drain scope
```

`drain-idle` is the headline: pages that *look* live-pinned but become fully
idle once drained. `drain-idle * 64KiB` is the RSS a quiescent drain+destroy
policy could reclaim on that row.

## Mechanism

Two complementary measurements, opt-in via a new probe knob (sibling to
`HZ10_SHIM_ORPHAN_REGISTRY_PROBE`):

### Primary: actual drain at process exit (ground truth)

After the workload and after the `post_rss` sample, walk every orphan
registry page and actually drain it -- reuse the existing
`hz10_page_drain_remote(page)` primitive (the same operation partial
adoption runs as transfer step T3 in
`HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md`). Then count pages with
`free_count == slot_count`.

This is the ground truth: it drains and observes, so there is no estimate
error. It runs at process exit (workers joined, post_rss already sampled),
so it does not affect the measured workload.

Output per row:

```text
depth, already-idle, drain-idle (idle AFTER actual drain), drain-idle bytes
```

### Secondary: read-only per-page breakdown

Read each page's stripe depths WITHOUT draining, apply the classification
above, and report:

```text
already-idle / drain-idle / drain-capacity / truly-live-pinned counts
```

The primary only gives the drain-idle count; the secondary explains the
non-idle remainder (how much is drain-capacity that adoption could consume
vs how much is truly-live that nothing in the trim/drain family can touch).
This routing matters for the NO-GO case.

### Why the exit drain is safe

- Drain (`hz10_page_drain_remote`) exchanges stripe heads and merges pending
  into `local_free_head` / `free_count`. The producer side (remote claim/
  publish into stripes) is owner-agnostic (A1 in the partial-adoption proof);
  the drain side is owner-only because it merges into owner state (O1). The
  probe therefore installs a temporary probe owner before draining, exactly
  like partial adoption installs the adopter before transfer step T3.
- At process exit the worker threads are joined, so there are no concurrent
  producers into the stripes. The drain has exclusive access; zero races.
- The drain mutates page state (`free_count`/`local_free_head` grow) but
  runs after `post_rss` is sampled, so the workload measurement is
  unaffected. It is a pure potential probe.

## Why not estimate-only

A read-only estimate (`free_count + pending >= slot_count`?) is an upper
bound on drain potential and can miscount if a stripe read disagrees with
what an actual drain merges (duplicate pending, ordering edge cases). The
actual drain is self-validating: it performs the real operation and observes
the result. Use the estimate only for the breakdown of the non-idle
remainder, never as the headline.

## Measurement plan

Same HZ8 harness, same five rows, same parameters, opt-in probe:

```text
HZ10_SHIM_ORPHAN_REGISTRY_DRAIN_PROBE=1
THREADS=16 ITERS=50000 RUNS=3
hakozuna-hz8/bench/bench_matrix_malloc.c
```

Expected output shape (the `?` cells are the point of the box):

```text
row                         depth  already  drain-idle  drain-capacity  truly-live  drain-idle bytes
small_interleaved_remote90   646      16        ?             ?              ?            ?
main_interleaved_r90        2763       6        ?             ?              ?            ?
medium_interleaved_r50      3139      29        ?             ?              ?            ?
main_local0                  570     570      (n/a)         (n/a)            0          (already idle)
medium_local0                160     160      (n/a)         (n/a)            0          (already idle)
```

Local rows are already fully idle, so the contest is the three remote-heavy
rows. The headline number is `drain-idle * 64KiB` versus the row's post RSS.

Record:

```text
bench_results/<stamp>_hz10_orphan_registry_drain_potential_l0/
```

## GO / NO-GO (gates the follow-up purge box)

```text
GO (open HZ10ExplicitQuiescentOrphanPurge-L0):
  drain-idle (plus drain-capacity where adoption demand exists) is a material
  fraction of the remote-heavy residual -- e.g. >=50% of depth, or
  drain-idle bytes >= half of post RSS on those rows.
  -> "drain -> idle destroy -> non-idle keep" is the real lever, and it fits
     HZ10's policy (no automatic destructor; the caller provides the boundary).

NO-GO:
  truly-live-pinned dominates even after drain.
  -> the residual is genuine live-set; trim/drain cannot help.
  -> reopen on the partial-adoption-reach or object-lifetime line instead.
```

## Why the follow-up must be an explicit quiescent API

The idle-age probe already established the timing facts that shape the
follow-up:

```text
- HZ8 post_rss is sampled BEFORE process exit.
- Orphan pages are published at worker-thread exit, immediately before that
  sample.
- All sampled pages are <100ms old, so a seconds-scale age policy cannot
  fire before the measurement.
```

Therefore an atexit drain is good hygiene but cannot move the `post_rss`
gate. Only a caller-provided quiescent boundary (drain + idle destroy BEFORE
the sample) can -- which is exactly `HZ10ExplicitQuiescentOrphanPurge-L0`,
and which matches HZ10's no-automatic-destructor contract. This box exists to
prove the drain potential is worth that API before building it.

## Risks

```text
R1 pending count accuracy (secondary estimate only):
   stripe-depth reads can disagree with actual drain merges. Mitigation:
   the primary (actual drain) is the headline and is self-validating; the
   secondary is for breakdown only.
R2 exit drain cost:
   draining every orphan page at exit is O(depth * stripes). It is opt-in
   and post-measurement, so it does not affect the workload, but the probe
   run itself must not take pathological time on deep registries (main_r90
   depth ~2763). Bound with a per-exit budget and continue across exits if
   needed; report incomplete sweeps honestly rather than truncating silently.
R3 measurement point vs in-workload state:
   the exit drain reflects post-workload state (workers joined, all remote
   frees landed), which is the same state the post_rss sample sees. So the
   drain potential measured at exit is the right number for the post_rss
   gate. Do not extrapolate it to in-workload steady state without a
   separate census-triggered probe.
```

## Implementation shape (preview, not commitment)

```text
- new opt-in knob HZ10_SHIM_ORPHAN_REGISTRY_DRAIN_PROBE=1 (sibling to the
  existing HZ10_SHIM_ORPHAN_REGISTRY_PROBE; both can live behind one probe
  build lane).
- at shim atexit (after any exit-stats dump), under the registry lock:
    for each class list, for each page (tail-to-head):
      pending_before = count stripe depths
      old_owner = page->owner_thread_token
      require old_owner state == EXITED
      page->owner_thread_token = probe_owner
      hz10_page_drain_remote(page)        # the partial-adoption T3 op
      classify by free_count / slot_count / pending_before
      page->owner_thread_token = old_owner
      tally
  No destroy in this box -- it measures only. Pages are left drained (fine;
  process is exiting).
- secondary breakdown: the pending_before read above already feeds the
  read-only classification, so the breakdown is a free byproduct of the
  primary sweep (no second walk needed).
- reuse the existing orphan-registry walk + lock; do not add a second
  traversal path.
```

## Open questions for reviewers

```text
1. Resolved before implementation: ownerless drain is not allowed. The probe
   transiently sets a sentinel owner before calling hz10_page_drain_remote()
   and restores the old owner token after classification.
2. Does the existing idle-adoption scan / partial adoption already drain
   some registry pages in-place (the P1 open question from the partial-
   adoption doc)? If so, the "hidden pending" the idle-age probe counted is
   already partially drained, and this box's drain potential is a lower
   bound. Reconcile the counts.
3. Stripe depth read primitive: is there an existing way to count pending
   slots without draining (for the secondary breakdown), or must the box
   add one? If adding one, it must match exactly what drain merges or R1
   bites. Prefer deriving the breakdown from the primary sweep's
   pending_before (see implementation shape) to avoid a second primitive.
4. Should the probe also report per-class drain-idle (class 8 dominated
   larson in the partial-adoption measurement)? The HZ8 rows may concentrate
   on different classes; per-class breakdown would tell the purge box which
   classes to target. Low cost to add.
```
