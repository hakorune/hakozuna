param(
    [string]$OutputDir,
    [ValidateRange(3, 21)]
    [int]$Runs = 5,
    [ValidateRange(100000, 100000000)]
    [int]$Iterations = 20000000,
    [ValidateRange(1, 1000000)]
    [int]$Warmup = 4096,
    [int[]]$Sizes = @(64, 128, 256)
)

$ErrorActionPreference = "Stop"

$Hz8Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz8Root
$Hz11Root = Join-Path $RepoRoot "hakozuna-hz11"
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$Compiler = (Get-Command "clang-cl" -ErrorAction Stop).Source
$Objdump = (Get-Command "llvm-objdump" -ErrorAction Stop).Source
$Nm = (Get-Command "llvm-nm" -ErrorAction Stop).Source
$AuditSource = Join-Path $Hz8Root "tests\h8_small_hot_path_audit.c"

if (-not $OutputDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDir = Join-Path $RepoRoot "results\hz8-small-hotpath-audit\$stamp"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

if (-not ("Hz8SmallAudit.NativeMethods" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
namespace Hz8SmallAudit {
  public static class NativeMethods {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool QueryProcessCycleTime(IntPtr processHandle,
                                                     out ulong cycles);
  }
}
"@
}

$BaseFlags = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD"
)
$Hz8Flags = @(
    "/I$Hz8Root\include", "/I$Hz8Root\src",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
    "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
    "/DH8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1=1",
    "/DH8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1=1"
)
$Hz11Flags = @(
    "/I$Hz11Root\include", "/I$Hz11Root\src",
    "/DHZ11_TLS_FASTPATH=1",
    "/DHZ11_ENABLE_HOT_COUNTERS=0",
    "/DHZ11_CACHE_BYTE_ACCOUNTING=0",
    "/DHZ11_CACHE_SOA=1",
    "/DHZ11_TRANSFER_STATS=0",
    "/DHZ11_CENTRAL_CLASS_DIAG=0",
    "/DHZ11_TRANSFER_CENTRAL_SPAN=1",
    "/DHZ11_CURRENT_SPAN_THREAD_EXIT=1",
    "/DHZ11_CENTRAL_CAP=65536",
    "/DHZ11_TRANSFER_BATCH=32",
    "/DHZ11_FINE_SIZE_CLASSES=1",
    "/DHZ11_FINE_LINEAR_MAX=128u"
)
$Hz8Sources = Get-ChildItem (Join-Path $Hz8Root "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }
$Hz11Sources = @(
    "hz11_size_class.c",
    "hz11_sys_alloc.c",
    "hz11_thread_cache.c",
    "hz11_public_entry.c",
    "hz11_span.c",
    "hz11_live_footprint.c",
    "hz11_transfer_cache.c",
    "hz11_alloc_diag.c"
) | ForEach-Object { Join-Path $Hz11Root "src\$_" }

function Invoke-Build {
    param([string]$Name, [string[]]$Arguments)
    Write-Host "[small-audit] building $Name"
    & $Compiler @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "build failed for $Name with exit code $LASTEXITCODE"
    }
}

$Hz8Exe = Join-Path $OutputDir "h8_small_pair_hz8.exe"
$Hz11Exe = Join-Path $OutputDir "h8_small_pair_hz11_fine128.exe"
$TcmallocExe = Join-Path $OutputDir "h8_small_pair_tcmalloc.exe"
Invoke-Build "HZ8 default" ($BaseFlags + @("/DHZ_SMALL_USE_HZ8=1") +
    $Hz8Flags + $Hz8Sources + @($AuditSource, "/Fe:$Hz8Exe"))
Invoke-Build "HZ11 fine128" ($BaseFlags + @("/DHZ_SMALL_USE_HZ11=1") +
    $Hz11Flags + $Hz11Sources + @($AuditSource, "/Fe:$Hz11Exe"))
Invoke-Build "tcmalloc adapter" ($BaseFlags +
    @("/DHZ_SMALL_USE_TCMALLOC_DYNAMIC=1", $AuditSource, "/Fe:$TcmallocExe"))

$TcmallocDll = Join-Path $SuiteDir "tcmalloc_minimal.dll"
if (-not (Test-Path $TcmallocDll)) {
    throw "tcmalloc_minimal.dll not found: $TcmallocDll"
}
Copy-Item -Force $TcmallocDll $OutputDir

$Hz8Obj = Join-Path $OutputDir "h8_small_local.obj"
$Hz11Obj = Join-Path $OutputDir "hz11_public_entry.obj"
Invoke-Build "HZ8 small entry object" ($BaseFlags + @("/c") + $Hz8Flags +
    @((Join-Path $Hz8Root "src\h8_small_local.c"), "/Fo$Hz8Obj"))
Invoke-Build "HZ11 public entry object" ($BaseFlags + @("/c") + $Hz11Flags +
    @((Join-Path $Hz11Root "src\hz11_public_entry.c"), "/Fo$Hz11Obj"))

foreach ($item in @(
    @{ Name = "h8_small_local"; Object = $Hz8Obj },
    @{ Name = "hz11_public_entry"; Object = $Hz11Obj }
)) {
    & $Nm --defined-only $item.Object |
        Set-Content -Encoding ascii (Join-Path $OutputDir "$($item.Name).symbols.txt")
    & $Objdump -dr --no-show-raw-insn $item.Object |
        Set-Content -Encoding ascii (Join-Path $OutputDir "$($item.Name).asm.txt")
}

function Get-FunctionStats {
    param([string]$AsmPath, [string]$Symbol)
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

$FunctionStats = @(
    Get-FunctionStats (Join-Path $OutputDir "h8_small_local.asm.txt") "h8_malloc_inner"
    Get-FunctionStats (Join-Path $OutputDir "h8_small_local.asm.txt") "h8_free_inner"
    Get-FunctionStats (Join-Path $OutputDir "hz11_public_entry.asm.txt") "hz11_malloc"
    Get-FunctionStats (Join-Path $OutputDir "hz11_public_entry.asm.txt") "hz11_free"
)
$FunctionStats | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "function_stats.csv")

# hz8-a and hz8-b intentionally use the exact same binary. Their separation is
# the in-session noise floor, not a behavior comparison.
$Bins = @(
    @{ Name = "hz8-a"; Path = $Hz8Exe },
    @{ Name = "hz11-fine128"; Path = $Hz11Exe },
    @{ Name = "hz8-b"; Path = $Hz8Exe },
    @{ Name = "tcmalloc"; Path = $TcmallocExe }
)
$Samples = [System.Collections.Generic.List[object]]::new()
for ($run = 0; $run -lt $Runs; ++$run) {
    for ($sizeIndex = 0; $sizeIndex -lt $Sizes.Count; ++$sizeIndex) {
        $size = $Sizes[($sizeIndex + $run) % $Sizes.Count]
        for ($allocatorIndex = 0; $allocatorIndex -lt $Bins.Count; ++$allocatorIndex) {
            $allocator = $Bins[($allocatorIndex + $run) % $Bins.Count]
            $psi = [System.Diagnostics.ProcessStartInfo]::new()
            $psi.FileName = $allocator.Path
            $psi.WorkingDirectory = $OutputDir
            $psi.Arguments = "$size $Iterations $Warmup"
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
            if (-not [Hz8SmallAudit.NativeMethods]::QueryProcessCycleTime(
                    $process.Handle, [ref]$cycles)) {
                throw "QueryProcessCycleTime failed for $($allocator.Name)/$size"
            }
            $raw = $stdoutTask.Result + " " + $stderrTask.Result
            $match = [regex]::Match(
                $raw,
                'ns_pair=([0-9.]+)\s+failures=([0-9]+)\s+sink=([0-9]+)'
            )
            if ($process.ExitCode -ne 0 -or -not $match.Success -or
                [int]$match.Groups[2].Value -ne 0) {
                throw "audit failed for $($allocator.Name)/$size run=${run}: $raw"
            }
            $Samples.Add([pscustomobject]@{
                run = $run
                size = $size
                allocator = $allocator.Name
                ns_pair = [double]$match.Groups[1].Value
                cycles_pair = [double]$cycles / [double]$Iterations
            })
        }
    }
}
$Samples | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "small_pair_samples.csv")

