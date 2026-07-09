param(
  [int]$Runs = 5,
  [UInt64]$Iters = 5000000,
  [int[]]$SlotSizes = @(64, 256, 1024, 4096),
  [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$repo = Split-Path -Parent $root
$clang = (Get-Command clang-cl -ErrorAction Stop).Source

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
  $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
  $OutputRoot = Join-Path $repo "results\windows\hz11_l0_fixed_local\$stamp"
}
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$benchSrc = Join-Path $root "bench\hz11_fixed_local_bench.c"
$commonSources = @(
  (Join-Path $root "src\hz11_size_class.c"),
  (Join-Path $root "src\hz11_sys_alloc.c"),
  (Join-Path $root "src\hz11_thread_cache.c"),
  (Join-Path $root "src\hz11_public_entry.c")
)

$systemExe = Join-Path $OutputRoot "hz11_fixed_local_system.exe"
$tokenExe = Join-Path $OutputRoot "hz11_fixed_local_token.exe"
$tlsfastExe = Join-Path $OutputRoot "hz11_fixed_local_tlsfast.exe"

function Invoke-Clang {
  param([Parameter(Mandatory = $true, Position = 0)][string[]]$ClangArgs)
  & $clang @ClangArgs
  if ($LASTEXITCODE -ne 0) {
    throw "clang-cl failed with exit code $LASTEXITCODE"
  }
}

Invoke-Clang @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/Fe$systemExe",
  $benchSrc
)

$tokenArgs = @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/DHZ11_BENCH_USE_HZ11_API=1",
  "/I", (Join-Path $root "include"),
  "/I", (Join-Path $root "src"),
  "/Fe$tokenExe",
  $benchSrc
)
Invoke-Clang ($tokenArgs + $commonSources)

$tlsfastArgs = @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/DHZ11_BENCH_USE_HZ11_API=1",
  "/DHZ11_TLS_FASTPATH=1",
  "/I", (Join-Path $root "include"),
  "/I", (Join-Path $root "src"),
  "/Fe$tlsfastExe",
  $benchSrc
)
Invoke-Clang ($tlsfastArgs + $commonSources)

$lanes = @(
  @{ name = "system-crt"; exe = $systemExe },
  @{ name = "hz11-token"; exe = $tokenExe },
  @{ name = "hz11-tlsfast"; exe = $tlsfastExe }
)

$rows = New-Object System.Collections.Generic.List[object]
$rawLog = Join-Path $OutputRoot "raw.log"
Set-Content -Path $rawLog -Value ""

foreach ($slot in $SlotSizes) {
  foreach ($lane in $lanes) {
    for ($run = 1; $run -le $Runs; ++$run) {
      $env:SLOT_SIZE = [string]$slot
      $env:ITERS = [string]$Iters
      $stdoutPath = Join-Path $OutputRoot ("tmp_{0}_{1}_{2}.out" -f $lane.name, $slot, $run)
      $stderrPath = Join-Path $OutputRoot ("tmp_{0}_{1}_{2}.err" -f $lane.name, $slot, $run)
      $proc = Start-Process -FilePath $lane.exe `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath
      $stdout = Get-Content -Path $stdoutPath
      $stderr = Get-Content -Path $stderrPath
      Remove-Item -Path $stdoutPath, $stderrPath -ErrorAction SilentlyContinue
      if ($proc.ExitCode -ne 0) {
        throw "$($lane.name) slot=$slot run=$run failed with exit code $($proc.ExitCode)`n$stdout`n$stderr"
      }
      Add-Content -Path $rawLog -Value "[$($lane.name) slot=$slot run=$run] stdout=$stdout stderr=$stderr"

      $line = ($stdout | Where-Object { $_ -match "ops_per_s=" } | Select-Object -First 1)
      if (-not $line) {
        throw "Could not parse ops_per_s for $($lane.name) slot=$slot run=$run"
      }
      if ($line -notmatch "seconds=([0-9.]+).*ops_per_s=([0-9.]+).*ns_per_op=([0-9.]+)") {
        throw "Unexpected bench output: $line"
      }
      $rows.Add([pscustomobject]@{
        slot = $slot
        lane = $lane.name
        run = $run
        seconds = [double]$Matches[1]
        ops_per_s = [double]$Matches[2]
        ns_per_op = [double]$Matches[3]
      })
    }
  }
}

Remove-Item Env:SLOT_SIZE -ErrorAction SilentlyContinue
Remove-Item Env:ITERS -ErrorAction SilentlyContinue

$samplesCsv = Join-Path $OutputRoot "samples.csv"
$rows | Export-Csv -NoTypeInformation -Path $samplesCsv

function Get-Median([double[]]$values) {
  $sorted = @($values | Sort-Object)
  $n = $sorted.Count
  if ($n -eq 0) { return 0.0 }
  if (($n % 2) -eq 1) { return [double]$sorted[[int]($n / 2)] }
  return ([double]$sorted[$n / 2 - 1] + [double]$sorted[$n / 2]) / 2.0
}

$summaryRows = foreach ($group in ($rows | Group-Object slot, lane)) {
  $first = $group.Group[0]
  [pscustomobject]@{
    slot = $first.slot
    lane = $first.lane
    median_ops_per_s = Get-Median @($group.Group | ForEach-Object { $_.ops_per_s })
    median_ns_per_op = Get-Median @($group.Group | ForEach-Object { $_.ns_per_op })
  }
}

$summaryCsv = Join-Path $OutputRoot "summary.csv"
$summaryRows | Sort-Object slot, lane | Export-Csv -NoTypeInformation -Path $summaryCsv

$md = New-Object System.Collections.Generic.List[string]
$md.Add('# HZ11 Windows L0 Fixed Local Bench')
$md.Add('')
$md.Add('Scope: standalone Windows L0 token/front-cache public-entry bench.')
$md.Add('This is not fine128 parity, not LD_PRELOAD parity, and not a malloc replacement claim.')
$md.Add('')
$md.Add('```text')
$md.Add(('runs: {0}' -f $Runs))
$md.Add(('iters_per_run: {0}' -f $Iters))
$md.Add(('slots: {0}' -f ($SlotSizes -join ', ')))
$md.Add('compiler: clang-cl /O2')
$md.Add('lanes: system-crt, hz11-token, hz11-tlsfast')
$md.Add('```')
$md.Add('')
$md.Add('| slot | lane | median ops/s | median ns/op |')
$md.Add('|---:|---|---:|---:|')
foreach ($row in ($summaryRows | Sort-Object slot, lane)) {
  $md.Add(("| {0} | {1} | {2:N2} | {3:N3} |" -f $row.slot, $row.lane, $row.median_ops_per_s, $row.median_ns_per_op))
}
$md.Add('')
$md.Add('Files:')
$md.Add('- `samples.csv`: per-run samples')
$md.Add('- `summary.csv`: median summary')
$md.Add('- `raw.log`: raw process output')

$summaryMd = Join-Path $OutputRoot "summary.md"
Set-Content -Path $summaryMd -Value $md

Write-Host "wrote $summaryMd"
Get-Content -Path $summaryMd
