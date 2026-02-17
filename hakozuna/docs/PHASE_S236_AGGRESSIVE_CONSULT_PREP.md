# PHASE S236 Aggressive Consult Prep

Last updated: 2026-02-12

Superseded by:
- `hakozuna/hz3/docs/PHASE_S236_AGGRESSIVE_CONSULT_RESULT.md`

## Purpose
- Prepare a design-first consultation packet for `hz3` aggressive lane.
- Target `main` (`MT remote r90 16..32768`) with meaningful uplift while keeping implementation fail-fast and reversible.

## Locked Objective
- Primary target:
  - `main` improvement `>= +10%` vs current default (`S222+S233` baseline).
- Aggressive lane acceptance:
  - `guard` (`16..2048`) may relax to `>= -6.0%`.
  - `larson` (`4KB..32KB`) may relax to `>= -4.0%`.
  - crash/abort/alloc-failed must remain zero.

## Why consultation now
- Many medium-path big boxes are already hard-archived:
  - `S220/S221/S223/S224/S225/S226/S228/S229/S230/S231`.
- Recent large/central experiments also archived:
  - `S232` large aggressive retain preset: large-only `-22.73%`.
  - `S234` central partial-pop CAS: main `-16.7%`, guard `-11.8%`.
- Current optimization attempts show mechanism engagement but throughput regression in key lanes.

## Current baseline observations (must-use)
- `S222` baseline source mix on main (`/tmp/s222_main_obs_20260212_025459`):
  - `from_inbox=62.84%`
  - `from_central=34.17%`
  - `from_segment=2.99%`
  - medium dispatch is still `n==1` dominant.
- `S230` (`n==1 -> central-fast direct`) (`/tmp/s230_ab_main_r10_20260212_exec`):
  - `to_central_s230: 0 -> 5.11M` (engaged)
  - `alloc_slow_calls: 1,671,751 -> 2,831,485` (slowpath amplification)
  - `main -59.94%` (hard NO-GO)
- `S231` (`inbox fast MPSC`) (`/tmp/s231_ab_main_20260212_092703`):
  - `main -0.267%`, `guard -2.440%`, `larson +2.730%`
  - mechanism did not break `n==1` dominance enough for main uplift.
- `S233` (default ON, large hotband always-admit):
  - large+ lane positive (`+11.89%`) and churn counters improved in lock run.

## Hard constraints for candidate designs
- Candidate must not be a replay of archived mechanism classes.
- Candidate must define:
  - minimal touch surface (specific files/functions),
  - opt-in knobs (default OFF),
  - explicit mechanism counters,
  - RUNS=10 fail-fast and RUNS=21 promotion gates.
- Candidate should prefer box boundaries:
  - producer boundary (`hz3_tcache_remote_stash.inc`),
  - consumer/slowpath boundary (`hz3_tcache_slowpath.inc`),
  - central boundary (`hz3_central.c`),
  - inbox boundary (`hz3_inbox.c`).

## Questions for consultation
1. What architecture should replace `n==1`-dominant medium remote publish to get `+10%` on main?
2. Should the next attempt favor:
   - sender-side scoped bucketization,
   - consumer-side batching,
   - central-first with better gating,
   - or a new ownership/queue model?
3. How to prevent `alloc_slow` amplification seen in `S230` while keeping mechanism engagement high?
4. What is the minimal counter set to prove root-cause movement before full A/B?
5. Which one candidate should be implemented first for highest information gain?

## Related docs
- `CURRENT_TASK.md`
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- `hakozuna/hz3/docs/HZ3_ARCHIVED_BOXES.md`
- `hakozuna/hz3/docs/HZ3_GO_BOXES.md`
