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

## Capacity Lane Commands

HZ6 Windows capacity lanes are built through the shared app-like benchmark
builder. See `docs/HZ6_LANE_GUIDE.md` for lane meanings before comparing rows.

Build a focused suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_hz6_capacity_suite.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap
```

Run the focused matrix:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap `
  -Runs 1
```

Run the focused Larson owner-locality comparison:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families larson `
  -Hz6Profiles speed `
  -CapacityLanes appcap,ownerlocality-appcap `
  -ThreadCounts 16 `
  -BenchmarkProfiles larson_t16_main_1k,larson_t16_worker_1k,larson_t16_main_4k,larson_t16_worker_4k `
  -Runs 1 `
  -DiagnosticHz6Probes `
  -SkipBuild
```

Run the appcap-only Larson baseline:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_larson_paper.ps1 `
  -LarsonAppcapOnly `
  -Runs 3 `
  -TimeoutSeconds 240 `
  -ContinueOnFailure
```

Default capacity matrix lanes:

```text
control:
  low-capacity / low-RSS baseline

route4k:
  historical low-capacity candidate-control lane. Use `HZ6_LANE_GUIDE.md` for
  the current selected profile-family rows.

noboost-route4k:
  route4k with starvation source-refill boost disabled. This is now a
  mechanism/control lane, not the current mixed_ws selected row.

appcap:
  high-capacity completion/control lane, not a default

ownerlocality-appcap:
  appcap plus the owner-locality/shared-directory exact route probe. Use this
  only for cross-owner Larson route-lifecycle evidence unless it is explicitly
  promoted later.

ownerlocalityfast-appcap:
  non-diagnostic owner-locality/shared-directory exact route lane. Use this for
  throughput/RSS checks after `ownerlocality-appcap` has validated the
  counters. Repeat-3 is now enough to treat it as the preferred
  candidate-control lane for fast owner-locality comparisons.

Larson appcap-only baseline:
  `run_win_larson_paper.ps1 -LarsonAppcapOnly`
  keeps the clean HZ6 appcap family together without the default warmup
  no-go rows. The narrow baseline currently includes
  `hz6-strict-appcap`, `hz6-speed-appcap`, `hz6-rss-appcap`, and
  `hz6-ownerlocality-appcap-speed`, so it is short enough to rerun quickly
  when the goal is the current Windows baseline rather than a full paper
  comparison. In the latest narrow rerun, the strongest row is
  `hz6-ownerlocality-appcap-speed` at `45.754M ops/s` and `2,250,016 KB`
  peak RSS.
```

Current selected HZ6 rows are maintained in:

```text
hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
```

Research lanes such as `sourcerun-route4k`,
`sourcerun-sameclass-route4k`, `spill-route4k`, `borrow-route4k`,
`cap-route4k`, and `sourcerun-reclaim-route4k` should be requested
explicitly. Do not mix diagnostic-only research lanes into speed or paper
runs unless the row is labelled as a research/control row.

## Status

This is a build-environment seed only, but it is now verified locally with
`clang-cl` by compiling and running the full R1 smoke set. The benchmark
entrypoints are wired so Windows can run HZ6-only, capacity-lane, and
app-like matrices. Cross-family claims still need fair row-by-row benchmark
attribution.
