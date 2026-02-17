# PHASE S236 Aggressive Consult Result

Last updated: 2026-02-13 (default promotion locked; S236-C/D/E/F/G archived)

## Verdict
- Consultation result is accepted as the next design baseline for aggressive lane.
- Core interpretation aligns with locked observations:
  - medium remote dispatch remains `n==1` dominant,
  - `S230` engaged but caused slowpath amplification (`alloc_slow_calls` surge),
  - therefore the next box must provide **pre-slowpath consumable supply**.

## Locked Objective (aggressive lane)
- Target:
  - `main` (`MT remote r90 16..32768`) `>= +10%` vs current `S222+S233` baseline.
- Acceptance:
  - `guard (16..2048) >= -6.0%`
  - `larson (4KB..32KB) >= -4.0%`
  - crash/abort/alloc-failed must stay zero.

## Candidate Order (locked)
1. `S236-A`: Medium remote mailbox + alloc-fast pop.
2. `S236-B`: mini-refill fast lane before full `alloc_slow`.
3. `S236-C`: pressure-aware BatchIt-lite (only after A/B signals from A/B).

## Candidate Specs

### S236-A (first to implement)
- Intent:
  - make `n==1` remote publish cheap and pre-slowpath consumable.
- Boundary:
  - producer: `hakozuna/hz3/src/hz3_tcache_remote_stash.inc`
  - consumer fast edge: `hakozuna/hz3/src/hz3_tcache_slowpath.inc` (or earliest pre-slowpath miss edge)
- Knobs (default OFF):
  - `HZ3_S236_MEDIUM_MAILBOX`
  - `HZ3_S236_SC_MIN=5`, `HZ3_S236_SC_MAX=7`
  - `HZ3_S236_ONLY_N_EQ_1=1`
  - `HZ3_S236_MAILBOX_SLOTS=1`
- Required counters:
  - `s236_mb_push_attempt`, `s236_mb_push_hit`, `s236_mb_push_full_fallback`
  - `s236_mb_pop_attempt`, `s236_mb_pop_hit`
  - `s236_fast_rescue_hits`
  - plus existing `S203` source mix and `alloc_slow_calls`.
- RUNS=10 fail-fast:
  - `main >= +5%`, `guard >= -8%`, `larson >= -6%`
  - and `alloc_slow_calls <= +5%` (hard kill if exceeded).

### S236-A Execution Result (2026-02-12)
- Implemented in:
  - `hakozuna/hz3/src/hz3_tcache_remote_stash.inc`
  - `hakozuna/hz3/src/hz3_tcache_remote.c`
  - `hakozuna/hz3/src/hz3_hot.c`
- `RUNS=10`:
  - `main`: `+4.771%`
  - `guard`: `+1.602%`
  - `medium (4KB..32KB)`: `+2.460%`
- `RUNS=21`:
  - `main`: `+4.035%`
  - `guard`: `+0.018%`
  - `medium (4KB..32KB)`: `+4.356%`
- Decision:
  - mechanism is positive and stable.
  - not enough for aggressive promotion gate (`main >= +10%`).
  - lock as opt-in baseline and move to `S236-B`.

### S236-B
- Intent:
  - reduce full-slowpath frequency by introducing a scoped mini-refill lane.
- Boundary:
  - `hakozuna/hz3/src/hz3_tcache_slowpath.inc` only (early mini-refill branch).
- Knobs (default OFF):
  - `HZ3_S236_MINIREFILL`
  - `HZ3_S236_SC_MIN=5`, `HZ3_S236_SC_MAX=7`
  - `HZ3_S236_MINIREFILL_K=8`
  - `HZ3_S236_MINIREFILL_TRY_INBOX=1`
  - `HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST=1`
- Required counters:
  - `s236_mini_calls`, `s236_mini_hit_inbox`, `s236_mini_hit_central`, `s236_mini_miss`
  - and `alloc_slow_calls` delta vs baseline.

### S236-B Execution Result (2026-02-12)
- Implemented in:
  - `hakozuna/hz3/src/hz3_tcache_slowpath.inc` (`hz3_s236_minirefill_try`)
  - `hakozuna/hz3/src/hz3_hot.c` (pre-`hz3_alloc_slow` call edge)
  - `hakozuna/hz3/src/hz3_tcache.c` (`S203` mini counters dump)
