# HZ6 Windows Build And Benchmark Seed

HZ6 now has Windows entrypoints for the R1 smoke suite and the HZ6-only
allocator benchmark. This is still a Windows build/measurement seed, not a
cross-family benchmark claim.

## What It Builds

- The HZ6 R1 smoke executables.
- The HZ6 allocator benchmark executable.
- The Windows source backend backed by `source/win_source_virtualalloc.*`.
- The same modular library set as the Linux R1/benchmark runners, except for
  the Linux-only `linux_source_mmap.*` files.
- A shared Windows build helper, `win/hz6_win_build_common.ps1`, keeps smoke
  and benchmark source selection aligned.

## Smoke Command

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\build_win_hz6_r1_smokes.ps1
```

Optional:

- `-SkipRun` builds the smoke executables without running them.
- `-OutDir <path>` writes the binaries somewhere else.
- `-CompilerPath <path-or-command>` overrides `clang-cl`.

## Output

Default output directory:

```text
hakozuna-hz6/out/win/r1_smokes
```

Smoke executable names match the Linux R1 suite:

```text
hz6_r1_core_contract_smoke.exe
hz6_r1_route_smoke.exe
hz6_r1_transfer_contract_smoke.exe
hz6_r1_source_contract_smoke.exe
hz6_r1_allocator_smoke.exe
hz6_r1_prefill_smoke.exe
hz6_r1_sourceblock_smoke.exe
hz6_r1_transfer_smoke.exe
hz6_r1_reclaim_smoke.exe
hz6_r1_safety_smoke.exe
```

## Benchmark Commands

Build only:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\build_win_hz6_benchmark.ps1
```

Build and run the Windows mirror of the Linux HZ6-only benchmark matrix:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\run_win_hz6_benchmark.ps1
```

Optional smoke-sized run:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz6\win\run_win_hz6_benchmark.ps1 `
  -Runs 1 -LocalIters 1000 -RemoteIters 500 -ReuseIters 500 `
  -LocalProfiles strict -RemoteProfiles rss -ReuseProfiles rss
```

Default benchmark output directory:

```text
hakozuna-hz6/private/raw-results/windows/hz6_benchmark_<timestamp>
```

The Windows runner writes a Linux-compatible `results.tsv` shape. The
`ru_maxrss_kb` column is populated from Windows `PeakWorkingSet64 / 1024`, so
it should be compared as a Windows peak working-set measurement rather than a
literal POSIX `ru_maxrss` value. Very short runs may report `NA` if Windows
does not expose a nonzero working-set sample before the process exits.

## Status

This is a build-environment seed only, but it is now verified locally with
`clang-cl` by compiling and running the full R1 smoke set. The benchmark
entrypoint is wired so Windows can run the same HZ6-only mode/profile/size
matrix as Linux. Cross-family claims still need a separate fair benchmark
pass.
