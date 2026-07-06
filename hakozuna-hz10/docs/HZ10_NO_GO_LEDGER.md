# HZ10 NO-GO Ledger

Durable ledger for HZ10 boxes that were measured or reasoned to a stop. Keep
the active restart surface in `current_task.md`; put closed negative decisions
here so they do not need to be rediscovered.

## 20260707 Shim Stack-Protector Removal

Status: `NO-GO`

Evidence:

- `bench_results/20260707T_shim_no_stack_protector_l0/notes.md`
- Motivation was valid: post-internal-binding `hz10_free` annotate showed
  visible stack-canary instructions (`%fs:0x28`) from fallback route-result
  stack storage.
- Probe added `-fno-stack-protector` to `SHIM_CFLAGS` only.
- Functional gates passed:
  `preload`, preload siblings, `smoke-shim-api`, `smoke-shim-foreign`,
  `hz10-standalone-check`.
- Codegen gate passed: canary instructions disappeared from `hz10_free`.
- RUNS=5 hz10-only macro regressed the target:
  - sh6bench: `0.470s -> 0.490s`
  - python_alloc: `0.850s -> 0.870s`
  - larson/mstress/RSS stayed flat or within noise.

Decision:

- Do not add `-fno-stack-protector` to HZ10 preload builds.
- The canary samples were real, but removing them changed code layout/register
  allocation enough to hurt wall time.
- The Makefile change was reverted; keep this as a codegen trap.
- Next measured target is runtime division in pagemap local route / interior
  validation, not another stack-protector attempt without new evidence.

## 20260705 Pending-Bit Correctness Design

Status: `NO-GO for now`

Evidence:

- `bench_results/20260704T205540Z_hz10_pending_order_ceiling_matrix/combined.log`
- THREADS=4 SLOT_COUNT=4096 REPEAT=20000 RUNS=3:
  - `pending_acqrel_fetch_or` ~3.33ns/op
  - `pending_relaxed_fetch_or` ~3.40ns/op
  - `treiber_no_pending_unsafe` ~24.86ns/op
  - `pending_acqrel_treiber` ~40.48ns/op
  - `pending_relaxed_treiber` ~40.47ns/op
  - `pending_counter_treiber` ~73.45ns/op

Decision:

- Do not open the pending-bit replacement/weakening box now.
- The pending bit is the remote-remote duplicate-free guard; changing it is a
  correctness-contract change, not a simple hot-path cleanup.
- The unsafe no-pending ceiling is useful attribution, but throughput already
  clears the HZ8 gate in the latest same-run comparison while the failing gate
  is RSS/lifecycle behavior.
- Do not generalize the x86_64 ordering result to ARM/AArch64 without direct
  measurement.

## 20260705 Move-To-Front Active List

Status: `NO-GO`

Evidence:

- `bench_results/20260704T215750Z_hz10_active_mtf_ab_l0/combined.log`
- main_r50 regressed 13.97M -> 13.20M ops/s (-5.5%).
- Scan counters worsened instead of improving.

Decision:

- Do not turn move-to-front on for HZ10 active lists.
- The active-list scan shape is not a simple repeated-hit locality problem for
  the dominant classes in main_r50/r90.

## 20260705 Two-Slot Active Ping-Pong

Status: `NO-GO`

Evidence:

- `bench_results/20260704T221256Z_hz10_two_slot_active_pattern_l0/combined.log`
- main_r50 class 20/21 median:
  - active-page activation handled ~4.22 allocs on average.
  - immediate exhaustion ~9.5%.
  - remembering the previous active page would catch only ~14% of misses.
- main_r90 class 20/21 median:
  - activation handled ~2.24 allocs on average.
  - immediate exhaustion ~18.1%.
  - previous-active hit rate ~5.8%.

Decision:

- Do not implement `HZ10SecondActiveByClass-AB-L0`.
- A true two-page ping-pong would hit close to 100%; this workload is spread
  across more than two live pages per thread.

## 20260705 HZ8-Style Reclaim As Immediate Fix

Status: `NO-GO as next step`

Evidence:

