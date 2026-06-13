$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $scriptDir "bench_app_like_allocator_build_hz5_flags.ps1")
. (Join-Path $scriptDir "bench_app_like_allocator_build_hz6_capacity_flags_core.ps1")
. (Join-Path $scriptDir "bench_app_like_allocator_build_hz6_capacity_flags_app.ps1")
. (Join-Path $scriptDir "bench_app_like_allocator_build_hz6_capacity_flags_elastic.ps1")
. (Join-Path $scriptDir "bench_app_like_allocator_build_invokes.ps1")
