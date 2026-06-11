# HZ7 TinyRoute V2 Handoff

Current local state:

```text
branch:
  main

status:
  pushed to origin/main

next intended action:
  ask external review using docs/HZ7_V2_STOP_OR_CONTINUE_PROMPT.md
  decide whether to accept closeout or authorize one more measured experiment
```

Current local recommendation:

```text
close HZ7 v2 here as the cap64 tiny reference allocator
```

Why:

```text
HZ7 v2 now has a clean identity:
  tiny
  route-safe
  low-RSS
  coarse-lock thread safe
  cross-thread free safe
  not remote-throughput optimized

The remaining obvious performance paths would either chase HZ3/tcmalloc
throughput or introduce owner/TLS/remote-fast policy, which would weaken the
HZ7 v2 identity.
```

Files to read first:

```text
docs/HZ7_V2_CLOSEOUT_REVIEW.md
docs/HZ7_V2_STOP_OR_CONTINUE_PROMPT.md
docs/HZ7_V2_TASKS.md
```

Baseline evidence:

```text
docs/benchmarks/windows/hz7_v2_baseline_snapshot/
20260611_174745_paper_random_mixed_windows.md
```

Safety gates already used for the closeout lane:

```text
Windows:
  hz7/v2/win/build_win_hz7_smokes.ps1

Linux:
  hz7/v2/linux/build_hz7_smoke.sh
```

If external review says continue:

```text
allowed:
  one focused experiment only
  compare against the baseline snapshot
  preserve low RSS and tiny readability

not allowed:
  owner-aware remote free
  owner inbox
  TLS ownership
  lock-free remote queue
  remote batching
  HZ6-style profile matrix
  production hot-path diagnostics
```