- Steady-state main_r50/r90 runs did not reproduce unbounded retired-page
  growth inside one live worker population.
- Fixed-iteration RUNS=10 RSS growth was reframed as a thread-lifecycle /
  fresh-worker abandonment signal, not proof that HZ8-style per-run reclaim is
  the right active-list fix.

Decision:

- Do not implement broad HZ8-style reclaim as the immediate answer for
  main_r50/r90 RSS.
- Keep lifecycle reclaim as an explicit quiescent-boundary tool, measured
  separately from work-loop throughput.

## 20260705 Remote Publish Batching For Normal Free

Status: `NO-GO for normal public free path`

Evidence:

- Contract design: `hakozuna-hz10/docs/HZ10_REMOTE_PUBLISH_BATCH_CONTRACT_L0.md`
- Locality log:
  `bench_results/20260704T235343Z_hz10_remote_publish_batch_locality_l0/combined.log`
- Same-call ideal publish CAS reduction ceilings:
  - main_r50: 4.92%
  - main_r90: 5.19%
  - small_remote50: 12.97%
  - small_remote90: 15.40%
  - slot_count1_r90: 0.00%

Decision:

- Do not implement remote publish batching for ordinary `hz10_free(ptr)` now.
- The main rows barely batch, and slot_count=1 cannot batch at all.
- Keep a same-call batch publish helper as a possible future primitive only
  for a bulk/inbox path that already owns a batch.
- Producer TLS staging remains out of scope until HZ10 has a producer-side
  quiescent flush contract; returning success before publication would break
  `accepted == drainable`.

## 20260705 Front-Cache Slot-Count Threshold

Status: `NO-GO`

Evidence:

- `bench_results/20260705T041052Z_hz10_front_cache_l1/notes.md`
- HZ10_FRONT_CACHE_MIN_CLASS=17 (classes below 8192 bytes bypass the front
  cache), same-session interleaved RUNS=6 medians:
  - small_local0: 144.3M vs 152.6M (all-front) vs 154.7M (flag-off)
  - main_local0: 153.3M vs 173.2M (all-front) vs 169.0M (flag-off)

Decision:

- Do not ship a per-class front-cache bypass threshold. On mixed-size rows
  the branch is data-dependent (~25% of byte-uniform ops fall below class
  17) and mispredicts its way to -9% on main_local0; even always-taken it
  did not recover the flag-off small_local0 number.
- The compile-time knob stays available for future measurement, default 0
  (disabled, zero cost when 0).
- If the small-class front-cache delta (-1.4%..-4.6% by session) must go
  to zero, attack the front fast path itself, not a bypass branch.

## 20260705 Route Reciprocal For Multi-Slot Records

Status: `NO-GO`

Evidence:

- E2b prototype added per-record `slot_magic` / `slot_shift` and changed the
  inline local route from `% slot_size` to reciprocal multiply + multiply-back.
- The differential route smoke was strengthened to compare fast vs slow for
  every offset in each multi-slot registration span; correctness passed.
- Sanitizers passed: ASan/UBSan smoke clean, TSan smoke clean via
  `smoke-tsan-aslr-off`.
- Standard stage-cost protocol
  (`THREADS=4 PAGES=4096 REPEAT=20000 RUNS=10`) regressed the inline route:
  - E1 baseline: `route_fast` median ~1.57ns.
  - E2b prototype: `route_fast` median ~1.81ns.
  - The prototype also enlarged `H10PageRecord` from 32B to 48B.

Decision:

- Do not land per-record reciprocal fields for multi-slot route.
- The extra record loads and larger record footprint cost more than the
  division they remove on this path.
- Keep the exhaustive differential route smoke; it is the right gate if a
  future division-free shape is attempted without growing the hot record.

## 20260705 Front-Cache Dense Array For All Classes

Status: `NO-GO as the all-class/default front-cache representation`

Evidence:

- Prototype added the opt-in lane `HZ10_ENABLE_FRONT_CACHE_ARRAY=1` and make
  targets:
  - `smoke-public-entry-front-array`
  - `bench-public-entry-front-array`
  - `bench-public-entry-local-path-front-array`
