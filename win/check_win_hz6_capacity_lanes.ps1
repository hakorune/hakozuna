param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$buildCommon = Join-Path $RepoRoot "win/bench_app_like_allocator_build_common.ps1"
$capacityMatrix = Join-Path $RepoRoot "win/run_win_hz6_capacity_matrix.ps1"

if (-not (Test-Path $buildCommon)) {
    throw "Missing build common script: $buildCommon"
}
if (-not (Test-Path $capacityMatrix)) {
    throw "Missing capacity matrix script: $capacityMatrix"
}

$buildText = Get-Content -Path $buildCommon -Raw
$matrixText = Get-Content -Path $capacityMatrix -Raw

$buildKeys = [regex]::Matches($buildText, '"([^"]+)"\s*=\s*@\{') |
    ForEach-Object { $_.Groups[1].Value } |
    Where-Object { $_.Contains("-") } |
    Sort-Object -Unique

$matrixKeys = [regex]::Matches($matrixText, '"([^"]+)"\s*=\s*"_') |
    ForEach-Object { $_.Groups[1].Value } |
    Where-Object { $_.Contains("-") } |
    Sort-Object -Unique

$missingInMatrix = Compare-Object -ReferenceObject $buildKeys -DifferenceObject $matrixKeys |
    Where-Object { $_.SideIndicator -eq "<=" } |
    ForEach-Object { $_.InputObject }

$missingInBuild = Compare-Object -ReferenceObject $buildKeys -DifferenceObject $matrixKeys |
    Where-Object { $_.SideIndicator -eq "=>" } |
    ForEach-Object { $_.InputObject }

if ($missingInMatrix.Count -gt 0 -or $missingInBuild.Count -gt 0) {
    if ($missingInMatrix.Count -gt 0) {
        Write-Host "Missing in run_win_hz6_capacity_matrix.ps1:"
        $missingInMatrix | ForEach-Object { Write-Host "  $_" }
    }
    if ($missingInBuild.Count -gt 0) {
        Write-Host "Missing in bench_app_like_allocator_build_common.ps1:"
        $missingInBuild | ForEach-Object { Write-Host "  $_" }
    }
    exit 1
}

Write-Host "HZ6 capacity lane maps are aligned ($($buildKeys.Count) lanes)."
