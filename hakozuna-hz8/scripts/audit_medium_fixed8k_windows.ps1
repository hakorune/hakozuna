param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int]$ItersPerThread = 1000000,
    [int[]]$Sizes = @(8192, 16384, 32768)
)

$ErrorActionPreference = "Stop"

$Hz8Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz8Root
$Hz10Root = Join-Path $RepoRoot "hakozuna-hz10"
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
$manifest.Add("sizes=$($Sizes -join ',')")

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

$hz10Bench = Join-Path $OutputDir "bench_mixed_ws_hz10_substrate_shadow.exe"
$hz10Sources = @(
    "hz10_public_entry.c", "hz10_public_entry_owner.c", "hz10_class_pages.c",
    "hz10_retired_ready.c", "hz10_size_class.c", "hz10_large_alloc.c",
    "hz10_pooled_page.c", "hz10_page_pool.c", "hz10_remote_stack.c",
    "hz10_freelist_page.c", "hz10_pagemap.c", "hz10_platform.c"
) | ForEach-Object { Join-Path $Hz10Root "src\$_" }
$hz10Args = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DHZ_BENCH_USE_HZ10=1", "/DHZ_BENCH_DISABLE_REALLOC=1",
    "/I$Hz10Root\src", "/I$RepoRoot\win\include"
) + $hz10Sources + @((Join-Path $RepoRoot "win\bench_allocator_compare.c"), "/Fe:$hz10Bench")
& $Compiler @hz10Args
if ($LASTEXITCODE -ne 0) {
    throw "HZ10 substrate shadow build failed"
}
$manifest.Add("hz10_shadow_flags=$($hz10Args -join ' ')")

$phaseSource = Join-Path $Hz8Root "tests\h8_medium_phase_audit_win.c"
$phaseHz8Sources = Get-ChildItem (Join-Path $Hz8Root "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }
$phaseHz8 = Join-Path $OutputDir "h8_medium_phase_hz8.exe"
$phaseHz8Diag = Join-Path $OutputDir "h8_medium_phase_hz8_diag.exe"
$phaseHz10 = Join-Path $OutputDir "h8_medium_phase_hz10.exe"
$phaseTcmalloc = Join-Path $OutputDir "h8_medium_phase_tcmalloc.exe"
$phaseHz8Args = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DHZ_PHASE_USE_HZ8=1"
) + $commonFlags[7..($commonFlags.Count - 1)] + $phaseHz8Sources + @(
    $phaseSource, "/Fe:$phaseHz8"
)
& $Compiler @phaseHz8Args
if ($LASTEXITCODE -ne 0) { throw "HZ8 phase audit build failed" }

$phaseHz8DiagArgs = $phaseHz8Args[0..($phaseHz8Args.Count - 2)] + @(
    "/DH8_ENABLE_DEBUG_STATS=1", "/DHZ_PHASE_REPORT_HZ8_STATS=1",
    "/Fe:$phaseHz8Diag"
)
& $Compiler @phaseHz8DiagArgs
if ($LASTEXITCODE -ne 0) { throw "HZ8 diagnostic phase audit build failed" }

$phaseHz10Args = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DHZ_PHASE_USE_HZ10=1", "/I$Hz10Root\src"
) + $hz10Sources + @($phaseSource, "/Fe:$phaseHz10")
& $Compiler @phaseHz10Args
if ($LASTEXITCODE -ne 0) { throw "HZ10 phase audit build failed" }

$phaseTcmallocArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DHZ_PHASE_USE_TCMALLOC_DYNAMIC=1", $phaseSource,
    "/Fe:$phaseTcmalloc"
)
& $Compiler @phaseTcmallocArgs
if ($LASTEXITCODE -ne 0) { throw "tcmalloc phase audit build failed" }
Copy-Item -Force (Join-Path $SuiteDir "tcmalloc_minimal.dll") $OutputDir
$manifest.Add("phase_hz8_flags=$($phaseHz8Args -join ' ')")
$manifest.Add("phase_hz8_diag_flags=$($phaseHz8DiagArgs -join ' ')")
$manifest.Add("phase_hz10_flags=$($phaseHz10Args -join ' ')")
$manifest.Add("phase_tcmalloc_flags=$($phaseTcmallocArgs -join ' ')")

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

$pathAuditExe = Join-Path $OutputDir "h8_medium_fixed8k_path_audit.exe"
$pathAuditSources = Get-ChildItem (Join-Path $Hz8Root "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }
$pathAuditArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DH8_ENABLE_DEBUG_STATS=1",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1",
    "/DH8_REUSABLE_SPAN_MAGAZINE_L1=1",
    "/I$Hz8Root\include", "/I$Hz8Root\src"
) + $pathAuditSources + @(
    (Join-Path $Hz8Root "tests\h8_medium_fixed8k_path_audit.c"),
    "/Fe:$pathAuditExe"
)
& $Compiler @pathAuditArgs
if ($LASTEXITCODE -ne 0) {
    throw "HZ8 fixed8K path audit build failed"
}
$pathAuditOutput = & $pathAuditExe 2>&1
$pathAuditOutput | Set-Content -Encoding ascii (Join-Path $OutputDir "path_audit.txt")
$manifest.Add("path_audit_flags=$($pathAuditArgs -join ' ')")