function Get-Median {
    param([double[]]$Values)
    $sorted = @($Values | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

$SummaryRows = @(
    foreach ($size in $Sizes) {
        foreach ($allocator in $Bins) {
            $rows = @($Samples | Where-Object {
                $_.size -eq $size -and $_.allocator -eq $allocator.Name
            })
            [pscustomobject]@{
                size = $size
                allocator = $allocator.Name
                ns_pair = Get-Median ([double[]]$rows.ns_pair)
                cycles_pair = Get-Median ([double[]]$rows.cycles_pair)
            }
        }
    }
)
$SummaryRows | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "small_pair_summary.csv")

$NoiseRows = @(
    foreach ($size in $Sizes) {
        $a = ($SummaryRows | Where-Object { $_.size -eq $size -and $_.allocator -eq "hz8-a" }).ns_pair
        $b = ($SummaryRows | Where-Object { $_.size -eq $size -and $_.allocator -eq "hz8-b" }).ns_pair
        [pscustomobject]@{
            size = $size
            hz8_aa_delta_pct = 100.0 * [math]::Abs($a - $b) / [math]::Min($a, $b)
        }
    }
)
$NoiseRows | Export-Csv -NoTypeInformation -Encoding ascii (Join-Path $OutputDir "hz8_aa_noise.csv")

$CompilerVersion = (& $Compiler --version 2>&1 | Select-Object -First 1)
$GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
$Manifest = @(
    "git_commit=$GitCommit",
    "compiler=$Compiler",
    "compiler_version=$CompilerVersion",
    "objdump=$Objdump",
    "nm=$Nm",
    "runs=$Runs",
    "iterations=$Iterations",
    "warmup=$Warmup",
    "sizes=$($Sizes -join ',')",
    "hz8_flags=$($Hz8Flags -join ' ')",
    "hz11_flags=$($Hz11Flags -join ' ')"
)
foreach ($path in @($Hz8Exe, $Hz11Exe, $TcmallocExe, (Join-Path $OutputDir "tcmalloc_minimal.dll"))) {
    $hash = Get-FileHash -Algorithm SHA256 $path
    $Manifest += "sha256_$([IO.Path]::GetFileName($path))=$($hash.Hash)"
}
$Manifest | Set-Content -Encoding ascii (Join-Path $OutputDir "build_manifest.txt")

$Markdown = [System.Collections.Generic.List[string]]::new()
$Markdown.Add("# HZ8 Small Hot-Path Audit L0")
$Markdown.Add("")
$Markdown.Add("Release speed binaries; one pinned thread; warmed alloc, touch, free pairs.")
$Markdown.Add("Allocator and size order are rotated. hz8-a and hz8-b are the same binary.")
$Markdown.Add("No diagnostic counter or atomic is compiled into the measured paths.")
$Markdown.Add("")
$Markdown.Add("## Pair Cost")
$Markdown.Add("")
$Markdown.Add("| size | allocator | median ns/pair | median process cycles/pair |")
$Markdown.Add("|---:|---|---:|---:|")
foreach ($row in $SummaryRows) {
    $Markdown.Add("| $($row.size) | $($row.allocator) | $([math]::Round($row.ns_pair, 3)) | $([math]::Round($row.cycles_pair, 2)) |")
}
$Markdown.Add("")
$Markdown.Add("## HZ8 A/A Noise")
$Markdown.Add("")
$Markdown.Add("| size | median separation |")
$Markdown.Add("|---:|---:|")
foreach ($row in $NoiseRows) {
    $Markdown.Add("| $($row.size) | $([math]::Round($row.hz8_aa_delta_pct, 2))% |")
}
$Markdown.Add("")
$Markdown.Add("## Static Entry Shape")
$Markdown.Add("")
$Markdown.Add("| symbol | instructions | calls | locked RMW |")
$Markdown.Add("|---|---:|---:|---:|")
foreach ($row in $FunctionStats) {
    $Markdown.Add("| $($row.symbol) | $($row.instructions) | $($row.calls) | $($row.locked_rmw) |")
}
$Markdown.Add("")
$Markdown.Add("Static counts include cold blocks and are attribution aids, not executed-path counts.")
$Markdown | Set-Content -Encoding utf8 (Join-Path $OutputDir "HZ8_SMALL_HOT_PATH_AUDIT_L0.md")

Write-Host "Wrote HZ8 small hot-path audit: $OutputDir"