- Variant compare:
  - base: `-DHZ3_S236_MEDIUM_MAILBOX=1`
  - var:  `-DHZ3_S236_MEDIUM_MAILBOX=1 -DHZ3_S236_MINIREFILL=1`
- `RUNS=21`:
  - `main (16..32768 r90)`: `30.081M -> 40.783M` (`+35.57%`)
    - log: `/tmp/ssot_ab_hz3_20260212_203124_2ead4787c`
  - `guard (16..2048 r90)`: `110.773M -> 114.909M` (`+3.73%`)
    - log: `/tmp/ssot_ab_hz3_20260212_203242_2ead4787c`
  - `medium (4096..32768 r90)`: `27.709M -> 38.022M` (`+37.22%`)
    - log: `/tmp/ssot_ab_hz3_20260212_203325_2ead4787c`
- Mechanism lock (`main`, S203 medians):
  - `alloc_slow_calls`: `1,645,625 -> 946,001` (`-42.51%`)
  - `mini_calls=683,877`, `hit_inbox=383,088`, `hit_central=276,184`, `miss=21,965`
- Decision:
  - `S236-B` is GO and promoted together with `S236-A` into scale default.
  - `larson` post-promotion sanity run is passed:
    - compare:
      - base: `-DHZ3_S236_MEDIUM_MAILBOX=0 -DHZ3_S236_MINIREFILL=0`
      - var: default (`S236-A/B=ON`)
    - `RUNS=10` (`larson 4KB..32KB`, `threads=4`):
      - `155,533,238.5 -> 155,882,097.0` (`+0.224%`)
      - log: `/tmp/ssot_ab_hz3_20260212_204353_2ead4787c`

### S236-C
- Intent:
  - pressure-aware producer batching (BatchIt-lite) without supply starvation.
- Boundary:
  - producer packaging in `hz3_tcache_remote_stash.inc`
  - pressure bit set/clear near slowpath refill edge in `hz3_tcache_slowpath.inc`
- Knobs (default OFF):
  - `HZ3_S236_BATCHIT_LITE`
  - `HZ3_S236_SC_MIN=5`, `HZ3_S236_SC_MAX=7`
  - `HZ3_S236_BATCH_CACHE_ENTRIES=16`
  - `HZ3_S236_BATCH_CACHE_WAYS=2`
  - `HZ3_S236_BATCH_FLUSH_N=8`
  - `HZ3_S236_PRESSURE_ENABLE=1`
- Required counters:
  - `s236_batch_cache_hit`, `s236_batch_cache_miss`
  - `s236_batch_flush_msgs`, `s236_batch_flush_objs`
  - `s236_pressure_set`, `s236_pressure_bypass`.

### S236-C Execution Result (2026-02-12)
- Implemented in:
  - `hakozuna/hz3/src/hz3_tcache_remote_stash.inc`
  - `hakozuna/hz3/src/hz3_tcache_remote.c`
  - `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`
- Variant compare:
  - base: `-DHZ3_S203_COUNTERS=1`
  - var:  `-DHZ3_S203_COUNTERS=1 -DHZ3_S236_BATCHIT_LITE=1`
- `RUNS=10`:
  - `main (16..32768 r90)`: `40.1838M -> 39.8070M` (`-0.938%`)
    - log: `/tmp/s236c_ab_main_r10_20260212_223933`
  - `guard (16..2048 r90)`: `111.7796M -> 106.6718M` (`-4.569%`)
    - log: `/tmp/s236c_ab_guard_r10_20260212_224019`
  - `larson (4KB..32KB)`: `156.5234M -> 151.1744M` (`-3.417%`)
    - log: `/tmp/s236c_ab_larson_r10_20260212_224040`
- Mechanism lock (`S236C` counters in `var`):
  - main median: `batch_push_attempts=4,804,293`, `cache_hits=1,583,779`,
    `flush_msgs=684,875`, `flush_objs=2,265,514`, `pressure_bypass=2,533,594`
  - larson median: `batch_push_attempts=320,601`, `cache_hits=139,056`,
    `flush_msgs=163,434`, `flush_objs=302,490`, `pressure_bypass=18,111`
