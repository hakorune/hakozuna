# HZ10 NO-GO Ledger

Durable ledger for HZ10 boxes that were measured or reasoned to a stop. Keep
the active restart surface in `current_task.md`; put closed negative decisions
here so they do not need to be rediscovered.

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
