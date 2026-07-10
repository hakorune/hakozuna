param(
    [string]$OutDirName = "out_win_shadow",
    [int]$Seconds = 1,
    [int]$Runs = 3,
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$Runner = Join-Path $Hz12Root "$OutDirName\bench_hz12_token_xowner_pipeline.exe"
$Intervals = @(1, 8, 16, 32, 64, 128, 256)

if ($Seconds -lt 1 -or $Runs -lt 1) {
    throw "Seconds and Runs must be positive."
}
if (-not (Test-Path $Runner)) {
    throw "Build HZ12 first: $Runner"
}
if (-not $OutputDir) {
    $OutputDir = Join-Path $Hz12Root ("bench_results\token_drain_sweep_" +
        (Get-Date -Format "yyyyMMdd_HHmmss"))
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$Rows = @()
foreach ($interval in $Intervals) {
    for ($run = 1; $run -le $Runs; ++$run) {
        $output = & $Runner $Seconds $interval 2>&1
        $exitCode = $LASTEXITCODE
        $line = $output | Select-String -SimpleMatch '[HZ12_TOKEN_XOWNER_L4B]' |
            ForEach-Object { $_.Line }
        $Rows += [pscustomobject]@{
            interval = $interval
            run = $run
            exit_code = $exitCode
            output = ($line -join " ")
        }
        if ($exitCode -ne 0) {
            Write-Warning "interval=$interval run=$run failed: $output"
        }
    }
}
$Rows | Export-Csv -NoTypeInformation -Encoding utf8 (Join-Path $OutputDir "token_drain_sweep.csv")
$Rows | Format-Table -AutoSize | Out-String | Set-Content -Encoding utf8 (Join-Path $OutputDir "token_drain_sweep.txt")
$Rows | Format-Table -AutoSize
Write-Host "Saved HZ12 token drain sweep: $OutputDir"