- Anti-regression guard:
  - main `alloc_slow_calls` median: `953,643 -> 955,616` (`+0.21%`, hard-kill not triggered)
- Decision:
  - mechanism engages but throughput gate is not met (`main >= +5%` fails).
  - classify `S236-C` as NO-GO and archive.

### S236-D (mailbox multi-slot sweep: slots=2)
- Intent:
  - keep `S236-A/B` default behavior and reduce mailbox overflow fallback by
    allowing a small multi-slot mailbox (`sc=5..7`, singleton remote publish).
- Boundary:
  - `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`
    (`HZ3_S236_MAILBOX_SLOTS` widened to `1..4`)
  - `hakozuna/hz3/src/hz3_tcache_remote.c`
    (slot array mailbox + slot-scan push/pop)
- Variant compare:
  - base: `-DHZ3_S203_COUNTERS=1`
  - var:  `-DHZ3_S203_COUNTERS=1 -DHZ3_S236_MAILBOX_SLOTS=2`
- `RUNS=10`:
  - `main (16..32768 r90)`: `39.7744M -> 39.8117M` (`+0.094%`)
    - log: `/tmp/s236d_slots2_ab_main_r10`
  - `guard (16..2048 r90)`: `109.2343M -> 112.0795M` (`+2.605%`)
    - log: `/tmp/s236d_slots2_ab_guard_r10`
  - `larson (4KB..32KB)`: `157.1846M -> 157.7083M` (`+0.333%`)
    - log: `/tmp/s236d_slots2_ab_larson_r10`
- Mechanism lock:
  - `mb_push_full_fallbacks`: `4,821,600 -> 4,343,330` (`-9.92%`)
  - `mb_push_hits`: `529,846 -> 1,009,660` (`+90.56%`)
  - `mb_pop_hits`: `529,816 -> 1,009,603` (`+90.56%`)
- Decision:
  - mechanism engages but primary screen gate (`main >= +2.0%`) fails.
  - classify `S236-D` as NO-GO and keep default `HZ3_S236_MAILBOX_SLOTS=1`.

### S236-E Execution Result (2026-02-13)
- Intent:
  - add one bounded retry after mini-refill central miss.
- Variant compare:
  - base: `-DHZ3_S203_COUNTERS=1`
  - var:  `-DHZ3_S203_COUNTERS=1 -DHZ3_S236E_MINI_CENTRAL_RETRY=1`
- `RUNS=10`:
  - `main (16..32768 r90)`: `40.623M -> 38.448M` (`-5.35%`)
    - log: `/tmp/s236e_ab_main_r10`
  - `guard (16..2048 r90)`: `100.411M -> 113.125M` (`+12.66%`)
    - log: `/tmp/s236e_ab_guard_r10`
  - `larson (4KB..32KB)`: `120.868M -> 121.695M` (`+0.68%`)
    - log: `/tmp/s236e_ab_larson_r10`
- Mechanism lock:
  - retry engages, but `retry_hits=0` and
    `retry_skipped_no_supply == retry_calls`.
- Decision:
  - no supply capture and main replay-negative; hard-archive.

### S236-F Execution Result (2026-02-13)
- Intent:
  - gate S236-E retry by miss-streak to reduce fixed overhead.
- Variant compare:
  - base: `-DHZ3_S203_COUNTERS=1`
  - var:  `-DHZ3_S203_COUNTERS=1 -DHZ3_S236F_MINI_CENTRAL_RETRY=1`
- `RUNS=10`:
  - `main (16..32768 r90)`: `39.150M -> 38.336M` (`-2.08%`)
    - log: `/tmp/s236f_ab_main_r10`
  - `guard (16..2048 r90)`: `111.709M -> 113.913M` (`+1.97%`)
    - log: `/tmp/s236f_ab_guard_r10`
  - `larson (4KB..32KB)`: `121.272M -> 121.636M` (`+0.30%`)
    - log: `/tmp/s236f_ab_larson_r10`
- Mechanism lock:
  - retry path engages, but `retry_hits=0` persists.
- Decision:
  - no throughput upside on main; hard-archive.

### S236-G Execution Result (2026-02-13)
- Intent:
  - gate mini central-pop by `hz3_central_has_supply()` hint to skip empty pops.