- The array lane keeps the existing front-cache marker/accounting contract:
  cached slots still count as allocated from the page's point of view, and
  overflow/lifecycle flush returns them through the page layer.
- Correctness checks passed during the probe: public-entry front smokes,
  standalone check, ASan/UBSan smoke, and the local TSan gate through
  `smoke-tsan-aslr-off`.
- Local-path comparison in the same session (`THREADS=4 RUNS=3`) showed a
  split result:
  - 65536-byte alloc_free improved: intrusive front median ~25.1ns -> array
    ~16.6ns.
  - Small classes regressed: 16B ~12.9ns -> ~13.9ns, 64B ~17.6ns -> ~21.4ns.

Decision:

- Do not make dense array storage the default representation for all
  front-cache classes.
- The measured reason is class-dependent: for small classes, the intrusive
  link reuses the same slot cache line already touched for the local-free
  marker, while the array adds a separate TLS-array line; for 64KiB
  slot_count=1 classes, the array removes the aliasing-dependent `*head`
  load and is materially faster.
- Keep the opt-in array lane for future large-class-only or hybrid-front
  experiments, but do not enable it for the current all-class front cache.
- The next speed box should be remote-gap attribution, not another unmeasured
  front-cache storage change.

## 20260705 Drain Micro-Trim

Status: `NO-GO`

Evidence:

- `bench_results/20260705T120506Z_hz10_remote_gap_attribution_l0/notes.md`
  (`F1 attempted same-day: NO-GO, reverted`).
- Prototype shortcut avoided the per-slot hardware division in
  `hz10_page_drain_remote()` for slot_count 1/2.
- First main_r50 A/B looked like +12%, but did not reproduce.
- Repeated alternating A/B (`5` alternations x `RUNS=6`, n=30/side):
  base median 13.76M ops/s, F1 median 13.30M ops/s, ratio 0.966.
- The row itself varied roughly 11.1M-16.0M ops/s (+/-18%) on the same
  machine because remote rows spend ~30% of wall time in futex sleep/wake.
- Isolated `bench-remote-stack-drain` slightly regressed the multi-slot path
  the shortcut cannot help: owner_drain base ~213-228M ops/s vs F1
  ~206-217M ops/s.

Decision:

- Do not land the slot_count 1/2 drain-division shortcut.
- The division is too small a contributor (~2% of the r50 row) to survive
  the remote-row noise floor, and the extra compares can hurt the multi-slot
  path.
- Future remote A/Bs need either n>=30/side with alternation or a direct
  perf-counter endpoint. For HZ10PendingStripeColocate-L0, cache-miss/op is
  the primary gate and ops/s is secondary.

## 20260706 Front Cache Default-On

Status: `NO-GO as a global default`

Evidence:

- `bench_results/20260706T180832Z_hz10_front_default_on_ab_l0/notes.md`
- Full public-entry matrix, `THREADS=4 ITERS=500000`, 10 off/on alternations
  per row, front ON built with `-DHZ10_ENABLE_FRONT_CACHE=1`.
- Median front ON / OFF:
  - main_local0: 0.978 (-2.2%)
  - main_r50: 1.089 (+8.9%)
  - main_r90: 1.080 (+8.0%)
  - medium_local0: 1.008 (+0.8%)
  - small_local0: 0.948 (-5.2%)
  - small_remote50: 1.016 (+1.6%)
  - small_remote90: 0.983 (-1.7%)
  - slot_count1_local0: 1.028 (+2.8%)
  - slot_count1_r90: 0.990 (-1.0%)

Decision:

- Do not flip `HZ10_ENABLE_FRONT_CACHE` to default ON.
- The remote-main gains are real, but the all-row default violates the
  small_local0 regression gate and also regresses main_local0 in this
  interleaved session.
- Keep the front cache opt-in for bulk/local-path and targeted experiments.
  Reopen only with a small-class-safe shape or a selective policy that avoids
  reintroducing the measured threshold-branch regression.

## 20260706 Fine Size Classes As Default

