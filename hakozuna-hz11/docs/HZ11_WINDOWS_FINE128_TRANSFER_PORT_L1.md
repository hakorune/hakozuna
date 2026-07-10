# HZ11 Windows Fine128 Transfer Port L1

## Decision

Port the established Linux HZ11 fine128 transfer backend to Windows as one
opt-in lane. This is a platform-backend port inside HZ11, not a new allocator
generation and not a change to the selected Windows row.

```text
selected Windows row:
  hz11-span-cache256

existing partial transfer control:
  hz11-span-transfer
  HZ11_TRANSFER_CENTRAL_SPAN=1 only

new candidate:
  hz11-span-transfer-fine128-win
  full Linux fine128 policy stack with Windows FLS thread-exit salvage
```

The existing Windows transfer control is not Linux fine128 parity. Its initial
balanced scout was about 11.69M ops/s. The Windows returned-refill classbatch32
control reached about 37.58M ops/s, but it also does not contain the Linux
transfer/central/current-span stack. Neither result is a verdict on this port.

## Exact Policy Stack

The first candidate mirrors the Linux
`libhz11_span_transfer_thread_exit_cap_batch32_fine128.so` build:

```text
HZ11_CLASSIFY_SPAN=1
HZ11_TLS_FASTPATH=1
HZ11_CACHE_BYTE_ACCOUNTING=0
HZ11_CACHE_SOA=1
HZ11_TRANSFER_CENTRAL_SPAN=1
HZ11_CURRENT_SPAN_THREAD_EXIT=1
HZ11_CENTRAL_CLASS_DIAG=1
HZ11_CENTRAL_CAP=65536
HZ11_TRANSFER_BATCH=32
HZ11_FINE_SIZE_CLASSES=1
HZ11_FINE_LINEAR_MAX=128
```

`HZ11_CENTRAL_CLASS_DIAG` and the historical transfer counters are part of the
established Linux artifact. The exact-port scout showed a strong signal, so the
Windows speed row now compiles with `HZ11_CENTRAL_CLASS_DIAG=0` and
`HZ11_TRANSFER_STATS=0`. The counters remain available in diagnostic builds;
they are not mixed into the speed result.

## Windows Backing

Windows keeps the shared HZ11 transfer contract but uses OS-specific backing:

```text
mutex:
  CRITICAL_SECTION

once:
  INIT_ONCE

thread-exit callback:
  FlsAlloc / FlsSetValue

allocation source:
  existing VirtualAlloc-backed HZ11 span arena
```

The FLS destructor flushes the exiting thread cache and offers unfinished
current spans to the existing bounded current-span pool. It does not add owner
inboxes, per-free ownership, lock-free queues, or a new route contract.

## Invariants

```text
- selected/default rows remain unchanged
- speed lane gets no new production counters or atomics
- thread-exit salvage is bounded by HZ11_CURRENT_SPAN_POOL_CAP
- FLS callback must not publish a dead thread-cache pointer
- route/classification and source lifetime remain HZ11 contracts
- build and benchmark output must identify the complete flag stack
```

## Gate

First run build, smoke, and the stable Windows MT matrix. Compare the new row
with `hz11-span-cache256`, HZ12 Batch32 evidence, and tcmalloc on the same
runner/session.

```powershell
.\hakozuna-hz12\scripts\run_hz12_windows_stable_mt_gate.ps1 `
  -IncludeRefillBatchProbe `
  -IncludeHz11Fine128TransferProbe
```

```text
continue:
  stable MT throughput reaches at least 70% of tcmalloc on a broad row
  safety and thread-exit smoke pass
  peak RSS is no more than 10% above tcmalloc

strong signal:
  stable MT throughput reaches at least 90% of tcmalloc
  local random_mixed stays within -3%

no-go:
  any crash, route/safety failure, or FLS teardown failure
  current-span pool corruption or unbounded retention
  broad MT throughput remains below 70% of tcmalloc
  peak RSS exceeds tcmalloc by more than 10%
  local random_mixed regresses by more than 5%
```

If the first broad gate is below 70%, freeze the row as Windows port evidence.
Do not open HZ13 and do not tune transfer knobs around a failed mechanism gate.

## Result

Windows R3 stable-duration gate, 2-second target, fixed affinity, rotating row
order:

| profile | selected HZ11 | fine128 transfer | tcmalloc | fine128/tcmalloc | fine128 RSS | tcmalloc RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 28.385M | 422.016M | 516.280M | 81.7% | 45.92 MiB | 46.68 MiB |
| wide_ws | 16.910M | 313.499M | 427.142M | 73.4% | 85.10 MiB | 74.01 MiB |
| larger_sizes | 48.244M | 365.921M | 245.605M | 149.0% | 60.74 MiB | 80.57 MiB |

Windows random_mixed R5 versus selected HZ11:

| profile | selected HZ11 | fine128 transfer | delta |
| --- | ---: | ---: | ---: |
| small | 155.388M | 151.014M | -2.8% |
| medium | 153.836M | 149.960M | -2.5% |
| mixed | 153.480M | 151.490M | -1.3% |

Decision:

```text
GO:
  Windows broad-MT opt-in lane
  exact Linux fine128 backend port

HOLD:
  selected/default promotion

why:
  balanced and wide_ws clear the 70% continuation gate
  larger_sizes exceeds tcmalloc with lower RSS
  local rows remain inside the preferred -3% promotion line
  wide_ws is 15.0% heavier than tcmalloc and misses the RSS gate
```

The port validates the shared transfer architecture on Windows without opening
a new allocator generation. Any follow-up must target the wide-workset RSS
gap or the small-local tax as a separate, measured experiment.

The accounting-off A/B also confirms that diagnostics were material: removing
the transfer statistic atomics improved the stable median from 398.183M to
422.016M on balanced, 294.397M to 313.499M on wide_ws, and 314.003M to
365.921M on larger_sizes. Keep `HZ11_TRANSFER_STATS=0` in speed lanes and use a
separate diagnostic build whenever these counters are needed.
