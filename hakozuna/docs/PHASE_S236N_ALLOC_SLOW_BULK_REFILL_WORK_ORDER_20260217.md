# PHASE S236-N: MediumAllocSlowBulkRefillBox Work Order

Date: 2026-02-17  
Status: promoted (default ON)

---

## 1) Goal

- Improve aggressive `main` lane (`16..32768 r90`) by reducing repeated `hz3_alloc_slow()` entries.
- Keep `guard` and `larson_proxy` within non-regression gate.
- Add work only at alloc-slow consumer boundary (not always-on on fast path).

Target gates:
- Screen (`RUNS=10`): `main >= +3.0%`, `guard >= -3.0%`, `larson_proxy >= -3.0%`
- Hard-kill: `alloc_slow_calls` must not exceed `+5%` vs base.

---

## 2) Why This Box

Recent lock:
- `S199` reduces `alloc_slow_calls` but does not reach promotion (`main` near flat across replay).
- `S236` micro lines (`K/L/M`, source arbiter, mailbox preload) are locked NO-GO.

Hypothesis:
- Current medium dispatch is heavily `n==1`.
- If one slow hit can locally prefill a few extra objects from the same source, subsequent allocations avoid slow re-entry.
- This is a consumer-side amortization box and is distinct from prior source-order or retry micro boxes.

---

## 3) Stage A (Observation First, No Code Change)

Build:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S203_COUNTERS=1'
```

Run (`RUNS=10`, interleaved):
- main: `16 2000000 400 16 32768 90 65536`
- guard: `16 2000000 400 16 2048 90 65536`
- larson_proxy: `4 2500000 400 4096 32768 0 65536`

Proceed to Stage B only if all are true on main:
1. `alloc_slow_calls` median remains high (`>= 900k`)
2. `medium_dispatch_n_hist` is still singleton-dominant (`n==1 >= 98%`)
3. `from_inbox + from_central` share of `alloc_slow` is high (`>= 90%`)

If any fails: stop and mark `S236-N` as NO-GO at Stage A.

---

## 4) Stage B (Implementation)

### 4.1 Boundary

- Primary boundary:
  - `hakozuna/hz3/src/hz3_tcache_slowpath.inc`
  - function: `hz3_alloc_slow(int sc)`
- Counter output:
  - `hakozuna/hz3/src/hz3_tcache.c` (`[HZ3_S203_*]`)
- Knobs:
  - `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`

No change allowed in:
- `hz3_free` path
- remote publish/dispatch path
- archived knobs in `hz3_config_archive.h`

### 4.2 New knobs (default ON)

```c
#ifndef HZ3_S236N_ALLOC_SLOW_BULK_REFILL
#define HZ3_S236N_ALLOC_SLOW_BULK_REFILL 1
#endif

#ifndef HZ3_S236N_SC_MIN
#define HZ3_S236N_SC_MIN HZ3_S236_SC_MIN
#endif

#ifndef HZ3_S236N_SC_MAX
#define HZ3_S236N_SC_MAX HZ3_S236_SC_MAX
#endif

#ifndef HZ3_S236N_BULK_WANT
#define HZ3_S236N_BULK_WANT 4
#endif
```

Compile guards:
- `HZ3_S236N_BULK_WANT` range: `2..8`
- only active for `sc in [S236N_SC_MIN, S236N_SC_MAX]`

### 4.3 Algorithm sketch

When `hz3_alloc_slow(sc)` gets one object from inbox/central:
1. return candidate object as today.
2. if `S236N` active and `sc` in range, try to pull up to `BULK_WANT-1` extra objects from the same source.
3. prepend extras to local bin (or existing local refill holder) in one local operation.
4. no retries beyond the defined bounded loop.

Important:
- no additional branch/work in `hz3_hot` fast alloc path.
- all extra work stays inside already-entered slow boundary.

---

## 5) Stage C (A/B)

Base:
```sh
-DHZ3_S203_COUNTERS=1
```

Var:
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236N_ALLOC_SLOW_BULK_REFILL=1
```

Lanes (`RUNS=10`, interleaved, pinned `0-15`):
- main: `16 2000000 400 16 32768 90 65536`
- guard: `16 2000000 400 16 2048 90 65536`
- larson_proxy: `4 2500000 400 4096 32768 0 65536`

Screen gates:
- `main >= +3.0%`
- `guard >= -3.0%`
- `larson_proxy >= -3.0%`
- hard-kill: `alloc_slow_calls` delta `<= +5%`

If pass:
- replay `RUNS=21` interleaved with the same gates.

If fail:
- revert implementation
- archive under:
  - `hakozuna/hz3/archive/research/s236n_alloc_slow_bulk_refill/README.md`
- update:
  - `hakozuna/hz3/docs/NO_GO_SUMMARY.md`

---

## 6) Required counters (S203 extension)

Add only if `S236N` is implemented:
- `s236n_bulk_calls`
- `s236n_bulk_inbox_extra_objs`
- `s236n_bulk_central_extra_objs`
- `s236n_bulk_zero_extra`

These are mechanism-proof counters, not promotion criteria by themselves.

---

## 7) Non-repeat lock

This box is NOT:
- source-priority arbiter (`S236-M`)
- central retry (`S236-E/F`)
- inbox second-chance (`S236-I`)
- mailbox-hit preload (`S236-J`)
- K-only sweep (`S236-L`)

It is a consumer-boundary amortization box (`slow-entry -> bounded local refill`).

---

## 8) Results (2026-02-17)

### Screen (`RUNS=10`, interleaved)
- `main`: `+4.26%`
- `guard`: `+4.38%`
- `larson_proxy`: `-0.86%`
- hard-kill (`alloc_slow_calls`): `-0.45%`
- Verdict: `GO`

### Replay (`RUNS=21`, interleaved)
- `main`: `+3.80%`
- `guard`: `-1.14%`
- `larson_proxy`: `+0.06%`
- hard-kill (`alloc_slow_calls`): `-1.41%`
- Verdict: `GO`

Result artifacts:
- Screen: `/tmp/s236n_screen_20260217_142345`
- Replay: `/tmp/s236n_replay_20260217_144657`

Promotion decision:
- `HZ3_S236N_ALLOC_SLOW_BULK_REFILL=1` as default in scale lane.
