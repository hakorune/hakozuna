# HZ11 Macro Failure Attribution L1

Status: measured.
Verdict: MIXED attribution result; cap-only fixes abort rows, but does not
promote the transfer lane.

## Purpose

Attribute the `HZ11MacroSpeedLaneGate-L1` macro failures before tuning transfer
batch/cap policy:

```text
1. measure central stack overflow pressure by class
2. test whether raising central cap makes python_alloc/mstress pass
3. if RSS still fails, move to CentralFreeList/span-return design
4. then attribute larson/sh6bench RSS
```

This box keeps the default `libhz11_span_transfer.so` policy unchanged. It adds
only diagnostic siblings and a measurement script.

## Instrumentation

Diagnostic builds are opt-in:

```text
libhz11_span_transfer_diag.so:
  transfer lane plus HZ11_CENTRAL_CLASS_DIAG=1

libhz11_span_transfer_cap.so:
  transfer lane plus HZ11_CENTRAL_CLASS_DIAG=1
  HZ11_CENTRAL_CAP overridden by HZ11_CENTRAL_CAP_AB
```

The normal transfer lane remains:

```text
libhz11_span_transfer.so:
  HZ11_CENTRAL_CAP=4096
  no class diagnostic counters
```

The diagnostic lane prints:

```text
hz11_central_overflow class=<id> count=<n> cap=<cap> incoming_left=<n> needed=<n>
hz11_central_class class=<id> count=<n> high_water=<n> insert=<n> remove=<n>
```

Successful runs can dump class high-water at exit with
`HZ11_DUMP_CENTRAL_CLASSES=1`.

## Script

```bash
RUNS=3 CAPS=4096,8192,16384,32768,65536 \
  ./hakozuna-hz11/scripts/run_hz11_macro_failure_attribution.sh
```

Output:

```text
bench_results/hz11_macro_failure_attr_<timestamp>/
  README.log
  samples.csv
  summary.md
  per-run stdout/stderr/log files
  libhz11_span_transfer_cap_<cap>.so
```

## Abort Rows Result

Run:

```text
bench_results/hz11_macro_failure_attr_20260707T223657Z/
```

Summary:

| Workload | cap | status | overflow class | overflow needed | max high-water | max RSS vs tcmalloc |
|---|---:|---|---:|---:|---:|---:|
| python_alloc | 4096 | FAIL 3/3 | 2 | 4128 | 4096 | NA |
| python_alloc | 8192 | FAIL 3/3 | 2 | 8224 | 8192 | NA |
| python_alloc | 16384 | FAIL 3/3 | 2 | 16416 | 16384 | NA |
| python_alloc | 32768 | FAIL 3/3 | 2 | 32800 | 32768 | NA |
| python_alloc | 65536 | OK 3/3 | NA | NA | 45200 | 1.137 |
| mstress | 4096 | FAIL 3/3 | 0 | 4128 | 4096 | NA |
| mstress | 8192 | FAIL 3/3 | 0 | 8224 | 8192 | NA |
| mstress | 16384 | FAIL 3/3 | 0 | 16416 | 16384 | NA |
| mstress | 32768 | FAIL 3/3 | 0 | 32800 | 32768 | NA |
| mstress | 65536 | OK 3/3 | NA | NA | 57392 | 1.189 |

Interpretation:

```text
The macro aborts are central stack capacity failures, not random corruption.
python_alloc fills class 2. mstress fills class 0.

Raising HZ11_CENTRAL_CAP to 65536 makes both rows pass, and these two rows stay
inside the tcmalloc * 1.25 max-RSS guard. This proves cap pressure explains the
abort rows, but a fixed 65536-per-class stack is not a product policy.
```

## Larson/Sh6bench RSS Result

Run:

```text
bench_results/hz11_macro_failure_attr_20260707T223856Z/
```

Summary:

| Workload | cap | status | transfer wall | tcmalloc wall | transfer/tcmalloc max RSS | max high-water |
|---|---:|---|---:|---:|---:|---:|
| larson | 65536 | OK 3/3 | 4.176s | 4.145s | 2.345 | NA |
| sh6bench | 65536 | OK 3/3 | 4.572s | 0.359s | 1.322 | 2976 |

Interpretation:

```text
Cap-only does not solve macro RSS.

larson does not materially use the central stack in this diagnostic run, yet RSS
remains 2.345x tcmalloc. sh6bench uses and drains central stack objects, but
still exceeds the RSS guard and remains far slower than tcmalloc.
```

## Decision

```text
MIXED.

Keep libhz11_span_transfer.so as the remote/mixed microbench lane only.
Do not promote it to macro speed lane.

The next implementation box should be CentralFreeList/span-return policy, not
another simple cap increase. A larger cap is useful as an attribution probe but
not a clean HZ11 policy.
```

## Next Box

```text
HZ11CentralFreeListSpanReturn-L1:
  replace the unbounded-retention central object stack shape with a policy that
  can return completely free spans or otherwise cap retained central memory.
```
