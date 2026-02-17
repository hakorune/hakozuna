# HZ4 Phase23: B80/B81 Knob Sweep Results (2026-02-17)

## Scope
- B80: `HZ4_MID_LOCK_SHARDS` sweep
- B81: `HZ4_MID_ALLOC_RUN_CACHE_LIMIT` x `HZ4_MID_FREE_BATCH_LIMIT` sweep

Baseline:
- `-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1`
- `-DHZ4_MID_OWNER_LOCAL_STACK_BOX=1`
- `-DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8`
- `-DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1`

Lanes (`RUNS=7`, median):
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

Gate:
- `guard_r0 >= -1.0%`
- `main_r0 >= +3.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= 0.0%`

---

## Baseline medians
- `guard_r0`: `247,043,805.63`
- `main_r0`: `136,633,678.08`
- `main_r50`: `80,391,335.92`
- `cross128_r90`: `41,199,351.28`

---

## B80 Result (`HZ4_MID_LOCK_SHARDS`)

| Config | guard_r0 | main_r0 | main_r50 | cross128_r90 | Verdict |
|---|---:|---:|---:|---:|---|
| `shards=16` | `-0.42%` | `-0.39%` | `-4.15%` | `+4.39%` | NO-GO |
| `shards=32` | `-1.89%` | `+4.28%` | `-7.93%` | `-17.16%` | NO-GO |
| `shards=64` | `-2.36%` | `+8.40%` | `-18.72%` | `-27.16%` | NO-GO |

Observation:
- `main_r0` and `main_r50/cross` moved in opposite directions.
- Higher shards worsened mixed/remote lanes materially.

---

## B81 Result (`alloc-run` x `free-batch`)

| Config | guard_r0 | main_r0 | main_r50 | cross128_r90 | Verdict |
|---|---:|---:|---:|---:|---|
| `ar8_fb32` | `+0.12%` | `+0.32%` | `-4.39%` | `+9.57%` | NO-GO |
| `ar8_fb64` | `-4.09%` | `-0.33%` | `-2.05%` | `+7.85%` | NO-GO |
| `ar16_fb32` | `-0.89%` | `-0.85%` | `-2.24%` | `+4.01%` | NO-GO |
| `ar16_fb64` | `-1.16%` | `-0.29%` | `+0.28%` | `+7.47%` | NO-GO |
| `ar24_fb32` | `-1.32%` | `-0.71%` | `-4.80%` | `+0.26%` | NO-GO |
| `ar24_fb64` | `-0.95%` | `-1.20%` | `-3.79%` | `-0.03%` | NO-GO |

Observation:
- Cross lane can be lifted, but `main_r0/main_r50` gate was not satisfied.
- Knob-only tuning in this region is near exhaustion.

---

## Decision
- B80/B81 are both **NO-GO**.
- Keep baseline unchanged.
- Move to one-shot final attempt (`B82`) with strict kill-switch:
  - if gate fails, freeze hz4 mid path and shift active optimization elsewhere.

## Evidence
- `/tmp/hz4_b80_b81_sweep_20260217_102132/base_summary.tsv`
- `/tmp/hz4_b80_b81_sweep_20260217_102132/b80_summary.tsv`
- `/tmp/hz4_b80_b81_sweep_20260217_102132/b81_summary.tsv`
- `/tmp/hz4_b80_b81_sweep_20260217_102132/b80_gated.tsv`
- `/tmp/hz4_b80_b81_sweep_20260217_102132/b81_gated.tsv`