function Invoke-FixedSize {
    param([string]$Name, [string]$Path, [int]$Size)
    if (-not (Test-Path $Path)) { return $null }
    [double[]]$values = @()
    [double[]]$cyclesPerOp = @()
    for ($run = 0; $run -lt $Runs; ++$run) {
        $psi = [System.Diagnostics.ProcessStartInfo]::new()
        $psi.FileName = $Path
        $psi.Arguments = "4 $ItersPerThread 1024 $Size $Size"
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
        if (-not $match.Success) { throw "cannot parse fixed-size output for ${Name}/${Size}: $raw" }
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
        size = $Size
        median_ops_s = $sorted[[int][math]::Floor($sorted.Count / 2)]
        median_cycles_op = $sortedCycles[[int][math]::Floor($sortedCycles.Count / 2)]
        samples = ($values -join ';')
        cycle_samples = ($cyclesPerOp -join ';')
    }
}

$allocatorBins = @(
    @{ Name = "hz8-v2"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz8_v2.exe") },
    @{ Name = "hz10-substrate-shadow"; Path = $hz10Bench },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe") }
)
$bench = @(
    foreach ($size in $Sizes) {
        foreach ($allocator in $allocatorBins) {
            Invoke-FixedSize $allocator.Name $allocator.Path $size
        }
    }
) | Where-Object { $_ }
$bench | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "medium_size_repeat.csv")
@($bench | Where-Object { $_.size -eq 8192 }) |
    Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "fixed8k_repeat.csv")

function Invoke-PhaseAudit {
    param([string]$Name, [string]$Path, [int]$Size)
    [double[]]$allocValues = @()
    [double[]]$freeValues = @()
    $oldPath = $env:PATH
    $env:PATH = "$SuiteDir;$oldPath"
    try {
        for ($run = 0; $run -lt $Runs; ++$run) {
            $raw = & $Path $Size 256 1024 64 2>&1 | Out-String
            if ($LASTEXITCODE -ne 0) { throw "phase audit failed for ${Name}/${Size}: $raw" }
            $match = [regex]::Match(
                $raw,
                'alloc_ns_op=([0-9.]+)\s+free_ns_op=([0-9.]+)\s+failures=([0-9]+)'
            )
            if (-not $match.Success -or [int]$match.Groups[3].Value -ne 0) {
                throw "cannot parse phase audit for ${Name}/${Size}: $raw"
            }
            $allocValues += [double]$match.Groups[1].Value
            $freeValues += [double]$match.Groups[2].Value
        }
    } finally {
        $env:PATH = $oldPath
    }
    $allocSorted = @($allocValues | Sort-Object)
    $freeSorted = @($freeValues | Sort-Object)
    [pscustomobject]@{
        allocator = $Name
        size = $Size
        median_alloc_ns_op = $allocSorted[[int][math]::Floor($allocSorted.Count / 2)]
        median_free_ns_op = $freeSorted[[int][math]::Floor($freeSorted.Count / 2)]
        alloc_samples = ($allocValues -join ';')
        free_samples = ($freeValues -join ';')
    }
}

$phaseBins = @(
    @{ Name = "hz8-v2"; Path = $phaseHz8 },
    @{ Name = "hz10-substrate-shadow"; Path = $phaseHz10 },
    @{ Name = "tcmalloc"; Path = $phaseTcmalloc }
)
$phase = @(
    foreach ($size in $Sizes) {
        foreach ($allocator in $phaseBins) {
            Invoke-PhaseAudit $allocator.Name $allocator.Path $size
        }
    }
)
$phase | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "phase_repeat.csv")

