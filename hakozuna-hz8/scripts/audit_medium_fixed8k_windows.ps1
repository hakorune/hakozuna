param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int]$ItersPerThread = 1000000
)

$ErrorActionPreference = "Stop"

$Hz8Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz8Root
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$Compiler = (Get-Command "clang-cl" -ErrorAction Stop).Source
$Objdump = (Get-Command "llvm-objdump" -ErrorAction Stop).Source
$Nm = (Get-Command "llvm-nm" -ErrorAction Stop).Source

if (-not $OutputDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDir = Join-Path $RepoRoot "results\hz8-medium-fixed8k-cost-audit\$stamp"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

$commonFlags = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD", "/c",
    "/I$Hz8Root\include", "/I$Hz8Root\src",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1"
)

$objects = @(
    @{ Name = "h8_medium"; Source = Join-Path $Hz8Root "src\h8_medium.c" },
    @{ Name = "h8_medium_slots"; Source = Join-Path $Hz8Root "src\h8_medium_slots.c" },
    @{ Name = "h8_small_local"; Source = Join-Path $Hz8Root "src\h8_small_local.c" }
)

$manifest = [System.Collections.Generic.List[string]]::new()
$manifest.Add("compiler=$Compiler")
$manifest.Add("objdump=$Objdump")
$manifest.Add("nm=$Nm")
$manifest.Add("flags=$($commonFlags -join ' ')")
$manifest.Add("runs=$Runs")
$manifest.Add("iters_per_thread=$ItersPerThread")

if (-not ("Hz8Audit.NativeMethods" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
namespace Hz8Audit {
  public static class NativeMethods {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool QueryProcessCycleTime(IntPtr processHandle,
                                                     out ulong cycles);
  }
}
"@
}

foreach ($item in $objects) {
    $obj = Join-Path $OutputDir ($item.Name + ".obj")
    & $Compiler @commonFlags $item.Source "/Fo$obj"
    if ($LASTEXITCODE -ne 0) {
        throw "audit object build failed: $($item.Name)"
    }
    & $Nm --defined-only $obj | Set-Content -Encoding ascii (Join-Path $OutputDir ($item.Name + ".symbols.txt"))
    & $Objdump -dr --no-show-raw-insn $obj |
        Set-Content -Encoding ascii (Join-Path $OutputDir ($item.Name + ".asm.txt"))
}

function Get-FunctionStats {
    param(
        [string]$AsmPath,
        [string]$Symbol
    )

    $inside = $false
    $instructions = 0
    $calls = 0
    $locked = 0
    foreach ($line in Get-Content $AsmPath) {
        if ($line -match '^\s*[0-9a-f]+\s+<([^>]+)>:') {
            if ($inside) { break }
            $inside = ($Matches[1] -eq $Symbol)
            continue
        }
        if (-not $inside -or $line -notmatch '^\s*[0-9a-f]+:\s+\S+') {
            continue
        }
        if ($line -match 'IMAGE_REL_') { continue }
        ++$instructions
        if ($line -match '\bcallq?\b') { ++$calls }
        if ($line -match '\block\b|\bxchg[a-z]*\b|\bxadd[a-z]*\b|\bcmpxchg[a-z]*\b') {
            ++$locked
        }
    }
    [pscustomobject]@{
        symbol = $Symbol
        instructions = $instructions
        calls = $calls
        locked_rmw = $locked
    }
}

$stats = @(
    Get-FunctionStats (Join-Path $OutputDir "h8_medium.asm.txt") "h8_medium_malloc_class_inner"
    Get-FunctionStats (Join-Path $OutputDir "h8_medium.asm.txt") "h8_medium_free_inner"
    Get-FunctionStats (Join-Path $OutputDir "h8_medium_slots.asm.txt") "h8_medium_run_alloc_local_scaffold"
    Get-FunctionStats (Join-Path $OutputDir "h8_medium_slots.asm.txt") "h8_medium_run_free_local_scaffold"
    Get-FunctionStats (Join-Path $OutputDir "h8_small_local.asm.txt") "h8_malloc_inner"
    Get-FunctionStats (Join-Path $OutputDir "h8_small_local.asm.txt") "h8_free_inner"
)
$stats | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "function_stats.csv")

function Get-CallTargets {
    param(
        [string]$AsmPath,
        [string]$Symbol
    )
    $inside = $false
    $pendingCall = $false
    $targets = @{}
    foreach ($line in Get-Content $AsmPath) {
        if ($line -match '^\s*[0-9a-f]+\s+<([^>]+)>:') {
            if ($inside) { break }
            $inside = ($Matches[1] -eq $Symbol)
            continue
        }
        if (-not $inside) { continue }
        if ($line -match '^\s*[0-9a-f]+:\s+callq?\b') {
            $pendingCall = $true
            continue
        }
        if ($pendingCall -and $line -match 'IMAGE_REL_AMD64_REL32\s+(.+)$') {
            $target = $Matches[1].Trim()
            if (-not $targets.ContainsKey($target)) { $targets[$target] = 0 }
            ++$targets[$target]
            $pendingCall = $false
        } elseif ($line -match '^\s*[0-9a-f]+:\s+\S+') {
            $pendingCall = $false
        }
    }
    foreach ($target in ($targets.Keys | Sort-Object)) {
        [pscustomobject]@{ symbol = $Symbol; target = $target; count = $targets[$target] }
    }
}

