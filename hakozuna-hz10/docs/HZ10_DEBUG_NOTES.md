# HZ10 Debug Notes

Durable debugging record for rare bugs, race fixes, and investigation trails
that are too detailed for `current_task.md`.

Keep entries short enough to scan. Put the full raw logs in `bench_results/`
or the relevant bench README, then link the important commit, symptom, cause,
fix, and verification here.

## 20260705 Pre-Resume Rare Bugs

HZ9 owner flush:

- Commit: `4e6a50d1` (`HZ9: release all-free owner pages on owner flush`)
- Durable HZ9 note: `hakozuna-hz9/docs/HZ9_DEBUG_NOTES.md`
- Symptom class: rare owner-exit retention. An owner flush could see a page
  that became all-free after pending remote frees were collected.
- Root cause: `h9_owner_page_flush_owner()` detached that all-free page like a
  partially live page. That left the mapping allocated with no future exact
  free left to trigger final release.
- Fix: collect pending under the global owner-page lock; if the page is
  all-free, unregister its route entry, mark it draining, detach it, queue it
  for release, drop the lock, then destroy it through the shared helper.
- Boundary: partially live pages still only detach and remain globally routeable
  until final exact frees arrive.

HZ10 retired-ready race series:

- Commits:
  - `4f1f5aff` (`Close two more HZ10RetiredReadyQueue-L0 races found by a thread-reuse bench`)
  - `199fc9c7` (`Document the claim/publish split, cancel() fix, and thread-reuse bench findings`)
- Detailed source notes: `src/hz10_retired_ready.h`
- Main debug trail: `current_task.md` entries named
  `HZ10RetiredReadyQueue-L0 wired into the real path` and
  `Thread-reuse bench finds two more real bugs`.
- Bug #1: ASan found a `retired_sweep_cursor` use-after-free after the ready
  path removed a page from the retired list. Fix: route retired-list removals
  through a helper that updates the sweep cursor before unlinking.
- Bug #2: a page could be both on the ready stack and reachable by the retired
  cursor walk. Fix: add `retired_ready_on_stack`; the cursor walk skips such
  pages and leaves them for ready pop.
- Bug #3: TSan found a cross-thread race where `hz10_page_remote_free()` made
  the slot visible to owner drain before `hz10_retired_ready_note_remote_free()`
  finished touching the page. Fix: split remote free into claim and publish,
  with the ready note between them.
- Bug #4: ready stack ABA if a page was destroyed and the address reused before
  pop. Fix: capture and validate `retired_ready_generation`.
- Bug #5: stale ready tracking after a budgeted retired walk destroyed or
  promoted the same live page that `retired_ready_mark()` still considered
  tracked. Fix: `hz10_retired_ready_cancel()` must win the flag CAS before
  direct destruction/removal; a plain clear was rejected because it reopens the
  stale-push window.
- Load-bearing future rule: any retired-page direct destruction path must skip
  `retired_ready_on_stack` pages and must require `hz10_retired_ready_cancel()`
  before destroying a tracked retired page.

Lifecycle reclaim follow-up:

- Commits:
  - `6c1bcd65` (`HZ10: add public entry lifecycle diagnostics`)
  - `d7063c4e` (`HZ10: drain ready pages during lifecycle reclaim`)
- Review finding: the first reclaim walker reused retired-list traversal but
  missed the ready-stack guards above. It was safe under the narrow benchmark
  call pattern, but unsafe as a reusable quiescent API.
- Fix: lifecycle reclaim now drains ready pages first, and the direct retired
  walk keeps `retired_ready_on_stack` skip plus `hz10_retired_ready_cancel()`
  guard. Busy, ready-deferred, and cancel-deferred pages are reported instead
  of destroyed.