$phaseDiag = @(
    foreach ($size in $Sizes) {
        $raw = & $phaseHz8Diag $size 256 1024 64 2>&1 | Out-String
        if ($LASTEXITCODE -ne 0) { throw "HZ8 diagnostic phase failed for ${size}: $raw" }
        $match = [regex]::Match(
            $raw,
            'diag run_create=([0-9]+) empty_transition=([0-9]+) ' +
            'empty_retain=([0-9]+) empty_budget_reject=([0-9]+) ' +
            'empty_reactivate=([0-9]+) mark_live_resident=([0-9]+) ' +
            'mark_live_decommitted=([0-9]+) reuse_active=([0-9]+) ' +
            'reuse_owner=([0-9]+) reuse_global=([0-9]+) ' +
            'owner_scan=([0-9]+) owner_steps=([0-9]+) ' +
            'global_scan=([0-9]+) global_steps=([0-9]+)'
        )
        if (-not $match.Success) { throw "cannot parse HZ8 diagnostic phase for ${size}: $raw" }
        [pscustomobject]@{
            size = $size
            run_create = [uint64]$match.Groups[1].Value
            empty_transition = [uint64]$match.Groups[2].Value
            empty_retain = [uint64]$match.Groups[3].Value
            empty_budget_reject = [uint64]$match.Groups[4].Value
            empty_reactivate = [uint64]$match.Groups[5].Value
            mark_live_resident = [uint64]$match.Groups[6].Value
            mark_live_decommitted = [uint64]$match.Groups[7].Value
            reuse_active = [uint64]$match.Groups[8].Value
            reuse_owner = [uint64]$match.Groups[9].Value
            reuse_global = [uint64]$match.Groups[10].Value
            owner_scan = [uint64]$match.Groups[11].Value
            owner_steps = [uint64]$match.Groups[12].Value
            global_scan = [uint64]$match.Groups[13].Value
            global_steps = [uint64]$match.Groups[14].Value
        }
    }
)
$phaseDiag | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "phase_hz8_diag.csv")

$manifest | Set-Content -Encoding ascii (Join-Path $OutputDir "build_manifest.txt")

$summary = [System.Collections.Generic.List[string]]::new()
$summary.Add("# HZ8 Windows Medium Hot-Path Attribution")
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
$summary.Add("## Exact-Size Throughput")
$summary.Add("")
$summary.Add("| size | allocator | median ops/s | median process cycles/op | runs |")
$summary.Add("|---:|---|---:|---:|---:|")
foreach ($row in $bench) {
    $summary.Add("| $($row.size) | $($row.allocator) | $([math]::Round($row.median_ops_s / 1e6, 3))M | $([math]::Round($row.median_cycles_op, 2)) | $Runs |")
}
$summary.Add("")
$summary.Add("## Batched Allocation/Free Phase Attribution")
$summary.Add("")
$summary.Add("Single-thread, batch 256, 64 warmup rounds, 1024 measured rounds.")
$summary.Add("")
$summary.Add("| size | allocator | alloc ns/op | free ns/op | runs |")
$summary.Add("|---:|---|---:|---:|---:|")
foreach ($row in $phase) {
    $summary.Add("| $($row.size) | $($row.allocator) | $([math]::Round($row.median_alloc_ns_op, 3)) | $([math]::Round($row.median_free_ns_op, 3)) | $Runs |")
}
$summary.Add("")
$summary.Add("## HZ8 Diagnostic Residency Attribution")
$summary.Add("")
$summary.Add("This table comes from a separate counter-bearing binary and is not a speed result.")
$summary.Add("")
$summary.Add("| size | run create | empty transitions | retained | budget reject | resident reactivate | decommitted reactivate |")
$summary.Add("|---:|---:|---:|---:|---:|---:|---:|")
foreach ($row in $phaseDiag) {
    $summary.Add("| $($row.size) | $($row.run_create) | $($row.empty_transition) | $($row.empty_retain) | $($row.empty_budget_reject) | $($row.mark_live_resident) | $($row.mark_live_decommitted) |")
}
$summary.Add("")
$summary.Add("| size | active reuse | owner reuse | global reuse | owner scans/steps | global scans/steps |")
$summary.Add("|---:|---:|---:|---:|---:|---:|")
foreach ($row in $phaseDiag) {
    $summary.Add("| $($row.size) | $($row.reuse_active) | $($row.reuse_owner) | $($row.reuse_global) | $($row.owner_scan)/$($row.owner_steps) | $($row.global_scan)/$($row.global_steps) |")
}
$summary.Add("")
$summary.Add("Static instruction counts are attribution aids, not cycle measurements. Calls may inline or share cold blocks; inspect the saved assembly before assigning contract cost.")
$summary.Add("")
$summary.Add("## L1 Active-Block Path Audit")
$summary.Add("")
$summary.Add("The diagnostic same-owner fixed-8K path audit output is saved in `path_audit.txt`.")
$summary.Add("")
foreach ($line in $pathAuditOutput) {
    $summary.Add("    " + $line.ToString())
}
$summary | Set-Content -Encoding utf8 (Join-Path $OutputDir "HZ8_WINDOWS_MEDIUM_HOT_PATH_ATTRIBUTION.md")
# Preserve the old report name for links and automation that still consume it.
$summary | Set-Content -Encoding utf8 (Join-Path $OutputDir "HZ8_MEDIUM_FIXED8K_COST_AUDIT.md")

Write-Host "Wrote HZ8 Windows medium hot-path attribution: $OutputDir"
