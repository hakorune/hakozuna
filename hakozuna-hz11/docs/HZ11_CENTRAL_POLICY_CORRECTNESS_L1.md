# HZ11CentralPolicyCorrectness-L1

Status: GO for focused correctness rows; not a macro promotion.

This box fixes the macro gate entrance failure rows for an opt-in candidate
lane. It does not make a default-path change and does not claim HZ11 is macro
safe.

## Boundary

```text
New sibling:
  libhz11_span_transfer_thread_exit_cap.so

Flags:
  HZ11_CURRENT_SPAN_THREAD_EXIT=1
  HZ11_CENTRAL_CAP=65536
  HZ11_CENTRAL_CLASS_DIAG=1

Runner:
  scripts/run_hz11_central_policy_correctness.sh

Output:
  bench_results/hz11_central_policy_correctness_20260708T003310Z/summary.md
```

The existing lanes remain unchanged:

```text
libhz11_span_transfer.so
libhz11_span_transfer_thread_exit.so
```

This is a central-policy correctness sibling. A fixed 65536 central cap is still
not a final memory policy; it is the smallest isolated lane that clears the
known fail-fast rows so later macro work can measure past the entrance gate.

## Focused Gate

Command:

```bash
scripts/run_hz11_central_policy_correctness.sh
```

Rows:

```text
python_alloc
mstress
```

RUNS=3.

| Workload | Allocator | Status | Median wall sec | Median max RSS KiB | Max RSS vs tcmalloc |
|---|---|---:|---:|---:|---:|
| python_alloc | tcmalloc | OK:3 | 0.032 | 8192 | 1.000 |
| python_alloc | hz11-thread-exit | FAIL:3 | NA | NA | NA |
| python_alloc | hz11-thread-exit-cap | OK:3 | 0.032 | 6656 | 0.812 |
| mstress | tcmalloc | OK:3 | 0.191 | 213504 | 1.000 |
| mstress | hz11-thread-exit | FAIL:3 | NA | NA | NA |
| mstress | hz11-thread-exit-cap | OK:3 | 0.220 | 236140 | 1.106 |

The summary verdict is `MIXED` because the generic macro gate also checks
`current_rss_kb`; `python_alloc` is too short-lived for that ratio to be stable
and reports `1.571x` current RSS against a tiny tcmalloc final sample. The
required evidence for this box is correctness plus max RSS guard on the two
fail rows, and both pass.

## Central Pressure

`python_alloc`:

```text
class 2 high_water=4848
class 3 high_water=1808
class 4 high_water=704
overflow=0
```

`mstress`:

```text
class 0 high_water=58541..59045
class 1 high_water=28866..29058
class 2 high_water=28929..29151
class 3 high_water=28936..29104
overflow=0
```

`mstress` class 0 again explains why the default 4096 cap fails and why 65536 is
enough for this focused row.

## Decision

GO for focused correctness:

```text
python_alloc: hz11-thread-exit-cap passes 3/3
mstress:      hz11-thread-exit-cap passes 3/3
```

Do not promote this lane yet. Remaining macro blockers:

```text
sh6bench:
  wall and RSS still need a separate fix.

central policy:
  a fixed 65536 per-class object stack is acceptable as a measurement sibling,
  but not yet a product/default policy.
```

Next step should re-run the broader macro gate with
`hz11-thread-exit-cap` before deciding whether to work on sh6bench metadata lock
batching or replace the fixed cap with a real central policy.