Status: `NO-GO as default; opt-in retained`

Evidence:

- `bench_results/20260705T205340Z_hz10_class_granularity_l1/`
- Prototype changed the default table from 24 classes to 38 classes:
  quarter-step spacing (`2^k * {1, 1.25, 1.5, 1.75}`) in the 64..8192
  band, with the old 1.5x/2x spacing above 8192 to avoid burning whole
  single-quantum page tails.
- The macro attribution was real: the `python_alloc` shim row improved
  median RSS from 116.9MB to 106.7MB and median wall time from 0.905s to
  0.890s in the same session.
- The default microbench gate failed. Initial full public-entry A/B
  (`THREADS=4 ITERS=200000`, 20 alternations, base vs default-fine):
  - main_local0: 0.750x ops/s, post_rss 1.531x
  - small_local0: 0.852x ops/s
  - medium_local0: 0.969x ops/s, post_rss 1.167x
  - remote rows were mixed/noisy rather than a compensating win.
- After trimming the fine lookup, a shorter 4-row A/B still showed the
  key local regression:
  - main_local0: ~0.77x
  - small_local0: ~0.93x
- Later macro-matrix evidence confirmed that the fine table is not a broad
  shim/product opt-in either:
  - `bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/`
  - `python_alloc` RSS improved (`hz10` 116.8MB -> `hz10+fine` 106.6MB),
    but wall time was slightly worse than default HZ10.
  - Redis server RSS worsened (`hz10` 7.6MB -> `hz10+fine` 8.2MB).
  - Larson worsened materially: sampled current RSS about 7.8GB -> 9.2GB,
    and throughput about 1.061M -> 0.971M ops/s.

Decision:

- Do not enable fine size classes by default.
- Keep `HZ10_ENABLE_FINE_SIZE_CLASSES=1` as an opt-in python/RSS diagnostic
  lane, not as a broad shim recommendation. It preserves the useful evidence
  that class rounding drives a large part of `python_alloc` RSS, while
  making rollback immediate.
- Reopen only with a policy that avoids spreading interleaved/local
  workloads across too many live classes/pages, or with a macro-specific
  shim mode justified by product-level measurements.

## 20260707 Retired Local Idle Reclaim As Default

Status: `NO-GO as default; opt-in research lane retained`

Evidence:

- `bench_results/20260706T220000Z_hz10_retired_local_idle_reclaim_l0/`
- `bench_results/20260707T004000Z_hz10_retired_local_macro_gate_bounded/`
- The structural diagnosis is real. In `python_alloc`, default HZ10 retained
  a large local-churn retired backlog:
  - default median: `active_pages=684`, `retired_pages=933`,
    `page_bytes=105971712`, `pool_reuse=230`.
  - retired-local opt-in median: `active_pages=684`, `retired_pages=2`,
    `page_bytes=44957696`, `pool_reuse=696`,
    `reclaimed_local_free=1807`.
- However, max RSS and wall time did not justify default enablement:
  - default `python_alloc`: `max_rss_kb=116824`, `wall=0.910s`.
  - retired-local opt-in: `max_rss_kb=113144`, `wall=0.900s`.
  - fine classes alone did better on this row:
    `max_rss_kb=106748`, `wall=0.870s`.
  - fine+retired-local reduced retained page footprint but lost to fine
    alone on max RSS/wall: `max_rss_kb=108156`, `wall=0.920s`.
- Selected micro A/B also found local-path risk even after bounding the hook
  to the 80..4096 slot-size band:
  - `small_local0` and `slot_count1_local0` were noisy across iterations,
    with slot-count-1 still showing about a 4-7% opt-in regression in
    several short alternating checks.

Decision:

- Do not enable `HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM` by default.
- Keep the lane for research because it cleanly proves and repairs the
  retired-backlog mechanism; use it when measuring retained footprint, not
  as a product default.
- The next RSS work should prefer a macro-specific class policy or a better
  peak-RSS reducer. Local idle reclaim needs a lower-cost trigger or a
  page-state design that has no measurable effect on excluded local rows
  before it can reopen as a default candidate.
