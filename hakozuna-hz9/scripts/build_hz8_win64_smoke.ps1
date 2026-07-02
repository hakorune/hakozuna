$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "build_hz9_win64_smoke.ps1"
& $script @args
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
