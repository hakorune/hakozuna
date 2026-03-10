param(
    [switch]$MmanStats
)

$ErrorActionPreference = "Stop"

$TargetScript = Join-Path $PSScriptRoot "..\hakozuna\win\build_win_link.ps1"
if (-not (Test-Path $TargetScript)) {
    throw "Target script not found: $TargetScript"
}

$ForwardArgs = @()
foreach ($entry in $PSBoundParameters.GetEnumerator()) {
    if ($entry.Value -is [System.Management.Automation.SwitchParameter]) {
        if ($entry.Value.IsPresent) {
            $ForwardArgs += "-$($entry.Key)"
        }
        continue
    }

    if ($null -ne $entry.Value -and $entry.Value -ne "") {
        $ForwardArgs += @("-$($entry.Key)", [string]$entry.Value)
    }
}

& $TargetScript @ForwardArgs
exit $LASTEXITCODE