$callTargets = @(
    Get-CallTargets (Join-Path $OutputDir "h8_medium.asm.txt") "h8_medium_malloc_class_inner"
    Get-CallTargets (Join-Path $OutputDir "h8_medium.asm.txt") "h8_medium_free_inner"
    Get-CallTargets (Join-Path $OutputDir "h8_medium_slots.asm.txt") "h8_medium_run_alloc_local_scaffold"
    Get-CallTargets (Join-Path $OutputDir "h8_medium_slots.asm.txt") "h8_medium_run_free_local_scaffold"
)
$callTargets | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "call_targets.csv")

function Invoke-Fixed8K {
    param([string]$Name, [string]$Path)
    if (-not (Test-Path $Path)) { return $null }
    [double[]]$values = @()
    [double[]]$cyclesPerOp = @()
    for ($run = 0; $run -lt $Runs; ++$run) {
        $psi = [System.Diagnostics.ProcessStartInfo]::new()
        $psi.FileName = $Path
        $psi.Arguments = "4 $ItersPerThread 1024 8192 8192"
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $process = [System.Diagnostics.Process]::new()
        $process.StartInfo = $psi
        [void]$process.Start()
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        [uint64]$cycles = 0
        if (-not [Hz8Audit.NativeMethods]::QueryProcessCycleTime($process.Handle, [ref]$cycles)) {
            throw "QueryProcessCycleTime failed for $Name"
        }
        $raw = $stdoutTask.Result + " " + $stderrTask.Result
        $match = [regex]::Match($raw, 'ops/s=([0-9.]+)')
        if (-not $match.Success) { throw "cannot parse fixed8K output for ${Name}: $raw" }
        $values += [double]$match.Groups[1].Value
        # bench_allocator_compare defines one logical operation per loop
        # iteration; cleanup frees are intentionally outside this denominator.
        $operations = 4.0 * [double]$ItersPerThread
        $cyclesPerOp += [double]$cycles / $operations
    }
    $sorted = @($values | Sort-Object)
    $sortedCycles = @($cyclesPerOp | Sort-Object)
    [pscustomobject]@{
        allocator = $Name
        median_ops_s = $sorted[[int][math]::Floor($sorted.Count / 2)]
        median_cycles_op = $sortedCycles[[int][math]::Floor($sortedCycles.Count / 2)]
        samples = ($values -join ';')
        cycle_samples = ($cyclesPerOp -join ';')
    }
}

$bench = @(
    Invoke-Fixed8K "hz8-v2" (Join-Path $SuiteDir "bench_mixed_ws_hz8_v2.exe")
    Invoke-Fixed8K "hz10" (Join-Path $SuiteDir "bench_mixed_ws_hz10.exe")
    Invoke-Fixed8K "tcmalloc" (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe")
) | Where-Object { $_ }
$bench | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "fixed8k_repeat.csv")

$manifest | Set-Content -Encoding ascii (Join-Path $OutputDir "build_manifest.txt")

$summary = [System.Collections.Generic.List[string]]::new()
$summary.Add("# HZ8 Medium Fixed 8K Cost Audit")
$summary.Add("")
$summary.Add("Diagnostic-only release-equivalent object audit. No production allocator flags or behavior are changed.")
$summary.Add("")
$summary.Add("## Static Function Shape")
$summary.Add("")
$summary.Add("| symbol | instructions | calls | locked RMW |")
$summary.Add("|---|---:|---:|---:|")
foreach ($row in $stats) {
    $summary.Add("| $($row.symbol) | $($row.instructions) | $($row.calls) | $($row.locked_rmw) |")
}
$summary.Add("")
$summary.Add("## Static Call Targets")
$summary.Add("")
$summary.Add("| symbol | target | count |")
$summary.Add("|---|---|---:|")
foreach ($row in $callTargets) {
    $summary.Add("| $($row.symbol) | $($row.target) | $($row.count) |")
}
$summary.Add("")
$summary.Add("## Fixed 8K Throughput")
$summary.Add("")
$summary.Add("| allocator | median ops/s | median process cycles/op | runs |")
$summary.Add("|---|---:|---:|---:|")
foreach ($row in $bench) {
    $summary.Add("| $($row.allocator) | $([math]::Round($row.median_ops_s / 1e6, 3))M | $([math]::Round($row.median_cycles_op, 2)) | $Runs |")
}
$summary.Add("")
$summary.Add("Static instruction counts are attribution aids, not cycle measurements. Calls may inline or share cold blocks; inspect the saved assembly before assigning contract cost.")
$summary | Set-Content -Encoding utf8 (Join-Path $OutputDir "HZ8_MEDIUM_FIXED8K_COST_AUDIT.md")

Write-Host "Wrote HZ8 medium fixed8K audit: $OutputDir"
