# HZ7 v3 Bootstrap Hotpath Snapshot

Generated on 2026-06-11 JST.

This is a tiny bootstrap check for the new `hakozuna-hz7-v3/` folder. It proves
that the copied v2 seed can build and run as an explicitly named v3 lane before
any performance policy changes are made.

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz7-v3\win\run_win_hz7_v3_hotpath.ps1 `
  -Runs 1 `
  -Iters 100000 `
  -OutputDir .\docs\benchmarks\windows\hz7_v3_bootstrap_hotpath
```

Artifact:

```text
20260611_221231_hz7_v3_hotpath_windows.md
```

Reading:

```text
status:
  v3 hotpath runner builds
  v3 hotpath runner writes v3-named artifacts
  no v3 performance change has been attempted yet

next:
  SpanPathAudit-L1
  then wire a v3 size-slice/random-mixed row if the audit needs broader data
```
