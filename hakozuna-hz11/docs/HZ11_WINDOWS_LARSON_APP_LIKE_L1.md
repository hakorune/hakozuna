# HZ11 Windows Larson App-Like L1

Status: GO for runner connectivity and safety smoke. Not a speed claim.

## Question

After selecting `hz11-span-cache256` as the Windows bring-up row and keeping
`ReturnedColdSkip-L2` as matrix evidence, the remaining Windows question was
whether HZ11 can be exercised by an app-like benchmark outside the
`random_mixed` and allocator-matrix runners.

## Added Rows

The Windows Larson suite now builds:

```text
hz11-span
hz11-span-cache256
hz11-span-cache512-classbatch16-coldskip
```

`hz11-span-cache256` is the selected Windows HZ11 bring-up row. The coldskip row
is present only to check whether the matrix specialist shape leaks into an
app-like workload.

## Smoke

Manual short-run smoke:

```text
runtime=2s min=8 max=1024 chunks=10000 rounds=1 seed=12345 threads=4
```

Worker-warmup:

| allocator | ops/s | result |
| --- | ---: | --- |
| hz11-span-cache256 | 26.999M | OK |
| hz11-span-cache512-classbatch16-coldskip | 22.583M | OK |
| tcmalloc | 39.121M | OK |

Main-warmup:

| allocator | ops/s | result |
| --- | ---: | --- |
| hz11-span-cache256 | 24.716M | OK |
| hz11-span-cache512-classbatch16-coldskip | 25.724M | OK |
| tcmalloc | 44.357M | OK |

## Reading

```text
GO:
  HZ11 Windows selected row is now connected to Larson.
  Both worker-warmup and main-warmup short smokes complete.

NO:
  This is not a speed win over tcmalloc.
  Do not promote coldskip based on Larson; it remains matrix/wide_ws evidence.
```

The current takeaway is that `hz11-span-cache256` is safe enough for app-like
Windows smoke coverage, while tcmalloc remains the stronger Larson throughput
baseline on this machine.

## Next

If this track continues, use the normal Larson runner for a repeat matrix:

```text
win/run_win_larson_paper.ps1
```

Keep any claim scoped to connectivity/safety unless a repeat run with RSS shows
a clear HZ11 advantage.
