# PHASE S236-J Mailbox-Hit Preload Work Order (Aggressive Lane)

Last updated: 2026-02-17 (archived NO-GO)

## Final Verdict
- `S236-J` is archived as NO-GO.
- Reason:
  - mechanism engaged (`preload_calls/hits` non-zero), but `main` uplift was not stable enough for promotion.
  - initial screen showed `main` regression; later interleaved reruns were high-variance and not reproducible in stricter checks.
  - independent perf recheck stayed near flat on elapsed/cycles with no robust replay-grade gain.
- Archive reference:
  - `hakozuna/archive/research/s236j_mailbox_hit_preload/README.md`

## Goal
- Improve `S236` aggressive `main` (`16..32768 r90`) from current `S236-A/B` default baseline.
- Keep `guard`/`larson` stable.
- Do not replay archived `S229/S230` style direct-to-central path.

## Why This Box
- `S236-A/B` is the current best promoted pair.
- Recent S236 branches failed mainly due to always-on checks or extra miss-path work.
- This box adds work only on mailbox-hit (already positive signal), so fixed overhead is low.

## Box Definition
- Name: `S236-J Mailbox-Hit Preload Box`
- Type: consumer-side micro box (opt-in, default OFF)
- Scope:
  - `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`
  - `hakozuna/hz3/src/hz3_hot.c`
  - `hakozuna/hz3/src/hz3_tcache.c` (S203 counters dump)

## Design
- Keep existing fast order:
  1) local bin pop
  2) mailbox pop
  3) mini-refill
  4) full slow path
- New behavior is inserted only in step (2) mailbox hit:
  - after `mailbox_obj` is popped and before return:
    - attempt one inbox drain for same `sc` and shard
    - if one object is found, push it to local bin (`bin`/`binref`)
- No central pop, no extra retry, no producer path mutation.

## Knobs
Add to `hz3_config_scale_part8_modern.inc` near S236 knobs:

```c
// S236-J: mailbox-hit preload one inbox object to local bin (aggressive opt-in)
#ifndef HZ3_S236J_MAILBOX_HIT_PRELOAD
#define HZ3_S236J_MAILBOX_HIT_PRELOAD 0
#endif

#if HZ3_S236J_MAILBOX_HIT_PRELOAD < 0 || HZ3_S236J_MAILBOX_HIT_PRELOAD > 1
#error "HZ3_S236J_MAILBOX_HIT_PRELOAD must be 0 or 1"
#endif
```

## Runtime Changes
### 1) `hz3_hot.c`
In mailbox-hit branch (`hz3_alloc_medium_fast`), add:
- `S203` counter `s236j_preload_calls++`
- `extra = hz3_inbox_drain_medium_from_shard_slow(t_hz3_cache.my_shard, sc)`
- if `extra`:
  - push to local medium bin
  - `s236j_preload_hits++`
  - `s236j_preload_pushed++`
- return original `mailbox_obj`

### 2) `hz3_tcache.c` counters
Add S203 counters:
- `s236j_preload_calls`
- `s236j_preload_hits`
- `s236j_preload_pushed`

Print one-shot line when calls > 0:
- `[HZ3_S203_S236J] preload_calls=... preload_hits=... preload_pushed=...`

## Fail-Fast / Anti-Repeat Guard
- Keep hard guard: no new direct-to-central branch before `hz3_alloc_slow`.
- If `alloc_slow_calls` increases by more than `+5%` in screen, auto NO-GO.
- If `s236j_preload_hits == 0` on `main`, classify as cold box and stop.

## Build Matrix
Base (current aggressive default):
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S203_COUNTERS=1'
```

Var (S236-J):
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S203_COUNTERS=1 -DHZ3_S236J_MAILBOX_HIT_PRELOAD=1'
```

## A/B Protocol
Use interleaved A/B with pinned cores (`0-15`).

### Screen (`RUNS=10`)
- `main` (`16 2000000 400 16 32768 90 65536`) gate: `>= +2.0%`
- `guard` (`16 2000000 400 16 2048 90 65536`) gate: `>= -2.0%`
- `larson` (`4KB..32KB`) gate: `>= -2.0%`
- hard kill: `alloc_slow_calls <= +5%`

### Replay (`RUNS=21`) if screen pass
- `main >= +4.0%`
- `guard >= -3.0%`
- `larson >= -3.0%`
- crash/abort/alloc-failed = 0

## Deliverables
- `/tmp/s236j_ab_main_r10_*`
- `/tmp/s236j_ab_guard_r10_*`
- `/tmp/s236j_ab_larson_r10_*`
- `summary.tsv`, `raw.tsv`, `REPORT.md`
- If NO-GO: archive under
  - `hakozuna/archive/research/s236j_mailbox_hit_preload/README.md`