- Variant compare:
  - base: `-DHZ3_S203_COUNTERS=1`
  - var:  `-DHZ3_S203_COUNTERS=1 -DHZ3_S236G_MINI_CENTRAL_HINT_GATE=1`
- `RUNS=10`:
  - `main (16..32768 r90)`: `39.552M -> 39.275M` (`-0.70%`)
    - log: `/tmp/s236g_ab_main_r10`
  - `guard (16..2048 r90)`: `109.553M -> 109.615M` (`+0.06%`)
    - log: `/tmp/s236g_ab_guard_r10`
  - `larson (4KB..32KB)`: `121.400M -> 120.138M` (`-1.04%`)
    - log: `/tmp/s236g_ab_larson_r10`
- Mechanism lock:
  - `[HZ3_S203_S236G]` counters show non-zero `hint_checks`,
    `hint_positive`, and `hint_empty_skips`.
- Decision:
  - mechanism engages but no gain; larson gate miss (`< -1.0%`), hard-archive.

### S236-H Execution Result (2026-02-13)
- Intent:
  - tune mini-refill budget only (`HZ3_S236_MINIREFILL_K`) with no producer-path
    changes, then keep the best `K` if guard/larson gates hold.
- Main sweep (`RUNS=10`, interleaved, pinned `0-15`):
  - `K=4`: `40.300M -> 39.099M` (`-2.98%`)
    - log: `/tmp/s236h_k_sweep_main_r10_20260212_234214/k4`
  - `K=6`: `40.983M -> 40.737M` (`-0.60%`)
    - log: `/tmp/s236h_k_sweep_main_r10_20260212_234214/k6`
  - `K=10`: `39.461M -> 39.670M` (`+0.53%`)
    - log: `/tmp/s236h_k_sweep_main_r10_20260212_234214/k10`
  - `K=12`: `40.223M -> 40.918M` (`+1.73%`)
    - log: `/tmp/s236h_k_sweep_main_r10_20260212_234214/k12`
- Top-candidate recheck (`K=10/12`):
  - guard (`16..2048 r90`):
    - `K=10`: `108.303M -> 106.837M` (`-1.35%`)
      - log: `/tmp/s236h_k_guard_larson_r10_20260212_234502/k10_guard`
    - `K=12`: `113.682M -> 107.179M` (`-5.72%`)
      - log: `/tmp/s236h_k_guard_larson_r10_20260212_234502/k12_guard`
  - larson (`4KB..32KB`):
    - `K=10`: `121.329M -> 120.784M` (`-0.45%`)
      - log: `/tmp/s236h_k_guard_larson_r10_20260212_234502/k10_larson`
    - `K=12`: `121.719M -> 121.110M` (`-0.50%`)
      - log: `/tmp/s236h_k_guard_larson_r10_20260212_234502/k12_larson`
- Decision:
  - NO-GO. main uplift is below screen target and both top candidates fail
    guard gate (`>= -1.0%`).
  - keep default `HZ3_S236_MINIREFILL_K=8`.
  - archive:
    `hakozuna/hz3/archive/research/s236h_mini_refill_k_tune/README.md`.

### Next box (locked): S236-I
- Goal:
  - define a mechanism-distinct consumer-path box after `S236-H` counter/perf
    review (avoid producer batching replay).
- Scope:
  - pending design freeze in `CURRENT_TASK.md`.

## Anti-Repeat Rules (from archived history)
- Do not run a pure `direct-to-central` reroute box (S229/S230 class) without a pre-slowpath consume path.
- Do not promote a box if mechanism engages but `alloc_slow_calls` rises materially.
- No multi-box stacking before single-box mechanism is proven.

## Promotion Gate (RUNS=21, interleaved)
- `main >= +10%`
- `guard >= -6.0%`
- `larson >= -4.0%`
- no crash/abort/alloc-failed
- one-shot `perf stat` check on promoted candidate (`cycles/op` must not regress).

## References
- prep doc: `hakozuna/hz3/docs/PHASE_S236_AGGRESSIVE_CONSULT_PREP.md`
- ledger: `CURRENT_TASK.md`
- archived history: `hakozuna/docs/NO_GO_SUMMARY.md`
