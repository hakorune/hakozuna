function Invoke-H7BenchmarkProbe {
    param(
        [string]$CompilerPath,
        [string]$Hz7Root,
        [string]$OutputDir,
        [string]$BenchSource,
        [string]$OutputExeName,
        [string]$RunnerTag,
        [string]$SummaryTitle,
        [string]$Benchmark,
        [string]$Allocator,
        [string]$Note,
        [int]$Runs,
        [int]$Iters,
        [int]$DirectRetainCap = 0,
        [int]$SpanClassMax = 0,
        [int]$EmptySpanCap = 0
    )

    New-Item -ItemType Directory -Force $OutputDir | Out-Null

    $Compiler = Get-Command $CompilerPath -ErrorAction Stop
    $Hz7Source = Join-Path $Hz7Root "hz7.c"
    $OutputPath = Join-Path $OutputDir $OutputExeName
    if (-not (Test-Path $BenchSource)) {
        throw "bench source not found: $BenchSource"
    }
    if (-not (Test-Path $Hz7Source)) {
        throw "HZ7 source not found: $Hz7Source"
    }

    $Defines = @("/D_CRT_SECURE_NO_WARNINGS")
    if ($DirectRetainCap -gt 0) {
        $Defines += "/DH7_DIRECT_RETAIN_CAP=$DirectRetainCap"
    }
    if ($SpanClassMax -gt 0) {
        $Defines += "/DH7_SPAN_CLASS_MAX=$SpanClassMax"
    }
    if ($EmptySpanCap -gt 0) {
        $Defines += "/DH7_EMPTY_SPAN_CAP=$EmptySpanCap"
    }

    $BuildArgs = @(
        "/nologo",
        "/O2",
        "/DNDEBUG",
        "/W4",
        "/WX"
    ) + $Defines + @(
        $Hz7Source,
        $BenchSource,
        "psapi.lib",
        "/Fe:$OutputPath"
    )

    Write-Host "[$RunnerTag] building $OutputExeName"
    & $Compiler.Source @BuildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "clang-cl $RunnerTag bench failed with exit code $LASTEXITCODE"
    }

    $Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $SummaryPath = Join-Path $OutputDir ($Stamp + "_" + $Benchmark + "_windows.md")
    $RawLogPath = Join-Path $OutputDir ($Stamp + "_" + $Benchmark + "_windows.log")
    $RawLines = New-Object System.Collections.Generic.List[string]
    $Rows = @{}

    for ($run = 1; $run -le $Runs; ++$run) {
        $result = Invoke-H7CapturedProcess -FilePath $OutputPath -Arguments @([string]$Iters)
        $RawLines.Add("=== run $run ===")
        foreach ($line in $result.Lines) {
            $RawLines.Add($line)
        }
        $RawLines.Add("")
        if ($result.ExitCode -ne 0) {
            throw "$RunnerTag bench failed run=$run exit=$($result.ExitCode)"
        }
        Add-H7HotpathRowsFromLines -Rows $Rows -Lines $result.Lines
    }

    $Summary = New-Object System.Collections.Generic.List[string]
    $Summary = New-H7BenchmarkSummaryLines -Title $SummaryTitle `
        -Benchmark $Benchmark -Allocator $Allocator -Runs $Runs -Iters $Iters `
        -Note $Note -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax

    Add-H7BenchmarkSummaryTable -Lines $Summary -Rows $Rows

    $Summary.Add("")
    $Summary.Add("Artifacts: $OutputDir")

    Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
    Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
    Write-Host "Wrote summary: $SummaryPath"
}

function Invoke-H7FilteredBenchmarkProbe {
    param(
        [string]$RunnerScriptPath,
        [string[]]$RunnerArguments,
        [string]$OutputDir,
        [string]$RunnerTag,
        [string]$SummaryTitle,
        [string]$Benchmark,
        [string]$Allocator,
        [string]$Note,
        [int]$Runs,
        [int]$Iters,
        [string[]]$OrderedKeys,
        [int]$DirectRetainCap = 0,
        [int]$SpanClassMax = 0,
        [int]$EmptySpanCap = 0
    )

    New-Item -ItemType Directory -Force $OutputDir | Out-Null

    if (-not (Test-Path $RunnerScriptPath)) {
        throw "runner script not found: $RunnerScriptPath"
    }

    $Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $result = Invoke-H7CapturedProcess -FilePath "powershell" -Arguments @(
        "-ExecutionPolicy", "Bypass",
        "-File", $RunnerScriptPath
    ) + $RunnerArguments
    if ($result.ExitCode -ne 0) {
        throw "$RunnerTag runner failed with exit code $($result.ExitCode)"
    }

    $summaryPath = Find-H7SummaryPathFromLines -Lines $result.Lines
    if (-not $summaryPath -or -not (Test-Path $summaryPath)) {
        throw "$RunnerTag summary not found"
    }

    $rows = Get-H7BenchmarkRowsFromMarkdownPath -Path $summaryPath

    $Summary = New-H7BenchmarkSummaryLines -Title $SummaryTitle `
        -Benchmark $Benchmark -Allocator $Allocator -Runs $Runs -Iters $Iters `
        -Note $Note -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax

    Add-H7BenchmarkSummaryTable -Lines $Summary -Rows $rows -OrderedKeys $OrderedKeys

    $Summary.Add("")
    $Summary.Add("Artifacts: $OutputDir")

    $summaryOut = Join-Path $OutputDir ($Stamp + "_" + $Benchmark + "_windows.md")
    $rawOut = Join-Path $OutputDir ($Stamp + "_" + $Benchmark + "_windows.log")
    Set-Content -Path $summaryOut -Value $Summary -Encoding UTF8
    Set-Content -Path $rawOut -Value ($result.Lines -join "`r`n") -Encoding UTF8
    Write-Host "Wrote summary: $summaryOut"
}
