param(
    [string]$OutputDir,
    [string[]]$Profiles
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\\benchmarks\\windows"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_mixed_ws_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz4.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Invoke-BenchProcess {
    param(
        [string]$Path,
        [string[]]$Args
    )

    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $Path @Args 2>&1
        $rc = $LASTEXITCODE
        return [pscustomobject]@{
            Output    = $output
            ExitCode  = $rc
        }
    } finally {
        $ErrorActionPreference = $prevEap
    }
}

$AllProfiles = @(
    @{ Name = "smoke"; Threads = 2; ItersPerThread = 10000; WorkingSet = 256; MinSize = 16; MaxSize = 128; Note = "quick sanity and regression check" },
    @{ Name = "balanced"; Threads = 8; ItersPerThread = 250000; WorkingSet = 4096; MinSize = 16; MaxSize = 2048; Note = "larger mixed run for first Windows compare" },
    @{ Name = "wide_ws"; Threads = 8; ItersPerThread = 200000; WorkingSet = 16384; MinSize = 16; MaxSize = 1024; Note = "wider working-set pressure" },
    @{ Name = "larger_sizes"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 256; MaxSize = 8192; Note = "shift toward larger allocations" },
    @{ Name = "heavy_mixed"; Threads = 8; ItersPerThread = 5000000; WorkingSet = 16384; MinSize = 16; MaxSize = 4096; Note = "heavier mixed run with longer timings" }
)

if ($Profiles -and $Profiles.Count -gt 0) {
    $Selected = @()
    foreach ($name in $Profiles) {
        $match = $AllProfiles | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown profile: $name"
        }
        $Selected += $match
    }
} else {
    $Selected = $AllProfiles
}

$ArtifactsPath = Join-Path $RepoRoot "out_win_suite"
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_allocator_matrix.md")
$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Windows Allocator Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Artifacts: [out_win_suite]($ArtifactsPath)")
$Summary.Add("")

foreach ($profile in $Selected) {
    $Summary.Add("## " + $profile.Name)
    $Summary.Add("")
    $Summary.Add("- Note: " + $profile.Note)
    $Summary.Add("- Args: threads=$($profile.Threads) iters=$($profile.ItersPerThread) ws=$($profile.WorkingSet) size=$($profile.MinSize)..$($profile.MaxSize)")
    $Summary.Add("")
    $Summary.Add("| allocator | ops/s | raw |")
    $Summary.Add("| --- | ---: | --- |")

    $ProfileLog = Join-Path $OutputDir ($Stamp + "_" + $profile.Name + ".log")
    $LogLines = New-Object System.Collections.Generic.List[string]

    foreach ($exe in $Executables) {
        $args = @(
            [string]$profile.Threads,
            [string]$profile.ItersPerThread,
            [string]$profile.WorkingSet,
            [string]$profile.MinSize,
            [string]$profile.MaxSize
        )
        $result = Invoke-BenchProcess -Path $exe.Path -Args $args
        $raw = (($result.Output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
        if (-not $raw) {
            $raw = "(no output)"
        }
        $LogLines.Add("=== " + $profile.Name + " / " + $exe.Name + " ===")
        $LogLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $LogLines.Add("rc: " + $result.ExitCode)
        $LogLines.Add($raw)
        $LogLines.Add("")
        if ($result.ExitCode -ne 0) {
            throw "Profile $($profile.Name) allocator $($exe.Name) failed with exit code $($result.ExitCode)"
        }

        $ops = ""
        if ($raw -match "ops/s=([0-9.]+)") {
            $ops = $Matches[1]
        } else {
            $ops = "n/a"
        }
        $Summary.Add(('| {0} | {1} | `{2}` |' -f $exe.Name, $ops, $raw))
    }

    Set-Content -Path $ProfileLog -Value $LogLines -Encoding UTF8
    $Summary.Add("")
}

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
